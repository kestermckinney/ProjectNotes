// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectNotesEmail/RecipientAudienceResolver.h"
#include "ProjectNotesEmail/RecipientSelectionModel.h"

#include <QtTest/QtTest>

#include <algorithm>

using namespace PN::Comm;

class AudienceResolverTest final : public QObject {
    Q_OBJECT

private slots:
    void resolvesCompanyAndSourceIntersections();
    void appliesRecipientOverridesSafely();
    void retainsRecipientEditsAndManualEntries();
    void preservesResolutionOrderWhileGroupingAudience();
};

void AudienceResolverTest::resolvesCompanyAndSourceIntersections()
{
    CommunicationSnapshot snapshot;
    snapshot.companyId = QStringLiteral("client");
    snapshot.managingCompanyId = QStringLiteral("ours");
    snapshot.projectManagerId = QStringLiteral("manager");
    snapshot.contacts = {{QStringLiteral("manager"), QStringLiteral("Manager"),
                        QStringLiteral("manager@example.test"), QStringLiteral("ours"),
                        QStringLiteral("Ours"), true, true, true},
                       {QStringLiteral("client"), QStringLiteral("Client"),
                        QStringLiteral("client@example.test"), QStringLiteral("client"),
                        QStringLiteral("Client Co"), true, false, true},
                       {QStringLiteral("partner"), QStringLiteral("Partner"),
                        QStringLiteral("partner@example.test"), QStringLiteral("partner"),
                        QStringLiteral("Partner Co"), false, true, true},
                       {QStringLiteral("unknown"), QStringLiteral("Unknown"),
                        QStringLiteral("unknown@example.test"), {}, {}, true, false, true}};

    AudienceRule rule;
    rule.source = ContactSource::StatusRecipients;
    rule.companyFilter = CompanyFilter::ExceptProjectCompany;
    auto result = resolveAudience(snapshot, rule);
    QVERIFY(result.contacts.isEmpty());
    rule.includeUnknownCompany = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 1);
    QCOMPARE(result.contacts.first().id, QStringLiteral("unknown"));

    rule.source = ContactSource::ChosenContacts;
    rule.chosenContactIds = {QStringLiteral("missing")};
    result = resolveAudience(snapshot, rule);
    QVERIFY(result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("audience-empty"));

    rule.source = ContactSource::CurrentSelection;
    snapshot.currentSelectionContactIds = {QStringLiteral("client"), QStringLiteral("partner")};
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 2);

    rule.source = ContactSource::ProjectTeam;
    rule.companyFilter = CompanyFilter::ManagingCompany;
    rule.includeUnknownCompany = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 1);
    QCOMPARE(result.contacts.first().id, QStringLiteral("manager"));

    rule.companyFilter = CompanyFilter::ProjectCompany;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 1);
    QCOMPARE(result.contacts.first().id, QStringLiteral("client"));

    rule.companyFilter = CompanyFilter::ExceptProjectCompany;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 2);
    QCOMPARE(result.contacts.at(0).id, QStringLiteral("manager"));
    QCOMPARE(result.contacts.at(1).id, QStringLiteral("partner"));

    rule.source = ContactSource::ChosenContacts;
    rule.chosenContactIds = {QStringLiteral("manager"), QStringLiteral("partner")};
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 1);
    QCOMPARE(result.contacts.first().id, QStringLiteral("partner"));

    rule.source = ContactSource::ProjectTeam;
    rule.companyFilter = CompanyFilter::SelectedCompanies;
    rule.companyIds = {QStringLiteral("partner")};
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 1);
    QCOMPARE(result.contacts.first().id, QStringLiteral("partner"));
    rule.includeUnknownCompany = true;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 2);

    snapshot.contacts.append({QStringLiteral("outside"), QStringLiteral("Outside"),
                            QStringLiteral("outside@example.test"), QStringLiteral("partner"),
                            QStringLiteral("Partner Co"), false, true, false});
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QCOMPARE(result.contacts.size(), 4);
    QVERIFY(std::none_of(result.contacts.cbegin(), result.contacts.cend(), [](const SnapshotContact &person) {
        return person.id == QStringLiteral("outside");
    }));

    rule.companyFilter = CompanyFilter::ManagingCompany;
    snapshot.managingCompanyId.clear();
    result = resolveAudience(snapshot, rule);
    QVERIFY(!result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("company-setup-required"));

    snapshot.managingCompanyId = QStringLiteral("ours");
    snapshot.companyId.clear();
    rule.companyFilter = CompanyFilter::ExceptProjectCompany;
    result = resolveAudience(snapshot, rule);
    QVERIFY(!result.validation.ok());
    QCOMPARE(result.validation.issues.constFirst().code, QStringLiteral("company-setup-required"));

    snapshot.companyId = QStringLiteral("client");
    snapshot.contacts.append({QStringLiteral("invalid"), QStringLiteral("Invalid"),
                            QStringLiteral("invalid@example.test;other@example.test"),
                            QStringLiteral("partner"), QStringLiteral("Partner Co"), false, true, true});
    rule.companyFilter = CompanyFilter::All;
    rule.excludeProjectManager = false;
    result = resolveAudience(snapshot, rule);
    QVERIFY(std::none_of(result.contacts.cbegin(), result.contacts.cend(), [](const SnapshotContact &person) {
        return person.id == QStringLiteral("invalid");
    }));
}

void AudienceResolverTest::appliesRecipientOverridesSafely()
{
    AudienceResolution audience;
    audience.contacts = {{QStringLiteral("one"), QStringLiteral("One"), QStringLiteral("same@example.test")},
                       {QStringLiteral("two"), QStringLiteral("Two"), QStringLiteral("SAME@example.test")},
                       {QStringLiteral("bad"), QStringLiteral("Bad"),
                        QStringLiteral("bad@example.test\r\nBcc:x@example.test")}};
    const auto result = applyRecipientOverrides(audience, {{QStringLiteral("one"), true, RecipientRole::Cc}});
    QCOMPARE(result.recipients.size(), 1);
    QCOMPARE(result.recipients.first().role, RecipientRole::Cc);
    QVERIFY(!result.validation.ok());

    const auto conflict = applyRecipientOverrides(audience, {{QStringLiteral("one"), true, RecipientRole::To},
                                                               {QStringLiteral("two"), true, RecipientRole::Bcc}});
    QCOMPARE(conflict.recipients.size(), 1);
    QCOMPARE(conflict.recipients.first().role, RecipientRole::To);
    QVERIFY(!conflict.validation.ok());
}

void AudienceResolverTest::retainsRecipientEditsAndManualEntries()
{
    RecipientSelectionModel model;
    AudienceResolution audience;
    audience.contacts = {{QStringLiteral("one"), QStringLiteral("One"), QStringLiteral("one@example.test")},
                       {QStringLiteral("two"), QStringLiteral("Two"), QStringLiteral("two@example.test")}};
    model.setAudience(audience);
    model.applyOverrides({{QStringLiteral("one"), false, RecipientRole::Bcc}});
    QCOMPARE(model.resolve().recipients.size(), 1);
    QCOMPARE(model.resolve().recipients.constFirst().address, QStringLiteral("two@example.test"));
    model.setAudience(audience);
    QCOMPARE(model.recipientCount(), 2);
    QCOMPARE(model.selectedRecipientCount(), 2);
    QVERIFY(model.audienceDiagnostic().isEmpty());
    QVERIFY(model.setSelected(QStringLiteral("one"), false));
    QVERIFY(model.setRecipientRole(QStringLiteral("two"), RecipientRole::Bcc));
    model.clearSelection();
    QCOMPARE(model.selectedRecipientCount(), 0);
    model.selectAll();
    QCOMPARE(model.selectedRecipientCount(), 2);
    QVERIFY(model.setSelected(QStringLiteral("one"), false));
    model.setAddressLaterExplicitlyChosen(true);
    const auto resolved = model.resolve();
    QCOMPARE(resolved.recipients.size(), 1);
    QCOMPARE(resolved.recipients.at(0).role, RecipientRole::Bcc);
    QVERIFY(resolved.addressLaterExplicitlyChosen);

    AudienceResolution replacement;
    replacement.contacts = {{QStringLiteral("replacement"), QStringLiteral("Replacement"), QStringLiteral("replacement@example.test")}};
    model.setAudience(replacement);
    QCOMPARE(model.recipientCount(), 1);
    QVERIFY(!model.resolve().addressLaterExplicitlyChosen);
    QVERIFY(!model.setSelected(QStringLiteral("one"), false));
    model.setAudience(audience);
    QVERIFY(model.setSelected(QStringLiteral("one"), false));
    model.reset();
    QCOMPARE(model.resolve().recipients.size(), 2);
    QVERIFY(!model.resolve().addressLaterExplicitlyChosen);

    AudienceResolution duplicateAddresses;
    duplicateAddresses.contacts = {{QStringLiteral("dup-one"), QStringLiteral("Duplicate one"), QStringLiteral("same@example.test")},
                                 {QStringLiteral("dup-two"), QStringLiteral("Duplicate two"), QStringLiteral("SAME@example.test")}};
    model.setAudience(duplicateAddresses);
    QCOMPARE(model.recipientCount(), 1);
    QCOMPARE(model.selectedRecipientCount(), 1);
    AudienceResolution empty;
    empty.validation.addWarning(QStringLiteral("audience-empty"), QStringLiteral("audience"));
    model.setAudience(empty);
    QCOMPARE(model.recipientCount(), 0);
    QCOMPARE(model.audienceDiagnostic(), QStringLiteral("audience-empty"));
    model.setAddressLaterExplicitlyChosen(true);
    QCOMPARE(model.resolve().validation.issues.constFirst().code, QStringLiteral("audience-empty"));
    model.reset();
    QCOMPARE(model.recipientCount(), 0);
    QVERIFY(!model.addressLaterExplicitlyChosen());
}

void AudienceResolverTest::preservesResolutionOrderWhileGroupingAudience()
{
    CommunicationSnapshot snapshot;
    snapshot.projectManagerId = QStringLiteral("manager");
    snapshot.contacts = {
        {QStringLiteral("partner"), QStringLiteral("Partner"), QStringLiteral("partner@example.test"),
         QStringLiteral("partner"), QStringLiteral("Partner Co"), false, false, true},
        {QStringLiteral("manager"), QStringLiteral("Manager"), QStringLiteral("manager@example.test"),
         QStringLiteral("ours"), QStringLiteral("Our Company"), false, false, true},
        {QStringLiteral("client"), QStringLiteral("Client"), QStringLiteral("client@example.test"),
         QStringLiteral("client"), QStringLiteral("Client Co"), false, false, true},
        {QStringLiteral("invalid"), QStringLiteral("Invalid"), QStringLiteral("not-an-address"),
         QStringLiteral("client"), QStringLiteral("Client Co"), false, false, true},
        {QStringLiteral("outside"), QStringLiteral("Outside"), QStringLiteral("outside@example.test"),
         QStringLiteral("other"), QStringLiteral("Other Co"), false, false, false}
    };
    AudienceRule rule;
    rule.source = ContactSource::ProjectTeam;
    rule.excludeProjectManager = true;
    const AudienceResolution audience = resolveAudience(snapshot, rule);
    QCOMPARE(audience.contacts.size(), 2);
    QCOMPARE(audience.exclusions.size(), 3);

    RecipientSelectionModel model;
    model.setAudience(audience);
    model.setInternalReportContext(true, QStringLiteral("ours"));
    QCOMPARE(model.internalAudienceWarning(),
             QStringLiteral("This internal report has 2 selected recipient(s) outside the managing company."));
    QCOMPARE(model.toRecipientCount(), 2);
    QVERIFY(model.setRecipientRole(QStringLiteral("client"), RecipientRole::Cc));
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
    QCOMPARE(resolved.recipients.at(0).address, QStringLiteral("partner@example.test"));
    QCOMPARE(resolved.recipients.at(1).address, QStringLiteral("client@example.test"));
    QCOMPARE(resolved.recipients.at(1).role, RecipientRole::Cc);

    model.setInternalReportContext(true, {});
    QCOMPARE(model.internalAudienceWarning(),
             QStringLiteral("This report is marked internal, but the managing company is not configured."));
}

QTEST_GUILESS_MAIN(AudienceResolverTest)

#include "tst_audienceresolver.moc"
