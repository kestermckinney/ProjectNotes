// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DesktopPluginServices.h"

#include "DesktopAppController.h"
#include "PluginUiController.h"

#include "ProjectNotesPlugins/PluginManifest.h"
#include "ProjectNotesPlugins/PublishedSchema.h"
#include "ProjectNotesContacts/ContactSyncSettingsModel.h"
#include "ProjectNotesContacts/VCard.h"
#include "personvcard.h"
#include "ProjectNotesIntegrations/HttpTransport.h"
#include "ProjectNotesIntegrations/Office365Service.h"

#include "credentialstore.h"
#include "databaseobjects.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QProcess>
#include <QSqlQuery>
#include <QUrlQuery>
#include <QUuid>

using PN::Plugins::PluginError;
using PN::Plugins::PluginManifest;
using PN::Plugins::Reply;

namespace {

QVariant arg(const QVariantList& args, int i)
{
    return i < args.size() ? args.at(i) : QVariant();
}

PluginError unavailable(const QString& what)
{
    return PluginError::make(QStringLiteral("unavailable"), what);
}

} // namespace

DesktopPluginServices::DesktopPluginServices(DesktopAppController* app, PluginUiController* ui, QObject* parent)
    : PluginServices(parent), m_app(app), m_ui(ui)
{
}

QString DesktopPluginServices::secretService(const QString& pluginId)
{
    return QStringLiteral("ProjectNotes plugin ") + pluginId;
}

QStringList DesktopPluginServices::capabilities() const
{
    QStringList caps {QStringLiteral("ui.dialogs"), QStringLiteral("clipboard"), QStringLiteral("email.compose"),
                      QStringLiteral("calendar.draft"), QStringLiteral("contacts.pick"), QStringLiteral("contacts.sync"),
                      QStringLiteral("office365"), QStringLiteral("secrets"), QStringLiteral("app.navigate"),
                      QStringLiteral("projectFiles")};
#ifndef PN_APP_STORE
    caps << QStringLiteral("process.launch");
#endif
    return caps;
}

void DesktopPluginServices::setCurrentView(const QVariantMap& view)
{
    {
        QMutexLocker locker(&m_viewMutex);
        if (m_view == view)
            return;
        m_view = view;
    }
    emit viewChanged(view);
}

QVariantMap DesktopPluginServices::currentView() const
{
    QMutexLocker locker(&m_viewMutex);
    return m_view;
}

void DesktopPluginServices::request(const PluginManifest& plugin, const QString& method,
                                    const QVariantList& args, Reply reply)
{
    if (method.startsWith(QLatin1String("ui."))) {
        requestUi(plugin, method, args, std::move(reply));
        return;
    }
    if (method.startsWith(QLatin1String("app.")) || method.startsWith(QLatin1String("files."))) {
        requestApp(plugin, method, args, std::move(reply));
        return;
    }
    if (method.startsWith(QLatin1String("secrets."))) {
        QString error;
        const QString service = secretService(plugin.id);
        const QString account = arg(args, 0).toString();
        if (method == QLatin1String("secrets.get")) {
            const QString secret = CredentialStore::read(service, account, &error);
            if (!error.isEmpty())
                reply({}, PluginError::make(QStringLiteral("secrets-failed"), error));
            else
                reply(secret.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(secret), {});
        } else if (method == QLatin1String("secrets.set")) {
            if (CredentialStore::write(service, account, arg(args, 1).toString(), &error))
                reply(QVariant(), {});
            else
                reply({}, PluginError::make(QStringLiteral("secrets-failed"), error));
        } else {
            if (CredentialStore::remove(service, account, &error))
                reply(QVariant(), {});
            else
                reply({}, PluginError::make(QStringLiteral("secrets-failed"), error));
        }
        return;
    }
    if (method.startsWith(QLatin1String("email."))) {
        if (!m_app) { reply({}, unavailable(tr("Email is not available."))); return; }
        if (method == QLatin1String("email.compose")) {
            m_app->composePluginEmail(plugin, arg(args, 0).toMap(), std::move(reply));
        } else if (method == QLatin1String("email.backends")) {
            reply(m_app->pluginEmailBackends(), {});
        } else if (method == QLatin1String("email.audiences")) {
            reply(m_app->pluginEmailAudiences(arg(args, 0).toString(), arg(args, 1).toString()), {});
        } else if (method == QLatin1String("email.resolveAudience")) {
            m_app->resolvePluginAudience(arg(args, 0).toString(), arg(args, 1).toString(), std::move(reply));
        } else if (method == QLatin1String("email.chooseRecipients")) {
            if (!m_ui) { reply({}, unavailable(tr("Dialogs are not available."))); return; }
            m_app->choosePluginRecipients(plugin, arg(args, 0).toString(), arg(args, 1).toMap(), std::move(reply));
        } else if (method == QLatin1String("email.templates")) {
            reply(m_app->pluginEmailTemplates(), {});
        } else if (method == QLatin1String("email.renderTemplate")) {
            m_app->renderPluginEmailTemplate(arg(args, 0).toString(), arg(args, 1).toMap(), std::move(reply));
        } else {
            reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
        }
        return;
    }
    if (method.startsWith(QLatin1String("calendar."))) {
        if (!m_app) { reply({}, unavailable(tr("Calendar is not available."))); return; }
        if (method == QLatin1String("calendar.draftMeeting")) {
            m_app->draftPluginMeeting(plugin, arg(args, 0).toMap(), std::move(reply));
        } else if (method == QLatin1String("calendar.backends")) {
            reply(m_app->pluginEmailBackends(), {});
        } else if (method == QLatin1String("calendar.writeIcs")) {
            PluginError error;
            const QString path = m_app->writePluginIcs(arg(args, 0).toMap(), arg(args, 1).toString(), &error);
            reply(path, error);
        } else {
            reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
        }
        return;
    }
    if (method.startsWith(QLatin1String("contacts."))) {
        requestContacts(plugin, method, args, std::move(reply));
        return;
    }
    if (method.startsWith(QLatin1String("office365."))) {
        requestOffice365(plugin, method, args, std::move(reply));
        return;
    }
    if (method.startsWith(QLatin1String("projectFiles."))) {
        requestProjectFiles(method, args, std::move(reply));
        return;
    }
    if (method == QLatin1String("process.launch")) {
#ifdef PN_APP_STORE
        reply({}, unavailable(tr("Launching programs is not available in this build.")));
#else
        qint64 pid = 0;
        const QString program = arg(args, 0).toString();
        const QStringList programArgs = arg(args, 1).toStringList();
        const QString workingDirectory = arg(args, 2).toMap().value(QStringLiteral("workingDirectory")).toString();
#ifdef Q_OS_MACOS
        // An application bundle is a folder; open it rather than exec it.
        if (program.endsWith(QLatin1String(".app")) && QFileInfo(program).isDir()) {
            const bool ok = QProcess::startDetached(QStringLiteral("open"), QStringList{program} + (programArgs.isEmpty()
                                                    ? QStringList() : QStringList{QStringLiteral("--args")} + programArgs),
                                                    workingDirectory, &pid);
            reply(ok ? QVariant(QVariantMap{{QStringLiteral("pid"), pid}}) : QVariant(),
                  ok ? PluginError() : PluginError::make(QStringLiteral("launch-failed"), tr("Could not start %1.").arg(program)));
            return;
        }
#endif
        const bool ok = QProcess::startDetached(program, programArgs, workingDirectory, &pid);
        reply(ok ? QVariant(QVariantMap{{QStringLiteral("pid"), pid}}) : QVariant(),
              ok ? PluginError() : PluginError::make(QStringLiteral("launch-failed"), tr("Could not start %1.").arg(program)));
#endif
        return;
    }
    if (method == QLatin1String("reports.generate")) {
        reply({}, unavailable(tr("reports.generate is planned for a later version of the plugin API.")));
        return;
    }
    reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
}

void DesktopPluginServices::requestUi(const PluginManifest& plugin, const QString& method,
                                      const QVariantList& args, Reply reply)
{
    if (!m_ui) {
        reply({}, unavailable(tr("Dialogs are not available.")));
        return;
    }
    const QString op = method.mid(3);
    const QVariantMap options = arg(args, 1).toMap();

    if (op == QLatin1String("message")) {
        m_ui->openRequest(plugin.id, QStringLiteral("message"),
                          {{QStringLiteral("text"), arg(args, 0)}, {QStringLiteral("title"), options.value(QStringLiteral("title"), plugin.name)},
                           {QStringLiteral("kind"), options.value(QStringLiteral("kind"), QStringLiteral("info"))}},
                          [reply](const QVariant&, const PluginError& e) { reply(QVariant(), e); });
    } else if (op == QLatin1String("confirm")) {
        m_ui->openRequest(plugin.id, QStringLiteral("confirm"),
                          {{QStringLiteral("text"), arg(args, 0)}, {QStringLiteral("title"), options.value(QStringLiteral("title"), plugin.name)},
                           {QStringLiteral("okText"), options.value(QStringLiteral("okText"), tr("OK"))},
                           {QStringLiteral("cancelText"), options.value(QStringLiteral("cancelText"), tr("Cancel"))},
                           {QStringLiteral("destructive"), options.value(QStringLiteral("destructive"), false)}},
                          [reply](const QVariant& v, const PluginError& e) { reply(v.toBool(), e); });
    } else if (op == QLatin1String("prompt")) {
        QVariantMap params = options;
        params.insert(QStringLiteral("label"), arg(args, 0));
        params.insert(QStringLiteral("title"), options.value(QStringLiteral("title"), plugin.name));
        m_ui->openRequest(plugin.id, QStringLiteral("prompt"), params, reply);
    } else if (op == QLatin1String("choose")) {
        QVariantMap params = arg(args, 2).toMap();
        params.insert(QStringLiteral("label"), arg(args, 0));
        params.insert(QStringLiteral("choices"), arg(args, 1));
        params.insert(QStringLiteral("title"), params.value(QStringLiteral("title"), plugin.name));
        m_ui->openRequest(plugin.id, QStringLiteral("choose"), params, reply);
    } else if (op == QLatin1String("showForm")) {
        QVariantMap params = arg(args, 2).toMap();
        params.insert(QStringLiteral("form"), arg(args, 0));
        params.insert(QStringLiteral("values"), arg(args, 1));
        params.insert(QStringLiteral("title"), params.value(QStringLiteral("title"), plugin.name));
        m_ui->openRequest(plugin.id, QStringLiteral("form"), params, reply);
    } else if (op == QLatin1String("showDialog")) {
        const QString relative = arg(args, 0).toString();
        const QString path = QDir::cleanPath(QDir(plugin.folder).filePath(relative));
        if (QDir::isAbsolutePath(relative) || !path.startsWith(QDir::cleanPath(plugin.folder) + QLatin1Char('/'))
            || !QFileInfo::exists(path)) {
            reply({}, PluginError::make(QStringLiteral("not-found"), tr("%1 is not a QML file in the plugin folder.").arg(relative)));
            return;
        }
        QVariantMap params = arg(args, 2).toMap();
        params.insert(QStringLiteral("qmlPath"), path);
        params.insert(QStringLiteral("data"), arg(args, 1));
        params.insert(QStringLiteral("title"), params.value(QStringLiteral("title"), plugin.name));
        m_ui->openRequest(plugin.id, QStringLiteral("dialog"), params, reply);
    } else if (op == QLatin1String("pickFile") || op == QLatin1String("pickFolder") || op == QLatin1String("saveFile")) {
        m_ui->openRequest(plugin.id, op, arg(args, 0).toMap(), reply);
    } else if (op == QLatin1String("notify")) {
        m_ui->notify(arg(args, 0).toString(), options.value(QStringLiteral("kind"), QStringLiteral("info")).toString(),
                     options.value(QStringLiteral("seconds"), 4).toInt());
        reply(QVariant(), {});
    } else if (op == QLatin1String("progressBegin")) {
        const QString token = QUuid::createUuid().toString(QUuid::Id128);
        QVariantMap params = arg(args, 1).toMap();
        params.insert(QStringLiteral("title"), arg(args, 0));
        params.insert(QStringLiteral("token"), token);
        const int id = m_ui->openRequest(plugin.id, QStringLiteral("progress"), params, [](const QVariant&, const PluginError&) {});
        m_progressRequests.insert(token, id);
        reply(token, {});
    } else if (op == QLatin1String("progressUpdate")) {
        m_ui->updateProgress(arg(args, 0).toString(), arg(args, 1).toDouble(), arg(args, 2).toString());
        reply(QVariant(), {});
    } else if (op == QLatin1String("progressEnd")) {
        const int id = m_progressRequests.take(arg(args, 0).toString());
        if (id)
            m_ui->closeRequest(id);
        reply(QVariant(), {});
    } else if (op == QLatin1String("openUrl")) {
        const QUrl url(arg(args, 0).toString());
        const QString scheme = url.scheme().toLower();
        // Other schemes can start applications, so they need the "urls" permission.
        if (scheme != QLatin1String("http") && scheme != QLatin1String("https") && scheme != QLatin1String("mailto")
            && !plugin.hasPermission(QStringLiteral("urls"))) {
            PluginError error = PluginError::make(QStringLiteral("permission-denied"),
                tr("Opening %1: URLs needs the \"urls\" permission.").arg(scheme));
            error.permission = QStringLiteral("urls");
            reply({}, error);
            return;
        }
        reply(QDesktopServices::openUrl(url), {});
    } else if (op == QLatin1String("clipboardSetText")) {
        QGuiApplication::clipboard()->setText(arg(args, 0).toString());
        reply(QVariant(), {});
    } else if (op == QLatin1String("clipboardText")) {
        reply(QGuiApplication::clipboard()->text(), {});
    } else {
        reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
    }
}

void DesktopPluginServices::requestApp(const PluginManifest& plugin, const QString& method,
                                       const QVariantList& args, Reply reply)
{
    Q_UNUSED(plugin);
    if (method == QLatin1String("app.navigate")) {
        if (!m_app) { reply({}, unavailable(tr("Navigation is not available."))); return; }
        PluginError error;
        m_app->navigateForPlugin(arg(args, 0).toString(), arg(args, 1).toString(),
                                 arg(args, 2).toMap().value(QStringLiteral("tab")).toString(), &error);
        reply(QVariant(), error);
    } else if (method == QLatin1String("app.projectFolder")) {
        // The project's primary project folder; a project may have none.
        QSqlQuery query(global_DBObjects.getDb());
        query.prepare(QStringLiteral(
            "SELECT l.full_path FROM projects p JOIN project_locations l ON l.id = p.primary_location_id "
            "WHERE p.id=? AND l.deleted=0 AND l.location_type='File Folder' "
            "AND trim(coalesce(l.full_path,'')) <> ''"));
        query.addBindValue(arg(args, 0).toString());
        if (query.exec() && query.next())
            reply(query.value(0).toString().trimmed(), {});
        else
            reply(QVariant::fromValue(nullptr), {});
    } else if (method == QLatin1String("app.defaults")) {
        reply(m_app ? m_app->pluginDefaults() : QVariantMap(), {});
    } else if (method == QLatin1String("app.refresh")) {
        if (m_app)
            m_app->refreshViewsForPlugin();
        reply(QVariant(), {});
    } else if (method == QLatin1String("files.openWithSystem")) {
        reply(QDesktopServices::openUrl(QUrl::fromLocalFile(arg(args, 0).toString())), {});
    } else if (method == QLatin1String("files.reveal")) {
        const QFileInfo info(arg(args, 0).toString());
        const QString folder = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        reply(QVariant(), {});
    } else {
        reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
    }
}

void DesktopPluginServices::requestProjectFiles(const QString& method, const QVariantList& args, Reply reply)
{
    PN::Comm::ProjectFolderStore* store = m_app ? m_app->projectFolderStore() : nullptr;
    if (!m_app || !store) {
        reply({}, unavailable(tr("Project folders are not available.")));
        return;
    }
    const QString projectKey = arg(args, 0).toString();
    const bool preferLocal = method == QLatin1String("projectFiles.location")
        && arg(args, 1).toMap().value(QStringLiteral("preferLocal")).toBool();
    const PN::Comm::ProjectFolder folder = m_app->projectFolderForFiles(projectKey, preferLocal);
    if (method == QLatin1String("projectFiles.location")) {
        if (!folder.isValid()) {
            reply(QVariant::fromValue(nullptr), {});
            return;
        }
        reply(QVariantMap{{QStringLiteral("kind"), folder.kindName()}, {QStringLiteral("path"), folder.path}}, {});
        return;
    }
    if (!folder.isValid()) {
        reply({}, PluginError::make(QStringLiteral("no-project-folder"),
                                    tr("The project has no project folder. Set one on the project's Files & Folders tab.")));
        return;
    }
    // Errors from the store keep their codes ("exists", "teams-access-denied", ...).
    auto failed = [](const PN::Comm::ServiceError& error) {
        return PluginError::make(error.code.isEmpty() ? QStringLiteral("io-error") : error.code, error.displayText);
    };
    const QString path = arg(args, 1).toString();
    if (method == QLatin1String("projectFiles.list")) {
        store->list(folder, path, [reply, failed](QList<PN::Comm::ProjectFileEntry> entries, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) { reply({}, failed(error)); return; }
            QVariantList out;
            for (const PN::Comm::ProjectFileEntry& entry : entries)
                out.append(QVariantMap{{QStringLiteral("name"), entry.name}, {QStringLiteral("isDir"), entry.isDir},
                                       {QStringLiteral("size"), entry.size}, {QStringLiteral("location"), entry.location}});
            reply(out, {});
        });
    } else if (method == QLatin1String("projectFiles.exists")) {
        store->exists(folder, path, [reply, failed](bool found, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) reply({}, failed(error));
            else reply(found, {});
        });
    } else if (method == QLatin1String("projectFiles.mkdir")) {
        store->mkdir(folder, path, [reply, failed](QString location, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) reply({}, failed(error));
            else reply(location, {});
        });
    } else if (method == QLatin1String("projectFiles.readText")) {
        store->read(folder, path, [reply, failed](std::optional<QByteArray> bytes, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) reply({}, failed(error));
            else reply(bytes ? QVariant(QString::fromUtf8(*bytes)) : QVariant::fromValue(nullptr), {});
        });
    } else if (method == QLatin1String("projectFiles.writeText")) {
        const QVariantMap options = arg(args, 3).toMap();
        store->write(folder, path, arg(args, 2).toString().toUtf8(),
                     options.value(QStringLiteral("overwrite"), true).toBool(),
                     [reply, failed](QString location, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) reply({}, failed(error));
            else reply(location, {});
        });
    } else if (method == QLatin1String("projectFiles.upload")) {
        // args: projectKey, localPath (checked by the plugin runtime), path, options
        QFile source(arg(args, 1).toString());
        if (!source.open(QIODevice::ReadOnly)) {
            reply({}, PluginError::make(QStringLiteral("io-error"),
                                        tr("Could not read %1: %2").arg(source.fileName(), source.errorString())));
            return;
        }
        const QByteArray bytes = source.readAll();
        source.close();
        const QVariantMap options = arg(args, 3).toMap();
        store->write(folder, arg(args, 2).toString(), bytes,
                     options.value(QStringLiteral("overwrite"), false).toBool(),
                     [reply, failed](QString location, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) reply({}, failed(error));
            else reply(location, {});
        });
    } else if (method == QLatin1String("projectFiles.open")) {
        QPointer<DesktopAppController> app = m_app;
        store->location(folder, path, [reply, failed, app, path](QString location, PN::Comm::ServiceError error) {
            if (!error.code.isEmpty()) { reply({}, failed(error)); return; }
            if (location.isEmpty()) {
                reply({}, PluginError::make(QStringLiteral("not-found"), tr("%1 is not in the project folder.").arg(path)));
                return;
            }
            if (app)
                app->openLocationPath(location);
            reply(QVariant(), {});
        });
    } else {
        reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
    }
}

void DesktopPluginServices::requestContacts(const PluginManifest& plugin, const QString& method,
                                            const QVariantList& args, Reply reply)
{
    PN::Contacts::ContactSyncSettingsModel* sync = m_app ? m_app->contactSyncModel() : nullptr;
    if (method == QLatin1String("contacts.sources")) {
        if (!sync) { reply(QVariantList(), {}); return; }
        const QVariantMap config = sync->configuration();
        const QString provider = config.value(QStringLiteral("provider")).toString();
        reply(QVariantList{QVariantMap{{QStringLiteral("id"), provider}, {QStringLiteral("name"), provider},
                                       {QStringLiteral("kind"), provider},
                                       {QStringLiteral("enabled"), !provider.isEmpty() && provider != QLatin1String("none")},
                                       {QStringLiteral("lastSync"), sync->lastSuccess().isEmpty() ? QVariant::fromValue(nullptr)
                                                                                                 : QVariant(sync->lastSuccess())}}}, {});
    } else if (method == QLatin1String("contacts.syncNow")) {
        if (!sync) { reply({}, unavailable(tr("Contact synchronization is not configured."))); return; }
        // Reply once the sync finishes (busy goes false), with the final status.
        auto connection = std::make_shared<QMetaObject::Connection>();
        auto started = std::make_shared<bool>(false);
        *connection = connect(sync, &PN::Contacts::ContactSyncSettingsModel::changed, this, [sync, reply, connection, started] {
            if (sync->busy()) { *started = true; return; }
            if (!*started) return;
            QObject::disconnect(*connection);
            reply(QVariantMap{{QStringLiteral("status"), sync->status()}, {QStringLiteral("added"), 0},
                              {QStringLiteral("updated"), 0}, {QStringLiteral("conflicts"), sync->reviews().size()}}, {});
        });
        sync->syncNow();
        if (!sync->busy()) {
            // Nothing to do (not configured): report immediately.
            QObject::disconnect(*connection);
            reply(QVariantMap{{QStringLiteral("status"), sync->status()}, {QStringLiteral("added"), 0},
                              {QStringLiteral("updated"), 0}, {QStringLiteral("conflicts"), 0}}, {});
        }
    } else if (method == QLatin1String("contacts.pick")) {
        if (!m_ui) { reply({}, unavailable(tr("Dialogs are not available."))); return; }
        m_ui->openRequest(plugin.id, QStringLiteral("contactPicker"), arg(args, 0).toMap(), reply);
    } else if (method == QLatin1String("contacts.toVCard")) {
        const QByteArray card = personVCard(global_DBObjects.getDb(), arg(args, 0).toString());
        if (card.isEmpty()) {
            reply({}, PluginError::make(QStringLiteral("not-found"), tr("No contact has key \"%1\".").arg(arg(args, 0).toString())));
            return;
        }
        reply(QString::fromUtf8(card), {});
    } else if (method == QLatin1String("contacts.fromVCard")) {
        // A file may hold several cards.
        const QString text = arg(args, 0).toString();
        QVariantList out;
        const QStringList cards = text.split(QStringLiteral("END:VCARD"), Qt::SkipEmptyParts, Qt::CaseInsensitive);
        for (const QString& card : cards) {
            if (!card.contains(QStringLiteral("BEGIN:VCARD"), Qt::CaseInsensitive))
                continue;
            const PN::Contacts::Contact c = PN::Contacts::parseVCard(card + QStringLiteral("END:VCARD"), {}, {});
            QVariantMap contact {{QStringLiteral("name"), c.name}};
            if (!c.emails.isEmpty()) contact.insert(QStringLiteral("email"), c.emails.first());
            if (!c.phones.isEmpty()) contact.insert(QStringLiteral("office_phone"), c.phones.first());
            if (c.phones.size() > 1) contact.insert(QStringLiteral("cell_phone"), c.phones.at(1));
            if (!c.company.isEmpty()) contact.insert(QStringLiteral("company_id"), QVariantMap{{QStringLiteral("lookup"), c.company}});
            if (c.fields.contains(QStringLiteral("role"))) contact.insert(QStringLiteral("role"), c.fields.value(QStringLiteral("role")));
            out.append(contact);
        }
        reply(out, {});
    } else {
        reply({}, PluginError::make(QStringLiteral("not-found"), tr("Unknown method %1.").arg(method)));
    }
}

void DesktopPluginServices::requestOffice365(const PluginManifest& plugin, const QString& method,
                                             const QVariantList& args, Reply reply)
{
    PN::Comm::Office365Service* office = m_app ? m_app->office365ServiceForPlugins() : nullptr;
    if (!office) {
        reply({}, unavailable(tr("Office 365 is not configured.")));
        return;
    }
    const PN::Comm::Office365Account account = office->account();
    if (method == QLatin1String("office365.status")) {
        reply(QVariantMap{{QStringLiteral("signedIn"), account.authenticated},
                          {QStringLiteral("account"), account.label.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(account.label)},
                          {QStringLiteral("scopes"), account.grantedScopes}}, {});
        return;
    }
    // A scope is satisfied by itself or, for X.Read, by X.ReadWrite.
    auto normalize = [](QString scope) {
        scope.remove(QStringLiteral("https://graph.microsoft.com/"), Qt::CaseInsensitive);
        return scope.toLower();
    };
    QStringList granted;
    for (const QString& scope : account.grantedScopes)
        granted.append(normalize(scope));
    QStringList missing;
    for (const QString& scope : plugin.office365Scopes) {
        const QString wanted = normalize(scope);
        const QString readWrite = QString(wanted).replace(QStringLiteral(".read"), QStringLiteral(".readwrite"));
        if (!granted.contains(wanted) && !granted.contains(readWrite))
            missing.append(scope);
    }
    if (method == QLatin1String("office365.signIn")) {
        if (account.authenticated && missing.isEmpty()) {
            reply(true, {});
            return;
        }
        if (!missing.isEmpty())
            office->requestAdditionalScopes(missing);
        office->startSignIn();
        reply(false, {});   // sign-in continues in Settings > Office 365 Integration
        return;
    }
    // office365.graph
    if (!account.authenticated) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), tr("Sign in to Office 365 in Settings first.")));
        return;
    }
    if (!missing.isEmpty()) {
        reply({}, PluginError::make(QStringLiteral("consent-required"),
                                    tr("Office 365 has not granted %1. Call office365.signIn() or sign in again from "
                                       "Settings > Office 365 Integration.").arg(missing.join(QStringLiteral(", "))),
                                    {{QStringLiteral("missing"), missing}}));
        return;
    }

    const QString verb = arg(args, 0).toString().toUpper();
    const QString path = arg(args, 1).toString();
    const QVariantMap options = arg(args, 2).toMap();
    QUrl url = path.startsWith(QLatin1String("https://"))
        ? QUrl(path)
        : QUrl(QStringLiteral("https://graph.microsoft.com/v1.0/") + (path.startsWith(QLatin1Char('/')) ? path.mid(1) : path));
    if (url.host().compare(QLatin1String("graph.microsoft.com"), Qt::CaseInsensitive) != 0) {
        reply({}, PluginError::make(QStringLiteral("validation-failed"), tr("office365.graph() only calls graph.microsoft.com.")));
        return;
    }
    const QVariantMap queryItems = options.value(QStringLiteral("query")).toMap();
    if (!queryItems.isEmpty()) {
        QUrlQuery query(url);
        for (auto it = queryItems.constBegin(); it != queryItems.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value().toString());
        url.setQuery(query);
    }
    PN::Comm::HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = verb.toUtf8();
    request.url = url;
    const QVariantMap headers = options.value(QStringLiteral("headers")).toMap();
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.headers.insert(it.key().toUtf8(), it.value().toString().toUtf8());
    const QVariant body = options.value(QStringLiteral("body"));
    if (body.isValid() && !body.isNull()) {
        if (body.typeId() == QMetaType::QVariantMap || body.typeId() == QMetaType::QVariantList) {
            request.body = QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact);
            request.headers.insert("Content-Type", "application/json");
        } else if (body.typeId() == QMetaType::QByteArray) {
            request.body = body.toByteArray();
        } else {
            request.body = body.toString().toUtf8();
        }
    }
    const QString responseType = options.value(QStringLiteral("responseType"), QStringLiteral("json")).toString();
    office->sendGraphRequest(request, [reply, responseType](PN::Comm::HttpResponse response) {
        if (!response.error.code.isEmpty() && response.statusCode == 0) {
            reply({}, PluginError::make(QStringLiteral("network"), response.error.displayText.isEmpty()
                                                                       ? response.error.code : response.error.displayText));
            return;
        }
        QVariantMap headers;
        for (auto it = response.headers.constBegin(); it != response.headers.constEnd(); ++it)
            headers.insert(QString::fromUtf8(it.key()).toLower(), QString::fromUtf8(it.value()));
        QVariant body;
        if (responseType == QLatin1String("arraybuffer"))
            body = response.body;
        else if (responseType == QLatin1String("text"))
            body = QString::fromUtf8(response.body);
        else
            body = response.body.isEmpty() ? QVariant::fromValue(nullptr) : QJsonDocument::fromJson(response.body).toVariant();
        reply(QVariantMap{{QStringLiteral("status"), response.statusCode}, {QStringLiteral("headers"), headers},
                          {QStringLiteral("body"), body}}, {});
    });
}
