#pragma once
#include "SnapshotTypes.h"
namespace PN::Comm {
enum class ContactSource { ProjectTeam, MeetingAttendees, StatusRecipients, CurrentSelection, ChosenContacts };
struct AudienceRule { ContactSource source=ContactSource::ProjectTeam; QStringList chosenContactIds; bool excludeProjectManager=true; };
// Keep exclusions alongside eligible contacts so the presentation layer can
// explain a decision without reimplementing audience filtering in QML.
struct AudienceExclusion { SnapshotContact contact; QString reason; };
struct RecipientOverride { QString contactId; bool selected=true; RecipientRole role=RecipientRole::To; };
struct AudienceResolution {
    QList<SnapshotContact> contacts;
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
