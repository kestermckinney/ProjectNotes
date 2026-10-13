// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// DesktopAppController's side of the JavaScript plugin system: creating the
// registry, plugin menus, data-change forwarding, navigation, and the email,
// calendar and template services plugins reach through DesktopPluginServices.

#include "DesktopAppController.h"

#include "DesktopPluginServices.h"
#include "FolderManager.h"
#include "PluginUiController.h"

#include "ProjectNotesPlugins/PluginRegistry.h"
#include "ProjectNotesPlugins/PublishedSchema.h"
#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmail/CommunicationRepository.h"
#include "ProjectNotesEmail/EmailService.h"
#include "ProjectNotesEmail/EmailSettingsStore.h"
#include "ProjectNotesEmail/ProjectEmailPreparationFactory.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "ProjectNotesIntegrations/TemplateEditorModel.h"
#include "ProjectNotesMeetings/IcsWriter.h"
#include "ProjectNotesMeetings/MeetingService.h"

#include "appresourcespath.h"
#include "databaseobjects.h"
#include "sqlquerymodel.h"
#include "version.h"

#include "QLogger.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>

using namespace QLogger;
using PN::Plugins::PluginError;
using PN::Plugins::PluginManifest;
using PN::Plugins::PublishedSchema;

namespace {

QString backendLabel(PN::Comm::BackendId id)
{
    switch (id) {
    case PN::Comm::BackendId::Graph: return QObject::tr("Microsoft 365");
    case PN::Comm::BackendId::DefaultApp: return QObject::tr("Default email application");
    case PN::Comm::BackendId::OutlookClassic: return QObject::tr("Outlook (classic)");
    case PN::Comm::BackendId::Thunderbird: return QObject::tr("Thunderbird");
    case PN::Comm::BackendId::MacMail: return QObject::tr("Apple Mail");
    case PN::Comm::BackendId::Outlook: return QObject::tr("Outlook");
    case PN::Comm::BackendId::ICloud: return QObject::tr("iCloud Mail");
    }
    return {};
}

QString addressLabel(const PN::Comm::EmailAddress& a)
{
    return a.displayName.isEmpty() ? a.address : QStringLiteral("%1 <%2>").arg(a.displayName, a.address);
}

} // namespace

// ── Registry ─────────────────────────────────────────────────────────────────

void DesktopAppController::ensureJsPlugins()
{
    if (m_jsPlugins)
        return;
    PluginUiController* ui = PluginUiController::instance();
    m_pluginServices = new DesktopPluginServices(this, ui, this);
    m_jsPlugins = new PN::Plugins::PluginRegistry(m_pluginServices, this);

    const QString organization = QStringLiteral("ProjectNotes") + s_developerProfile;
    PN::Plugins::PluginRegistry::Paths paths;
    paths.bundled = appResourcesPath() + QStringLiteral("/plugins");
    paths.user = QDir::homePath() + QStringLiteral("/Project Notes/plugins");
    // Tests point both plugin folders somewhere disposable.
    if (qEnvironmentVariableIsSet("PN_PLUGIN_USER_DIR"))
        paths.user = qEnvironmentVariable("PN_PLUGIN_USER_DIR");
    if (qEnvironmentVariableIsSet("PN_PLUGIN_BUNDLED_DIR"))
        paths.bundled = qEnvironmentVariable("PN_PLUGIN_BUNDLED_DIR");
    // A developer profile keeps its own plugin data, as it keeps its own database.
    const QString profileDir = s_developerProfile.isEmpty() ? QString() : QLatin1Char('/') + s_developerProfile;
    paths.data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + profileDir + QStringLiteral("/plugin-data");
    paths.temp = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + profileDir + QStringLiteral("/plugin-temp");
    QDir(paths.temp).removeRecursively();   // per-run temporary files
    m_jsPlugins->setPaths(paths);
    m_jsPlugins->setDatabasePath(global_DBObjects.getDatabaseFile());
    m_jsPlugins->setSettingsOrganization(organization);
    m_jsPlugins->setProfile(s_developerProfile);
    m_jsPlugins->setAppVersion(QStringLiteral(APP_VERSION_STRING));
#ifdef PN_APP_STORE
    m_jsPlugins->setBuildFlavor(QStringLiteral("appstore"));
#endif
    ui->setSettingsOrganization(organization);
    ui->setRegistry(m_jsPlugins);

    // Rows a plugin wrote: refresh the lists that show them.
    connect(m_jsPlugins, &PN::Plugins::PluginRegistry::pluginDataChanged, this, [this](const QVariantList& changes) {
        for (const QVariant& c : changes) {
            const QVariantMap change = c.toMap();
            global_DBObjects.pushRowChange(change.value(QStringLiteral("table")).toString(), change.value(QStringLiteral("key")),
                                           static_cast<KeyColumnChange::OperationType>(change.value(QStringLiteral("operation")).toInt()));
        }
        m_forwardingPluginRows = true;
        global_DBObjects.updateDisplayData();
        m_forwardingPluginRows = false;
        if (FolderManager* fm = FolderManager::instance())
            fm->reload();
    });
    // Rows the application changed: tell plugins (data.onChanged).
    connect(&global_DBObjects, &DatabaseObjects::displayChangesPending, this, [this](const QVariantList& changes) {
        if (!m_forwardingPluginRows && m_jsPlugins)
            m_jsPlugins->notifyDataChanged(changes);
    });
    connect(m_pluginServices, &PN::Plugins::PluginServices::viewChanged, m_jsPlugins,
            &PN::Plugins::PluginRegistry::notifyViewChanged);

    m_jsPlugins->loadAll();

    // Python plugins no longer run. Point out any the user added, once.
    const QStringList leftovers = QDir(paths.user).entryList({QStringLiteral("*.py")}, QDir::Files);
    QSettings state(organization, QStringLiteral("AppSettings"));
    if (!leftovers.isEmpty() && !state.value(QStringLiteral("PythonPluginsNoticeShown")).toBool()) {
        state.setValue(QStringLiteral("PythonPluginsNoticeShown"), true);
        const QString text = tr("Project Notes no longer runs Python plugins, so %1 in your plugins folder will not load. "
                                "See Help > User Guide > JavaScript Plugins to rewrite them.")
                                 .arg(leftovers.size() == 1 ? leftovers.first() : tr("%n file(s)", nullptr, int(leftovers.size())));
        QTimer::singleShot(3000, ui, [ui, text] { emit ui->notificationRequested(text, QStringLiteral("warning"), 15); });
    }
}

void DesktopAppController::setCurrentView(const QString& section, const QString& entity,
                                          const QString& key, const QString& tab)
{
    if (!m_pluginServices)
        return;
    m_pluginServices->setCurrentView({
        {QStringLiteral("section"), section},
        {QStringLiteral("entity"), entity.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(entity)},
        {QStringLiteral("key"), key.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(key)},
        {QStringLiteral("tab"), tab.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(tab)}});
}

void DesktopAppController::navigateForPlugin(const QString& entity, const QString& key, const QString& tab,
                                             PluginError* error)
{
    Q_UNUSED(tab);
    auto parentOf = [](const QString& table, const QString& column, const QString& id) {
        QSqlQuery query(global_DBObjects.getDb());
        query.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE id = ?").arg(column, table));
        query.addBindValue(id);
        return (query.exec() && query.next()) ? query.value(0).toString() : QString();
    };
    const QString table = PublishedSchema::instance().storedTable(entity);
    if (entity == QLatin1String("projects"))
        emit pluginNavigateRequested(QStringLiteral("Project"), key, {}, {});
    else if (entity == QLatin1String("contacts"))
        emit pluginNavigateRequested(QStringLiteral("Contact"), key, {}, {});
    else if (entity == QLatin1String("companies"))
        emit pluginNavigateRequested(QStringLiteral("Company"), key, {}, {});
    else if (entity == QLatin1String("item_tracker"))
        emit pluginNavigateRequested(QStringLiteral("Item Tracker"), key, {}, {});
    else if (entity == QLatin1String("item_tracker_updates"))
        emit pluginNavigateRequested(QStringLiteral("Tracker Update"), key, {}, parentOf(table, QStringLiteral("item_id"), key));
    else if (entity == QLatin1String("project_notes"))
        emit pluginNavigateRequested(QStringLiteral("Project Notes"), key, parentOf(table, QStringLiteral("project_id"), key), {});
    else if (entity == QLatin1String("meeting_attendees"))
        emit pluginNavigateRequested(QStringLiteral("Meeting Attendees"), key, parentOf(table, QStringLiteral("note_id"), key), {});
    else if (PublishedSchema::instance().isEntity(entity))
        emit pluginNavigateRequested(QString(), key, parentOf(table, QStringLiteral("project_id"), key), {});
    else
        *error = PluginError::make(QStringLiteral("not-found"), tr("Unknown entity \"%1\".").arg(entity));
}

void DesktopAppController::refreshViewsForPlugin()
{
    for (SqlQueryModel* model : global_DBObjects.getOpenModels())
        if (model->isLoaded())
            model->refresh();
}

QVariantMap DesktopAppController::pluginDefaults() const
{
    const QString companyId = managingCompanyId();
    const QString managerId = projectManagerId();
    auto name = [](const QString& sql, const QString& id) {
        QSqlQuery query(global_DBObjects.getDb());
        query.prepare(sql);
        query.addBindValue(id);
        return (query.exec() && query.next()) ? query.value(0).toString() : QString();
    };
    auto orNull = [](const QString& v) { return v.isEmpty() ? QVariant::fromValue(nullptr) : QVariant(v); };
    return {{QStringLiteral("managingCompanyId"), orNull(companyId)},
            {QStringLiteral("managingCompanyName"), orNull(name(QStringLiteral("SELECT client_name FROM clients WHERE id = ?"), companyId))},
            {QStringLiteral("projectManagerId"), orNull(managerId)},
            {QStringLiteral("projectManagerName"), orNull(name(QStringLiteral("SELECT name FROM people WHERE id = ?"), managerId))}};
}

// ── Email ────────────────────────────────────────────────────────────────────

PN::Comm::EmailAddress DesktopAppController::pluginRecipient(const QVariant& value, PN::Comm::RecipientRole role) const
{
    PN::Comm::EmailAddress address;
    address.role = role;
    if (value.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        if (map.contains(QStringLiteral("contact"))) {
            QSqlQuery query(global_DBObjects.getDb());
            query.prepare(QStringLiteral("SELECT name, email FROM people WHERE id = ? AND (deleted IS NULL OR deleted = 0)"));
            query.addBindValue(map.value(QStringLiteral("contact")).toString());
            if (query.exec() && query.next()) {
                address.displayName = query.value(0).toString();
                address.address = query.value(1).toString().trimmed();
            }
        } else {
            address.displayName = map.value(QStringLiteral("name")).toString();
            address.address = map.value(QStringLiteral("email")).toString().trimmed();
        }
        return address;
    }
    // "Name <a@b>" or "a@b".
    static const QRegularExpression named(QStringLiteral("^\\s*(.*?)\\s*<([^>]+)>\\s*$"));
    const QString text = value.toString();
    const auto match = named.match(text);
    if (match.hasMatch()) {
        address.displayName = match.captured(1).remove(QLatin1Char('"'));
        address.address = match.captured(2).trimmed();
    } else {
        address.address = text.trimmed();
    }
    return address;
}

QVariantList DesktopAppController::pluginEmailBackends() const
{
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    const PN::Comm::BackendId preferred = PN::Comm::EmailSettingsStore(settings).preferredBackend();
    const bool rich = preferred != PN::Comm::BackendId::DefaultApp;
    return {QVariantMap{{QStringLiteral("id"), PN::Comm::toStableString(preferred)},
                        {QStringLiteral("name"), backendLabel(preferred)},
                        {QStringLiteral("available"), true}, {QStringLiteral("preferred"), true},
                        {QStringLiteral("supportsHtml"), rich}, {QStringLiteral("supportsAttachments"), rich}}};
}

void DesktopAppController::composePluginEmail(const PluginManifest& plugin, const QVariantMap& message,
                                              std::function<void(const QVariant&, const PluginError&)> reply)
{
    if (!m_emailService || !m_databaseOpen) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), tr("Email is not available.")));
        return;
    }
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    const PN::Comm::EmailSettingsStore emailSettings(settings);

    PN::Comm::EmailPreparation prep;
    prep.operationId = QUuid::createUuid();
    prep.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration,
                   message.value(QStringLiteral("projectKey")).toString(), {}, PN::Comm::Workflow::ProjectEmail, {}, {}};
    prep.backend = emailSettings.preferredBackend();
    prep.mode = PN::Comm::EmailMode::InlineHtml;
    prep.senderAddress = emailSettings.senderAddress();

    QStringList toLabels, ccLabels, bccLabels;
    const QList<QPair<QString, PN::Comm::RecipientRole>> roles {
        {QStringLiteral("to"), PN::Comm::RecipientRole::To}, {QStringLiteral("cc"), PN::Comm::RecipientRole::Cc},
        {QStringLiteral("bcc"), PN::Comm::RecipientRole::Bcc}};
    for (const auto& [key, role] : roles) {
        for (const QVariant& value : message.value(key).toList()) {
            const PN::Comm::EmailAddress address = pluginRecipient(value, role);
            if (address.address.isEmpty() || !address.address.contains(QLatin1Char('@'))) {
                reply({}, PluginError::make(QStringLiteral("validation-failed"),
                                            tr("Recipient \"%1\" has no valid email address.").arg(value.toString()),
                                            {{QStringLiteral("column"), key}}));
                return;
            }
            prep.recipients.append(address);
            (role == PN::Comm::RecipientRole::To ? toLabels : role == PN::Comm::RecipientRole::Cc ? ccLabels : bccLabels)
                .append(addressLabel(address));
        }
    }
    if (prep.recipients.isEmpty()) {
        reply({}, PluginError::make(QStringLiteral("validation-failed"), tr("compose() needs at least one recipient.")));
        return;
    }

    const QString subject = message.value(QStringLiteral("subject")).toString();
    QString html = message.value(QStringLiteral("html")).toString();
    if (html.isEmpty())
        html = message.value(QStringLiteral("text")).toString().toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    prep.document.workflow = PN::Comm::Workflow::ProjectEmail;
    prep.document.defaultSubject = subject;
    prep.document.emailFragment = html;
    prep.document.htmlDocument = QStringLiteral("<!DOCTYPE html><html><body>%1</body></html>").arg(html);
    prep.document.fileStem = QStringLiteral("Email");

    QStringList attachmentNames;
    const QStringList attachments = message.value(QStringLiteral("attachments")).toStringList();
    if (!attachments.isEmpty()) {
        PN::Comm::ArtifactStore store(PN::Comm::ArtifactStore::cacheDirectory(s_developerProfile));
        PN::Comm::OperationManifest manifest;
        manifest.operationId = prep.operationId;
        manifest.workflow = PN::Comm::Workflow::ProjectEmail;
        manifest.databaseKey = global_DBObjects.getDatabaseFile();
        manifest.projectId = prep.source.projectId;
        manifest.backend = prep.backend;
        manifest.createdAt = QDateTime::currentDateTimeUtc();
        PN::Comm::ServiceError storeError;
        if (!store.createOperation(manifest, &storeError)) {
            reply({}, PluginError::make(QStringLiteral("email-failed"), storeError.displayText.isEmpty() ? storeError.code : storeError.displayText));
            return;
        }
        for (const QString& path : attachments) {
            const PN::Comm::ArtifactStoreResult staged = store.stageUserAttachment(prep.operationId, path);
            if (!staged.ok()) {
                reply({}, PluginError::make(QStringLiteral("email-failed"),
                                            tr("Could not attach %1: %2").arg(QFileInfo(path).fileName(),
                                                                              staged.error.displayText.isEmpty() ? staged.error.code : staged.error.displayText)));
                return;
            }
            prep.attachments.append(staged.artifact);
            attachmentNames.append(staged.artifact.displayName);
        }
    }
    if (prep.backend == PN::Comm::BackendId::Graph && m_office365Service) {
        const PN::Comm::Office365Account account = m_office365Service->account();
        prep.accountKey = account.label;
        prep.accountGeneration = account.generation;
    }

    const QString backendName = backendLabel(prep.backend);
    QPointer<DesktopAppController> self(this);
    PluginUiController::instance()->openRequest(plugin.id, QStringLiteral("email"),
        {{QStringLiteral("to"), toLabels}, {QStringLiteral("cc"), ccLabels}, {QStringLiteral("bcc"), bccLabels},
         {QStringLiteral("subject"), subject}, {QStringLiteral("html"), html},
         {QStringLiteral("attachments"), attachmentNames}, {QStringLiteral("backend"), backendName}},
        [self, prep, reply, backendName](const QVariant& result, const PluginError& error) {
            if (!self)
                return;
            if (error.isError()) { reply({}, error); return; }
            if (result.isNull() || !result.toBool()) {
                reply(QVariantMap{{QStringLiteral("status"), QStringLiteral("cancelled")}}, {});
                return;
            }
            const PN::Comm::EmailPreparationResult built = PN::Comm::EmailContentBuilder::prepare(prep);
            if (!built.request) {
                const QString message = built.validation.issues.isEmpty() ? tr("The email could not be prepared.")
                    : (built.validation.issues.first().displayText.isEmpty() ? built.validation.issues.first().code
                                                                            : built.validation.issues.first().displayText);
                reply({}, PluginError::make(QStringLiteral("email-failed"), message));
                return;
            }
            self->m_emailService->handoff(*built.request, [self, reply, backendName](PN::Comm::EmailHandoffResult handoff) {
                if (!handoff.error.code.isEmpty()) {
                    reply({}, PluginError::make(QStringLiteral("email-failed"),
                                                handoff.error.displayText.isEmpty() ? handoff.error.code : handoff.error.displayText));
                    return;
                }
                if (handoff.presentationUrl.isValid())
                    QDesktopServices::openUrl(handoff.presentationUrl);
                reply(QVariantMap{{QStringLiteral("status"), QStringLiteral("handed-off")},
                                  {QStringLiteral("backend"), backendName}}, {});
            });
        });
}

QVariantList DesktopAppController::pluginEmailAudiences(const QString& projectId, const QString& workflow) const
{
    QVariantList out;
    QSqlQuery query(global_DBObjects.getDb());
    query.prepare(QStringLiteral("SELECT id, audience_name, workflow, is_default FROM project_email_audiences "
                                 "WHERE project_id = ? AND (deleted IS NULL OR deleted = 0) ORDER BY audience_name"));
    query.addBindValue(projectId);
    if (!query.exec())
        return out;
    while (query.next()) {
        if (!workflow.isEmpty() && query.value(2).toString() != workflow)
            continue;
        out.append(QVariantMap{{QStringLiteral("id"), query.value(0).toString()}, {QStringLiteral("name"), query.value(1).toString()},
                               {QStringLiteral("workflow"), query.value(2).toString()},
                               {QStringLiteral("isDefault"), query.value(3).toInt() != 0}});
    }
    return out;
}

QVariant DesktopAppController::pluginResolveAudience(const QString& projectId, const QString& audienceId, PluginError* error) const
{
    QSqlQuery query(global_DBObjects.getDb());
    query.prepare(QStringLiteral("SELECT people_source, recipient_distribution_json FROM project_email_audiences "
                                 "WHERE id = ? AND project_id = ? AND (deleted IS NULL OR deleted = 0)"));
    query.addBindValue(audienceId);
    query.addBindValue(projectId);
    if (!query.exec() || !query.next()) {
        *error = PluginError::make(QStringLiteral("not-found"), tr("No audience \"%1\" exists for this project.").arg(audienceId));
        return {};
    }
    QVariantList to, cc, bcc;
    const QJsonArray distribution = QJsonDocument::fromJson(query.value(1).toString().toUtf8()).array();
    for (const QJsonValue& entry : distribution) {
        const QJsonObject o = entry.toObject();
        if (!o.value(QStringLiteral("selected")).toBool(true))
            continue;
        const QVariantMap recipient {{QStringLiteral("contact"), o.value(QStringLiteral("personId")).toString()}};
        const QString role = o.value(QStringLiteral("role")).toString(QStringLiteral("to"));
        (role == QLatin1String("cc") ? cc : role == QLatin1String("bcc") ? bcc : to).append(recipient);
    }
    if (distribution.isEmpty() && query.value(0).toString() == QLatin1String("project-team")) {
        QSqlQuery team(global_DBObjects.getDb());
        team.prepare(QStringLiteral("SELECT people_id FROM project_people WHERE project_id = ? AND (deleted IS NULL OR deleted = 0)"));
        team.addBindValue(projectId);
        if (team.exec())
            while (team.next())
                to.append(QVariantMap{{QStringLiteral("contact"), team.value(0).toString()}});
    }
    return QVariantMap{{QStringLiteral("to"), to}, {QStringLiteral("cc"), cc}, {QStringLiteral("bcc"), bcc}};
}

QVariantList DesktopAppController::pluginEmailTemplates()
{
    QVariantList out;
    if (!m_templateEditorModel)
        return out;
    const QString previous = m_templateEditorModel->workflow();
    for (PN::Comm::Workflow workflow : {PN::Comm::Workflow::ProjectEmail, PN::Comm::Workflow::ShareLocation,
                                        PN::Comm::Workflow::SendMeetingNotes}) {
        m_templateEditorModel->setWorkflow(PN::Comm::toStableString(workflow));
        for (const QVariant& t : m_templateEditorModel->templates()) {
            const QVariantMap map = t.toMap();
            out.append(QVariantMap{{QStringLiteral("id"), map.value(QStringLiteral("id"))},
                                   {QStringLiteral("name"), map.value(QStringLiteral("name"))},
                                   {QStringLiteral("workflow"), PN::Comm::toStableString(workflow)}});
        }
    }
    m_templateEditorModel->setWorkflow(previous);
    return out;
}

void DesktopAppController::renderPluginEmailTemplate(const QString& templateId, const QVariantMap& context,
                                                     std::function<void(const QVariant&, const PluginError&)> reply)
{
    if (!m_templateEditorModel || !m_communicationRepository || !m_databaseOpen) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), tr("Email templates are not available.")));
        return;
    }
    std::optional<PN::Comm::CommunicationTemplate> found;
    const QString previous = m_templateEditorModel->workflow();
    for (PN::Comm::Workflow workflow : {PN::Comm::Workflow::ProjectEmail, PN::Comm::Workflow::ShareLocation,
                                        PN::Comm::Workflow::SendMeetingNotes}) {
        m_templateEditorModel->setWorkflow(PN::Comm::toStableString(workflow));
        if ((found = m_templateEditorModel->templateById(templateId)))
            break;
    }
    m_templateEditorModel->setWorkflow(previous);
    if (!found) {
        reply({}, PluginError::make(QStringLiteral("not-found"), tr("No email template has id \"%1\".").arg(templateId)));
        return;
    }
    const QString projectId = context.value(QStringLiteral("projectKey")).toString();
    if (projectId.isEmpty()) {
        reply({}, PluginError::make(QStringLiteral("validation-failed"), tr("renderTemplate() needs a projectKey.")));
        return;
    }
    PN::Comm::SnapshotRequest request;
    request.operationId = QUuid::createUuid();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration, projectId,
                      context.contains(QStringLiteral("noteKey")) ? QStringList{context.value(QStringLiteral("noteKey")).toString()} : QStringList{},
                      PN::Comm::Workflow::ProjectEmail, {}, {}};
    request.managingCompanyId = managingCompanyId();
    request.projectManagerId = projectManagerId();
    request.capturedAt = QDateTime::currentDateTimeUtc();
    const PN::Comm::CommunicationTemplate contentTemplate = *found;
    QPointer<DesktopAppController> self(this);
    m_communicationRepository->loadSnapshot(request, [self, request, contentTemplate, reply](PN::Comm::SnapshotResult result) {
        if (!self)
            return;
        if (!result.error.code.isEmpty()) {
            reply({}, PluginError::make(QStringLiteral("not-found"), result.error.displayText.isEmpty() ? result.error.code : result.error.displayText));
            return;
        }
        QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
        const auto review = PN::Comm::ProjectEmailPreparationFactory::create(result.snapshot, request.source,
            PN::Comm::EmailSettingsStore(settings).preferredBackend(), contentTemplate, false);
        if (!review.validation.ok()) {
            reply({}, PluginError::make(QStringLiteral("validation-failed"),
                                        review.validation.issues.isEmpty() ? tr("The template could not be applied.")
                                                                           : review.validation.issues.first().displayText));
            return;
        }
        reply(QVariantMap{{QStringLiteral("subject"), review.preparation.document.defaultSubject},
                          {QStringLiteral("html"), review.preparation.document.emailFragment}}, {});
    });
}

// ── Calendar ─────────────────────────────────────────────────────────────────

namespace {

bool pluginMeetingRequest(const QVariantMap& meeting, PN::Comm::MeetingRequest* request,
                          const std::function<PN::Comm::EmailAddress(const QVariant&)>& recipient, PluginError* error)
{
    request->operationId = QUuid::createUuid();
    request->subject = meeting.value(QStringLiteral("subject")).toString().simplified();
    request->location = meeting.value(QStringLiteral("location")).toString().simplified();
    const QString html = meeting.value(QStringLiteral("html")).toString();
    QTextDocument agenda;
    agenda.setHtml(html);
    request->plainAgenda = agenda.toPlainText().trimmed();
    request->htmlAgenda = request->plainAgenda.isEmpty() ? QString() : html;
    request->start = QDateTime::fromString(meeting.value(QStringLiteral("start")).toString(), Qt::ISODate);
    request->end = QDateTime::fromString(meeting.value(QStringLiteral("end")).toString(), Qt::ISODate);
    if (meeting.value(QStringLiteral("allDay")).toBool() && request->start.isValid()) {
        request->start = QDateTime(request->start.date(), QTime(0, 0));
        request->end = request->start.addDays(1);
    }
    if (request->subject.isEmpty()) {
        *error = PluginError::make(QStringLiteral("validation-failed"), QObject::tr("A meeting needs a subject."));
        return false;
    }
    if (!request->start.isValid() || !request->end.isValid() || request->end <= request->start) {
        *error = PluginError::make(QStringLiteral("validation-failed"), QObject::tr("A meeting needs a valid start and an end after it (ISO 8601)."));
        return false;
    }
    for (const QVariant& a : meeting.value(QStringLiteral("attendees")).toList()) {
        const PN::Comm::EmailAddress address = recipient(a);
        if (address.address.isEmpty())
            continue;
        PN::Comm::MeetingAttendee attendee;
        attendee.displayName = address.displayName;
        attendee.address = address.address;
        request->attendees.append(attendee);
    }
    return true;
}

} // namespace

void DesktopAppController::draftPluginMeeting(const PluginManifest& plugin, const QVariantMap& meeting,
                                              std::function<void(const QVariant&, const PluginError&)> reply)
{
    if (!m_meetingService || !m_databaseOpen) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), tr("Calendar is not available.")));
        return;
    }
    PN::Comm::MeetingRequest request;
    PluginError error;
    const auto recipient = [this](const QVariant& v) { return pluginRecipient(v, PN::Comm::RecipientRole::To); };
    if (!pluginMeetingRequest(meeting, &request, recipient, &error)) {
        reply({}, error);
        return;
    }
    QSettings settings(QStringLiteral("ProjectNotes") + s_developerProfile, QStringLiteral("AppSettings"));
    const PN::Comm::EmailSettingsStore emailSettings(settings);
    request.backend = emailSettings.preferredBackend();
    request.organizerAddress = emailSettings.senderAddress();
    request.source = {global_DBObjects.getDatabaseFile(), m_communicationDatabaseGeneration,
                      meeting.value(QStringLiteral("projectKey")).toString(), {}, PN::Comm::Workflow::ScheduleMeeting, {}, {}};
    if (request.backend == PN::Comm::BackendId::Graph && m_office365Service) {
        const PN::Comm::Office365Account account = m_office365Service->account();
        if (!account.authenticated) {
            reply({}, PluginError::make(QStringLiteral("unavailable"), tr("Sign in to Office 365 in Settings first.")));
            return;
        }
        request.accountKey = account.label;
        request.accountGeneration = account.generation;
    }

    QStringList attendees;
    for (const PN::Comm::MeetingAttendee& a : std::as_const(request.attendees))
        attendees.append(a.displayName.isEmpty() ? a.address : QStringLiteral("%1 <%2>").arg(a.displayName, a.address));
    const QString backendName = backendLabel(request.backend);
    QPointer<DesktopAppController> self(this);
    PluginUiController::instance()->openRequest(plugin.id, QStringLiteral("meeting"),
        {{QStringLiteral("subject"), request.subject}, {QStringLiteral("location"), request.location},
         {QStringLiteral("start"), QLocale().toString(request.start, QLocale::LongFormat)},
         {QStringLiteral("end"), QLocale().toString(request.end, QLocale::LongFormat)},
         {QStringLiteral("attendees"), attendees}, {QStringLiteral("html"), request.htmlAgenda},
         {QStringLiteral("backend"), backendName}},
        [self, request, reply, backendName](const QVariant& result, const PluginError& error) {
            if (!self)
                return;
            if (error.isError()) { reply({}, error); return; }
            if (result.isNull() || !result.toBool()) {
                reply(QVariantMap{{QStringLiteral("status"), QStringLiteral("cancelled")}}, {});
                return;
            }
            self->m_meetingService->handoff(request, [reply, backendName](PN::Comm::MeetingHandoffResult handoff) {
                if (!handoff.error.code.isEmpty()) {
                    reply({}, PluginError::make(QStringLiteral("calendar-failed"),
                                                handoff.error.displayText.isEmpty() ? handoff.error.code : handoff.error.displayText));
                    return;
                }
                if (handoff.presentationUrl.isValid())
                    QDesktopServices::openUrl(handoff.presentationUrl);
                reply(QVariantMap{{QStringLiteral("status"), QStringLiteral("handed-off")},
                                  {QStringLiteral("backend"), backendName}}, {});
            });
        });
}

QString DesktopAppController::writePluginIcs(const QVariantMap& meeting, const QString& path, PluginError* error)
{
    PN::Comm::MeetingRequest request;
    const auto recipient = [this](const QVariant& v) { return pluginRecipient(v, PN::Comm::RecipientRole::To); };
    if (!pluginMeetingRequest(meeting, &request, recipient, error))
        return {};
    PN::Comm::ValidationResult validation;
    const QByteArray ics = PN::Comm::IcsWriter::write(request, QDateTime::currentDateTimeUtc(), &validation);
    if (ics.isEmpty() || !validation.ok()) {
        *error = PluginError::make(QStringLiteral("validation-failed"),
                                   validation.issues.isEmpty() ? tr("The meeting could not be written.") : validation.issues.first().displayText);
        return {};
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = PluginError::make(QStringLiteral("io-error"), file.errorString());
        return {};
    }
    file.write(ics);
    return path;
}
