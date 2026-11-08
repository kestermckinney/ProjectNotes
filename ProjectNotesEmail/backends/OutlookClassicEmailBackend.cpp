// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
#include "OutlookClassicEmailBackend.h"
#include <QCoreApplication>
#include <QPointer>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <thread>
#include <vector>
#include <windows.h>
#include <oleauto.h>
#include <wrl/client.h>
namespace PN::Comm {
namespace {
using Microsoft::WRL::ComPtr;
VARIANT text(const QString &value)
{
    VARIANT variant;
    VariantInit(&variant);
    variant.vt = VT_BSTR;
    variant.bstrVal = SysAllocStringLen(reinterpret_cast<const OLECHAR *>(value.utf16()), UINT(value.size()));
    return variant;
}

VARIANT integer(long value)
{
    VARIANT variant;
    VariantInit(&variant);
    variant.vt = VT_I4;
    variant.lVal = value;
    return variant;
}

HRESULT call(IDispatch *target, const wchar_t *name, WORD flags, std::vector<VARIANT> args, VARIANT *result)
{
    HRESULT hr = E_POINTER;
    if (target) {
        DISPID id = DISPID_UNKNOWN;
        LPOLESTR member = const_cast<LPOLESTR>(name);
        hr = target->GetIDsOfNames(IID_NULL, &member, 1, LOCALE_USER_DEFAULT, &id);
        if (SUCCEEDED(hr)) {
            // IDispatch takes positional arguments last to first.
            std::vector<VARIANT> reversed(args.rbegin(), args.rend());
            DISPID namedPut = DISPID_PROPERTYPUT;
            DISPPARAMS params{reversed.data(), nullptr, UINT(reversed.size()), 0};
            if (flags & DISPATCH_PROPERTYPUT) {
                params.rgdispidNamedArgs = &namedPut;
                params.cNamedArgs = 1;
            }
            hr = target->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, flags, &params, result, nullptr, nullptr);
        }
    }
    for (VARIANT &arg : args)
        VariantClear(&arg);
    return hr;
}

// A method or property that returns an object; null on any failure.
ComPtr<IDispatch> get(IDispatch *target, const wchar_t *name, std::vector<VARIANT> args = {})
{
    VARIANT result;
    VariantInit(&result);
    ComPtr<IDispatch> dispatch;
    if (SUCCEEDED(call(target, name, DISPATCH_METHOD | DISPATCH_PROPERTYGET, std::move(args), &result))
        && result.vt == VT_DISPATCH)
        dispatch = result.pdispVal;
    VariantClear(&result);
    return dispatch;
}

bool put(IDispatch *target, const wchar_t *name, VARIANT value)
{
    return SUCCEEDED(call(target, name, DISPATCH_PROPERTYPUT, {value}, nullptr));
}

bool invoke(IDispatch *target, const wchar_t *name, std::vector<VARIANT> args = {})
{
    VARIANT result;
    VariantInit(&result);
    const HRESULT hr = call(target, name, DISPATCH_METHOD, std::move(args), &result);
    VariantClear(&result);
    return SUCCEEDED(hr);
}


EmailHandoffResult present(const EmailRequest &request)
{
    EmailHandoffResult result;
    result.operationId = request.operationId;
    ComPtr<IDispatch> mail;
    auto fail = [&]() {
        // Discard our incomplete composer so retry cannot leave duplicate drafts.
        if (mail) invoke(mail.Get(), L"Close", {integer(1)});
        result.error = {QStringLiteral("outlook-classic-compose-failed"), {},
                        RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        return result;
    };
    for (const Artifact &attachment : request.attachments) {
        const QFileInfo file(attachment.absolutePath);
        if (!file.isFile() || !file.isReadable()) return fail();
    }
    CLSID classId;
    if (FAILED(CLSIDFromProgID(L"Outlook.Application", &classId))) return fail();
    ComPtr<IDispatch> outlook;
    ComPtr<IUnknown> running;
    if (SUCCEEDED(GetActiveObject(classId, nullptr, &running))) running.As(&outlook);
    if (!outlook && FAILED(CoCreateInstance(classId, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&outlook))))
        return fail();
    mail = get(outlook.Get(), L"CreateItem", {integer(0)});
    if (!mail || !put(mail.Get(), L"Subject", text(request.subject))) return fail();
    // Select HTML before signature initialization; changing format afterwards
    // can strip the signature formatting and its embedded images.
    if (!request.html.isEmpty() && !put(mail.Get(), L"BodyFormat", integer(2))) return fail();
    const auto recipients = get(mail.Get(), L"Recipients");
    for (const EmailAddress &address : request.recipients) {
        const auto recipient = get(recipients.Get(), L"Add", {text(address.address)});
        const long role = address.role == RecipientRole::To ? 1 : address.role == RecipientRole::Cc ? 2 : 3;
        if (!recipient || !put(recipient.Get(), L"Type", integer(role))) return fail();
    }
    const auto attachments = get(mail.Get(), L"Attachments");
    for (const Artifact &attachment : request.attachments) {
        if (!get(attachments.Get(), L"Add", {text(QDir::toNativeSeparators(attachment.absolutePath))})) return fail();
    }
    // Display BEFORE editing the body so Outlook creates its normal signature,
    // including account-specific formatting and embedded signature images.
    AllowSetForegroundWindow(ASFW_ANY);
    if (!invoke(mail.Get(), L"Display")) return fail();
    const auto inspector = get(mail.Get(), L"GetInspector");
    if (!request.html.isEmpty()) {
        QTemporaryFile body(QDir::tempPath() + QStringLiteral("/projectnotes-email-XXXXXX.html"));
        // The BOM makes Word import non-ASCII names and report text as UTF-8.
        const QByteArray html = QByteArray::fromHex("efbbbf") + request.html.toUtf8();
        if (!body.open() || body.write(html) != html.size() || !body.flush()) return fail();
        body.close();
        const auto editor = get(inspector.Get(), L"WordEditor");
        const auto insertion = get(editor.Get(), L"Range", {integer(0), integer(0)});
        // Import HTML into a zero-length Word range. Never replace HTMLBody or
        // the document Content: those operations erase the existing signature.
        if (!insertion || !invoke(insertion.Get(), L"InsertFile", {text(QDir::toNativeSeparators(body.fileName()))}))
            return fail();
    }
    invoke(inspector.Get(), L"Activate");
    result.certainty = OutcomeCertainty::Certain;
    return result;
}
}
void OutlookClassicEmailBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }
void OutlookClassicEmailBackend::handoff(EmailRequest request, Completion completion)
{
    if (m_cancelled.remove(request.operationId)) {
        EmailHandoffResult result;
        result.operationId = request.operationId;
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    const QPointer<QCoreApplication> application = QCoreApplication::instance();
    std::thread([request = std::move(request), completion = std::move(completion), application]() mutable {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        EmailHandoffResult result;
        if (SUCCEEDED(initialized)) {
            result = present(request);
            CoUninitialize();
        } else {
            result.operationId = request.operationId;
            result.error = {QStringLiteral("outlook-classic-unavailable"), {}, RetryKind::ReviewAndRetry, OutcomeCertainty::Certain};
        }
        if (application)
            QMetaObject::invokeMethod(application, [completion = std::move(completion), result] { completion(result); }, Qt::QueuedConnection);
    }).detach();
}
}
