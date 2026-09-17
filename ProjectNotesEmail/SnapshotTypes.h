#pragma once
#include "EmailTypes.h"
namespace PN::Comm {
struct SnapshotPerson {
    QString id,name,email,companyId,companyName;
    bool receivesStatus=false;
    bool meetingAttendee=false;
    bool projectTeamMember=false;
};
struct SnapshotNote { QString id,title,html; QDateTime date; bool internal=false; QStringList attendeeIds; };
struct SnapshotActionItem { QString noteId,name,assignedTo,status,dueDate; };
struct SnapshotStatusItem { QString category,description; };
struct SnapshotTrackerItem { QString number,name,identifiedBy,dateIdentified,description,assignedTo,priority,status,dueDate,lastUpdate,dateResolved,comments,itemType; bool internal=false; };
struct CommunicationSnapshot {
    QString projectId, projectNumber, projectName, clientCompanyId, managingCompanyId, projectManagerId;
    // Optional local root discovered/stored for report publication. This is a
    // captured database value, never a live filesystem lookup by rendering/UI.
    QString projectFolderPath;
    QString statusReportPeriod,budget,actual,bcwp,bcws,bac;
    quint64 databaseGeneration=0; QDateTime loadedAt;
    QList<SnapshotPerson> people; QList<SnapshotNote> notes; QList<SnapshotActionItem> actionItems;
    QList<SnapshotStatusItem> statusItems; QList<SnapshotTrackerItem> trackerItems;
    // Content selection is note-oriented, while the audience's current
    // selection is person-oriented. Keeping both avoids treating note IDs as
    // recipient IDs when a review switches audience sources.
    QStringList currentSelectionIds;
    QStringList currentSelectionPersonIds;
    QByteArray fingerprint;
};
}
