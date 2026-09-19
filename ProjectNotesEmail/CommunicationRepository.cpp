#include "CommunicationRepository.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <QIODevice>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSet>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QUuid>

#include <algorithm>

namespace PN::Comm {
namespace {
QByteArray canonicalFingerprint(const CommunicationSnapshot &snapshot)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_5);
    stream << snapshot.projectId << snapshot.projectNumber << snapshot.projectName
           << snapshot.clientCompanyId << snapshot.managingCompanyId << snapshot.projectManagerId
           << snapshot.clientName << snapshot.managingCompanyName << snapshot.projectManagerName
           << snapshot.projectFolderPath
           << snapshot.statusReportPeriod << snapshot.budget << snapshot.actual << snapshot.bcwp << snapshot.bcws << snapshot.bac
           << snapshot.databaseGeneration << snapshot.currentSelectionIds
           << snapshot.currentSelectionPersonIds;
    for (const SnapshotPerson &person : snapshot.people)
        stream << person.id << person.name << person.email << person.companyId << person.companyName
               << person.receivesStatus << person.meetingAttendee << person.projectTeamMember;
    for (const SnapshotNote &note : snapshot.notes)
        stream << note.id << note.title << note.html << note.date.toSecsSinceEpoch() << note.internal
               << note.attendeeIds;
    for (const SnapshotActionItem &item : snapshot.actionItems)
        stream << item.noteId << item.name << item.assignedTo << item.status << item.dueDate;
    for (const SnapshotStatusItem &item : snapshot.statusItems)
        stream << item.category << item.description;
    for (const SnapshotTrackerItem &item : snapshot.trackerItems)
        stream << item.number << item.name << item.identifiedBy << item.dateIdentified << item.description
               << item.assignedTo << item.priority << item.status << item.dueDate << item.lastUpdate
               << item.dateResolved << item.comments << item.itemType << item.internal;
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
}

QDateTime epochSeconds(const QVariant &value)
{
    bool valid = false;
    const qint64 seconds = value.toString().toLongLong(&valid);
    return valid ? QDateTime::fromSecsSinceEpoch(seconds) : QDateTime();
}
}
SqliteSnapshotLoader::SqliteSnapshotLoader(QString path, QReadWriteLock *lock)
    : m_databasePath(std::move(path)), m_lock(lock) {}

SnapshotResult SqliteSnapshotLoader::load(const SnapshotRequest &request) const
{
    SnapshotResult result{request.operationId, request.previewRevision};
    if (!m_lock) {
        result.error = {QStringLiteral("snapshot-lock-required"), QStringLiteral("A database read lock is required.")};
        return result;
    }
    const QString name = QStringLiteral("CommSnapshot-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QReadLocker locker(m_lock);
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        db.setDatabaseName(m_databasePath);
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000;QSQLITE_OPEN_READONLY"));
        if (!db.open()) result.error = {QStringLiteral("snapshot-open-failed"), db.lastError().text()};
        else if (!db.transaction()) result.error = {QStringLiteral("snapshot-transaction-failed"), db.lastError().text()};
        else {
            QSqlQuery project(db);
            project.prepare(QStringLiteral("SELECT id, project_number, project_name, client_id, status_report_period, "
                                           "budget, actual, bcwp, bcws, bac FROM projects WHERE id=? AND deleted=0"));
            project.addBindValue(request.source.projectId);
            if (!project.exec() || !project.next()) result.error = {QStringLiteral("project-not-found"), project.lastError().text()};
            else {
                result.snapshot.projectId=project.value(0).toString(); result.snapshot.projectNumber=project.value(1).toString(); result.snapshot.projectName=project.value(2).toString(); result.snapshot.clientCompanyId=project.value(3).toString(); result.snapshot.statusReportPeriod=project.value(4).toString(); result.snapshot.budget=project.value(5).toString(); result.snapshot.actual=project.value(6).toString(); result.snapshot.bcwp=project.value(7).toString(); result.snapshot.bcws=project.value(8).toString(); result.snapshot.bac=project.value(9).toString(); result.snapshot.managingCompanyId=request.managingCompanyId; result.snapshot.projectManagerId=request.projectManagerId; result.snapshot.databaseGeneration=request.source.databaseGeneration; result.snapshot.loadedAt=request.capturedAt;
                // Template fields need names, not IDs. Each lookup tolerates a
                // missing or deleted row by leaving the name empty.
                const auto lookupName = [&db](const QString &sql, const QString &id) {
                    if (id.trimmed().isEmpty()) return QString();
                    QSqlQuery query(db);
                    query.prepare(sql);
                    query.addBindValue(id);
                    return query.exec() && query.next() ? query.value(0).toString() : QString();
                };
                const QString clientNameSql =
                    QStringLiteral("SELECT client_name FROM clients WHERE id=? AND deleted=0");
                result.snapshot.clientName = lookupName(clientNameSql, result.snapshot.clientCompanyId);
                result.snapshot.managingCompanyName = lookupName(clientNameSql, result.snapshot.managingCompanyId);
                result.snapshot.projectManagerName = lookupName(
                    QStringLiteral("SELECT name FROM people WHERE id=? AND deleted=0"),
                    result.snapshot.projectManagerId);
                QSqlQuery projectFolder(db);
                projectFolder.prepare(QStringLiteral(
                    "SELECT full_path FROM project_locations "
                    "WHERE project_id=? AND deleted=0 AND location_type='File Folder' "
                    "AND trim(coalesce(full_path,'')) <> '' "
                    "ORDER BY CASE WHEN location_description='Project Folder' THEN 0 ELSE 1 END, "
                    "location_description,id LIMIT 1"));
                projectFolder.addBindValue(request.source.projectId);
                if (!projectFolder.exec()) {
                    result.error = {QStringLiteral("snapshot-query-failed"), projectFolder.lastError().text()};
                } else if (projectFolder.next()) {
                    result.snapshot.projectFolderPath = projectFolder.value(0).toString().trimmed();
                }
                QSqlQuery people(db);
                people.prepare(QStringLiteral(
                    "SELECT p.id,p.name,p.email,p.client_id,COALESCE(c.client_name,''),"
                    "COALESCE(pp.receive_status_report,0) "
                    "FROM people p JOIN project_people pp "
                    "ON pp.people_id=p.id AND pp.project_id=? AND pp.deleted=0 "
                    "LEFT JOIN clients c ON c.id=p.client_id AND c.deleted=0 "
                    "WHERE p.deleted=0 ORDER BY p.name,p.id"));
                people.addBindValue(request.source.projectId);
                if (!people.exec()) result.error={QStringLiteral("snapshot-query-failed"),people.lastError().text()};
                while (result.error.code.isEmpty() && people.next()) result.snapshot.people.append({people.value(0).toString(),people.value(1).toString(),people.value(2).toString(),people.value(3).toString(),people.value(4).toString(),people.value(5).toBool(),false,true});
                // "Current selection" is the legacy workflow's initial
                // recipient context, not the selected note-ID set below.
                // Capture it before adding non-team meeting attendees.
                for (const SnapshotPerson &person : result.snapshot.people) {
                    if (request.source.workflow != Workflow::StatusReport || person.receivesStatus)
                        result.snapshot.currentSelectionPersonIds.append(person.id);
                }
                result.snapshot.currentSelectionPersonIds.sort();
                QSqlQuery notes(db);
                notes.prepare(QStringLiteral("SELECT id,note_title,note_date,note,internal_item FROM project_notes WHERE project_id=? AND deleted=0 ORDER BY note_date DESC,id"));
                notes.addBindValue(request.source.projectId);
                if (result.error.code.isEmpty() && !notes.exec()) result.error={QStringLiteral("snapshot-query-failed"),notes.lastError().text()};
                while (result.error.code.isEmpty() && notes.next()) result.snapshot.notes.append({notes.value(0).toString(),notes.value(1).toString(),notes.value(3).toString(),epochSeconds(notes.value(2)),notes.value(4).toBool()});

                QSqlQuery statusItems(db);
                statusItems.prepare(QStringLiteral("SELECT task_category,task_description FROM status_report_items "
                                                   "WHERE project_id=? AND deleted=0 ORDER BY id"));
                statusItems.addBindValue(request.source.projectId);
                if (result.error.code.isEmpty() && !statusItems.exec()) result.error={QStringLiteral("snapshot-query-failed"),statusItems.lastError().text()};
                while (result.error.code.isEmpty() && statusItems.next())
                    result.snapshot.statusItems.append({statusItems.value(0).toString(), statusItems.value(1).toString()});

                QSqlQuery trackerItems(db);
                trackerItems.prepare(QStringLiteral("SELECT it.item_number,it.item_name,COALESCE(identified.name,it.identified_by,''),"
                    "COALESCE(strftime('%m/%d/%Y',datetime(it.date_identified,'unixepoch')),it.date_identified,''),it.description,"
                    "COALESCE(assigned.name,it.assigned_to,''),it.priority,it.status,"
                    "COALESCE(strftime('%m/%d/%Y',datetime(it.date_due,'unixepoch')),it.date_due,''),"
                    "COALESCE(strftime('%m/%d/%Y',datetime(it.last_update,'unixepoch')),it.last_update,''),"
                    "COALESCE(strftime('%m/%d/%Y',datetime(it.date_resolved,'unixepoch')),it.date_resolved,''),"
                    "COALESCE((SELECT GROUP_CONCAT(update_note, ',') FROM item_tracker_updates u WHERE u.item_id=it.id),''),"
                    "it.item_type,it.internal_item FROM item_tracker it "
                    "LEFT JOIN people identified ON identified.id=it.identified_by AND identified.deleted=0 "
                    "LEFT JOIN people assigned ON assigned.id=it.assigned_to AND assigned.deleted=0 "
                    "WHERE it.project_id=? AND it.deleted=0 ORDER BY it.item_number,it.id"));
                trackerItems.addBindValue(request.source.projectId);
                if (result.error.code.isEmpty() && !trackerItems.exec()) result.error={QStringLiteral("snapshot-query-failed"),trackerItems.lastError().text()};
                while (result.error.code.isEmpty() && trackerItems.next()) {
                    SnapshotTrackerItem item;
                    item.number=trackerItems.value(0).toString(); item.name=trackerItems.value(1).toString(); item.identifiedBy=trackerItems.value(2).toString(); item.dateIdentified=trackerItems.value(3).toString(); item.description=trackerItems.value(4).toString(); item.assignedTo=trackerItems.value(5).toString(); item.priority=trackerItems.value(6).toString(); item.status=trackerItems.value(7).toString(); item.dueDate=trackerItems.value(8).toString(); item.lastUpdate=trackerItems.value(9).toString(); item.dateResolved=trackerItems.value(10).toString(); item.comments=trackerItems.value(11).toString(); item.itemType=trackerItems.value(12).toString(); item.internal=trackerItems.value(13).toBool();
                    result.snapshot.trackerItems.append(std::move(item));
                }

                QSet<QString> availableNotes;
                for (const SnapshotNote &note : result.snapshot.notes)
                    availableNotes.insert(note.id);
                QSet<QString> selectedNotes;
                if (request.source.workflow == Workflow::MeetingNotesReport
                    && request.source.noteIds.isEmpty()) {
                    // A project-wide report renders every in-scope note, so its
                    // attendee provenance and note-bound actions must cover the
                    // same complete set rather than an empty current selection.
                    selectedNotes = availableNotes;
                } else {
                    for (const QString &noteId : request.source.noteIds) {
                        if (!availableNotes.contains(noteId)) {
                            result.error = {QStringLiteral("note-not-in-project"), noteId};
                            break;
                        }
                        selectedNotes.insert(noteId);
                    }
                }
                result.snapshot.currentSelectionIds = selectedNotes.values(); result.snapshot.currentSelectionIds.sort();

                if (result.error.code.isEmpty() && !selectedNotes.isEmpty()) {
                    QStringList placeholders;
                    for (qsizetype i = 0; i < selectedNotes.size(); ++i)
                        placeholders.append(QStringLiteral("?"));
                    QSqlQuery attendees(db);
                    attendees.prepare(QStringLiteral(
                                      "SELECT attendee.note_id,attendee.person_id,p.name,p.email,p.client_id,"
                                      "COALESCE(c.client_name,'') FROM meeting_attendees attendee "
                                      "JOIN people p ON p.id=attendee.person_id AND p.deleted=0 "
                                      "LEFT JOIN clients c ON c.id=p.client_id AND c.deleted=0 "
                                      "WHERE attendee.deleted=0 AND attendee.note_id IN (%1)")
                                      .arg(placeholders.join(',')));
                    for (const QString &noteId : selectedNotes)
                        attendees.addBindValue(noteId);
                    if (!attendees.exec()) {
                        result.error = {QStringLiteral("snapshot-query-failed"), attendees.lastError().text()};
                    } else {
                        QSet<QString> attendeeIds;
                        QHash<QString, QStringList> attendeeIdsByNote;
                        while (attendees.next()) {
                            const QString personId = attendees.value(1).toString();
                            attendeeIds.insert(personId);
                            attendeeIdsByNote[attendees.value(0).toString()].append(personId);
                            const auto knownPerson = std::find_if(result.snapshot.people.cbegin(), result.snapshot.people.cend(),
                                [&personId](const SnapshotPerson &person) { return person.id == personId; });
                            if (knownPerson == result.snapshot.people.cend()) {
                                result.snapshot.people.append({personId, attendees.value(2).toString(),
                                    attendees.value(3).toString(), attendees.value(4).toString(),
                                    attendees.value(5).toString(), false, false});
                            }
                        }
                        std::sort(result.snapshot.people.begin(), result.snapshot.people.end(),
                                  [](const SnapshotPerson &left, const SnapshotPerson &right) {
                            return left.name == right.name ? left.id < right.id : left.name < right.name;
                        });
                        for (SnapshotPerson &person : result.snapshot.people)
                            person.meetingAttendee = attendeeIds.contains(person.id);
                        for (SnapshotNote &note : result.snapshot.notes) {
                            note.attendeeIds = attendeeIdsByNote.value(note.id);
                            note.attendeeIds.sort();
                        }
                    }
                }
                if (result.error.code.isEmpty() && !selectedNotes.isEmpty()) {
                    QStringList placeholders;
                    for (qsizetype i = 0; i < selectedNotes.size(); ++i) placeholders.append(QStringLiteral("?"));
                    QSqlQuery actions(db);
                    actions.prepare(QStringLiteral("SELECT it.note_id,it.item_name,COALESCE(p.name,it.assigned_to),it.status,it.date_due "
                        "FROM item_tracker it LEFT JOIN people p ON p.id=it.assigned_to AND p.deleted=0 "
                        "WHERE it.project_id=? AND it.deleted=0 AND it.note_id IN (%1) ORDER BY it.item_number,it.id")
                        .arg(placeholders.join(',')));
                    actions.addBindValue(request.source.projectId);
                    for (const QString &noteId : selectedNotes) actions.addBindValue(noteId);
                    if (!actions.exec()) result.error = {QStringLiteral("snapshot-query-failed"), actions.lastError().text()};
                    while (result.error.code.isEmpty() && actions.next()) result.snapshot.actionItems.append({actions.value(0).toString(), actions.value(1).toString(), actions.value(2).toString(), actions.value(3).toString(), actions.value(4).toString()});
                }
            }
            if (!db.commit() && result.error.code.isEmpty())
                result.error = {QStringLiteral("snapshot-transaction-failed"), db.lastError().text()};
        }
        db.close(); db=QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    if (result.error.code.isEmpty()) result.snapshot.fingerprint=canonicalFingerprint(result.snapshot);
    return result;
}

AsyncSqliteCommunicationRepository::AsyncSqliteCommunicationRepository(
        QString databasePath, QReadWriteLock *lock, quint64 databaseGeneration, QObject *parent)
    : QObject(parent), m_databasePath(std::move(databasePath)), m_lock(lock),
      m_databaseGeneration(databaseGeneration)
{
    m_workers.setMaxThreadCount(1);
    m_workers.setExpiryTimeout(-1);
}

AsyncSqliteCommunicationRepository::~AsyncSqliteCommunicationRepository()
{
    // The lock is owned by the database controller.  Join the worker before
    // destruction so it cannot be touched after that controller tears down.
    m_workers.waitForDone();
}

bool AsyncSqliteCommunicationRepository::isCancelled(const QUuid &operationId) const
{
    QMutexLocker locker(&m_mutex);
    return m_cancelled.contains(operationId);
}

void AsyncSqliteCommunicationRepository::cancel(const QUuid &operationId)
{
    QMutexLocker locker(&m_mutex);
    m_cancelled.insert(operationId);
}

void AsyncSqliteCommunicationRepository::loadSnapshot(SnapshotRequest request, Completion completion)
{
    if (request.operationId.isNull()) {
        SnapshotResult rejected{request.operationId, request.previewRevision};
        rejected.error = {QStringLiteral("invalid-operation"), QStringLiteral("A snapshot operation ID is required.")};
        completion(std::move(rejected));
        return;
    }
    if (request.source.databaseGeneration != m_databaseGeneration) {
        SnapshotResult stale{request.operationId, request.previewRevision};
        stale.error = {QStringLiteral("database-generation-changed"), QStringLiteral("The selected database changed.")};
        completion(std::move(stale));
        return;
    }

    QPointer<AsyncSqliteCommunicationRepository> owner(this);
    const QString path = m_databasePath;
    QReadWriteLock *const lock = m_lock;
    m_workers.start(QRunnable::create([owner, path, lock, request, completion = std::move(completion)]() mutable {
        if (!owner || owner->isCancelled(request.operationId))
            return;
        SnapshotResult result = SqliteSnapshotLoader(path, lock).load(request);
        if (!owner || owner->isCancelled(request.operationId))
            return;
        QMetaObject::invokeMethod(owner, [owner, request, completion = std::move(completion), result = std::move(result)]() mutable {
            if (owner && !owner->isCancelled(request.operationId))
                completion(std::move(result));
        }, Qt::QueuedConnection);
    }));
}
} // namespace PN::Comm
