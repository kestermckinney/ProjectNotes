#include "RecipientAudienceResolver.h"

namespace {

bool isValidRecipientAddress(const QString &value)
{
    const QString address = value.trimmed();
    return !address.isEmpty() && address.contains(u'@')
        && !address.contains(u'\r') && !address.contains(u'\n')
        && !address.contains(u',') && !address.contains(u';');
}

} // namespace

namespace PN::Comm {

AudienceResolution resolveAudience(const CommunicationSnapshot &snapshot, const AudienceRule &rule)
{
    AudienceResolution result;
    QString requiredCompany;
    if (rule.companyFilter == CompanyFilter::ManagingCompany)
        requiredCompany = snapshot.managingCompanyId;
    else if (rule.companyFilter == CompanyFilter::ProjectCompany
             || rule.companyFilter == CompanyFilter::ExceptProjectClient)
        requiredCompany = snapshot.clientCompanyId;

    if ((rule.companyFilter == CompanyFilter::ManagingCompany
         || rule.companyFilter == CompanyFilter::ProjectCompany
         || rule.companyFilter == CompanyFilter::ExceptProjectClient)
        && requiredCompany.isEmpty()) {
        result.validation.addError("company-setup-required", "audience.companyFilter");
        return result;
    }

    for (const SnapshotContact &contact : snapshot.contacts) {
        const bool inSource = (rule.source == ContactSource::ProjectTeam && contact.projectTeamMember)
            || (rule.source == ContactSource::MeetingAttendees && contact.meetingAttendee)
            || (rule.source == ContactSource::StatusRecipients && contact.receivesStatus)
            || (rule.source == ContactSource::CurrentSelection
                && snapshot.currentSelectionContactIds.contains(contact.id))
            || (rule.source == ContactSource::ChosenContacts && rule.chosenContactIds.contains(contact.id));
        if (!inSource) {
            result.exclusions.append({contact, QStringLiteral("source-mismatch")});
            continue;
        }
        if (!isValidRecipientAddress(contact.email)) {
            result.exclusions.append({contact, QStringLiteral("invalid-address")});
            continue;
        }
        if (rule.excludeProjectManager && contact.id == snapshot.projectManagerId) {
            result.exclusions.append({contact, QStringLiteral("project-manager-excluded")});
            continue;
        }

        bool companyMatches = rule.companyFilter == CompanyFilter::All
            || (rule.companyFilter == CompanyFilter::ManagingCompany && contact.companyId == requiredCompany)
            || (rule.companyFilter == CompanyFilter::ProjectCompany && contact.companyId == requiredCompany)
            || (rule.companyFilter == CompanyFilter::ExceptProjectClient
                && !contact.companyId.isEmpty() && contact.companyId != requiredCompany)
            || (rule.companyFilter == CompanyFilter::SelectedCompanies
                && rule.companyIds.contains(contact.companyId));
        if (contact.companyId.isEmpty() && rule.companyFilter != CompanyFilter::All)
            companyMatches = rule.includeUnknownCompany;
        if (companyMatches)
            result.contacts.append(contact);
        else
            result.exclusions.append({contact, QStringLiteral("company-filter")});
    }

    if (result.contacts.isEmpty() && result.validation.ok())
        result.validation.addWarning("audience-empty", "audience");
    return result;
}

RecipientResolution applyRecipientOverrides(const AudienceResolution &audience,
                                            const QList<RecipientOverride> &overrides)
{
    RecipientResolution result;
    QHash<QString, RecipientRole> seen;
    for (const SnapshotContact &contact : audience.contacts) {
        RecipientRole role = RecipientRole::To;
        bool selected = true;
        for (const RecipientOverride &overrideValue : overrides) {
            if (overrideValue.contactId == contact.id) {
                selected = overrideValue.selected;
                role = overrideValue.role;
            }
        }
        const QString address = contact.email.trimmed();
        const QString key = address.toCaseFolded();
        if (selected && !isValidRecipientAddress(address)) {
            result.validation.addError("recipient-address-invalid", contact.id);
            continue;
        }
        if (!selected || key.isEmpty())
            continue;
        if (seen.contains(key)) {
            if (seen.value(key) != role)
                result.validation.addError("recipient-role-conflict", contact.id);
            continue;
        }
        seen.insert(key, role);
        result.recipients.append({contact.name, address, role});
    }
    return result;
}

} // namespace PN::Comm
