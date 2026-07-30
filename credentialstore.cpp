// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "credentialstore.h"

#include <QByteArray>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <wincred.h>
#elif defined(Q_OS_DARWIN)
#include <Security/Security.h>
#elif defined(Q_OS_LINUX)
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QMap>
#include <QVariantMap>
#endif

#if defined(Q_OS_LINUX)
struct SecretServiceSecret
{
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

QDBusArgument& operator<<(QDBusArgument& argument, const SecretServiceSecret& secret)
{
    argument.beginStructure();
    argument << secret.session << secret.parameters << secret.value << secret.contentType;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, SecretServiceSecret& secret)
{
    argument.beginStructure();
    argument >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(SecretServiceSecret)
#endif

namespace {
QString storeKey(const QString& service, const QString& account)
{
    return QStringLiteral("com.projectnotespro.ProjectNotes/%1/%2").arg(service, account);
}

#if defined(Q_OS_LINUX)
const char kSecretService[] = "org.freedesktop.secrets";
const char kServicePath[] = "/org/freedesktop/secrets";

struct SecretSession
{
    QDBusObjectPath path;
    QString error;
};

void ensureSecretServiceTypesRegistered()
{
    static const QMetaType attributesType = qDBusRegisterMetaType<QMap<QString, QString>>();
    static const QMetaType secretType = qDBusRegisterMetaType<SecretServiceSecret>();
    Q_UNUSED(attributesType);
    Q_UNUSED(secretType);
}

SecretSession openSession()
{
    ensureSecretServiceTypesRegistered();
    QDBusInterface service(kSecretService, kServicePath,
                           "org.freedesktop.Secret.Service", QDBusConnection::sessionBus());
    QDBusMessage reply = service.call("OpenSession", QStringLiteral("plain"),
                                      QVariant::fromValue(QDBusVariant(QString())));
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() < 2)
        return {{}, reply.errorMessage().isEmpty() ? QStringLiteral("Secret Service is unavailable.") : reply.errorMessage()};
    return {qvariant_cast<QDBusObjectPath>(reply.arguments().at(1)), {}};
}

QMap<QString, QString> secretAttributes(const QString& service, const QString& account)
{
    return {{QStringLiteral("application"), QStringLiteral("Project Notes")},
            {QStringLiteral("service"), service},
            {QStringLiteral("account"), account}};
}

QList<QDBusObjectPath> searchUnlocked(const QMap<QString, QString>& attributes, QString* error)
{
    QDBusInterface service(kSecretService, kServicePath,
                           "org.freedesktop.Secret.Service", QDBusConnection::sessionBus());
    QDBusMessage reply = service.call("SearchItems", QVariant::fromValue(attributes));
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() < 2)
    {
        if (error) *error = reply.errorMessage();
        return {};
    }
    const auto unlocked = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().at(0));
    const auto locked = qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().at(1));
    if (unlocked.isEmpty() && !locked.isEmpty() && error)
        *error = QStringLiteral("The desktop keyring is locked; unlock it and try again.");
    return unlocked;
}

QVariant makeSecret(const QDBusObjectPath& session, const QByteArray& value)
{
    return QVariant::fromValue(SecretServiceSecret{
        session, {}, value, QStringLiteral("text/plain; charset=utf-8")});
}

QByteArray readSecretValue(const QVariant& value)
{
    return qdbus_cast<SecretServiceSecret>(value).value;
}
#endif
}

QString CredentialStore::read(const QString& service, const QString& account, QString* error)
{
    if (error) error->clear();
#if defined(Q_OS_WIN)
    PCREDENTIALW credential = nullptr;
    const std::wstring target = storeKey(service, account).toStdWString();
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential))
    {
        if (GetLastError() != ERROR_NOT_FOUND && error)
            *error = QStringLiteral("Windows Credential Manager error %1.").arg(GetLastError());
        return {};
    }
    const QString result = QString::fromUtf8(
        reinterpret_cast<const char*>(credential->CredentialBlob), credential->CredentialBlobSize);
    CredFree(credential);
    return result;
#elif defined(Q_OS_DARWIN)
    const QByteArray serviceUtf8 = service.toUtf8();
    const QByteArray accountUtf8 = account.toUtf8();
    const void* keys[] = {kSecClass, kSecAttrService, kSecAttrAccount, kSecReturnData, kSecMatchLimit};
    const void* values[] = {kSecClassGenericPassword,
                            CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(serviceUtf8.constData()), serviceUtf8.size(), kCFStringEncodingUTF8, false),
                            CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(accountUtf8.constData()), accountUtf8.size(), kCFStringEncodingUTF8, false),
                            kCFBooleanTrue, kSecMatchLimitOne};
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 5, nullptr, nullptr);
    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(query, &result);
    CFRelease(query);
    CFRelease(values[1]);
    CFRelease(values[2]);
    if (status == errSecItemNotFound) return {};
    if (status != errSecSuccess)
    {
        if (error) *error = QStringLiteral("macOS Keychain error %1.").arg(status);
        return {};
    }
    CFDataRef data = static_cast<CFDataRef>(result);
    QString secret = QString::fromUtf8(reinterpret_cast<const char*>(CFDataGetBytePtr(data)), CFDataGetLength(data));
    CFRelease(result);
    return secret;
#elif defined(Q_OS_LINUX)
    SecretSession session = openSession();
    if (!session.error.isEmpty()) { if (error) *error = session.error; return {}; }
    QString searchError;
    const auto items = searchUnlocked(secretAttributes(service, account), &searchError);
    if (items.isEmpty()) { if (error) *error = searchError; return {}; }
    QDBusInterface item(kSecretService, items.first().path(), "org.freedesktop.Secret.Item",
                        QDBusConnection::sessionBus());
    QDBusMessage reply = item.call("GetSecret", QVariant::fromValue(session.path));
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
    {
        if (error) *error = reply.errorMessage();
        return {};
    }
    return QString::fromUtf8(readSecretValue(reply.arguments().first()));
#else
    if (error) *error = QStringLiteral("Secure credential storage is not supported on this platform.");
    return {};
#endif
}

bool CredentialStore::write(const QString& service, const QString& account, const QString& secret, QString* error)
{
    if (error) error->clear();
#if defined(Q_OS_WIN)
    const QByteArray bytes = secret.toUtf8();
    const std::wstring target = storeKey(service, account).toStdWString();
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(target.c_str());
    credential.CredentialBlobSize = bytes.size();
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(bytes.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<LPWSTR>(L"Project Notes");
    if (!CredWriteW(&credential, 0))
    {
        if (error) *error = QStringLiteral("Windows Credential Manager error %1.").arg(GetLastError());
        return false;
    }
    return true;
#elif defined(Q_OS_DARWIN)
    QString ignored;
    remove(service, account, &ignored);
    const QByteArray s = service.toUtf8(), a = account.toUtf8(), value = secret.toUtf8();
    OSStatus status = SecKeychainAddGenericPassword(nullptr, s.size(), s.constData(), a.size(), a.constData(),
                                                     value.size(), value.constData(), nullptr);
    if (status != errSecSuccess && error) *error = QStringLiteral("macOS Keychain error %1.").arg(status);
    return status == errSecSuccess;
#elif defined(Q_OS_LINUX)
    SecretSession session = openSession();
    if (!session.error.isEmpty()) { if (error) *error = session.error; return false; }
    QVariantMap properties;
    properties.insert(QStringLiteral("org.freedesktop.Secret.Item.Label"), QStringLiteral("Project Notes: %1").arg(service));
    properties.insert(QStringLiteral("org.freedesktop.Secret.Item.Attributes"),
                      QVariant::fromValue(secretAttributes(service, account)));
    QDBusInterface collection(kSecretService, "/org/freedesktop/secrets/aliases/default",
                              "org.freedesktop.Secret.Collection", QDBusConnection::sessionBus());
    QDBusMessage reply = collection.call("CreateItem", properties, makeSecret(session.path, secret.toUtf8()), true);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() < 2)
    {
        if (error) *error = reply.errorMessage();
        return false;
    }
    const QDBusObjectPath itemPath = qvariant_cast<QDBusObjectPath>(reply.arguments().at(0));
    const QDBusObjectPath promptPath = qvariant_cast<QDBusObjectPath>(reply.arguments().at(1));
    if (itemPath.path() == QStringLiteral("/") || promptPath.path() != QStringLiteral("/"))
    {
        if (error) *error = QStringLiteral("Unlock the desktop keyring and try again.");
        return false;
    }
    return true;
#else
    Q_UNUSED(service);
    Q_UNUSED(account);
    Q_UNUSED(secret);
    if (error) *error = QStringLiteral("Secure credential storage is not supported on this platform.");
    return false;
#endif
}

bool CredentialStore::remove(const QString& service, const QString& account, QString* error)
{
    if (error) error->clear();
#if defined(Q_OS_WIN)
    const std::wstring target = storeKey(service, account).toStdWString();
    if (CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) || GetLastError() == ERROR_NOT_FOUND) return true;
    if (error) *error = QStringLiteral("Windows Credential Manager error %1.").arg(GetLastError());
    return false;
#elif defined(Q_OS_DARWIN)
    const QByteArray s = service.toUtf8(), a = account.toUtf8();
    SecKeychainItemRef item = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(nullptr, s.size(), s.constData(), a.size(), a.constData(), nullptr, nullptr, &item);
    if (status == errSecItemNotFound) return true;
    if (status == errSecSuccess) status = SecKeychainItemDelete(item);
    if (item) CFRelease(item);
    if (status != errSecSuccess && error) *error = QStringLiteral("macOS Keychain error %1.").arg(status);
    return status == errSecSuccess;
#elif defined(Q_OS_LINUX)
    QString searchError;
    const auto items = searchUnlocked(secretAttributes(service, account), &searchError);
    if (items.isEmpty()) { if (error) *error = searchError; return searchError.isEmpty(); }
    bool ok = true;
    for (const auto& path : items)
    {
        QDBusInterface item(kSecretService, path.path(), "org.freedesktop.Secret.Item", QDBusConnection::sessionBus());
        QDBusMessage reply = item.call("Delete");
        if (reply.type() == QDBusMessage::ErrorMessage) { ok = false; if (error) *error = reply.errorMessage(); }
        else if (!reply.arguments().isEmpty() &&
                 qvariant_cast<QDBusObjectPath>(reply.arguments().first()).path() != QStringLiteral("/"))
        {
            ok = false;
            if (error) *error = QStringLiteral("Unlock the desktop keyring and try again.");
        }
    }
    return ok;
#else
    Q_UNUSED(service);
    Q_UNUSED(account);
    if (error) *error = QStringLiteral("Secure credential storage is not supported on this platform.");
    return false;
#endif
}
