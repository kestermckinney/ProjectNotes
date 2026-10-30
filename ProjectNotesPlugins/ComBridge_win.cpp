// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Windows COM automation through IDispatch, shared by the plugin `com` API.
// It consolidates the call/property helpers that OutlookContacts.cpp and
// OutlookClassicMeetingBackend.cpp each carry. ActiveQt's QAxObject is not
// used: it pulls in Qt::Widgets and cannot bind monikers such as winmgmts:.

#include "ComBridge.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QUuid>

#include <objbase.h>
#include <oleauto.h>
#include <windows.h>
#include <wrl/client.h>

#include <vector>

using Microsoft::WRL::ComPtr;

namespace PN::Plugins {
namespace {

QString hresultText(HRESULT hr)
{
    wchar_t* buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, DWORD(hr), 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    QString text = buffer ? QString::fromWCharArray(buffer).trimmed() : QString();
    if (buffer)
        LocalFree(buffer);
    return text;
}

PluginError comError(const QString& what, HRESULT hr, const EXCEPINFO* exception = nullptr)
{
    QString message = what;
    if (exception && exception->bstrDescription)
        message += QStringLiteral(": ") + QString::fromWCharArray(exception->bstrDescription);
    else if (!hresultText(hr).isEmpty())
        message += QStringLiteral(": ") + hresultText(hr);
    return PluginError::make(QStringLiteral("com-failed"), message,
                             {{QStringLiteral("hresult"), QStringLiteral("0x%1").arg(quint32(hr), 8, 16, QLatin1Char('0'))}});
}

// Retries calls an Office application rejects while it is busy (a dialog is
// open, it is still starting), for a bounded time, instead of failing at once
// or blocking forever.
class RetryMessageFilter final : public IMessageFilter {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override
    {
        if (riid == IID_IUnknown || riid == IID_IMessageFilter) {
            *object = static_cast<IMessageFilter*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refs); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0)
            delete this;
        return refs;
    }
    DWORD STDMETHODCALLTYPE HandleInComingCall(DWORD, HTASK, DWORD, LPINTERFACEINFO) override { return SERVERCALL_ISHANDLED; }
    DWORD STDMETHODCALLTYPE RetryRejectedCall(HTASK, DWORD elapsedMs, DWORD rejectType) override
    {
        // Retry after 250 ms while the server is busy, for up to 30 seconds.
        if (rejectType == SERVERCALL_RETRYLATER && elapsedMs < 30000)
            return 250;
        return DWORD(-1);
    }
    DWORD STDMETHODCALLTYPE MessagePending(HTASK, DWORD, DWORD) override { return PENDINGMSG_WAITDEFPROCESS; }
private:
    LONG m_refs = 1;
};

class WindowsComBridge final : public ComBridge {
public:
    WindowsComBridge()
    {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        m_initialized = SUCCEEDED(hr);
        if (m_initialized) {
            m_filter = new RetryMessageFilter;
            CoRegisterMessageFilter(m_filter, &m_previousFilter);
        }
    }
    ~WindowsComBridge() override
    {
        releaseAll();
        if (m_initialized) {
            IMessageFilter* ours = nullptr;
            CoRegisterMessageFilter(m_previousFilter, &ours);
            if (ours)
                ours->Release();
            if (m_previousFilter)
                m_previousFilter->Release();
            m_filter->Release();
            CoUninitialize();
        }
    }

    bool available() const override { return m_initialized; }

    QVariant createObject(const QString& progId, PluginError* error) override
    {
        CLSID clsid;
        HRESULT hr = CLSIDFromProgID(reinterpret_cast<LPCOLESTR>(progId.utf16()), &clsid);
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("Unknown COM object \"%1\"").arg(progId), hr);
            return {};
        }
        ComPtr<IDispatch> object;
        hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER, IID_IDispatch,
                              reinterpret_cast<void**>(object.GetAddressOf()));
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("Could not start \"%1\"").arg(progId), hr);
            return {};
        }
        return wrap(object);
    }

    QVariant getActiveObject(const QString& progId, PluginError* error) override
    {
        CLSID clsid;
        HRESULT hr = CLSIDFromProgID(reinterpret_cast<LPCOLESTR>(progId.utf16()), &clsid);
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("Unknown COM object \"%1\"").arg(progId), hr);
            return {};
        }
        ComPtr<IUnknown> unknown;
        hr = GetActiveObject(clsid, nullptr, unknown.GetAddressOf());
        if (FAILED(hr))
            return QVariant::fromValue(nullptr);
        ComPtr<IDispatch> object;
        if (FAILED(unknown.As(&object)))
            return QVariant::fromValue(nullptr);
        return wrap(object);
    }

    QVariant getObject(const QString& moniker, PluginError* error) override
    {
        ComPtr<IDispatch> object;
        const HRESULT hr = CoGetObject(reinterpret_cast<LPCWSTR>(moniker.utf16()), nullptr, IID_IDispatch,
                                       reinterpret_cast<void**>(object.GetAddressOf()));
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("Could not bind \"%1\"").arg(moniker), hr);
            return {};
        }
        return wrap(object);
    }

    QVariant call(const QString& handle, const QString& member, const QVariantList& args, PluginError* error) override
    {
        IDispatch* object = find(handle, error);
        if (!object)
            return {};
        QVariant result;
        invoke(object, member, DISPATCH_METHOD | DISPATCH_PROPERTYGET, args, nullptr, &result, error);
        return result;
    }

    QVariant get(const QString& handle, const QString& member, const QVariantList& index, PluginError* error) override
    {
        IDispatch* object = find(handle, error);
        if (!object)
            return {};
        // Reading a method would run it, and some servers (Word) answer a
        // property get on a method with a plain exception, so ask first.
        if (index.isEmpty() && isMethod(object, member))
            return QVariantMap{{QStringLiteral("$method"), true}};
        QVariant result;
        HRESULT hr = S_OK;
        if (!invoke(object, member, DISPATCH_PROPERTYGET, index, nullptr, &result, error, &hr)) {
            // The member exists but is a method (or needs arguments): let the
            // script call it.
            if (hr == DISP_E_MEMBERNOTFOUND || hr == DISP_E_BADPARAMCOUNT || hr == DISP_E_PARAMNOTOPTIONAL
                || hr == DISP_E_PARAMNOTFOUND) {
                *error = PluginError();
                return QVariantMap{{QStringLiteral("$method"), true}};
            }
            return {};
        }
        return result;
    }

    void set(const QString& handle, const QString& member, const QVariant& value,
             const QVariantList& index, PluginError* error) override
    {
        IDispatch* object = find(handle, error);
        if (!object)
            return;
        // A dispatch value is assigned by reference (PROPERTYPUTREF), others by value.
        const bool byRef = value.typeId() == QMetaType::QVariantMap
                        && value.toMap().contains(QStringLiteral("$handle"));
        QVariant ignored;
        invoke(object, member, byRef ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT, index, &value, &ignored, error);
    }

    QVariant enumerate(const QString& handle, PluginError* error) override
    {
        IDispatch* object = find(handle, error);
        if (!object)
            return {};
        DISPPARAMS none {nullptr, nullptr, 0, 0};
        VARIANT result;
        VariantInit(&result);
        EXCEPINFO exception {};
        HRESULT hr = object->Invoke(DISPID_NEWENUM, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                    &none, &result, &exception, nullptr);
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("The object is not a collection"), hr, &exception);
            return {};
        }
        ComPtr<IEnumVARIANT> items;
        if (result.vt == VT_UNKNOWN || result.vt == VT_DISPATCH)
            result.punkVal->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(items.GetAddressOf()));
        VariantClear(&result);
        if (!items) {
            *error = PluginError::make(QStringLiteral("com-failed"), QStringLiteral("The object did not return an enumerator."));
            return {};
        }
        QVariantList out;
        VARIANT item;
        VariantInit(&item);
        ULONG fetched = 0;
        while (items->Next(1, &item, &fetched) == S_OK && fetched == 1) {
            out.append(fromVariant(item));
            VariantClear(&item);
        }
        return out;
    }

    void release(const QString& handle) override { m_objects.remove(handle); }
    void releaseAll() override { m_objects.clear(); }

private:
    QVariant wrap(const ComPtr<IDispatch>& object)
    {
        const QString handle = QStringLiteral("com-%1").arg(++m_nextHandle);
        m_objects.insert(handle, Held{object});
        return QVariantMap{{QStringLiteral("$handle"), handle}};
    }

    // True when the object's type information lists `member` as a method and
    // not as a readable property. False when there is no type information.
    static bool isMethod(IDispatch* object, const QString& member)
    {
        LPOLESTR name = const_cast<LPOLESTR>(reinterpret_cast<LPCOLESTR>(member.utf16()));
        DISPID dispid = DISPID_UNKNOWN;
        if (FAILED(object->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid)))
            return false;
        ComPtr<ITypeInfo> info;
        if (FAILED(object->GetTypeInfo(0, LOCALE_USER_DEFAULT, info.GetAddressOf())) || !info)
            return false;
        TYPEATTR* attr = nullptr;
        if (FAILED(info->GetTypeAttr(&attr)) || !attr)
            return false;
        bool method = false;
        bool property = false;
        for (UINT i = 0; i < attr->cFuncs; ++i) {
            FUNCDESC* desc = nullptr;
            if (FAILED(info->GetFuncDesc(i, &desc)) || !desc)
                continue;
            if (desc->memid == dispid) {
                if (desc->invkind == INVOKE_FUNC)
                    method = true;
                else if (desc->invkind == INVOKE_PROPERTYGET)
                    property = true;
            }
            info->ReleaseFuncDesc(desc);
        }
        info->ReleaseTypeAttr(attr);
        return method && !property;
    }

    IDispatch* find(const QString& handle, PluginError* error)
    {
        const auto it = m_objects.constFind(handle);
        if (it == m_objects.constEnd()) {
            *error = PluginError::make(QStringLiteral("com-failed"), QStringLiteral("The COM object was released."));
            return nullptr;
        }
        return it->object.Get();
    }

    bool toVariant(const QVariant& value, VARIANT* out, PluginError* error)
    {
        VariantInit(out);
        if (!value.isValid() || value.isNull()) {
            out->vt = VT_ERROR;              // "missing" optional argument
            out->scode = DISP_E_PARAMNOTFOUND;
            return true;
        }
        switch (value.typeId()) {
        case QMetaType::Bool:
            out->vt = VT_BOOL;
            out->boolVal = value.toBool() ? VARIANT_TRUE : VARIANT_FALSE;
            return true;
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt:
        case QMetaType::ULongLong:
            out->vt = VT_I4;
            out->lVal = value.toInt();
            return true;
        case QMetaType::Double:
        case QMetaType::Float: {
            const double d = value.toDouble();
            if (d == double(qint32(d))) {
                out->vt = VT_I4;
                out->lVal = qint32(d);
            } else {
                out->vt = VT_R8;
                out->dblVal = d;
            }
            return true;
        }
        case QMetaType::QVariantMap: {
            const QVariantMap map = value.toMap();
            if (map.contains(QStringLiteral("$handle"))) {
                IDispatch* object = find(map.value(QStringLiteral("$handle")).toString(), error);
                if (!object)
                    return false;
                out->vt = VT_DISPATCH;
                out->pdispVal = object;
                object->AddRef();
                return true;
            }
            break;
        }
        case QMetaType::QVariantList: {
            const QVariantList list = value.toList();
            SAFEARRAY* array = SafeArrayCreateVector(VT_VARIANT, 0, ULONG(list.size()));
            for (LONG i = 0; i < LONG(list.size()); ++i) {
                VARIANT element;
                if (!toVariant(list.at(i), &element, error)) {
                    SafeArrayDestroy(array);
                    return false;
                }
                SafeArrayPutElement(array, &i, &element);
                VariantClear(&element);
            }
            out->vt = VT_ARRAY | VT_VARIANT;
            out->parray = array;
            return true;
        }
        default:
            break;
        }
        out->vt = VT_BSTR;
        out->bstrVal = SysAllocString(reinterpret_cast<const OLECHAR*>(value.toString().utf16()));
        return true;
    }

    QVariant fromVariant(const VARIANT& in)
    {
        VARIANT v;
        VariantInit(&v);
        // Unwrap by-reference values first.
        if (FAILED(VariantCopyInd(&v, &in)))
            return {};
        QVariant result;
        switch (v.vt) {
        case VT_EMPTY:
        case VT_NULL: result = QVariant::fromValue(nullptr); break;
        case VT_BOOL: result = v.boolVal != VARIANT_FALSE; break;
        case VT_I1: result = int(v.cVal); break;
        case VT_UI1: result = int(v.bVal); break;
        case VT_I2: result = int(v.iVal); break;
        case VT_UI2: result = int(v.uiVal); break;
        case VT_I4: case VT_INT: result = int(v.lVal); break;
        case VT_UI4: case VT_UINT: result = double(v.ulVal); break;
        case VT_I8: result = double(v.llVal); break;
        case VT_UI8: result = double(v.ullVal); break;
        case VT_R4: result = double(v.fltVal); break;
        case VT_R8: result = v.dblVal; break;
        case VT_CY: result = double(v.cyVal.int64) / 10000.0; break;
        case VT_BSTR: result = v.bstrVal ? QString::fromWCharArray(v.bstrVal, int(SysStringLen(v.bstrVal))) : QString(); break;
        case VT_DATE: {
            SYSTEMTIME st;
            if (VariantTimeToSystemTime(v.date, &st))
                result = QDateTime(QDate(st.wYear, st.wMonth, st.wDay), QTime(st.wHour, st.wMinute, st.wSecond))
                             .toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
            break;
        }
        case VT_DISPATCH:
            result = v.pdispVal ? wrap(ComPtr<IDispatch>(v.pdispVal)) : QVariant::fromValue(nullptr);
            break;
        case VT_UNKNOWN: {
            ComPtr<IDispatch> dispatch;
            if (v.punkVal && SUCCEEDED(v.punkVal->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(dispatch.GetAddressOf()))))
                result = wrap(dispatch);
            else
                result = QVariant::fromValue(nullptr);
            break;
        }
        default:
            if (v.vt & VT_ARRAY) {
                QVariantList list;
                SAFEARRAY* array = v.parray;
                LONG lower = 0, upper = -1;
                SafeArrayGetLBound(array, 1, &lower);
                SafeArrayGetUBound(array, 1, &upper);
                for (LONG i = lower; i <= upper; ++i) {
                    VARIANT element;
                    VariantInit(&element);
                    if ((v.vt & VT_TYPEMASK) == VT_VARIANT) {
                        SafeArrayGetElement(array, &i, &element);
                    } else {
                        element.vt = VARTYPE(v.vt & VT_TYPEMASK);
                        SafeArrayGetElement(array, &i, &element.byref);
                    }
                    list.append(fromVariant(element));
                    VariantClear(&element);
                }
                result = list;
            } else {
                VARIANT text;
                VariantInit(&text);
                if (SUCCEEDED(VariantChangeType(&text, &v, 0, VT_BSTR)))
                    result = QString::fromWCharArray(text.bstrVal);
                VariantClear(&text);
            }
            break;
        }
        VariantClear(&v);
        return result;
    }

    bool invoke(IDispatch* object, const QString& member, WORD flags, const QVariantList& args,
                const QVariant* putValue, QVariant* result, PluginError* error, HRESULT* status = nullptr)
    {
        DISPID dispid = DISPID_VALUE;
        if (!member.isEmpty()) {
            LPOLESTR name = const_cast<LPOLESTR>(reinterpret_cast<LPCOLESTR>(member.utf16()));
            const HRESULT hr = object->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
            if (FAILED(hr)) {
                if (status) *status = hr;
                *error = comError(QStringLiteral("The object has no member \"%1\"").arg(member), hr);
                return false;
            }
        }

        // IDispatch takes arguments in reverse order.
        std::vector<VARIANT> values;
        values.reserve(size_t(args.size()) + (putValue ? 1 : 0));
        if (putValue) {
            VARIANT v;
            if (!toVariant(*putValue, &v, error))
                return false;
            values.push_back(v);
        }
        for (qsizetype i = args.size() - 1; i >= 0; --i) {
            VARIANT v;
            if (!toVariant(args.at(i), &v, error)) {
                for (VARIANT& made : values) VariantClear(&made);
                return false;
            }
            values.push_back(v);
        }

        DISPID named = DISPID_PROPERTYPUT;
        DISPPARAMS params {values.empty() ? nullptr : values.data(), nullptr, UINT(values.size()), 0};
        if (putValue) {
            params.rgdispidNamedArgs = &named;
            params.cNamedArgs = 1;
        }
        VARIANT out;
        VariantInit(&out);
        EXCEPINFO exception {};
        UINT badArgument = 0;
        const HRESULT hr = object->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, flags, &params,
                                          putValue ? nullptr : &out, &exception, &badArgument);
        for (VARIANT& made : values)
            VariantClear(&made);
        if (status) *status = hr;
        if (FAILED(hr)) {
            *error = comError(QStringLiteral("%1 failed").arg(member.isEmpty() ? QStringLiteral("The call") : member), hr, &exception);
            SysFreeString(exception.bstrDescription);
            SysFreeString(exception.bstrSource);
            SysFreeString(exception.bstrHelpFile);
            return false;
        }
        *result = putValue ? QVariant() : fromVariant(out);
        VariantClear(&out);
        return true;
    }

    bool m_initialized = false;
    RetryMessageFilter* m_filter = nullptr;
    IMessageFilter* m_previousFilter = nullptr;
    // ComPtr overloads operator& to release what it holds, and QHash takes
    // the address of its values, so the pointer lives inside a plain struct.
    struct Held { ComPtr<IDispatch> object; };
    QHash<QString, Held> m_objects;
    quint64 m_nextHandle = 0;
};

} // namespace

std::unique_ptr<ComBridge> ComBridge::create()
{
    return std::make_unique<WindowsComBridge>();
}

} // namespace PN::Plugins
