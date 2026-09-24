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
    else if (rule.companyFilter == CompanyFilter::ProjectClient
             || rule.companyFilter == CompanyFilter::ExceptProjectClient)
        requiredCompany = snapshot.clientCompanyId;

    if ((rule.companyFilter == CompanyFilter::ManagingCompany
         || rule.companyFilter == CompanyFilter::ProjectClient
         || rule.companyFilter == CompanyFilter::ExceptProjectClient)
        && requiredCompany.isEmpty()) {
        result.validation.addError("company-setup-required", "audience.companyFilter");
        return result;
    }

    for (const SnapshotPerson &person : snapshot.people) {
        const bool inSource = (rule.source == PeopleSource::ProjectTeam && person.projectTeamMember)
            || (rule.source == PeopleSource::MeetingAttendees && person.meetingAttendee)
            || (rule.source == PeopleSource::StatusRecipients && person.receivesStatus)
            || (rule.source == PeopleSource::CurrentSelection
                && snapshot.currentSelectionPersonIds.contains(person.id))
            || (rule.source == PeopleSource::ChosenPeople && rule.chosenPersonIds.contains(person.id));
        if (!inSource) {
            result.exclusions.append({person, QStringLiteral("source-mismatch")});
            continue;
        }
        if (!isValidRecipientAddress(person.email)) {
            result.exclusions.append({person, QStringLiteral("invalid-address")});
            continue;
        }
        if (rule.excludeProjectManager && person.id == snapshot.projectManagerId) {
            result.exclusions.append({person, QStringLiteral("project-manager-excluded")});
            continue;
        }

        bool companyMatches = rule.companyFilter == CompanyFilter::All
            || (rule.companyFilter == CompanyFilter::ManagingCompany && person.companyId == requiredCompany)
            || (rule.companyFilter == CompanyFilter::ProjectClient && person.companyId == requiredCompany)
            || (rule.companyFilter == CompanyFilter::ExceptProjectClient
                && !person.companyId.isEmpty() && person.companyId != requiredCompany)
            || (rule.companyFilter == CompanyFilter::SelectedCompanies
                && rule.companyIds.contains(person.companyId));
        if (person.companyId.isEmpty() && rule.companyFilter != CompanyFilter::All)
            companyMatches = rule.includeUnknownCompany;
        if (companyMatches)
            result.people.append(person);
        else
            result.exclusions.append({person, QStringLiteral("company-filter")});
    }

    if (result.people.isEmpty() && result.validation.ok())
        result.validation.addWarning("audience-empty", "audience");
    return result;
}

RecipientResolution applyRecipientOverrides(const AudienceResolution &audience,
                                            const QList<RecipientOverride> &overrides)
{
    RecipientResolution result;
    QHash<QString, RecipientRole> seen;
    for (const SnapshotPerson &person : audience.people) {
        RecipientRole role = RecipientRole::To;
        bool selected = true;
        for (const RecipientOverride &overrideValue : overrides) {
            if (overrideValue.personId == person.id) {
                selected = overrideValue.selected;
                role = overrideValue.role;
            }
        }
        const QString address = person.email.trimmed();
        const QString key = address.toCaseFolded();
        if (selected && !isValidRecipientAddress(address)) {
            result.validation.addError("recipient-address-invalid", person.id);
            continue;
        }
        if (!selected || key.isEmpty())
            continue;
        if (seen.contains(key)) {
            if (seen.value(key) != role)
                result.validation.addError("recipient-role-conflict", person.id);
            continue;
        }
        seen.insert(key, role);
        result.recipients.append({person.name, address, role});
    }
    return result;
}

} // namespace PN::Comm
