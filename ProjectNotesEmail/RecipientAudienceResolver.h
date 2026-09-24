#pragma once
#include "SnapshotTypes.h"
namespace PN::Comm {
enum class PeopleSource { ProjectTeam, MeetingAttendees, StatusRecipients, CurrentSelection, ChosenPeople };
enum class CompanyFilter { All, ManagingCompany, ProjectClient, ExceptProjectClient, SelectedCompanies };
struct AudienceRule { PeopleSource source=PeopleSource::ProjectTeam; CompanyFilter companyFilter=CompanyFilter::All; QStringList companyIds,chosenPersonIds; bool includeUnknownCompany=false,excludeProjectManager=true; };
// Keep exclusions alongside eligible people so the presentation layer can
// explain a decision without reimplementing audience filtering in QML.
struct AudienceExclusion { SnapshotPerson person; QString reason; };
struct RecipientOverride { QString personId; bool selected=true; RecipientRole role=RecipientRole::To; };
struct AudienceResolution {
    QList<SnapshotPerson> people;
    QList<AudienceExclusion> exclusions;
    ValidationResult validation;
    // Initial choices are part of a workflow's default audience, rather than
    // a persisted user override.  The selection model consumes these only
    // when an audience is first presented.
    QList<RecipientOverride> initialOverrides;
};
struct RecipientResolution { QList<EmailAddress> recipients; ValidationResult validation; bool addressLaterExplicitlyChosen = false; };
AudienceResolution resolveAudience(const CommunicationSnapshot &, const AudienceRule &);
RecipientResolution applyRecipientOverrides(const AudienceResolution &, const QList<RecipientOverride> &);
}
