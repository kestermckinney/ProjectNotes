// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/EmailTypes.h"
#include "ProjectNotesEmail/ArtifactStore.h"
#include "ProjectNotesEmail/RecipientAudienceResolver.h"
#include "ProjectNotesEmail/RecipientSelectionModel.h"
#include "ProjectNotesEmail/MeetingNotesEmailBuilder.h"
#include "ProjectNotesEmail/MeetingNotesPreparationFactory.h"
#include "ProjectNotesEmail/MeetingNotesReportBuilder.h"
#include "ProjectNotesEmail/MeetingNotesReportPreparationFactory.h"
#include "ProjectNotesEmail/ProjectReportPreparationFactory.h"
#include "ProjectNotesEmail/backends/MailtoEmailBackend.h"
#include "ProjectNotesEmail/backends/ThunderbirdEmailBackend.h"
#include "ProjectNotesEmail/backends/GraphEmailBackend.h"
#include "ProjectNotesEmail/backends/OutlookHelperProtocol.h"
#include "ProjectNotesEmail/backends/MacMailBackend.h"
#include "ProjectNotesEmail/EmailService.h"
#include "ProjectNotesEmail/AudiencePresetStore.h"
#include "ProjectNotesEmail/StatusReportBuilder.h"
#include "ProjectNotesEmail/TrackerItemsReportBuilder.h"
#include "ProjectNotesEmail/ReportService.h"
#include "ProjectNotesEmail/EmailContentBuilder.h"
#include "ProjectNotesEmail/CommunicationsController.h"
#include "ProjectNotesIntegrations/TemplateParser.h"
#include "ProjectNotesIntegrations/ComputedFieldRunner.h"
#include "ProjectNotesIntegrations/ComputedFieldService.h"
#include "ProjectNotesIntegrations/CommunicationTemplateStore.h"
#include "ProjectNotesIntegrations/TemplateEditorModel.h"
#include "ProjectNotesIntegrations/TemplateApplication.h"
#include "ProjectNotesIntegrations/MicrosoftOAuthManager.h"
#include "ProjectNotesIntegrations/Office365Service.h"
#include "databaseobjects.h"
#include "email/fakes/RecordingEmailBackend.h"
#include "email/fakes/DeferredEmailBackend.h"

#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QSettings>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <thread>

using namespace PN::Comm;

class ScriptedHttpTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    QList<HttpResponse> responses;
    QList<HttpRequest> identityRequests;
    void send(HttpRequest request, Completion completion) override {
        // Account identity is fetched on sign-in independently of the draft
        // workflow. Keep it out of the scripted draft/upload response queue.
        if (request.method == "GET" && request.url.path() == "/v1.0/me") {
            identityRequests.append(request);
            completion({request.operationId, 200, {},
                        QByteArrayLiteral("{\"displayName\":\"Test User\",\"mail\":\"user@example.test\"}"), {}});
            return;
        }
        requests.append(request);
        HttpResponse response = responses.isEmpty() ? HttpResponse{} : responses.takeFirst();
        response.operationId = request.operationId;
        completion(std::move(response));
    }
    void cancel(const QUuid &) override {}
};

class DeferredHttpTransport final : public HttpTransport {
public:
    QList<HttpRequest> requests;
    QList<Completion> completions;
    void send(HttpRequest request, Completion completion) override {
        requests.append(std::move(request));
        completions.append(std::move(completion));
    }
    void cancel(const QUuid &) override {}
    void respondNext(HttpResponse response) {
        const HttpRequest request = requests.at(m_nextResponse++);
        response.operationId = request.operationId;
        Completion completion = std::move(completions.takeFirst());
        completion(std::move(response));
    }
private:
    qsizetype m_nextResponse = 0;
};

static QByteArray sha256(const QByteArray &value)
{
    return QCryptographicHash::hash(value, QCryptographicHash::Sha256);
}

class CommunicationContractsTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void stableEnumRoundTrips();
    void rejectsUnknownStableValues();
    void validatesWorkflowSpecificOptions();
    void recordsFakeHandoffWithOperationIdentity();
    void resolvesAudienceWithoutExpandingSource();
    void appliesRecipientOverridesSafely();
    void retainsRecipientEditsAndManualEntries();
    void preservesRecipientResolutionOrderWhileGroupingAudience();
    void parsesLiteralFieldsWithoutEvaluation();
    void limitsFieldsToCatalogAndWorkflow();
    void runsBoundedComputedFields();
    void evaluatesComputedFieldsAsynchronously();
    void storesTemplatesIdempotently();
    void exposesBundledTemplateForEveryNativeWorkflow();
    void switchingTemplateDatabaseDiscardsPriorSelection();
    void editsTemplatesWithoutQmlEvaluation();
    void appliesTemplateAroundProtectedNativeContent();
    void buildsContextSpecificMeetingNotes();
    void createsMeetingNotesReviewFromSnapshot();
    void buildsProjectWideMeetingNotesReport();
    void preparesProjectWideMeetingNotesReport();
    void preparesStatusAndTrackerReportsFromSnapshot();
    void encodesAndBoundsMailtoRequests();
    void createsThunderbirdComposeArguments();
    void launchesThunderbirdCommandWithFixedArguments();
    void resolvesThunderbirdExecutable();
    void validatesOutlookHelperProtocolFrames();
    void reportsMacMailPlatformAvailability();
    void serializesEmailHandoffsAndRejectsStaleCompletion();
    void scopesAudiencePresetsByDatabaseAndProject();
    void buildsStatusReportWithEvmAndEscapedIssues();
    void buildsFilteredAndSortedTrackerReport();
    void dispatchesAllNativeReportBuilders();
    void preparesImmutableRequestsForEachEmailMode();
    void createsGraphDraftWithoutSend_data();
    void createsGraphDraftWithoutSend();
    void retainsUncertainOutcomeForLostGraphDraftResponse();
    void rejectsModifiedGraphAttachmentBeforeDraftCreation();
    void uploadsSimpleGraphAttachment();
    void uploadsLargeGraphAttachmentInUnauthenticatedChunks();
    void cancelsGraphUploadBeforeFirstChunk();
    void coordinatesPreparationHandoffAndNoEmail();

private:
    std::unique_ptr<QTemporaryDir> m_templateDatabaseDirectory;
    QString m_templateDatabasePath;
};

namespace {
constexpr auto kTemplateSettingKey = "Email/v1/Templates";
}

void CommunicationContractsTest::initTestCase()
{
    m_templateDatabaseDirectory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_templateDatabaseDirectory->isValid());
    m_templateDatabasePath = m_templateDatabaseDirectory->filePath(QStringLiteral("templates.db"));
    QVERIFY(global_DBObjects.createDatabase(m_templateDatabasePath));
    QVERIFY(global_DBObjects.openDatabase(m_templateDatabasePath, QStringLiteral("communication-contracts"), false));
}

void CommunicationContractsTest::cleanupTestCase()
{
    global_DBObjects.closeDatabase();
}

void CommunicationContractsTest::init()
{
    QVERIFY(global_DBObjects.saveParameter(QString::fromLatin1(kTemplateSettingKey), QString()));
}

void CommunicationContractsTest::stableEnumRoundTrips()
{
    QVERIFY(workflowFromStableString(toStableString(Workflow::StatusReport)) == Workflow::StatusReport);
    QVERIFY(emailModeFromStableString(toStableString(EmailMode::PdfAttachment)) == EmailMode::PdfAttachment);
    QVERIFY(backendIdFromStableString(toStableString(BackendId::Thunderbird)) == BackendId::Thunderbird);
    QVERIFY(recipientRoleFromStableString(toStableString(RecipientRole::Bcc)) == RecipientRole::Bcc);
}

void CommunicationContractsTest::rejectsUnknownStableValues()
{
    QVERIFY(!workflowFromStableString(QStringLiteral("2")));
    QVERIFY(!emailModeFromStableString(QStringLiteral("send")));
    QVERIFY(!backendIdFromStableString(QStringLiteral("default-client")));
    QVERIFY(!recipientRoleFromStableString(QStringLiteral("reply-to")));
}

void CommunicationContractsTest::validatesWorkflowSpecificOptions()
{
    const auto tracker = defaultReportOptions(Workflow::TrackerItemsReport, QDate(2026, 9, 15));
    QVERIFY(tracker.tracker.has_value());
    QVERIFY(validate(tracker).ok());

    auto noTypes = tracker;
    noTypes.tracker->itemTypes.clear();
    QCOMPARE(validate(noTypes).issues.constFirst().code, QStringLiteral("tracker-item-types-required"));
    auto noStatuses = tracker;
    noStatuses.tracker->statuses.clear();
    QCOMPARE(validate(noStatuses).issues.constFirst().code, QStringLiteral("tracker-statuses-required"));

    auto invalidStatus = defaultReportOptions(Workflow::StatusReport, QDate(2026, 9, 15));
    invalidStatus.tracker.emplace();
    QVERIFY(!validate(invalidStatus).ok());

    const SourceContext invalidNotes {QStringLiteral("test-db"), 1, QStringLiteral("project-1"), {},
                                      Workflow::SendMeetingNotes};
    QVERIFY(!validate(invalidNotes).ok());
}

void CommunicationContractsTest::recordsFakeHandoffWithOperationIdentity()
{
    Test::RecordingEmailBackend backend;
    EmailRequest request;
    request.operationId = QUuid::createUuid();
    bool completed = false;
    backend.handoff(request, [&completed, &request](EmailHandoffResult result) {
        completed = true;
        QCOMPARE(result.operationId, request.operationId);
        QCOMPARE(result.certainty, OutcomeCertainty::Certain);
    });

    QVERIFY(completed);
    QCOMPARE(backend.requests.size(), 1);
    QCOMPARE(backend.requests.constFirst().operationId, request.operationId);
}

void CommunicationContractsTest::resolvesAudienceWithoutExpandingSource()
{
    CommunicationSnapshot snapshot;
    snapshot.clientCompanyId = "client";
    snapshot.managingCompanyId = "ours";
    snapshot.projectManagerId = "manager";
    snapshot.people = {{"manager", "Manager", "manager@example.test", "ours", "Ours", true, true, true},
                       {"client", "Client", "client@example.test", "client", "Client Co", true, false, true},
                       {"partner", "Partner", "partner@example.test", "partner", "Partner Co", false, true, true},
                       {"unknown", "Unknown", "unknown@example.test", {}, {}, true, false, true}};
    AudienceRule rule; rule.source = PeopleSource::StatusRecipients; rule.companyFilter = CompanyFilter::ExceptProjectClient;
    auto result = resolveAudience(snapshot, rule);
    QVERIFY(result.people.isEmpty());
    rule.includeUnknownCompany = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 1);
    QCOMPARE(result.people.first().id, QStringLiteral("unknown"));
    rule.source = PeopleSource::ChosenPeople;
    rule.chosenPersonIds = {QStringLiteral("missing")};
    result = resolveAudience(snapshot, rule);
    QVERIFY(result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("audience-empty"));

    rule.source = PeopleSource::CurrentSelection;
    snapshot.currentSelectionPersonIds = {QStringLiteral("client"), QStringLiteral("partner")};
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 2);

    rule.source = PeopleSource::ProjectTeam;
    rule.companyFilter = CompanyFilter::ManagingCompany;
    rule.includeUnknownCompany = false;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 1);
    QCOMPARE(result.people.first().id, QStringLiteral("manager"));

    rule.companyFilter = CompanyFilter::ProjectClient;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 1);
    QCOMPARE(result.people.first().id, QStringLiteral("client"));

    rule.companyFilter = CompanyFilter::ExceptProjectClient;
    rule.includeUnknownCompany = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 2);
    QCOMPARE(result.people.at(0).id, QStringLiteral("manager"));
    QCOMPARE(result.people.at(1).id, QStringLiteral("partner"));

    rule.source = PeopleSource::ChosenPeople;
    rule.chosenPersonIds = {QStringLiteral("manager"), QStringLiteral("partner")};
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 1);
    QCOMPARE(result.people.first().id, QStringLiteral("partner"));

    rule.source = PeopleSource::ProjectTeam;
    rule.companyFilter = CompanyFilter::SelectedCompanies;
    rule.companyIds = {QStringLiteral("partner")};
    rule.includeUnknownCompany = false;
    rule.excludeProjectManager = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 1);
    QCOMPARE(result.people.first().id, QStringLiteral("partner"));
    rule.includeUnknownCompany = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 2);

    snapshot.people.append({"outside", "Outside", "outside@example.test", "partner", "Partner Co", false, true, false});
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.people.size(), 4);
    QVERIFY(std::none_of(result.people.cbegin(), result.people.cend(),
                         [](const SnapshotPerson &person) { return person.id == QStringLiteral("outside"); }));

    rule.companyFilter = CompanyFilter::ManagingCompany;
    snapshot.managingCompanyId.clear();
    result = resolveAudience(snapshot, rule);
    QVERIFY(!result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("company-setup-required"));

    snapshot.managingCompanyId = QStringLiteral("ours");
    snapshot.clientCompanyId.clear();
    rule.companyFilter = CompanyFilter::ExceptProjectClient;
    result = resolveAudience(snapshot, rule);
    QVERIFY(!result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("company-setup-required"));

    snapshot.clientCompanyId = QStringLiteral("client");
    snapshot.people.append({"invalid", "Invalid", "invalid@example.test;other@example.test",
                            "partner", "Partner Co", false, true, true});
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QVERIFY(std::none_of(result.people.cbegin(), result.people.cend(),
                         [](const SnapshotPerson &person) { return person.id == QStringLiteral("invalid"); }));
}

void CommunicationContractsTest::appliesRecipientOverridesSafely()
{
    AudienceResolution audience;
    audience.people = {{"one", "One", "same@example.test", {}, {}, false, false},
                       {"two", "Two", "SAME@example.test", {}, {}, false, false},
                       {"bad", "Bad", "bad@example.test\r\nBcc:x@example.test", {}, {}, false, false}};
    const auto result = applyRecipientOverrides(audience, {{"one", true, RecipientRole::Cc}});
    QCOMPARE(result.recipients.size(), 1);
    QCOMPARE(result.recipients.first().role, RecipientRole::Cc);
    QVERIFY(!result.validation.ok());

    const auto conflict = applyRecipientOverrides(audience, {{"one", true, RecipientRole::To},
                                                               {"two", true, RecipientRole::Bcc}});
    QCOMPARE(conflict.recipients.size(), 1);
    QCOMPARE(conflict.recipients.first().role, RecipientRole::To);
    QVERIFY(!conflict.validation.ok());
}

void CommunicationContractsTest::retainsRecipientEditsAndManualEntries()
{
    RecipientSelectionModel model;
    AudienceResolution audience;
    audience.people = {{"one", "One", "one@example.test"}, {"two", "Two", "two@example.test"}};
    model.setAudience(audience);
    model.applyOverrides({{"one", false, RecipientRole::Bcc}});
    QCOMPARE(model.resolve().recipients.size(), 1);
    QCOMPARE(model.resolve().recipients.constFirst().address, QStringLiteral("two@example.test"));
    model.setAudience(audience);
    QCOMPARE(model.recipientCount(), 2);
    QCOMPARE(model.selectedRecipientCount(), 2);
    QVERIFY(model.audienceDiagnostic().isEmpty());
    QVERIFY(model.setSelected("one", false));
    QVERIFY(model.setRecipientRole("two", RecipientRole::Bcc));
    QVERIFY(model.addManual("Manual", "manual@example.test", RecipientRole::Cc));
    QVERIFY(!model.addManual("Duplicate source", "TWO@example.test", RecipientRole::Bcc));
    QVERIFY(!model.addManual("Duplicate manual", "MANUAL@example.test", RecipientRole::To));
    QVERIFY(!model.addManual("Invalid role", "other@example.test", static_cast<RecipientRole>(99)));
    QVERIFY(!model.addManual("Bad", "bad@example.test\r\nBcc:x@example.test"));
    model.setAddressLaterExplicitlyChosen(true);
    const auto resolved = model.resolve();
    QCOMPARE(resolved.recipients.size(), 2);
    QCOMPARE(resolved.recipients.at(0).role, RecipientRole::Bcc);
    QCOMPARE(resolved.recipients.at(1).role, RecipientRole::Cc);
    QVERIFY(resolved.addressLaterExplicitlyChosen);
    AudienceResolution replacement;
    replacement.people = {{"replacement", "Replacement", "replacement@example.test"}};
    model.setAudience(replacement);
    QCOMPARE(model.recipientCount(), 1);
    QVERIFY(!model.resolve().addressLaterExplicitlyChosen);
    QVERIFY(!model.setSelected("one", false));
    model.setAudience(audience);
    QVERIFY(model.addManual("Manual", "manual@example.test", RecipientRole::Cc));
    const QString manualId = model.data(model.index(2), RecipientSelectionModel::PersonIdRole).toString();
    QVERIFY(model.removeManual(manualId));
    QCOMPARE(model.recipientCount(), 2);
    QVERIFY(!model.removeManual(QStringLiteral("one")));
    model.reset();
    QCOMPARE(model.resolve().recipients.size(), 2);
    QVERIFY(!model.resolve().addressLaterExplicitlyChosen);
    AudienceResolution duplicateAddresses;
    duplicateAddresses.people = {{"dup-one", "Duplicate one", "same@example.test"},
                                 {"dup-two", "Duplicate two", "SAME@example.test"}};
    model.setAudience(duplicateAddresses);
    QCOMPARE(model.recipientCount(), 1);
    QCOMPARE(model.selectedRecipientCount(), 1);
    AudienceResolution empty; empty.validation.addWarning("audience-empty", "audience");
    model.setAudience(empty);
    QCOMPARE(model.recipientCount(), 0);
    QCOMPARE(model.audienceDiagnostic(), QStringLiteral("audience-empty"));
    QVERIFY(model.addManual("Manual", "manual@example.test"));
    QCOMPARE(model.resolve().validation.issues.constFirst().code, QStringLiteral("audience-empty"));
    model.reset();
    QCOMPARE(model.recipientCount(), 0);
    QVERIFY(!model.addressLaterExplicitlyChosen());
}

void CommunicationContractsTest::preservesRecipientResolutionOrderWhileGroupingAudience()
{
    CommunicationSnapshot snapshot;
    snapshot.projectManagerId = QStringLiteral("manager");
    snapshot.people = {
        {"partner", "Partner", "partner@example.test", "partner", "Partner Co", false, false, true},
        {"manager", "Manager", "manager@example.test", "ours", "Our Company", false, false, true},
        {"client", "Client", "client@example.test", "client", "Client Co", false, false, true},
        {"invalid", "Invalid", "not-an-address", "client", "Client Co", false, false, true},
        {"outside", "Outside", "outside@example.test", "other", "Other Co", false, false, false}
    };
    AudienceRule rule;
    rule.source = PeopleSource::ProjectTeam;
    rule.excludeProjectManager = true;
    const AudienceResolution audience = resolveAudience(snapshot, rule);
    QCOMPARE(audience.people.size(), 2);
    QCOMPARE(audience.exclusions.size(), 3);

    RecipientSelectionModel model;
    model.setAudience(audience);
    model.setInternalReportContext(true, QStringLiteral("ours"));
    QCOMPARE(model.internalAudienceWarning(),
             QStringLiteral("This internal report has 2 selected recipient(s) outside the managing company."));
    QCOMPARE(model.toRecipientCount(), 2);
    QVERIFY(model.setRecipientRole("client", RecipientRole::Cc));
    QCOMPARE(model.toRecipientCount(), 1);
    QCOMPARE(model.ccRecipientCount(), 1);
    QCOMPARE(model.bccRecipientCount(), 0);
    const QVariantList excluded = model.excludedRecipients();
    QCOMPARE(excluded.size(), 3);
    QVERIFY(std::any_of(excluded.cbegin(), excluded.cend(), [](const QVariant &value) {
        return value.toMap().value(QStringLiteral("reason")).toString()
            == QStringLiteral("Project manager is excluded");
    }));

    const RecipientResolution resolved = model.resolve();
    QCOMPARE(resolved.recipients.size(), 2);
    // The checklist is company-grouped, but output remains source order.
    QCOMPARE(resolved.recipients.at(0).address, QStringLiteral("partner@example.test"));
    QCOMPARE(resolved.recipients.at(1).address, QStringLiteral("client@example.test"));
    QCOMPARE(resolved.recipients.at(1).role, RecipientRole::Cc);

    model.setInternalReportContext(true, {});
    QCOMPARE(model.internalAudienceWarning(),
             QStringLiteral("This report is marked internal, but the managing company is not configured."));
}

void CommunicationContractsTest::parsesLiteralFieldsWithoutEvaluation()
{
    TemplateContext context; context.values.insert("project.name", "Safe <name>");
    const auto rendered = renderTemplateFields(R"(Hi {{ project.name }} \{{ literal }})", context);
    QVERIFY(rendered.validation.ok());
    QCOMPARE(rendered.text, QStringLiteral("Hi Safe <name> {{ literal }}"));
    const auto html = renderTemplateFields("<p>{{ project.name }}</p>", context, TemplateRenderMode::Html);
    QCOMPARE(html.text, QStringLiteral("<p>Safe &lt;name&gt;</p>"));
    const auto optional = renderTemplateFields("Meeting {{ meeting.date }}", context);
    QVERIFY(optional.validation.ok());
    QCOMPARE(optional.text, QStringLiteral("Meeting "));
    QVERIFY(!renderTemplateFields("{{ constructor.x }}", context).validation.ok());
    QVERIFY(!renderTemplateFields("{{ project.name() }}", context).validation.ok());
}

void CommunicationContractsTest::limitsFieldsToCatalogAndWorkflow()
{
    QVERIFY(templateFieldPaths(Workflow::SendMeetingNotes).contains(QStringLiteral("meeting.date")));
    QVERIFY(!templateFieldPaths(Workflow::StatusReport).contains(QStringLiteral("meeting.date")));
    QVERIFY(templateFieldIsApplicable(QStringLiteral("project.name"), Workflow::StatusReport));
    QVERIFY(templateFieldIsApplicable(QStringLiteral("meeting.date"), Workflow::SendMeetingNotes));
    QVERIFY(!templateFieldIsApplicable(QStringLiteral("meeting.date"), Workflow::StatusReport));
    QVERIFY(!templateFieldIsApplicable(QStringLiteral("arbitrary.secret"), Workflow::StatusReport));
    TemplateContext context;
    context.workflow = Workflow::StatusReport;
    context.values.insert(QStringLiteral("meeting.date"), QStringLiteral("09/15/2026"));
    QVERIFY(!renderTemplateFields(QStringLiteral("{{ meeting.date }}"), context).validation.ok());
}

void CommunicationContractsTest::runsBoundedComputedFields()
{
    TemplateContext context;
    context.values.insert("project.name", "North");
    context.values.insert("report.internal", "true");
    ComputedFieldRunner runner;
    const auto result = runner.evaluate({{"custom.label", "context['project.name'] + ' report'", true},
                                         {"custom.structured", "context.project.name + (context.report.internal === 'true' ? ' internal' : '')", true},
                                         {"custom.frozen", "(() => { try { context.project.name = 'Changed'; } catch (e) {} return context.project.name; })()", true},
                                         {"custom.function", "function compute(context, helpers) { return context.project.name + ' function'; }", true},
                                         {"custom.bad", "({ unexpected", true},
                                         {"custom.object", "({ value: 1 })", true}}, context);
    QCOMPARE(result.values.value("custom.label"), QStringLiteral("North report"));
    QCOMPARE(result.values.value("custom.structured"), QStringLiteral("North internal"));
    QCOMPARE(result.values.value("custom.frozen"), QStringLiteral("North"));
    QCOMPARE(result.values.value("custom.function"), QStringLiteral("North function"));
    QVERIFY(!result.validation.ok());
    std::atomic_bool cancelled = true;
    const auto stopped = runner.evaluate({{"custom.never", "'no'", true}}, context, &cancelled);
    QVERIFY(stopped.cancelled);
    const auto timedOut = runner.evaluate(
        {{"custom.loop", "(() => { while (true) {} })()", true}}, context);
    QVERIFY(!timedOut.validation.ok());
    QCOMPARE(timedOut.validation.issues.constFirst().code, QStringLiteral("computed-field-timeout"));

    const auto duplicate = runner.evaluate(
        {{"custom.label", "'first'", true}, {"custom.label", "'second'", true}}, context);
    QCOMPARE(duplicate.values.value("custom.label"), QStringLiteral("first"));
    QCOMPARE(duplicate.validation.issues.constFirst().code, QStringLiteral("computed-field-duplicate"));

    std::atomic_bool cancelDuringEvaluation = false;
    std::thread canceller([&cancelDuringEvaluation] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        cancelDuringEvaluation.store(true);
    });
    const auto interrupted = runner.evaluate(
        {{"custom.cancel", "(() => { while (true) {} })()", true}}, context,
        &cancelDuringEvaluation);
    canceller.join();
    QVERIFY(interrupted.cancelled);

    TemplateContext oversizedContext;
    oversizedContext.values.insert(QStringLiteral("project.name"),
                                   QString(ComputedFieldRunner::maximumContextCharacters, u'x'));
    const auto oversized = runner.evaluate({{"custom.context", "context.project.name", true}}, oversizedContext);
    QVERIFY(!oversized.validation.ok());
    QCOMPARE(oversized.validation.issues.constFirst().code, QStringLiteral("computed-field-context-invalid"));

    QList<ComputedFieldDefinition> tooMany;
    for (int index = 0; index <= ComputedFieldRunner::maximumDefinitions; ++index)
        tooMany.append({QStringLiteral("custom.limit%1").arg(index), QStringLiteral("'x'"), true});
    const auto definitionLimit = runner.evaluate(tooMany, context);
    QCOMPARE(definitionLimit.validation.issues.constFirst().code, QStringLiteral("computed-field-limit"));

    const auto expressionLimit = runner.evaluate(
        {{QStringLiteral("custom.expression"), QString(ComputedFieldRunner::maximumExpressionCharacters + 1, u'x'), true}}, context);
    QCOMPARE(expressionLimit.validation.issues.constFirst().code, QStringLiteral("computed-field-invalid"));
    const auto outputLimit = runner.evaluate(
        {{QStringLiteral("custom.output"), QStringLiteral("'x'.repeat(%1)").arg(ComputedFieldRunner::maximumOutputCharacters + 1), true}}, context);
    QCOMPARE(outputLimit.validation.issues.constFirst().code, QStringLiteral("computed-field-output-limit"));
    const auto unicodeOutputLimit = runner.evaluate(
        {{QStringLiteral("custom.unicodeOutput"), QStringLiteral("'🙂'.repeat(%1)")
            .arg(ComputedFieldRunner::maximumOutputCharacters / 4 + 1), true}}, context);
    QCOMPARE(unicodeOutputLimit.validation.issues.constFirst().code, QStringLiteral("computed-field-output-limit"));
}

void CommunicationContractsTest::evaluatesComputedFieldsAsynchronously()
{
    TemplateContext context;
    context.values.insert(QStringLiteral("project.name"), QStringLiteral("North"));
    ComputedFieldService service;
    const QUuid operation = QUuid::createUuid();
    bool completed = false;
    bool deliveredOnOwnerThread = false;
    ComputedFieldResult completedResult;
    QVERIFY(service.evaluate(operation,
                             {{QStringLiteral("custom.label"), QStringLiteral("context.project.name + ' report'"), true}},
                             context,
                             [operation, &completed, &deliveredOnOwnerThread, &completedResult](QUuid resultId,
                                                                                                ComputedFieldResult result) {
        QCOMPARE(resultId, operation);
        deliveredOnOwnerThread = QThread::currentThread() == QCoreApplication::instance()->thread();
        completedResult = std::move(result);
        completed = true;
    }));
    QVERIFY(!service.evaluate(operation, {}, context, [](QUuid, ComputedFieldResult) {}));
    QTRY_VERIFY_WITH_TIMEOUT(completed, 1000);
    QVERIFY(deliveredOnOwnerThread);
    QCOMPARE(completedResult.values.value(QStringLiteral("custom.label")), QStringLiteral("North report"));

    const QUuid cancelledOperation = QUuid::createUuid();
    bool cancelledCompleted = false;
    ComputedFieldResult cancelledResult;
    QVERIFY(service.evaluate(cancelledOperation,
                             {{QStringLiteral("custom.loop"), QStringLiteral("(() => { while (true) {} })()"), true}},
                             context,
                             [&cancelledCompleted, &cancelledResult](QUuid, ComputedFieldResult result) {
        cancelledResult = std::move(result);
        cancelledCompleted = true;
    }));
    service.cancel(cancelledOperation);
    QTRY_VERIFY_WITH_TIMEOUT(cancelledCompleted, 1000);
    QVERIFY(cancelledResult.cancelled);
}

void CommunicationContractsTest::storesTemplatesIdempotently()
{
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QSettings settings(directory.filePath("templates.ini"), QSettings::IniFormat);
    CommunicationTemplate bundled{"default", "Default", Workflow::StatusReport, "Built in", "<p>body</p>"};
    CommunicationTemplateStore store(settings, "db-a", {bundled});
    QVERIFY(store.save({"custom", "Custom", Workflow::StatusReport, "Subject", "Body"}).ok());
    QVERIFY(store.save({"incomplete", "Incomplete", Workflow::StatusReport, {},
                         "<p>Missing protected content</p>"}).ok());
    QVERIFY(!store.save({"bad-subject", "Bad", Workflow::StatusReport,
                         "Subject\r\nBcc: injected@example.test", "Body"}).ok());
    QCOMPARE(store.templates(Workflow::StatusReport).size(), 3);
    const QList<CommunicationTemplate> statusTemplates = store.templates(Workflow::StatusReport);
    const auto custom = std::find_if(statusTemplates.cbegin(), statusTemplates.cend(),
                                     [](const CommunicationTemplate &value) { return value.id == "custom"; });
    QVERIFY(custom != statusTemplates.cend());
    QVERIFY(std::any_of(statusTemplates.cbegin(), statusTemplates.cend(), [](const CommunicationTemplate &value) {
        return value.id == QStringLiteral("incomplete");
    }));
    CommunicationTemplateStore sameDatabase(settings, "db-b");
    QCOMPARE(sameDatabase.templates(Workflow::StatusReport).size(), 2);

    // A template imported by a prior release of the Python plug-in must not
    // become available to native workflows.
    const QJsonObject importedDefinition{{QStringLiteral("id"), QStringLiteral("legacy-imported")},
                                         {QStringLiteral("name"), QStringLiteral("Old plug-in template")},
                                         {QStringLiteral("workflow"), QStringLiteral("send-meeting-notes")},
                                         {QStringLiteral("subject"), QStringLiteral("Old subject")},
                                         {QStringLiteral("body"), QStringLiteral("<p>Old body</p>")},
                                         {QStringLiteral("legacyOriginal"), QStringLiteral("{\"Template\":\"<p>Old body</p>\"}")}};
    settings.setValue(QStringLiteral("Email/v1/Templates/db-legacy"),
                      QJsonDocument(QJsonArray{importedDefinition}).toJson(QJsonDocument::Compact));
    QVERIFY(global_DBObjects.saveParameter(QString::fromLatin1(kTemplateSettingKey), QString()));
    CommunicationTemplateStore legacy(settings, "db-legacy");
    QVERIFY(legacy.templates(Workflow::SendMeetingNotes).isEmpty());

    QVERIFY(global_DBObjects.saveParameter(QString::fromLatin1(kTemplateSettingKey), QByteArrayLiteral("{not-an-array}")));
    CommunicationTemplateStore corrupt(settings, "db-corrupt", {bundled});
    // Corrupt user state never hides the shipped fallback and is never
    // overwritten by a later save without surfacing the problem.
    QCOMPARE(corrupt.templates(Workflow::StatusReport).size(), 1);
    QCOMPARE(corrupt.save({"repair", "Repair", Workflow::StatusReport, "Subject", "Body"})
             .issues.constFirst().code, QStringLiteral("template-store-corrupt"));

}

void CommunicationContractsTest::exposesBundledTemplateForEveryNativeWorkflow()
{
    const QString organization = QStringLiteral("ProjectNotesBundledTemplatesTest-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    TemplateEditorModel editor(organization, m_templateDatabasePath);
    const QList<QPair<QString, QString>> expected = {
        {QStringLiteral("send-meeting-notes"), QStringLiteral("send-meeting-notes-native-v1")},
        {QStringLiteral("meeting-notes-report"), QStringLiteral("meeting-notes-report-native-v1")},
        {QStringLiteral("status-report"), QStringLiteral("status-report-native-v1")},
        {QStringLiteral("tracker-items-report"), QStringLiteral("tracker-items-report-native-v1")}
    };
    for (const auto &[workflow, id] : expected) {
        editor.setWorkflow(workflow);
        QCOMPARE(editor.templates().size(), 1);
        QCOMPARE(editor.templates().constFirst().toMap().value(QStringLiteral("id")).toString(), id);
        QVERIFY(editor.draftRichBody().contains(QStringLiteral("{{ content.body }}")));
        QVERIFY(editor.draftPlainBody().isEmpty());
    }
}

void CommunicationContractsTest::switchingTemplateDatabaseDiscardsPriorSelection()
{
    const QString organization = QStringLiteral("ProjectNotesTemplateDatabaseSwitchTest-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    TemplateEditorModel editor(organization, m_templateDatabasePath);
    editor.setWorkflow(QStringLiteral("status-report"));
    editor.beginNew();
    editor.setDraftName(QStringLiteral("Database A only"));
    editor.setDraftSubject(QStringLiteral("Status"));
    editor.setDraftRichBody(QStringLiteral("<p>Database A</p>{{ content.body }}"));
    QVERIFY(editor.save());
    QCOMPARE(editor.selectedTemplateId(), QStringLiteral("status-report-native-v1"));

    editor.setDatabaseKey(QStringLiteral("database-b"));
    QCOMPARE(editor.templates().size(), 1);
    QCOMPARE(editor.selectedTemplateId(), QStringLiteral("status-report-native-v1"));
}

void CommunicationContractsTest::editsTemplatesWithoutQmlEvaluation()
{
    const QString organization = QStringLiteral("ProjectNotesTemplateEditorTest-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    TemplateEditorModel editor(organization, m_templateDatabasePath);
    QCOMPARE(editor.templates().size(), 1);
    QCOMPARE(editor.templates().constFirst().toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("send-meeting-notes-native-v1"));
    editor.setWorkflow(QStringLiteral("status-report"));
    QCOMPARE(editor.templates().size(), 1);
    QCOMPARE(editor.templates().constFirst().toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("status-report-native-v1"));
    editor.beginNew();
    editor.setDraftName(QStringLiteral("Status prose"));
    editor.setDraftSubject(QStringLiteral("{{ project.name }} status"));
    editor.setDraftRichBody(QStringLiteral("<p>Hello "));
    QVERIFY(editor.appendField(QStringLiteral("project.name"), QStringLiteral("richBody")));
    QVERIFY(!editor.appendField(QStringLiteral("meeting.date"), QStringLiteral("richBody")));
    editor.setDraftRichBody(editor.draftRichBody() + QStringLiteral("</p>{{ content.body }}"));
    editor.setDraftPlainBody(QStringLiteral("Hello {{ project.name }}\n{{ content.body }}"));
    QVERIFY(editor.validateDraft());
    QVERIFY(editor.save());
    QVERIFY(!editor.selectedTemplateId().isEmpty());

    TemplateEditorModel reopened(organization, m_templateDatabasePath);
    reopened.setWorkflow(QStringLiteral("status-report"));
    QVERIFY(reopened.selectTemplate(editor.selectedTemplateId()));
    QCOMPARE(reopened.draftPlainBody(), QStringLiteral("Hello {{ project.name }}\n{{ content.body }}"));
    reopened.beginNew();
    reopened.setDraftName(QStringLiteral("Incomplete prose"));
    reopened.setDraftSubject(QStringLiteral("{{ meeting.date }}"));
    reopened.setDraftRichBody(QStringLiteral("<p>Missing protected content</p>"));
    QVERIFY(!reopened.validateDraft());
    QVERIFY(reopened.save());
    QVERIFY(reopened.diagnostic() != QString());
    QVERIFY(!reopened.duplicateSelected());
    QVERIFY(reopened.save());
    QCOMPARE(reopened.templates().size(), 1);
    QVERIFY(reopened.resetSelected());
    QCOMPARE(reopened.templates().size(), 1);

    TemplateEditorModel isolated(organization, QStringLiteral("database-b"));
    isolated.setWorkflow(QStringLiteral("status-report"));
    QCOMPARE(isolated.templates().size(), 1);
}

void CommunicationContractsTest::appliesTemplateAroundProtectedNativeContent()
{
    CommunicationTemplate templateValue{"meeting-prose", "Meeting prose", Workflow::SendMeetingNotes,
                                        "{{ project.name }} notes", "<p>Hello {{ client.name }}</p>{{ content.body }}",
                                        false, "Hello {{ client.name }}\n{{ content.body }}"};
    TemplateContext context;
    context.workflow = Workflow::SendMeetingNotes;
    context.values = {{"project.name", "North"}, {"client.name", "Safe <Client>"}};
    const TemplateApplicationResult applied = TemplateApplication::apply(
        templateValue, context, QStringLiteral("<table><tr><td>Native</td></tr></table>"),
        QStringLiteral("Native plain"));
    QVERIFY(applied.validation.ok());
    QCOMPARE(applied.subject, QStringLiteral("North notes"));
    QVERIFY(applied.html.contains(QStringLiteral("Safe &lt;Client&gt;")));
    QVERIFY(applied.html.contains(QStringLiteral("<table><tr><td>Native</td></tr></table>")));
    QCOMPARE(applied.plainText, QStringLiteral("Hello Safe <Client>\nNative plain"));

    // The protected block must not be represented by a marker that field data
    // can reproduce; that would turn ordinary text into native report markup.
    context.values.insert(QStringLiteral("client.name"), QString(QChar(0xe000)));
    const auto markerValue = TemplateApplication::apply(
        templateValue, context, QStringLiteral("<table>Native once</table>"), QStringLiteral("Native plain"));
    QVERIFY(markerValue.validation.ok());
    QCOMPARE(markerValue.html.count(QStringLiteral("<table>Native once</table>")), 1);
    QVERIFY(markerValue.html.contains(QString(QChar(0xe000))));
    context.values.insert(QStringLiteral("client.name"), QStringLiteral("Safe <Client>"));

    context.values.insert(QStringLiteral("project.name"), QStringLiteral("North\r\nBcc: injected@example.test"));
    const auto expandedHeader = TemplateApplication::apply(
        templateValue, context, QStringLiteral("<table>Native</table>"), QStringLiteral("Native plain"));
    QVERIFY(!expandedHeader.validation.ok());
    QCOMPARE(expandedHeader.validation.issues.constLast().code,
             QStringLiteral("template-subject-header-injection"));
    context.values.insert(QStringLiteral("project.name"), QStringLiteral("North"));

    templateValue.body = QStringLiteral("<p>Missing protected content</p>");
    QVERIFY(!TemplateApplication::apply(templateValue, context, "<p>Native</p>", "Native").validation.ok());
    templateValue.body = QStringLiteral("{{ content.body }} {{ content.body }}");
    QVERIFY(!TemplateApplication::apply(templateValue, context, "<p>Native</p>", "Native").validation.ok());
}

void CommunicationContractsTest::buildsContextSpecificMeetingNotes()
{
    MeetingNotesBuildInput input;
    input.snapshot.projectNumber = "P-1"; input.snapshot.projectName = "North";
    input.snapshot.people = {{"a", "Alice", {}, {}, {}, false, true}};
    input.snapshot.notes = {{"note", "Kickoff", "<p>Discussed <b>scope</b></p>",
                             QDateTime(QDate(2026, 9, 12), QTime(9, 0)), false, {"a"}}};
    input.noteId = "note"; input.snapshot.actionItems = {{"note", "Draft", "Alice", "Assigned", "09/20/2026"}};
    ValidationResult validation;
    const auto document = MeetingNotesEmailBuilder::build(input, &validation);
    QVERIFY(validation.ok()); QVERIFY(document.has_value());
    QVERIFY(document->htmlDocument.contains("Discussed <b>scope</b>"));
    QVERIFY(document->htmlDocument.contains("Alice"));
    QVERIFY(document->htmlDocument.contains("Draft"));
    QVERIFY(document->emailFragment.contains("border-collapse:collapse;width:100%;"));
    QVERIFY(document->emailFragment.contains("bgcolor='#e7e6e6'"));
    QVERIFY(document->emailFragment.contains("background-color:#e7e6e6;"));
    QVERIFY(document->emailFragment.contains("bgcolor='#d9e1f2'"));
    QVERIFY(document->emailFragment.contains("background-color:#d9e1f2;"));
    QCOMPARE(document->plainText, QStringLiteral("Discussed scope\n\nAction Items:\nDraft  Assigned To: Alice  Due By: 09/20/2026\n\nAttendees:\nAlice"));
    const QRegularExpression cells(QStringLiteral("<(?:th|td)\\b[^>]*>"));
    auto matches = cells.globalMatch(document->emailFragment);
    while (matches.hasNext())
        QVERIFY(matches.next().captured().contains("border:1px solid #808080;"));
    // Styling the generated table must not rewrite tables inside the note.
    input.snapshot.notes[0].html = "<table><tr><td style='background-color:red'>Custom</td></tr></table>";
    input.snapshot.actionItems.clear();
    const auto emptyActions = MeetingNotesEmailBuilder::build(input);
    QVERIFY(emptyActions.has_value());
    QVERIFY(emptyActions->emailFragment.contains(input.snapshot.notes[0].html));
    QVERIFY(emptyActions->emailFragment.contains("No action items."));
    QCOMPARE(document->defaultSubject, QStringLiteral("P-1 North - 09/12/2026 Kickoff Notes"));
}

void CommunicationContractsTest::createsMeetingNotesReviewFromSnapshot()
{
    CommunicationSnapshot snapshot;
    snapshot.projectId = "project"; snapshot.projectNumber = "P-1"; snapshot.projectName = "North";
    snapshot.databaseGeneration = 4;
    snapshot.projectManagerId = "manager";
    snapshot.people = {{"manager", "Manager", "manager@example.test", {}, {}, false, false, true},
                       {"member", "Member", "member@example.test", {}, {}, false, false, true},
                       {"absent", "Absent", "absent@example.test", {}, {}, false, true, true},
                       {"guest", "Guest", "guest@example.test", {}, {}, false, false, false},
                       {"other", "Other note attendee", "other@example.test", {}, {}, false, true, false}};
    snapshot.notes = {{"note", "Kickoff", "<p>Notes</p>",
                       QDateTime(QDate(2026, 9, 12), QTime(9, 0)), false, {"manager", "member", "guest"}}};
    SourceContext source{"database", 4, "project", {"note"}, Workflow::SendMeetingNotes};
    const auto review = MeetingNotesPreparationFactory::create(snapshot, source,
                                                                BackendId::Thunderbird,
                                                                EmailMode::HtmlAttachment);
    QVERIFY(review.has_value());
    QVERIFY(review->validation.ok());
    QCOMPARE(review->preparation.source.databaseGeneration, quint64(4));
    QCOMPARE(review->preparation.backend, BackendId::Thunderbird);
    QCOMPARE(review->preparation.mode, EmailMode::HtmlAttachment);
    // The desktop stages an operation before populating the recipient model.
    QTemporaryDir staging;
    QVERIFY(staging.isValid());
    ArtifactStore store(staging.path());
    OperationManifest manifest;
    manifest.operationId = review->preparation.operationId;
    manifest.workflow = review->preparation.source.workflow;
    manifest.databaseKey = review->preparation.source.databaseKey;
    manifest.projectId = review->preparation.source.projectId;
    manifest.backend = review->preparation.backend;
    ServiceError stagingError;
    QVERIFY2(store.createOperation(manifest, &stagingError), qPrintable(stagingError.code));
    QCOMPARE(review->preparation.recipients.size(), 2);
    QCOMPARE(review->audience.people.size(), 3); // Attendees and absent team, excluding the manager.
    RecipientSelectionModel recipients;
    recipients.setAudience(review->audience);
    QCOMPARE(recipients.rowCount(), 3);
    QCOMPARE(recipients.selectedRecipientCount(), 2);
    QCOMPARE(recipients.toRecipientCount(), 2);
    QCOMPARE(recipients.ccRecipientCount(), 0);
    for (int row = 0; row < recipients.rowCount(); ++row) {
        const auto index = recipients.index(row);
        const bool absent = recipients.data(index, RecipientSelectionModel::PersonIdRole).toString() == "absent";
        QCOMPARE(recipients.data(index, RecipientSelectionModel::SelectedRole).toBool(), !absent);
        QCOMPARE(recipients.data(index, RecipientSelectionModel::RecipientRoleRole).toInt(),
                 static_cast<int>(absent ? RecipientRole::Cc : RecipientRole::To));
    }
    QVERIFY(!recipients.setSelected("manager", true));
    QVERIFY(!recipients.setSelected("other", true));
    QVERIFY(recipients.setSelected("absent", true));
    QCOMPARE(recipients.resolve().recipients.size(), 3);
    QCOMPARE(recipients.ccRecipientCount(), 1);
    QCOMPARE(recipients.resolve().recipients.at(1).role, RecipientRole::Cc);
    QVERIFY(review->preparation.document.emailFragment.contains("Kickoff"));

    auto noAttendees = snapshot;
    noAttendees.notes.first().attendeeIds.clear();
    const auto teamOnly = MeetingNotesPreparationFactory::create(noAttendees, source);
    QVERIFY(teamOnly.has_value());
    QVERIFY(teamOnly->preparation.recipients.isEmpty());
    recipients.setAudience(teamOnly->audience);
    QCOMPARE(recipients.rowCount(), 2);
    QCOMPARE(recipients.selectedRecipientCount(), 0);
    QVERIFY(recipients.setSelected("member", true));
    QCOMPARE(recipients.ccRecipientCount(), 1);

    CommunicationTemplate templateValue{"meeting-prose", "Meeting prose", Workflow::SendMeetingNotes,
                                        "{{ project.name }} notes", "<p>Prepared for {{ project.name }}</p>{{ content.body }}"};
    const auto templated = MeetingNotesPreparationFactory::create(snapshot, source, BackendId::Mailto,
                                                                    EmailMode::InlineHtml, &templateValue);
    QVERIFY(templated.has_value());
    QCOMPARE(templated->preparation.document.defaultSubject, QStringLiteral("North notes"));
    QVERIFY(templated->preparation.document.emailFragment.contains(QStringLiteral("Prepared for North")));
    QVERIFY(templated->preparation.document.emailFragment.contains(QStringLiteral("Kickoff")));

    templateValue.body = QStringLiteral("<p>{{ client.name }}</p>{{ content.body }}");
    ValidationResult unavailableTemplate;
    QVERIFY(!MeetingNotesPreparationFactory::create(snapshot, source, BackendId::Mailto,
                                                     EmailMode::InlineHtml, &templateValue,
                                                     &unavailableTemplate).has_value());
    QVERIFY(!unavailableTemplate.ok());
    QCOMPARE(unavailableTemplate.issues.constFirst().code, QStringLiteral("template-field-unresolved"));

    source.noteIds = {"note", "second"};
    QVERIFY(!MeetingNotesPreparationFactory::create(snapshot, source).has_value());
    source.noteIds = {"note"}; source.databaseGeneration = 5;
    QVERIFY(!MeetingNotesPreparationFactory::create(snapshot, source).has_value());
}

void CommunicationContractsTest::buildsProjectWideMeetingNotesReport()
{
    MeetingNotesReportInput input;
    input.snapshot.projectNumber = "P-1"; input.snapshot.projectName = "North";
    input.snapshot.notes = {{"external", "External", "<p>Public</p>", QDateTime(QDate(2026, 9, 10), {}), false},
                            {"internal", "Internal", "<p>Private</p>", QDateTime(QDate(2026, 9, 11), {}), true},
                            {"future", "Future", "<p>Later</p>", QDateTime(QDate(2026, 9, 13), {}), false}};
    const QString noteHtml = "<p class='cell-value' style='color:#123456'>Public<br><b>Formatted notes</b></p>";
    input.snapshot.notes[0].html = noteHtml;
    input.snapshot.actionItems = {{"external", "Follow <up>", "Alice", "Assigned", "09/20/2026"}};
    input.reportingDate = QDate(2026, 9, 12);
    ValidationResult validation; auto external = MeetingNotesReportBuilder::build(input, &validation);
    QVERIFY(validation.ok()); QVERIFY(external.has_value()); QVERIFY(external->htmlDocument.contains("Public"));
    QVERIFY(!external->htmlDocument.contains("Private")); QVERIFY(!external->htmlDocument.contains("Later"));
    const QString email = external->emailFragment;
    QVERIFY(!email.contains("<style>"));
    QVERIFY(email.contains("font-family:Calibri,Arial,sans-serif"));
    QVERIFY(email.contains("border-collapse:collapse;width:100%"));
    QVERIFY(email.contains("border:1px solid #808080;padding:3px 6px"));
    QVERIFY(email.contains("background-color:#DCE6F1"));
    QVERIFY(email.contains("background-color:#EEECE1"));
    QVERIFY(email.contains("font-size:13pt;color:#1F497D"));
    QVERIFY(email.contains("font-size:11pt;font-weight:bold;color:#1F497D"));
    QVERIFY(email.contains("width:55%"));
    QVERIFY(email.contains("background-color:#DCE6F1;text-align:center"));
    QVERIFY(email.contains("Follow &lt;up&gt;"));
    QVERIFY(email.contains(noteHtml)); // Do not rewrite user-authored classes/styles.
    QVERIFY(!email.contains("Private"));
    QVERIFY(!email.contains("Later"));
    QCOMPARE(external->fileStem, QStringLiteral("P-1 Meeting Minutes"));
    input.internalReport = true; const auto internal = MeetingNotesReportBuilder::build(input);
    QVERIFY(internal->htmlDocument.contains("Private"));
    QVERIFY(internal->fileStem.endsWith(" Internal"));
}

void CommunicationContractsTest::preparesProjectWideMeetingNotesReport()
{
    CommunicationSnapshot snapshot; snapshot.projectId = "project"; snapshot.projectNumber = "P"; snapshot.projectName = "North"; snapshot.databaseGeneration = 3;
    snapshot.notes = {{"note", "Kickoff", "<p>Body</p>", QDateTime(QDate(2026, 9, 1), {}), false}};
    SourceContext source{"db", 3, "project", {}, Workflow::MeetingNotesReport};
    const auto preparation = MeetingNotesReportPreparationFactory::create(snapshot, source, QDate(2026, 9, 2), false);
    QVERIFY(preparation.has_value());
    QCOMPARE(preparation->document.workflow, Workflow::MeetingNotesReport);
    QCOMPARE(preparation->source.workflow, Workflow::MeetingNotesReport);
    QVERIFY(preparation->document.emailFragment.contains("Kickoff"));
    const auto retained = MeetingNotesReportPreparationFactory::create(
        snapshot, source, QDate(2026, 9, 2), false, BackendId::Mailto,
        EmailMode::None, true);
    QVERIFY(retained.has_value());
    QVERIFY(retained->retainHtml);

    CommunicationTemplate templateValue{"report-prose", "Report prose", Workflow::MeetingNotesReport,
                                        "{{ project.name }} report through {{ meeting.date }}",
                                        "<p>Prepared report</p>{{ content.body }}"};
    ValidationResult templateValidation;
    const auto templated = MeetingNotesReportPreparationFactory::create(
        snapshot, source, QDate(2026, 9, 2), false, BackendId::Mailto,
        EmailMode::InlineHtml, false, &templateValidation, &templateValue);
    QVERIFY(templateValidation.ok());
    QVERIFY(templated.has_value());
    QCOMPARE(templated->document.defaultSubject, QStringLiteral("North report through 09/02/2026"));
    QVERIFY(templated->document.emailFragment.contains(QStringLiteral("Prepared report")));
    QVERIFY(templated->document.emailFragment.contains(QStringLiteral("Kickoff")));
}

void CommunicationContractsTest::preparesStatusAndTrackerReportsFromSnapshot()
{
    CommunicationSnapshot snapshot; snapshot.projectId="project"; snapshot.projectNumber="P";
    snapshot.projectName="North"; snapshot.databaseGeneration=3; snapshot.projectManagerId="manager";
    snapshot.statusReportPeriod="Weekly"; snapshot.budget="100"; snapshot.actual="25";
    snapshot.bcwp="30"; snapshot.bcws="35"; snapshot.bac="100";
    snapshot.people={{"manager","Manager","manager@example.test",{}, {}, false, false, true},
                     {"status","Status","status@example.test",{}, {}, true, false, true},
                     {"outside","Outside","outside@example.test",{}, {}, true, false, false}};
    snapshot.statusItems={{"In Progress", "Build"}, {"Next Period", "Review"}, {"Completed", "Plan"}};
    snapshot.trackerItems={{"001","Risk","Manager","09/01/2026","Description","Status","High","New","09/20/2026","09/01/2026",{},"Still open","Tracker",false},
                           {"002","Task","Manager","09/01/2026","Implement","Status","Medium","Assigned","09/21/2026","09/02/2026",{}, {},"Tracker",false},
                           {"003","Resolved risk","Manager","09/01/2026","Done","Status","High","Resolved","09/22/2026","09/02/2026",{}, {},"Tracker",false},
                           {"004","Internal risk","Manager","09/01/2026","Private","Status","Low","New","09/23/2026","09/02/2026",{}, {},"Tracker",true}};
    SourceContext source{"db",3,"project",{},Workflow::StatusReport};
    auto statusOptions=defaultReportOptions(Workflow::StatusReport,QDate(2026,9,15));
    statusOptions.emailMode = EmailMode::HtmlAttachment;
    statusOptions.retainHtml = true;
    statusOptions.displayPdf = true;
    const auto status=ProjectReportPreparationFactory::createStatus(snapshot,source,statusOptions);
    QVERIFY(status.has_value()); QCOMPARE(status->preparation.document.workflow,Workflow::StatusReport);
    QCOMPARE(status->preparation.mode, EmailMode::HtmlAttachment);
    QVERIFY(status->preparation.retainHtml);
    QVERIFY(status->preparation.displayPdf);
    QVERIFY(status->preparation.document.emailFragment.contains("Build")); QVERIFY(status->preparation.document.emailFragment.contains("Risk"));
    QVERIFY(!status->preparation.document.emailFragment.contains("Resolved risk"));
    QVERIFY(!status->preparation.document.emailFragment.contains("Internal risk"));
    QCOMPARE(status->audience.people.size(), 2);
    QCOMPARE(status->audience.initialOverrides.size(), 1);
    QCOMPARE(status->audience.initialOverrides.constFirst().personId, QStringLiteral("manager"));
    RecipientSelectionModel statusRecipients;
    statusRecipients.setAudience(status->audience);
    QCOMPARE(statusRecipients.rowCount(), 2);
    QCOMPARE(statusRecipients.selectedRecipientCount(), 1);
    QCOMPARE(statusRecipients.data(statusRecipients.index(0), RecipientSelectionModel::SelectedRole).toBool(),
             false);
    QCOMPARE(status->preparation.recipients.size(), 1);
    QCOMPARE(status->preparation.recipients.constFirst().address,QStringLiteral("status@example.test"));

    CommunicationTemplate statusTemplate{"status-prose", "Status prose", Workflow::StatusReport,
                                         "{{ project.number }} {{ report.type }} {{ report.date }}",
                                         "<p>Prepared {{ report.internal }}</p>{{ content.body }}"};
    const auto templatedStatus = ProjectReportPreparationFactory::createStatus(
        snapshot, source, statusOptions, BackendId::Mailto, &statusTemplate);
    QVERIFY(templatedStatus.has_value());
    QCOMPARE(templatedStatus->preparation.document.defaultSubject,
             QStringLiteral("P Status Report 09/15/2026"));
    QVERIFY(templatedStatus->preparation.document.emailFragment.contains(QStringLiteral("Prepared No")));
    QVERIFY(templatedStatus->preparation.document.emailFragment.contains(QStringLiteral("Build")));

    statusOptions.internalReport = true;
    const auto internalStatus = ProjectReportPreparationFactory::createStatus(snapshot, source, statusOptions);
    QVERIFY(internalStatus.has_value());
    QVERIFY(internalStatus->preparation.document.emailFragment.contains(QStringLiteral("Internal risk")));

    auto trackerOptions=defaultReportOptions(Workflow::TrackerItemsReport,QDate(2026,9,15));
    trackerOptions.emailMode = EmailMode::None;
    trackerOptions.retainHtml = true;
    trackerOptions.displayPdf = true;
    const auto tracker=ProjectReportPreparationFactory::createTracker(snapshot,source,trackerOptions);
    QVERIFY(tracker.has_value()); QCOMPARE(tracker->preparation.document.workflow,Workflow::TrackerItemsReport);
    QCOMPARE(tracker->preparation.mode, EmailMode::None);
    QVERIFY(tracker->preparation.retainHtml);
    QVERIFY(tracker->preparation.displayPdf);
    QVERIFY(tracker->preparation.document.emailFragment.contains("Task"));
    QCOMPARE(tracker->audience.people.size(), 2);
    QCOMPARE(tracker->preparation.recipients.size(), 1);
    QCOMPARE(tracker->preparation.recipients.constFirst().address, QStringLiteral("status@example.test"));
}

void CommunicationContractsTest::encodesAndBoundsMailtoRequests()
{
    EmailRequest request; request.operationId = QUuid::createUuid(); request.subject = "A & B + C";
    request.plainText = "First line\nSecond & line";
    request.recipients = {{"To", "to@example.test", RecipientRole::To}, {"Bcc", "bcc@example.test", RecipientRole::Bcc}};
    ValidationResult validation; const auto url = MailtoEmailBackend::buildUrl(request, &validation);
    QVERIFY(validation.ok()); QVERIFY(url.has_value());
    QVERIFY(url->toEncoded().contains("subject=A%20%26%20B%20%2B%20C"));
    QVERIFY(url->toEncoded().contains("bcc=bcc%40example.test"));
    QVERIFY(url->toEncoded().contains("body=First%20line%0ASecond%20%26%20line"));
    request.html = "<p>not mailto</p>"; QVERIFY(!MailtoEmailBackend::buildUrl(request).has_value());
    request.html.clear(); request.subject = "bad\r\nBcc:x@example.test"; QVERIFY(!MailtoEmailBackend::buildUrl(request).has_value());
    request.subject.clear(); request.plainText = QString(2000, 'x'); QVERIFY(!MailtoEmailBackend::buildUrl(request).has_value());
    QUrl launched; int launches = 0; MailtoEmailBackend backend([&launched, &launches](const QUrl &value) { launched = value; ++launches; return true; });
    request.plainText = "ok"; bool complete = false; backend.handoff(request, [&complete](EmailHandoffResult result) { complete = true; QCOMPARE(result.certainty, OutcomeCertainty::Uncertain); });
    QVERIFY(complete); QVERIFY(!launched.isEmpty());
    QString repeatedError;
    backend.handoff(request, [&repeatedError](EmailHandoffResult result) { repeatedError = result.error.code; });
    QCOMPARE(repeatedError, QStringLiteral("mailto-launch-already-attempted"));
    QCOMPARE(launches, 1);

    request.operationId = QUuid::createUuid();
    int failedLaunches = 0;
    MailtoEmailBackend failingBackend([&failedLaunches](const QUrl &) { ++failedLaunches; return false; });
    QString failedError;
    failingBackend.handoff(request, [&failedError](EmailHandoffResult result) { failedError = result.error.code; });
    QCOMPARE(failedError, QStringLiteral("mailto-launch-failed"));
    failedError.clear();
    failingBackend.handoff(request, [&failedError](EmailHandoffResult result) { failedError = result.error.code; });
    QCOMPARE(failedError, QStringLiteral("mailto-launch-already-attempted"));
    QCOMPARE(failedLaunches, 1);
}

void CommunicationContractsTest::createsThunderbirdComposeArguments()
{
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString bodyPath = directory.filePath("body with spaces.html"); QFile body(bodyPath); QVERIFY(body.open(QIODevice::WriteOnly)); body.write("<p>body</p>"); body.close();
    const QString attachmentPath = directory.filePath("attachment.pdf"); QFile attachment(attachmentPath); QVERIFY(attachment.open(QIODevice::WriteOnly)); attachment.write("pdf"); attachment.close();
    EmailRequest request; request.subject = "Subject, Unicode ✓";
    request.html = QStringLiteral("<p>body</p>");
    request.recipients = {{"To", "to@example.test", RecipientRole::To}, {"Cc", "cc@example.test", RecipientRole::Cc}, {"Bcc", "bcc@example.test", RecipientRole::Bcc}};
    request.attachments = {{QUuid::createUuid(), attachmentPath, "attachment.pdf", "application/pdf", 3}};
    ValidationResult validation; const auto arguments = ThunderbirdEmailBackend::composeArguments(
        request, bodyPath, directory.path(), &validation);
    QVERIFY(validation.ok()); QVERIFY(arguments.has_value()); QCOMPARE(arguments->first(), QStringLiteral("-compose"));
    QVERIFY(arguments->at(1).contains("to='to@example.test'")); QVERIFY(arguments->at(1).contains("cc='cc@example.test'"));
    QVERIFY(arguments->at(1).contains("bcc='bcc@example.test'")); QVERIFY(arguments->at(1).contains("file:///"));
    QVERIFY(arguments->at(1).contains("message='" + bodyPath + "'"));
    QVERIFY(arguments->at(1).contains("format=html"));
    QVERIFY(!arguments->at(1).contains("body="));
    request.html.clear();
    const auto plainArguments = ThunderbirdEmailBackend::composeArguments(
        request, bodyPath, directory.path(), &validation);
    QVERIFY(plainArguments.has_value());
    QVERIFY(plainArguments->at(1).contains("format=text"));
}

void CommunicationContractsTest::launchesThunderbirdCommandWithFixedArguments()
{
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString bodyPath = directory.filePath(QStringLiteral("message.html"));
    QFile body(bodyPath); QVERIFY(body.open(QIODevice::WriteOnly)); body.write("<p>body</p>"); body.close();
    QString program;
    QStringList arguments;
    ThunderbirdEmailBackend backend(QStringLiteral("/bin/flatpak run org.mozilla.Thunderbird"),
        [&program, &arguments](const QString &receivedProgram, const QStringList &receivedArguments) {
            program = receivedProgram; arguments = receivedArguments; return true;
        });
    const QUuid operationId = QUuid::createUuid();
    backend.setBodyFile(operationId, bodyPath, directory.path());
    EmailRequest request; request.operationId = operationId; request.subject = QStringLiteral("Subject");
    request.recipients = {{QStringLiteral("To"), QStringLiteral("to@example.test"), RecipientRole::To}};
    bool completed = false;
    backend.handoff(request, [&completed](EmailHandoffResult result) { completed = result.error.code.isEmpty(); });
    QVERIFY(completed);
    QCOMPARE(program, QStringLiteral("/bin/flatpak"));
    QCOMPARE(arguments.first(), QStringLiteral("run"));
    const QString stagingRoot = QFileInfo(directory.path()).dir().absolutePath();
    QCOMPARE(arguments.at(1), QStringLiteral("--filesystem=%1:ro").arg(stagingRoot));
    QCOMPARE(arguments.at(2), QStringLiteral("org.mozilla.Thunderbird"));
    QCOMPARE(arguments.at(3), QStringLiteral("-compose"));
}

void CommunicationContractsTest::resolvesThunderbirdExecutable()
{
    QStringList probed;
    const auto probe = [&probed](const QString &path) {
        probed.append(path);
        return path == QLatin1String("/candidates/thunderbird");
    };
    QCOMPARE(ThunderbirdEmailBackend::resolveExecutable(
                 QStringLiteral("/configured/missing"),
                 {QStringLiteral("/missing"), QStringLiteral("/candidates/thunderbird"),
                  QStringLiteral("/candidates/thunderbird")}, probe),
             QStringLiteral("/candidates/thunderbird"));
    QCOMPARE(probed, QStringList({QStringLiteral("/configured/missing"), QStringLiteral("/missing"),
                                  QStringLiteral("/candidates/thunderbird")}));
    QCOMPARE(ThunderbirdEmailBackend::resolveExecutable(
                 QStringLiteral("/candidates/thunderbird"), {QStringLiteral("/ignored")}, probe),
             QStringLiteral("/candidates/thunderbird"));
}

void CommunicationContractsTest::validatesOutlookHelperProtocolFrames()
{
    OutlookHelperRequest request;
    request.operationId = QUuid::createUuid();
    request.command = OutlookHelperCommand::Prepare;
    request.payload = {{QStringLiteral("artifactPath"), QStringLiteral("/staging/message.html")},
                       {QStringLiteral("recipientCount"), 2}};
    ValidationResult validation;
    const auto encoded = encodeOutlookHelperRequest(request, &validation);
    QVERIFY(validation.ok());
    QVERIFY(encoded.has_value());
    QVERIFY(encoded->endsWith('\n'));
    const auto decoded = decodeOutlookHelperRequest(encoded->chopped(1), &validation);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->operationId, request.operationId);
    QCOMPARE(decoded->command, OutlookHelperCommand::Prepare);
    QCOMPARE(decoded->payload, request.payload);

    EmailRequest emailRequest;
    emailRequest.operationId = request.operationId;
    emailRequest.subject = QStringLiteral("Unicode ✓");
    emailRequest.html = QStringLiteral("<p>Body</p>");
    emailRequest.recipients = {{QStringLiteral("To"), QStringLiteral("to@example.test"), RecipientRole::To},
                               {QStringLiteral("Cc"), QStringLiteral("cc@example.test"), RecipientRole::Cc}};
    emailRequest.attachments = {{QUuid::createUuid(), QStringLiteral("/staging/report.pdf"),
                                 QStringLiteral("report.pdf"), QStringLiteral("application/pdf"), 1, {}, true}};
    const auto typedRequest = makeOutlookPrepareRequest(emailRequest, QStringLiteral("/staging/body.html"),
                                                         QStringLiteral("/staging"), &validation);
    QVERIFY(typedRequest.has_value());
    QCOMPARE(typedRequest->command, OutlookHelperCommand::Prepare);
    QCOMPARE(typedRequest->payload.value(QStringLiteral("recipients")).toArray().size(), 2);
    QCOMPARE(typedRequest->payload.value(QStringLiteral("recipients")).toArray().at(1).toObject()
             .value(QStringLiteral("role")).toString(), QStringLiteral("cc"));
    emailRequest.subject = QStringLiteral("bad\r\nBcc:bad@example.test");
    QVERIFY(!makeOutlookPrepareRequest(emailRequest, QStringLiteral("/staging/body.html"),
                                       QStringLiteral("/staging"), &validation));
    emailRequest.subject = QStringLiteral("Safe subject");
    emailRequest.recipients = {{QStringLiteral("Invalid role"), QStringLiteral("role@example.test"),
                                static_cast<RecipientRole>(99)}};
    QVERIFY(!makeOutlookPrepareRequest(emailRequest, QStringLiteral("/staging/body.html"),
                                       QStringLiteral("/staging"), &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("recipient-role-invalid"));
    emailRequest.recipients = {{QStringLiteral("To"), QStringLiteral("to@example.test"), RecipientRole::To}};
    emailRequest.attachments = {{QUuid::createUuid(), QStringLiteral("/staging-sibling/report.pdf"),
                                 QStringLiteral("report.pdf"), QStringLiteral("application/pdf"), 1, {}, true}};
    QVERIFY(!makeOutlookPrepareRequest(emailRequest, QStringLiteral("/staging/body.html"),
                                       QStringLiteral("/staging"), &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("outlook-helper-path-not-operation-owned"));
    emailRequest.attachments = {{QUuid::createUuid(), QStringLiteral("/staging/../outside/report.pdf"),
                                 QStringLiteral("report.pdf"), QStringLiteral("application/pdf"), 1, {}, true}};
    QVERIFY(!makeOutlookPrepareRequest(emailRequest, QStringLiteral("/staging/body.html"),
                                       QStringLiteral("/staging"), &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("outlook-helper-path-not-operation-owned"));

    OutlookHelperResponse response{request.operationId, OutlookHelperStage::Presented,
                                   OutcomeCertainty::Certain, QStringLiteral("opaque-item-42"), {}};
    const auto encodedResponse = encodeOutlookHelperResponse(response, &validation);
    QVERIFY(encodedResponse.has_value());
    const auto decodedResponse = decodeOutlookHelperResponse(encodedResponse->chopped(1), &validation);
    QVERIFY(decodedResponse.has_value());
    QCOMPARE(decodedResponse->operationId, response.operationId);
    QCOMPARE(decodedResponse->stage, OutlookHelperStage::Presented);
    QCOMPARE(decodedResponse->certainty, OutcomeCertainty::Certain);
    QCOMPARE(decodedResponse->itemHandle, QStringLiteral("opaque-item-42"));

    QVERIFY(!outlookHelperCommandFromStableString(QStringLiteral("send")));
    request.command = static_cast<OutlookHelperCommand>(99);
    QVERIFY(!encodeOutlookHelperRequest(request, &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("outlook-helper-request-invalid"));
    response.stage = static_cast<OutlookHelperStage>(99);
    QVERIFY(!encodeOutlookHelperResponse(response, &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("outlook-helper-response-invalid"));
    response.stage = OutlookHelperStage::Presented;
    response.certainty = static_cast<OutcomeCertainty>(99);
    QVERIFY(!encodeOutlookHelperResponse(response, &validation));
    QCOMPARE(validation.issues.constLast().code, QStringLiteral("outlook-helper-response-invalid"));
    QVERIFY(!decodeOutlookHelperRequest("{\"protocolVersion\":2}", &validation));
    QVERIFY(!decodeOutlookHelperRequest("{\"protocolVersion\":1,\"operationId\":\"bad\",\"command\":\"send\",\"payload\":{}}", &validation));
    QVERIFY(!decodeOutlookHelperRequest(QByteArray(maximumOutlookHelperMessageBytes + 1, 'x'), &validation));
    QVERIFY(!decodeOutlookHelperResponse("{\"protocolVersion\":1,\"operationId\":\"bad\",\"stage\":\"sent\",\"certainty\":8}", &validation));
}

void CommunicationContractsTest::reportsMacMailPlatformAvailability()
{
    MacMailBackend backend;
    EmailRequest request;
    request.operationId = QUuid::createUuid();
    QString errorCode;
    backend.handoff(request, [&errorCode](EmailHandoffResult result) { errorCode = result.error.code; });
#if defined(Q_OS_DARWIN)
    QCOMPARE(errorCode, QStringLiteral("mac-mail-bridge-unavailable"));
#else
    QCOMPARE(errorCode, QStringLiteral("mac-mail-unavailable"));
#endif
    request.operationId = QUuid::createUuid();
    backend.cancel(request.operationId);
    backend.handoff(request, [&errorCode](EmailHandoffResult result) { errorCode = result.error.code; });
    QCOMPARE(errorCode, QStringLiteral("operation-cancelled"));
}

void CommunicationContractsTest::serializesEmailHandoffsAndRejectsStaleCompletion()
{
    Test::DeferredEmailBackend backend; Test::DeferredEmailBackend unrelated; EmailService service; service.registerBackend(BackendId::Mailto, &backend); service.registerBackend(BackendId::Thunderbird, &unrelated);
    EmailRequest first; first.operationId=QUuid::createUuid(); first.backend=BackendId::Mailto;
    bool firstComplete=false; service.handoff(first, [&firstComplete](EmailHandoffResult) { firstComplete=true; });
    QCOMPARE(service.activeOperation(), first.operationId);
    EmailRequest second; second.operationId=QUuid::createUuid(); second.backend=BackendId::Mailto;
    QString busy; service.handoff(second, [&busy](EmailHandoffResult result) { busy=result.error.code; });
    QCOMPARE(busy, QStringLiteral("email-operation-busy"));
    service.cancel(first.operationId); QCOMPARE(backend.cancelled, QList<QUuid>({first.operationId}));
    QVERIFY(unrelated.cancelled.isEmpty());
    backend.finish({first.operationId, OutcomeCertainty::Uncertain}); QVERIFY(firstComplete); QVERIFY(service.activeOperation().isNull());
    backend.finish({first.operationId, OutcomeCertainty::Certain}); QVERIFY(service.activeOperation().isNull());
}

void CommunicationContractsTest::scopesAudiencePresetsByDatabaseAndProject()
{
    QTemporaryDir directory; QVERIFY(directory.isValid()); QSettings settings(directory.filePath("audiences.ini"), QSettings::IniFormat);
    AudiencePresetStore store(settings, "database-a"); AudiencePreset preset{"team", "Team", {}, Workflow::StatusReport};
    preset.rule.source=PeopleSource::StatusRecipients; preset.rule.companyFilter=CompanyFilter::ManagingCompany; preset.overrides={{"person", false, RecipientRole::To}};
    QVERIFY(store.save(preset).ok()); QVERIFY(store.setDefault("team", Workflow::StatusReport).ok());
    QVERIFY(store.defaultFor(Workflow::StatusReport).has_value());
    AudiencePreset project=preset; project.id="project-team"; project.name="Project Team"; project.projectId="p";
    QVERIFY(store.save(project).ok()); QVERIFY(store.setDefault("project-team", Workflow::StatusReport, "p").ok());
    QVERIFY(!store.setDefault("project-team", Workflow::StatusReport).ok());
    QVERIFY(!store.setDefault("project-team", Workflow::StatusReport, "other").ok());
    QCOMPARE(store.defaultFor(Workflow::StatusReport, "p")->id, QStringLiteral("project-team"));
    QCOMPARE(store.defaultFor(Workflow::StatusReport, "other")->id, QStringLiteral("team"));
    AudiencePresetStore other(settings, "database-b"); QVERIFY(!other.defaultFor(Workflow::StatusReport).has_value());
}

void CommunicationContractsTest::buildsStatusReportWithEvmAndEscapedIssues()
{
    StatusReportInput input; input.projectNumber="P-1"; input.projectName="North"; input.managerName="Manager"; input.reportingPeriod="Weekly";
    input.reportingDate=QDate(2026,9,15); input.actual="$25"; input.bcwp="$30"; input.bcws="$35"; input.bac="$100";
    input.stakeholders={"Alice", "Bob"}; input.activitiesInProgress={"Build <review>"}; input.issues={{"Earlier high", "Alice", "High", "09/19/2026", "New"},{"Later high", "Alice", "High", "09/20/2026", "New"},{"Risk <one>", "Alice", "Medium", "09/21/2026", "New"}};
    ValidationResult validation; const auto document=StatusReportBuilder::build(input,&validation);
    QVERIFY(validation.ok()); QVERIFY(document.has_value()); QVERIFY(document->htmlDocument.contains("Risk &lt;one&gt;"));
    QVERIFY(document->htmlDocument.indexOf("Later high") < document->htmlDocument.indexOf("Earlier high"));
    QVERIFY(document->htmlDocument.indexOf("Earlier high") < document->htmlDocument.indexOf("Risk &lt;one&gt;"));
    QVERIFY(document->htmlDocument.contains("-16.67%")); QCOMPARE(document->defaultSubject,QStringLiteral("P-1 North - Status Report 09/15/2026"));
    QVERIFY(document->htmlDocument.contains("Earned Value Project Management Terms"));
    QVERIFY(document->htmlDocument.contains("table class='ev-table'"));
    QVERIFY(document->htmlDocument.contains("priority-medium"));
    QVERIFY(document->fileStem.endsWith("Status Report"));
    QCOMPARE(document->pdfLayout.pageSize().id(), QPageSize::Letter);
    QCOMPARE(document->pdfLayout.orientation(), QPageLayout::Portrait);
    QCOMPARE(document->pdfLayout.margins(QPageLayout::Millimeter).left(), 20.0);
}

void CommunicationContractsTest::buildsFilteredAndSortedTrackerReport()
{
    TrackerItemsReportInput input; input.projectNumber="P-1"; input.projectName="North"; input.options=defaultReportOptions(Workflow::TrackerItemsReport,QDate(2026,9,15));
    input.options.tracker->itemTypes={"Tracker","Action"}; input.options.tracker->statuses={"New","Assigned"};
    input.items={{"2","Medium <item>",{}, {}, {}, {}, "Medium","New","09/20/2026",{}, {}, {},"Tracker",false},
                 {"1","High",{}, {}, {}, {}, "High","New","09/19/2026",{}, {}, {},"Tracker",false},
                 {"3","Hidden",{}, {}, {}, {}, "High","Resolved","09/25/2026",{}, {}, {},"Tracker",false},
                 {"4","Internal",{}, {}, {}, {}, "High","Assigned","09/21/2026",{}, {}, {},"Action",true}};
    input.items[0].description = "First line\nSecond <line>";
    ValidationResult validation; const auto document=TrackerItemsReportBuilder::build(input,&validation);
    QVERIFY(validation.ok()); QVERIFY(document.has_value()); QVERIFY(document->htmlDocument.contains("Medium &lt;item&gt;"));
    QVERIFY(!document->htmlDocument.contains("Hidden")); QVERIFY(!document->htmlDocument.contains("Internal"));
    QVERIFY(document->htmlDocument.indexOf(">High<") < document->htmlDocument.indexOf("Medium &lt;item&gt;"));
    // Inline mail must be self-contained: the standalone document's stylesheet
    // is not passed to EmailContentBuilder or the mail backend.
    const QString email = document->emailFragment;
    QVERIFY(!email.contains("<style>"));
    QVERIFY(email.contains("background-color:#1F497D;color:#FFFFFF"));
    QVERIFY(email.contains("background-color:#DCE6F1"));
    QVERIFY(email.contains("background-color:#EEECE1"));
    QVERIFY(email.contains("font-family:Calibri,Arial,sans-serif"));
    QVERIFY(email.contains("border-collapse:collapse;width:100%;table-layout:fixed"));
    QVERIFY(email.contains("class='col-comments' width='21%'"));
    QVERIFY(email.contains("style='color:#C00000;font-weight:bold;'"));
    QVERIFY(email.contains("First line<br>Second &lt;line&gt;"));
    QVERIFY(!email.contains("Hidden"));
    QVERIFY(!email.contains("class='col-int'"));
    input.options.internalReport = true;
    const auto internal = TrackerItemsReportBuilder::build(input);
    QVERIFY(internal.has_value());
    QVERIFY(internal->emailFragment.contains("class='col-int' width='3%'"));
    QVERIFY(internal->emailFragment.contains("colspan='13' bgcolor='#1F497D' style="));
    QCOMPARE(document->defaultSubject,QStringLiteral("P-1 North - Tracker Items 09/15/2026"));
    QCOMPARE(document->pdfLayout.pageSize().id(), QPageSize::Letter);
    QCOMPARE(document->pdfLayout.orientation(), QPageLayout::Landscape);
    QCOMPARE(document->pdfLayout.margins(QPageLayout::Millimeter).left(), 12.0);
}

void CommunicationContractsTest::dispatchesAllNativeReportBuilders()
{
    MeetingNotesBuildInput meeting; meeting.snapshot.projectNumber="P"; meeting.snapshot.projectName="N";
    meeting.snapshot.notes={{"n","Note","<p>Body</p>",QDateTime(QDate(2026,9,1),{}),false,{}}}; meeting.noteId="n";
    MeetingNotesReportInput notesReport; notesReport.snapshot=meeting.snapshot; notesReport.reportingDate=QDate(2026,9,2);
    StatusReportInput status; status.projectNumber="P"; status.projectName="N"; status.reportingDate=QDate(2026,9,2);
    TrackerItemsReportInput tracker; tracker.projectNumber="P"; tracker.projectName="N"; tracker.options=defaultReportOptions(Workflow::TrackerItemsReport,QDate(2026,9,2));
    QVERIFY(ReportService::build(meeting).has_value()); QVERIFY(ReportService::build(notesReport).has_value());
    QVERIFY(ReportService::build(status).has_value()); QVERIFY(ReportService::build(tracker).has_value());
}

void CommunicationContractsTest::preparesImmutableRequestsForEachEmailMode()
{
    EmailPreparation input; input.operationId=QUuid::createUuid(); input.source={"db",1,"p",{},Workflow::StatusReport}; input.accountKey="Ada <ada@example.test>"; input.accountGeneration=42; input.document.workflow=Workflow::StatusReport;
    input.document.defaultSubject="Report"; input.document.emailFragment="<p>HTML</p>"; input.document.plainText="HTML"; input.recipients={{"One","one@example.test",RecipientRole::To}};
    auto inlineResult=EmailContentBuilder::prepare(input); QVERIFY(inlineResult.validation.ok()); QVERIFY(inlineResult.request.has_value()); QVERIFY(inlineResult.request->html.isEmpty()); QCOMPARE(inlineResult.request->plainText,QStringLiteral("HTML")); QCOMPARE(inlineResult.request->accountKey,input.accountKey); QCOMPARE(inlineResult.request->accountGeneration,input.accountGeneration);
    input.backend=BackendId::Graph; inlineResult=EmailContentBuilder::prepare(input); QVERIFY(inlineResult.validation.ok()); QCOMPARE(inlineResult.request->html,QStringLiteral("<p>HTML</p>"));
    input.mode=EmailMode::PdfAttachment; input.backend=BackendId::Mailto; input.attachments.clear(); auto mailtoReport=EmailContentBuilder::prepare(input); QVERIFY(mailtoReport.validation.ok()); QVERIFY(mailtoReport.request->attachments.isEmpty()); QVERIFY(mailtoReport.request->html.isEmpty());
    input.mode=EmailMode::InlineHtml; input.backend=BackendId::Graph;
    input.document.defaultSubject = QStringLiteral(" \t ");
    const auto missingSubject = EmailContentBuilder::prepare(input);
    QVERIFY(!missingSubject.validation.ok());
    QCOMPARE(missingSubject.validation.issues.constLast().code, QStringLiteral("email-subject-required"));
    input.document.defaultSubject = QStringLiteral("Report");
    input.recipients = {{QStringLiteral("Invalid"), QStringLiteral("bad@example.test\r\nBcc: injected@example.test"), RecipientRole::To}};
    const auto invalidRecipient = EmailContentBuilder::prepare(input);
    QVERIFY(!invalidRecipient.validation.ok());
    QCOMPARE(invalidRecipient.validation.issues.constFirst().code, QStringLiteral("recipient-address-invalid"));
    input.recipients = {{QStringLiteral("One"), QStringLiteral("one@example.test"), RecipientRole::To},
                        {QStringLiteral("Duplicate"), QStringLiteral("ONE@example.test"), RecipientRole::Cc}};
    const auto duplicateRecipient = EmailContentBuilder::prepare(input);
    QVERIFY(!duplicateRecipient.validation.ok());
    QCOMPARE(duplicateRecipient.validation.issues.constLast().code, QStringLiteral("recipient-address-duplicate"));
    input.recipients = {{QStringLiteral("One"), QStringLiteral("one@example.test"), RecipientRole::To}};
    input.mode=EmailMode::HtmlAttachment; auto missing=EmailContentBuilder::prepare(input); QVERIFY(!missing.validation.ok());
    input.attachments={{QUuid::createUuid(),"/staged/report.html","report.html","text/html",10,{ },true}}; auto html=EmailContentBuilder::prepare(input); QVERIFY(html.validation.ok()); QVERIFY(html.request->html.isEmpty());
    input.mode=EmailMode::None; auto none=EmailContentBuilder::prepare(input); QVERIFY(none.validation.ok()); QVERIFY(none.noEmail); QVERIFY(!none.request.has_value());
}

void CommunicationContractsTest::createsGraphDraftWithoutSend_data()
{
    QTest::addColumn<QString>("webLink");
    QTest::addColumn<QByteArray>("composeUrl");
    QTest::newRow("office") << QStringLiteral("https://outlook.office.com/owa/?ItemID=old&viewmodel=ReadMessageItem")
        << QByteArray("https://outlook.office.com/mail/deeplink/compose/draft%2Fid%2B%3D%3F%23%25");
    QTest::newRow("office365") << QStringLiteral("https://outlook.office365.com/owa/?ItemID=old")
        << QByteArray("https://outlook.office365.com/mail/deeplink/compose/draft%2Fid%2B%3D%3F%23%25");
    QTest::newRow("untrusted") << QStringLiteral("https://outlook.office.com.evil.test/draft") << QByteArray();
    QTest::newRow("missing") << QString() << QByteArray();
}

void CommunicationContractsTest::createsGraphDraftWithoutSend()
{
    QFETCH(QString, webLink);
    QFETCH(QByteArray, composeUrl);
    ScriptedHttpTransport transport;
    transport.responses = {
        {{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "offline_access"}}).toJson(QJsonDocument::Compact), {}},
        {{}, 201, {}, QJsonDocument(QJsonObject{{"id", "draft/id+=?#%"}, {"webLink", webLink}}).toJson(QJsonDocument::Compact), {}}
    };
    MicrosoftOAuthManager oauth;
    oauth.setHttpTransport(&transport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service;
    service.setOAuthManager(&oauth);
    service.setHttpTransport(&transport);
    oauth.restoreSession();

    GraphEmailBackend backend(&service);
    EmailRequest request;
    request.operationId = QUuid::createUuid();
    request.accountGeneration = service.account().generation;
    request.subject = "Synthetic draft";
    MeetingNotesBuildInput notes;
    notes.noteId = "note";
    notes.snapshot.notes = {{"note", "Kickoff", "<p>Notes</p>",
                             QDateTime(QDate(2026, 9, 12), QTime(9, 0)), false, {}}};
    const auto document = MeetingNotesEmailBuilder::build(notes);
    QVERIFY(document.has_value());
    request.html = document->emailFragment;
    request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    bool complete = false;
    backend.handoff(request, [&complete, &composeUrl](EmailHandoffResult result) {
        QCOMPARE(result.certainty, OutcomeCertainty::Certain);
        QCOMPARE(result.draftIdentity, QStringLiteral("draft/id+=?#%"));
        QCOMPARE(result.presentationUrl.toEncoded(), composeUrl);
        QVERIFY(result.error.code.isEmpty());
        complete = true;
    });
    QVERIFY(complete);
    QVERIFY(GraphEmailBackend::isAllowedPresentationUrl(QUrl(QStringLiteral("https://outlook.office365.com/mail/inbox"))));
    QVERIFY(!GraphEmailBackend::isAllowedPresentationUrl(QUrl(QStringLiteral("http://outlook.office.com/draft"))));
    QVERIFY(!GraphEmailBackend::isAllowedPresentationUrl(QUrl(QStringLiteral("https://outlook.office.com.evil.test/draft"))));
    QCOMPARE(transport.requests.size(), 2);
    const HttpRequest &draft = transport.requests.constLast();
    QCOMPARE(draft.method, QByteArray("POST"));
    QVERIFY(draft.url.path().endsWith("/me/messages"));
    QVERIFY(!draft.url.toString().contains("send", Qt::CaseInsensitive));
    QCOMPARE(draft.headers.value("Authorization"), QByteArray("Bearer token"));
    QVERIFY(draft.body.contains("to@example.test"));
    const auto graphBody = QJsonDocument::fromJson(draft.body).object().value("body").toObject();
    QCOMPARE(graphBody.value("contentType").toString(), QStringLiteral("HTML"));
    QCOMPARE(graphBody.value("content").toString(), document->emailFragment);
    QVERIFY(graphBody.value("content").toString().contains("background-color:#d9e1f2;"));

    request.operationId = QUuid::createUuid();
    request.accountGeneration = service.account().generation + 1;
    QString changedAccountError;
    backend.handoff(request, [&changedAccountError](EmailHandoffResult result) {
        changedAccountError = result.error.code;
    });
    QCOMPARE(changedAccountError, QStringLiteral("account-generation-changed"));
    QCOMPARE(transport.requests.size(), 2);

    ScriptedHttpTransport missingScopeTransport;
    missingScopeTransport.responses = {
        {{}, 200, {}, QJsonDocument(QJsonObject{
            {"access_token", "token"}, {"expires_in", 3600}, {"scope", "Files.Read.All"}}).toJson(QJsonDocument::Compact), {}},
        {{}, 403, {}, QJsonDocument(QJsonObject{{"error", QJsonObject{{"code", "ErrorAccessDenied"}, {"message", "Access is denied."}}}}).toJson(QJsonDocument::Compact), {}}
    };
    MicrosoftOAuthManager missingScopeOAuth;
    missingScopeOAuth.setHttpTransport(&missingScopeTransport);
    missingScopeOAuth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    missingScopeOAuth.configure("tenant", "client");
    Office365Service missingScopeService;
    missingScopeService.setOAuthManager(&missingScopeOAuth);
    missingScopeService.setHttpTransport(&missingScopeTransport);
    missingScopeOAuth.restoreSession();
    GraphEmailBackend missingScopeBackend(&missingScopeService);
    request.operationId = QUuid::createUuid();
    request.accountGeneration = missingScopeService.account().generation;
    QString missingScopeError;
    missingScopeBackend.handoff(request, [&missingScopeError](EmailHandoffResult result) {
        missingScopeError = result.error.code;
    });
    QCOMPARE(missingScopeError, QStringLiteral("ErrorAccessDenied"));
    QCOMPARE(missingScopeTransport.requests.size(), 2);
}

void CommunicationContractsTest::retainsUncertainOutcomeForLostGraphDraftResponse()
{
    ScriptedHttpTransport transport;
    transport.responses = {
        {{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "Mail.ReadWrite offline_access"}}).toJson(QJsonDocument::Compact), {}},
        {{}, 0, {}, {}, {"network-timeout", {}, RetryKind::RetryOperation, OutcomeCertainty::NotStarted}}
    };
    MicrosoftOAuthManager oauth; oauth.setHttpTransport(&transport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service; service.setOAuthManager(&oauth); service.setHttpTransport(&transport);
    oauth.restoreSession();
    GraphEmailBackend backend(&service);
    EmailRequest request; request.operationId = QUuid::createUuid(); request.accountGeneration = service.account().generation;
    request.subject = "Lost response"; request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    bool complete = false;
    backend.handoff(request, [&complete](EmailHandoffResult result) {
        QCOMPARE(result.error.code, QStringLiteral("network-timeout"));
        QCOMPARE(result.error.certainty, OutcomeCertainty::Uncertain);
        complete = true;
    });
    QVERIFY(complete);
    QCOMPARE(transport.requests.size(), 2);
    QCOMPARE(transport.requests.constLast().url.path(), QStringLiteral("/v1.0/me/messages"));
}

void CommunicationContractsTest::rejectsModifiedGraphAttachmentBeforeDraftCreation()
{
    ScriptedHttpTransport transport;
    transport.responses = {{{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "Mail.ReadWrite offline_access"}}).toJson(QJsonDocument::Compact), {}}};
    MicrosoftOAuthManager oauth; oauth.setHttpTransport(&transport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service; service.setOAuthManager(&oauth); service.setHttpTransport(&transport);
    oauth.restoreSession();
    QTemporaryFile attachment; QVERIFY(attachment.open());
    const QByteArray staged = "staged attachment";
    QCOMPARE(attachment.write(staged), qint64(staged.size())); attachment.flush();
    QVERIFY(attachment.resize(0));
    QCOMPARE(attachment.write("modified attachment"), qint64(19)); attachment.flush();
    GraphEmailBackend backend(&service);
    EmailRequest request; request.operationId = QUuid::createUuid(); request.accountGeneration = service.account().generation;
    request.subject = "Modified attachment"; request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    request.attachments = {{QUuid::createUuid(), attachment.fileName(), "note.txt", "text/plain", staged.size(), sha256(staged), true}};
    bool complete = false;
    backend.handoff(request, [&complete](EmailHandoffResult result) {
        QCOMPARE(result.error.code, QStringLiteral("attachment-integrity-mismatch"));
        QCOMPARE(result.error.certainty, OutcomeCertainty::Certain);
        complete = true;
    });
    QVERIFY(complete);
    QCOMPARE(transport.requests.size(), 1);
}

void CommunicationContractsTest::uploadsSimpleGraphAttachment()
{
    ScriptedHttpTransport transport;
    transport.responses = {
        {{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "Mail.ReadWrite offline_access"}}).toJson(QJsonDocument::Compact), {}},
        {{}, 201, {}, QJsonDocument(QJsonObject{{"id", "draft-id"}}).toJson(QJsonDocument::Compact), {}},
        {{}, 201, {}, "{}", {}}
    };
    MicrosoftOAuthManager oauth; oauth.setHttpTransport(&transport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service; service.setOAuthManager(&oauth); service.setHttpTransport(&transport);
    oauth.restoreSession();
    QTemporaryFile attachment; QVERIFY(attachment.open());
    QCOMPARE(attachment.write("attachment data"), qint64(15)); attachment.flush();
    GraphEmailBackend backend(&service);
    EmailRequest request; request.operationId = QUuid::createUuid(); request.accountGeneration = service.account().generation;
    request.subject = "Attachment draft"; request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    request.attachments = {{QUuid::createUuid(), attachment.fileName(), "note.txt", "text/plain", 15, sha256("attachment data"), true}};
    bool complete = false;
    backend.handoff(request, [&complete](EmailHandoffResult result) { QVERIFY(result.error.code.isEmpty()); QCOMPARE(result.draftIdentity, QStringLiteral("draft-id")); complete = true; });
    QVERIFY(complete); QCOMPARE(transport.requests.size(), 3);
    const HttpRequest &upload = transport.requests.constLast();
    QVERIFY(upload.url.path().endsWith("/me/messages/draft-id/attachments"));
    QVERIFY(upload.body.contains("YXR0YWNobWVudCBkYXRh"));
}

void CommunicationContractsTest::uploadsLargeGraphAttachmentInUnauthenticatedChunks()
{
    ScriptedHttpTransport transport;
    transport.responses.append({{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "Mail.ReadWrite offline_access"}}).toJson(QJsonDocument::Compact), {}});
    transport.responses.append({{}, 201, {}, QJsonDocument(QJsonObject{{"id", "draft-id"}}).toJson(QJsonDocument::Compact), {}});
    transport.responses.append({{}, 201, {}, QJsonDocument(QJsonObject{{"uploadUrl", "https://upload.example.test/session"}}).toJson(QJsonDocument::Compact), {}});
    const qint64 size = GraphEmailBackend::maximumSimpleAttachmentBytes + 1;
    const int chunks = int((size + GraphEmailBackend::uploadChunkBytes - 1) / GraphEmailBackend::uploadChunkBytes);
    for (int index = 0; index < chunks; ++index)
        transport.responses.append({{}, index + 1 == chunks ? 201 : 202, {}, "{}", {}});
    MicrosoftOAuthManager oauth; oauth.setHttpTransport(&transport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service; service.setOAuthManager(&oauth); service.setHttpTransport(&transport);
    oauth.restoreSession();
    QTemporaryFile attachment; QVERIFY(attachment.open());
    QCOMPARE(attachment.write(QByteArray(size, 'x')), size); attachment.flush();
    GraphEmailBackend backend(&service);
    EmailRequest request; request.operationId = QUuid::createUuid(); request.accountGeneration = service.account().generation;
    request.subject = "Large attachment"; request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    request.attachments = {{QUuid::createUuid(), attachment.fileName(), "large.bin", "application/octet-stream", size, sha256(QByteArray(size, 'x')), true}};
    bool complete = false;
    backend.handoff(request, [&complete](EmailHandoffResult result) { QVERIFY(result.error.code.isEmpty()); complete = true; });
    QVERIFY(complete);
    QCOMPARE(transport.requests.size(), 3 + chunks);
    const HttpRequest &session = transport.requests.at(2);
    QVERIFY(session.url.path().endsWith("/createUploadSession"));
    const HttpRequest &firstChunk = transport.requests.at(3);
    QCOMPARE(firstChunk.method, QByteArray("PUT"));
    QCOMPARE(firstChunk.url, QUrl(QStringLiteral("https://upload.example.test/session")));
    QVERIFY(!firstChunk.headers.contains("Authorization"));
    QCOMPARE(firstChunk.headers.value("Content-Range"), QByteArray("bytes 0-327679/3145729"));
    const HttpRequest &lastChunk = transport.requests.constLast();
    QVERIFY(lastChunk.headers.value("Content-Range").endsWith("/3145729"));
}

void CommunicationContractsTest::cancelsGraphUploadBeforeFirstChunk()
{
    ScriptedHttpTransport oauthTransport;
    oauthTransport.responses = {{{}, 200, {}, QJsonDocument(QJsonObject{{"access_token", "token"}, {"expires_in", 3600}, {"scope", "Mail.ReadWrite offline_access"}}).toJson(QJsonDocument::Compact), {}}};
    MicrosoftOAuthManager oauth; oauth.setHttpTransport(&oauthTransport);
    oauth.setSecretStore([](const QString &, QString *) { return QStringLiteral("refresh"); }, {}, {});
    oauth.configure("tenant", "client");
    Office365Service service; service.setOAuthManager(&oauth); service.setHttpTransport(&oauthTransport);
    oauth.restoreSession();
    DeferredHttpTransport graphTransport; service.setHttpTransport(&graphTransport);
    const qint64 size = GraphEmailBackend::maximumSimpleAttachmentBytes + 1;
    QTemporaryFile attachment; QVERIFY(attachment.open());
    QCOMPARE(attachment.write(QByteArray(size, 'x')), size); attachment.flush();
    GraphEmailBackend backend(&service);
    EmailRequest request; request.operationId = QUuid::createUuid(); request.accountGeneration = service.account().generation;
    request.subject = "Cancelled upload"; request.recipients = {{"To", "to@example.test", RecipientRole::To}};
    request.attachments = {{QUuid::createUuid(), attachment.fileName(), "large.bin", "application/octet-stream", size, sha256(QByteArray(size, 'x')), true}};
    bool complete = false;
    QString errorCode;
    OutcomeCertainty certainty = OutcomeCertainty::Certain;
    backend.handoff(request, [&complete, &errorCode, &certainty](EmailHandoffResult result) {
        complete = true; errorCode = result.error.code; certainty = result.error.certainty;
    });
    QCOMPARE(graphTransport.requests.size(), 1);
    graphTransport.respondNext({{}, 201, {}, QJsonDocument(QJsonObject{{"id", "draft-id"}}).toJson(QJsonDocument::Compact), {}});
    QCOMPARE(graphTransport.requests.size(), 2);
    backend.cancel(request.operationId);
    graphTransport.respondNext({{}, 201, {}, QJsonDocument(QJsonObject{{"uploadUrl", "https://upload.example.test/session"}}).toJson(QJsonDocument::Compact), {}});
    QVERIFY(complete);
    QCOMPARE(errorCode, QStringLiteral("operation-cancelled"));
    QCOMPARE(certainty, OutcomeCertainty::Uncertain);
    QCOMPARE(graphTransport.requests.size(), 2);
}

void CommunicationContractsTest::coordinatesPreparationHandoffAndNoEmail()
{
    Test::DeferredEmailBackend backend;
    EmailService emailService; emailService.registerBackend(BackendId::Mailto, &backend);
    CommunicationsController controller(&emailService);
    QVERIFY(!controller.setSubject(QStringLiteral("No preparation")));
    QVERIFY(controller.beginReviewPreparation());
    QCOMPARE(controller.stageName(), QStringLiteral("editing"));
    QCOMPARE(controller.subject(), QString());
    EmailPreparation preparation;
    preparation.source = {"db", 1, "project", {}, Workflow::StatusReport};
    preparation.document.workflow = Workflow::StatusReport;
    preparation.document.defaultSubject = "Draft";
    preparation.document.emailFragment = "<p>Body</p>";
    preparation.document.plainText = "Body";
    preparation.addressLater = true;
    QVERIFY(controller.setPreparation(preparation));
    QCOMPARE(controller.stageName(), QStringLiteral("reviewing"));
    QVERIFY(controller.diagnostic().isEmpty());
    QCOMPARE(controller.subject(), QStringLiteral("Draft"));
    QCOMPARE(controller.plainText(), QStringLiteral("Body"));
    QVERIFY(controller.inlineHtmlMode());
    QVERIFY(controller.setHtmlBody(QStringLiteral("<p>Edited <b>body</b></p>")));
    QCOMPARE(controller.htmlBody(), QStringLiteral("<p>Edited <b>body</b></p>"));
    QCOMPARE(controller.plainText(), QStringLiteral("Edited body"));
    QVERIFY(controller.appendAttachment({QUuid::createUuid(), QStringLiteral("/staged/user.txt"),
                                         QStringLiteral("user.txt"), QStringLiteral("text/plain"), 1, {}, false}));
    QCOMPARE(controller.attachmentNames(), QStringList{QStringLiteral("user.txt")});
    QCOMPARE(controller.generatedAttachmentNames(), QStringList{});
    QVERIFY(controller.appendAttachment({QUuid::createUuid(), QStringLiteral("/staged/generated.html"),
                                         QStringLiteral("generated.html"), QStringLiteral("text/html"), 1, {}, true}));
    QCOMPARE(controller.generatedAttachmentNames(), QStringList{QStringLiteral("generated.html")});
    QVERIFY(controller.setSubject(QStringLiteral("Edited draft")));
    QCOMPARE(controller.subject(), QStringLiteral("Edited draft"));
    QVERIFY(!controller.setSubject(QStringLiteral("Bad\r\nBcc: bad@example.test")));
    ValidationResult recipientFailure;
    recipientFailure.addError("recipient-role-conflict", "recipients", "Recipient roles conflict.");
    controller.setReviewValidation(recipientFailure);
    QCOMPARE(controller.stageName(), QStringLiteral("failed"));
    QCOMPARE(controller.diagnostic(), QStringLiteral("Recipient roles conflict."));
    QVERIFY(controller.setPreparation(preparation));
    QVERIFY(controller.beginSourceRevalidation());
    QVERIFY(controller.busy());
    QCOMPARE(controller.stageName(), QStringLiteral("revalidating-source"));
    QVERIFY(!controller.setSubject(QStringLiteral("Cannot edit while revalidating")));
    ValidationResult sourceChanged;
    sourceChanged.addError("review-source-changed", "source", "Regenerate the review.");
    controller.failSourceRevalidation(sourceChanged);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.stageName(), QStringLiteral("failed"));
    QCOMPARE(controller.diagnostic(), QStringLiteral("Regenerate the review."));
    QVERIFY(!controller.handoff());
    QVERIFY(controller.setPreparation(preparation));
    QVERIFY(controller.beginSourceRevalidation());
    QSignalSpy revalidationCancelled(&controller, &CommunicationsController::sourceRevalidationCancelled);
    controller.cancel();
    QVERIFY(!controller.busy());
    QCOMPARE(controller.stageName(), QStringLiteral("reviewing"));
    QCOMPARE(revalidationCancelled.count(), 1);
    preparation.mode = EmailMode::HtmlAttachment;
    QVERIFY(controller.setPreparation(preparation));
    QVERIFY(!controller.setHtmlBody(QStringLiteral("<p>Not staged</p>")));
    preparation.mode = EmailMode::InlineHtml;
    QVERIFY(controller.setPreparation(preparation));
    const quint64 revision = controller.previewRevision();
    QVERIFY(controller.setSubject(QStringLiteral("Edited draft")));
    QVERIFY(controller.beginSourceRevalidation());
    QVERIFY(controller.handoffAfterSourceRevalidation(
        {{"Final recipient", "final@example.test", RecipientRole::Cc}}, false));
    QVERIFY(controller.busy());
    QVERIFY(!controller.appendAttachment({QUuid::createUuid(), QStringLiteral("/staged/late.txt"),
                                          QStringLiteral("late.txt"), QStringLiteral("text/plain"), 1, {}, false}));
    QCOMPARE(controller.stage(), PreparationStage::PreparingBackend);
    QCOMPARE(controller.stageName(), QStringLiteral("preparing-backend"));
    QCOMPARE(backend.requestSeen.previewRevision, revision);
    QCOMPARE(backend.requestSeen.subject, QStringLiteral("Edited draft"));
    QCOMPARE(backend.requestSeen.recipients.size(), 1);
    QCOMPARE(backend.requestSeen.recipients.first().address, QStringLiteral("final@example.test"));
    QCOMPARE(backend.requestSeen.recipients.first().role, RecipientRole::Cc);
    QVERIFY(!backend.requestSeen.addressLaterExplicitlyChosen);
    QVERIFY(!controller.setPreparation(preparation));
    backend.finish({backend.requestSeen.operationId, OutcomeCertainty::Certain, "draft",
                    QUrl(QStringLiteral("https://outlook.office.com/draft")), {}});
    QVERIFY(!controller.busy());
    QCOMPARE(controller.stage(), PreparationStage::Completed);
    QCOMPARE(controller.stageName(), QStringLiteral("completed"));
    QCOMPARE(controller.lastResult().draftIdentity, QStringLiteral("draft"));
    QCOMPARE(controller.draftIdentity(), QStringLiteral("draft"));
    QCOMPARE(controller.presentationUrl(), QUrl(QStringLiteral("https://outlook.office.com/draft")));

    preparation.mode = EmailMode::None;
    QVERIFY(controller.setPreparation(preparation));
    QVERIFY(controller.handoff());
    QCOMPARE(controller.stage(), PreparationStage::Completed);
    QVERIFY(!controller.busy());
}

QTEST_GUILESS_MAIN(CommunicationContractsTest)
#include "tst_communicationcontracts.moc"
