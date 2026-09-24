// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "OutlookClassicMeetingBackend.h"

#include <QCoreApplication>
#include <QPointer>

#include <thread>
#include <vector>

#include <windows.h>
#include <oleauto.h>
#include <wrl/client.h>

namespace PN::Comm {
namespace {

using Microsoft::WRL::ComPtr;

// Outlook object model constants.
constexpr long olMailItem = 0;
constexpr long olAppointmentItem = 1;
constexpr long olMeeting = 1;
constexpr long olRequired = 1;
constexpr long olOptional = 2;
constexpr long olDiscard = 1;

// Argument builders.  call() takes ownership of every VARIANT it is passed.
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

// Outlook reads Start and End as local wall-clock time.
VARIANT date(const QDateTime &value)
{
    const QDateTime local = value.toLocalTime();
    SYSTEMTIME time{};
    time.wYear = WORD(local.date().year());
    time.wMonth = WORD(local.date().month());
    time.wDay = WORD(local.date().day());
    time.wHour = WORD(local.time().hour());
    time.wMinute = WORD(local.time().minute());
    time.wSecond = WORD(local.time().second());
    VARIANT variant;
    VariantInit(&variant);
    variant.vt = VT_DATE;
    SystemTimeToVariantTime(&time, &variant.date);
    return variant;
}

VARIANT object(IDispatch *value)
{
    VARIANT variant;
    VariantInit(&variant);
    variant.vt = VT_DISPATCH;
    variant.pdispVal = value;
    if (value) value->AddRef();
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

// An appointment has no HTMLBody.  A scratch message converts the HTML in
// Outlook's Word editor, and its formatted text replaces the plain agenda
// already in the open meeting window.  Best effort: on failure the plain
// agenda stays.
void applyHtmlAgenda(IDispatch *outlook, IDispatch *meeting, const QString &html)
{
    const ComPtr<IDispatch> scratch = get(outlook, L"CreateItem", {integer(olMailItem)});
    if (!scratch)
        return;
    if (put(scratch.Get(), L"HTMLBody", text(html))) {
        const ComPtr<IDispatch> formatted = get(
            get(get(get(scratch.Get(), L"GetInspector").Get(), L"WordEditor").Get(), L"Content").Get(),
            L"FormattedText");
        const ComPtr<IDispatch> target =
            get(get(get(meeting, L"GetInspector").Get(), L"WordEditor").Get(), L"Content");
        if (formatted && target)
            put(target.Get(), L"FormattedText", object(formatted.Get()));
    }
    invoke(scratch.Get(), L"Close", {integer(olDiscard)});
}

MeetingHandoffResult present(const MeetingRequest &request)
{
    MeetingHandoffResult result;
    result.operationId = request.operationId;
    const auto fail = [&result](const QString &code) {
        result.error = {code, {}, RetryKind::RetryOperation, OutcomeCertainty::Certain};
        return result;
    };

    // Only Outlook (classic) registers this ProgID; the new Outlook has no
    // object model.
    CLSID classId;
    if (FAILED(CLSIDFromProgID(L"Outlook.Application", &classId)))
        return fail(QStringLiteral("outlook-classic-unavailable"));
    ComPtr<IDispatch> outlook;
    ComPtr<IUnknown> running;
    if (SUCCEEDED(GetActiveObject(classId, nullptr, &running)))
        running.As(&outlook);
    if (!outlook && FAILED(CoCreateInstance(classId, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&outlook))))
        return fail(QStringLiteral("outlook-classic-unavailable"));

    const ComPtr<IDispatch> meeting = get(outlook.Get(), L"CreateItem", {integer(olAppointmentItem)});
    bool ok = meeting && put(meeting.Get(), L"MeetingStatus", integer(olMeeting))
        && put(meeting.Get(), L"Subject", text(request.subject.trimmed()))
        && put(meeting.Get(), L"Start", date(request.start))
        && put(meeting.Get(), L"End", date(request.end));
    if (ok && !request.location.trimmed().isEmpty())
        ok = put(meeting.Get(), L"Location", text(request.location.trimmed()));
    if (ok && !request.plainAgenda.trimmed().isEmpty())
        ok = put(meeting.Get(), L"Body", text(request.plainAgenda));
    const ComPtr<IDispatch> recipients = ok ? get(meeting.Get(), L"Recipients") : ComPtr<IDispatch>();
    if (!recipients)
        return fail(QStringLiteral("outlook-classic-failed"));
    for (const MeetingAttendee &attendee : request.attendees) {
        const ComPtr<IDispatch> recipient = get(recipients.Get(), L"Add", {text(attendee.address.trimmed())});
        const long type = attendee.role == AttendeeRole::Required ? olRequired : olOptional;
        if (!recipient || !put(recipient.Get(), L"Type", integer(type)))
            return fail(QStringLiteral("outlook-classic-failed"));
    }
    // An address Outlook cannot resolve stays underlined in the window for
    // the user to correct, so the result is not checked.
    invoke(recipients.Get(), L"ResolveAll");

    // The meeting is never saved or sent here; the user sends it from the window.
    AllowSetForegroundWindow(ASFW_ANY);
    if (!invoke(meeting.Get(), L"Display"))
        return fail(QStringLiteral("outlook-classic-failed"));
    if (!request.htmlAgenda.trimmed().isEmpty())
        applyHtmlAgenda(outlook.Get(), meeting.Get(), request.htmlAgenda);
    if (const ComPtr<IDispatch> inspector = get(meeting.Get(), L"GetInspector"))
        invoke(inspector.Get(), L"Activate");
    result.certainty = OutcomeCertainty::Certain;
    return result;
}

} // namespace

void OutlookClassicMeetingBackend::cancel(const QUuid &operationId) { m_cancelled.insert(operationId); }

void OutlookClassicMeetingBackend::handoff(MeetingRequest request, Completion completion)
{
    if (m_cancelled.remove(request.operationId)) {
        MeetingHandoffResult result;
        result.operationId = request.operationId;
        result.error = {QStringLiteral("operation-cancelled"), {}, RetryKind::None, OutcomeCertainty::Certain};
        completion(result);
        return;
    }
    // Completion runs back on the application thread, like the other backends'.
    const QPointer<QCoreApplication> application = QCoreApplication::instance();
    std::thread([request = std::move(request), completion = std::move(completion), application]() mutable {
        const bool initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
        const MeetingHandoffResult result = present(request);
        if (initialized)
            CoUninitialize();
        if (application)
            QMetaObject::invokeMethod(application, [completion = std::move(completion), result] { completion(result); },
                                      Qt::QueuedConnection);
    }).detach();
}

} // namespace PN::Comm
