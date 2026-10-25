// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginManifest.h"
#include "PublishedSchema.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace PN::Plugins {

QString currentPlatform()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    return QStringLiteral("linux");
#endif
}

namespace {

QStringList stringList(const QJsonValue& value)
{
    QStringList out;
    for (const QJsonValue& item : value.toArray())
        if (item.isString())
            out.append(item.toString());
    return out;
}

bool validId(const QString& id)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{1,127}$"));
    return pattern.match(id).hasMatch() && !id.contains(QStringLiteral(".."));
}

// "data.read:contacts,companies" -> {"contacts", "companies"}; "*" = all.
bool entityGranted(const QStringList& permissions, const QString& prefix, const QString& entity)
{
    for (const QString& permission : permissions) {
        if (!permission.startsWith(prefix + QLatin1Char(':')))
            continue;
        const QStringList entities = permission.mid(prefix.size() + 1).split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& e : entities)
            if (e.trimmed() == QLatin1String("*") || e.trimmed() == entity)
                return true;
    }
    return false;
}

} // namespace

bool PluginManifest::load(const QString& folder, PluginManifest* out, QString* error)
{
    QFile file(QDir(folder).filePath(QStringLiteral("plugin.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("plugin.json is missing.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        *error = QStringLiteral("plugin.json is not valid JSON: %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject json = document.object();

    PluginManifest m;
    m.folder = QDir(folder).absolutePath();
    m.id = json.value(QStringLiteral("id")).toString();
    m.name = json.value(QStringLiteral("name")).toString();
    m.version = json.value(QStringLiteral("version")).toString();
    m.apiVersion = json.value(QStringLiteral("apiVersion")).toInt();
    m.description = json.value(QStringLiteral("description")).toString();
    m.author = json.value(QStringLiteral("author")).toString();
    m.homepage = json.value(QStringLiteral("homepage")).toString();
    m.minAppVersion = json.value(QStringLiteral("minAppVersion")).toString();
    m.platforms = stringList(json.value(QStringLiteral("platforms")));
    m.main = json.value(QStringLiteral("main")).toString(QStringLiteral("main.mjs"));
    m.permissions = stringList(json.value(QStringLiteral("permissions")));
    m.httpHosts = stringList(json.value(QStringLiteral("http")));
    m.comObjects = stringList(json.value(QStringLiteral("com")));
    m.office365Scopes = stringList(json.value(QStringLiteral("office365")).toObject().value(QStringLiteral("scopes")));

    if (!validId(m.id)) {
        *error = QStringLiteral("\"id\" must be 2-128 letters, digits, dots, dashes or underscores.");
        return false;
    }
    if (m.name.isEmpty()) {
        *error = QStringLiteral("\"name\" is required.");
        return false;
    }
    if (m.apiVersion != kSupportedApiVersion) {
        *error = QStringLiteral("apiVersion %1 is not supported (this version of Project Notes supports %2).")
                     .arg(m.apiVersion).arg(kSupportedApiVersion);
        return false;
    }
    const QString mainPath = QDir(m.folder).filePath(m.main);
    if (QDir::isAbsolutePath(m.main) || m.main.contains(QStringLiteral("..")) || !QFileInfo::exists(mainPath)) {
        *error = QStringLiteral("\"main\" must name a module inside the plugin folder (%1).").arg(m.main);
        return false;
    }

    const PublishedSchema& schema = PublishedSchema::instance();
    for (const QString& permission : std::as_const(m.permissions)) {
        if (!permission.startsWith(QStringLiteral("data.")))
            continue;
        const QStringList entities = permission.section(QLatin1Char(':'), 1).split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString& entity : entities) {
            const QString e = entity.trimmed();
            if (e == QLatin1String("*"))
                continue;
            if (schema.isRetiredName(e) && !schema.isEntity(e)) {
                *error = QStringLiteral("Permission \"%1\" uses the stored name \"%2\"; use \"%3\".")
                             .arg(permission, e, schema.publishedEntity(e));
                return false;
            }
        }
    }

    for (const QJsonValue& value : json.value(QStringLiteral("legacySettings")).toArray()) {
        if (value.isString())
            m.legacySettings.append({value.toString(), QString()});
        else
            m.legacySettings.append({value.toObject().value(QStringLiteral("group")).toString(),
                                     value.toObject().value(QStringLiteral("prefix")).toString()});
    }

    const QJsonObject contributes = json.value(QStringLiteral("contributes")).toObject();
    for (const QJsonValue& value : contributes.value(QStringLiteral("menus")).toArray()) {
        const QJsonObject o = value.toObject();
        MenuContribution menu;
        menu.id = o.value(QStringLiteral("id")).toString();
        menu.title = o.value(QStringLiteral("title")).toString();
        menu.context = o.value(QStringLiteral("context")).toString(QStringLiteral("global"));
        menu.entity = o.value(QStringLiteral("entity")).toString(o.value(QStringLiteral("table")).toString());
        menu.submenu = o.value(QStringLiteral("submenu")).toString();
        menu.include = stringList(o.value(QStringLiteral("include")));
        menu.parameter = o.value(QStringLiteral("parameter")).toString();
        menu.platforms = stringList(o.value(QStringLiteral("platforms")));
        if (menu.id.isEmpty() || menu.title.isEmpty()) {
            *error = QStringLiteral("Every menu needs an \"id\" and a \"title\".");
            return false;
        }
        if (menu.context == QLatin1String("record")) {
            if (menu.entity.isEmpty()) {
                *error = QStringLiteral("Record menu \"%1\" needs an \"entity\".").arg(menu.id);
                return false;
            }
            if (!schema.isEntity(menu.entity)) {
                *error = schema.isRetiredName(menu.entity)
                    ? QStringLiteral("Menu \"%1\" uses the stored name \"%2\"; use \"%3\".")
                          .arg(menu.id, menu.entity, schema.publishedEntity(menu.entity))
                    : QStringLiteral("Menu \"%1\" names unknown entity \"%2\".").arg(menu.id, menu.entity);
                return false;
            }
        } else if (menu.context != QLatin1String("global")) {
            *error = QStringLiteral("Menu \"%1\" context must be \"record\" or \"global\".").arg(menu.id);
            return false;
        }
        m.menus.append(menu);
    }
    for (const QJsonValue& value : contributes.value(QStringLiteral("settings")).toArray()) {
        const QJsonObject o = value.toObject();
        SettingsContribution section;
        section.id = o.value(QStringLiteral("id")).toString();
        section.title = o.value(QStringLiteral("title")).toString(m.name);
        section.schema = o.value(QStringLiteral("schema")).toString();
        section.qml = o.value(QStringLiteral("qml")).toString();
        if (section.id.isEmpty() || (section.schema.isEmpty() == section.qml.isEmpty())) {
            *error = QStringLiteral("Every settings section needs an \"id\" and exactly one of \"schema\" or \"qml\".");
            return false;
        }
        m.settings.append(section);
    }

    *out = m;
    return true;
}

bool PluginManifest::supportsPlatform(const QString& platform) const
{
    return platforms.isEmpty() || platforms.contains(platform);
}

bool PluginManifest::shipsQml() const
{
    QDirIterator it(folder, {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext();
}

QString PluginManifest::approvalFingerprint() const
{
    QStringList parts;
    QStringList sorted = permissions; sorted.sort(); parts << sorted.join(QLatin1Char('|'));
    sorted = httpHosts; sorted.sort(); parts << sorted.join(QLatin1Char('|'));
    sorted = comObjects; sorted.sort(); parts << sorted.join(QLatin1Char('|'));
    sorted = office365Scopes; sorted.sort(); parts << sorted.join(QLatin1Char('|'));
    parts << (shipsQml() ? QStringLiteral("qml") : QString());
    return QString::fromLatin1(QCryptographicHash::hash(parts.join(QLatin1Char('\n')).toUtf8(),
                                                        QCryptographicHash::Sha256).toHex());
}

bool PluginManifest::hasPermission(const QString& permission) const
{
    return permissions.contains(permission);
}

bool PluginManifest::canRead(const QString& entity) const
{
    return entityGranted(permissions, QStringLiteral("data.read"), entity)
        || entityGranted(permissions, QStringLiteral("data.write"), entity);
}

bool PluginManifest::canWrite(const QString& entity) const
{
    return entityGranted(permissions, QStringLiteral("data.write"), entity);
}

bool PluginManifest::allowsHost(const QString& host) const
{
    const QString h = host.toLower();
    for (const QString& pattern : httpHosts) {
        const QString p = pattern.toLower();
        if (p == h)
            return true;
        if (p.startsWith(QStringLiteral("*.")) && (h.endsWith(p.mid(1)) || h == p.mid(2)))
            return true;
    }
    return false;
}

bool PluginManifest::allowsComObject(const QString& progIdOrMoniker) const
{
    for (const QString& allowed : comObjects) {
        if (allowed.compare(progIdOrMoniker, Qt::CaseInsensitive) == 0)
            return true;
        // A moniker entry such as "winmgmts:" covers every moniker with that scheme.
        if (allowed.endsWith(QLatin1Char(':')) && progIdOrMoniker.startsWith(allowed, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace PN::Plugins
