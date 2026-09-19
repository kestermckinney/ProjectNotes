// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "RecipientSelectionModel.h"

#include <QSet>
#include <QUuid>

#include <algorithm>

namespace PN::Comm {

RecipientSelectionModel::RecipientSelectionModel(QObject *parent) : QAbstractListModel(parent) {}

int RecipientSelectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant RecipientSelectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case PersonIdRole: return entry.person.id;
    case NameRole: return entry.person.name;
    case AddressRole: return entry.person.email;
    case CompanyNameRole: return entry.person.companyName;
    case SelectedRole: return entry.selected;
    case RecipientRoleRole: return static_cast<int>(entry.role);
    case ManualRole: return entry.manual;
    case CompanyGroupRole:
        return entry.manual ? tr("Manual additions")
                            : (entry.person.companyName.trimmed().isEmpty()
                               ? tr("Unknown company") : entry.person.companyName.trimmed());
    case SourceReasonRole: return entry.manual ? tr("Added manually") : sourceReason(entry.person);
    default: return {};
    }
}

QHash<int, QByteArray> RecipientSelectionModel::roleNames() const
{
    return {{PersonIdRole, "personId"}, {NameRole, "name"}, {AddressRole, "address"},
            {CompanyNameRole, "companyName"}, {SelectedRole, "selected"},
            {RecipientRoleRole, "recipientRole"}, {ManualRole, "manual"},
            {CompanyGroupRole, "companyGroup"}, {SourceReasonRole, "sourceReason"}};
}

void RecipientSelectionModel::setAudience(AudienceResolution audience)
{
    // A changed source is a new recipient decision.  Preserve neither manual
    // entries nor the explicit "address later" escape hatch from the prior
    // audience, otherwise a later source change could silently carry it over.
    const bool addressLaterChanged = m_addressLaterExplicitlyChosen;
    beginResetModel();
    m_entries.clear();
    int sourceOrder = 0;
    for (const SnapshotPerson &person : audience.people) {
        bool selected = true;
        RecipientRole role = RecipientRole::To;
        for (const RecipientOverride &overrideValue : audience.initialOverrides) {
            if (overrideValue.personId == person.id) {
                selected = overrideValue.selected;
                role = overrideValue.role;
            }
        }
        m_entries.append({person, selected, role, false, sourceOrder++});
    }
    // Grouping affects only presentation. resolve() restores this recorded
    // source order before applying overrides, preserving the frozen native
    // recipient order supplied by the repository/resolver.
    std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry &left, const Entry &right) {
        const QString leftGroup = left.person.companyName.trimmed();
        const QString rightGroup = right.person.companyName.trimmed();
        if (leftGroup.isEmpty() != rightGroup.isEmpty())
            return !leftGroup.isEmpty();
        return leftGroup.localeAwareCompare(rightGroup) < 0;
    });
    m_exclusions = std::move(audience.exclusions);
    m_audienceValidation = std::move(audience.validation);
    m_addressLaterExplicitlyChosen = false;
    endResetModel();
    if (addressLaterChanged)
        emit addressLaterExplicitlyChosenChanged();
    emit selectionStateChanged();
}

void RecipientSelectionModel::setInternalReportContext(bool internalReport, QString managingCompanyId)
{
    managingCompanyId = managingCompanyId.trimmed();
    if (m_internalReport == internalReport && m_managingCompanyId == managingCompanyId)
        return;
    m_internalReport = internalReport;
    m_managingCompanyId = std::move(managingCompanyId);
    emit selectionStateChanged();
}

int RecipientSelectionModel::indexOf(const QString &personId) const
{
    for (int index = 0; index < m_entries.size(); ++index)
        if (m_entries.at(index).person.id == personId) return index;
    return -1;
}

bool RecipientSelectionModel::setSelected(const QString &personId, bool selected)
{
    const int index = indexOf(personId); if (index < 0) return false;
    if (m_entries[index].selected == selected) return true;
    m_entries[index].selected = selected;
    emit dataChanged(this->index(index), this->index(index), {SelectedRole});
    emit selectionStateChanged();
    return true;
}

void RecipientSelectionModel::selectAll()
{
    bool changed = false;
    for (Entry &entry : m_entries) {
        if (!entry.selected) {
            entry.selected = true;
            changed = true;
        }
    }
    if (!changed)
        return;
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1), {SelectedRole});
    emit selectionStateChanged();
}

void RecipientSelectionModel::clearSelection()
{
    bool changed = false;
    for (Entry &entry : m_entries) {
        if (entry.selected) {
            entry.selected = false;
            changed = true;
        }
    }
    if (!changed)
        return;
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1), {SelectedRole});
    emit selectionStateChanged();
}

bool RecipientSelectionModel::setRecipientRole(const QString &personId, RecipientRole role)
{
    const int index = indexOf(personId); if (index < 0) return false;
    if (m_entries[index].role == role) return true;
    m_entries[index].role = role;
    emit dataChanged(this->index(index), this->index(index), {RecipientRoleRole});
    emit selectionStateChanged();
    return true;
}

bool RecipientSelectionModel::setRecipientRoleValue(const QString &personId, int role)
{
    if (role < static_cast<int>(RecipientRole::To) || role > static_cast<int>(RecipientRole::Bcc))
        return false;
    return setRecipientRole(personId, static_cast<RecipientRole>(role));
}

bool RecipientSelectionModel::addManual(QString displayName, QString address, RecipientRole role)
{
    const QString trimmed = address.trimmed();
    if (trimmed.isEmpty() || trimmed.contains('\r') || trimmed.contains('\n') ||
        trimmed.contains(',') || trimmed.contains(';') || !trimmed.contains('@') ||
        role < RecipientRole::To || role > RecipientRole::Bcc)
        return false;
    const QString key = trimmed.toCaseFolded();
    for (const AudienceExclusion &exclusion : m_exclusions)
        if (exclusion.reason == QLatin1String("project-manager-excluded")
            && exclusion.person.email.trimmed().toCaseFolded() == key)
            return false;
    for (const Entry &entry : m_entries)
        if (entry.person.email.trimmed().toCaseFolded() == key)
            return false;
    const int row = m_entries.size();
    beginInsertRows({}, row, row);
    int sourceOrder = 0;
    for (const Entry &entry : m_entries)
        sourceOrder = qMax(sourceOrder, entry.sourceOrder + 1);
    m_entries.append({{QStringLiteral("manual-") + QUuid::createUuid().toString(QUuid::WithoutBraces),
                       std::move(displayName), trimmed}, true, role, true, sourceOrder});
    endInsertRows();
    emit selectionStateChanged();
    return true;
}

bool RecipientSelectionModel::removeManual(const QString &personId)
{
    const int row = indexOf(personId);
    if (row < 0 || !m_entries.at(row).manual) return false;
    beginRemoveRows({}, row, row);
    m_entries.removeAt(row);
    endRemoveRows();
    emit selectionStateChanged();
    return true;
}

void RecipientSelectionModel::setAddressLaterExplicitlyChosen(bool chosen)
{
    if (m_addressLaterExplicitlyChosen == chosen)
        return;
    m_addressLaterExplicitlyChosen = chosen;
    emit addressLaterExplicitlyChosenChanged();
    emit selectionStateChanged();
}

bool RecipientSelectionModel::addressLaterExplicitlyChosen() const
{
    return m_addressLaterExplicitlyChosen;
}

int RecipientSelectionModel::selectedRecipientCount() const
{
    QSet<QString> addresses;
    for (const Entry &entry : m_entries) {
        const QString address = entry.person.email.trimmed();
        if (entry.selected && !address.isEmpty())
            addresses.insert(address.toCaseFolded());
    }
    return addresses.size();
}

int RecipientSelectionModel::recipientCount() const
{
    QSet<QString> addresses;
    for (const Entry &entry : m_entries) {
        const QString address = entry.person.email.trimmed();
        if (!address.isEmpty())
            addresses.insert(address.toCaseFolded());
    }
    return addresses.size();
}

int RecipientSelectionModel::recipientRoleCount(RecipientRole role) const
{
    QSet<QString> addresses;
    for (const Entry &entry : m_entries) {
        const QString address = entry.person.email.trimmed();
        if (entry.selected && entry.role == role && !address.isEmpty())
            addresses.insert(address.toCaseFolded());
    }
    return addresses.size();
}

int RecipientSelectionModel::toRecipientCount() const { return recipientRoleCount(RecipientRole::To); }
int RecipientSelectionModel::ccRecipientCount() const { return recipientRoleCount(RecipientRole::Cc); }
int RecipientSelectionModel::bccRecipientCount() const { return recipientRoleCount(RecipientRole::Bcc); }

QVariantList RecipientSelectionModel::excludedRecipients() const
{
    QVariantList values;
    values.reserve(m_exclusions.size());
    for (const AudienceExclusion &exclusion : m_exclusions) {
        const SnapshotPerson &person = exclusion.person;
        QString reason;
        if (exclusion.reason == QLatin1String("source-mismatch")) reason = tr("Not in the selected audience source");
        else if (exclusion.reason == QLatin1String("invalid-address")) reason = tr("Email address is invalid or unavailable");
        else if (exclusion.reason == QLatin1String("project-manager-excluded")) reason = tr("Project manager is excluded");
        else if (exclusion.reason == QLatin1String("company-filter")) reason = tr("Does not match the company filter");
        else reason = exclusion.reason;
        values.append(QVariantMap{{QStringLiteral("personId"), person.id},
                                  {QStringLiteral("name"), person.name},
                                  {QStringLiteral("companyName"), person.companyName},
                                  {QStringLiteral("reason"), reason}});
    }
    return values;
}

QString RecipientSelectionModel::internalAudienceWarning() const
{
    if (!m_internalReport)
        return {};
    if (m_managingCompanyId.isEmpty())
        return tr("This report is marked internal, but the managing company is not configured.");
    QSet<QString> outsideAddresses;
    for (const Entry &entry : m_entries) {
        const QString address = entry.person.email.trimmed();
        if (entry.selected && entry.person.companyId != m_managingCompanyId && !address.isEmpty())
            outsideAddresses.insert(address.toCaseFolded());
    }
    if (outsideAddresses.isEmpty())
        return {};
    return tr("This internal report has %n selected recipient(s) outside the managing company.",
              nullptr, outsideAddresses.size());
}

QString RecipientSelectionModel::sourceReason(const SnapshotPerson &person) const
{
    QStringList sources;
    if (person.projectTeamMember) sources.append(tr("Project team"));
    if (person.meetingAttendee) sources.append(tr("Meeting attendee"));
    if (person.receivesStatus) sources.append(tr("Receives status reports"));
    return sources.isEmpty() ? tr("Selected audience") : sources.join(QStringLiteral(" · "));
}

QString RecipientSelectionModel::audienceDiagnostic() const
{
    if (m_audienceValidation.issues.isEmpty()) return {};
    const ValidationIssue &issue = m_audienceValidation.issues.constFirst();
    return issue.displayText.isEmpty() ? issue.code : issue.displayText;
}

QList<RecipientOverride> RecipientSelectionModel::overrides() const
{
    QList<RecipientOverride> result;
    for (const Entry &entry : m_entries)
        if (!entry.manual)
            result.append({entry.person.id, entry.selected, entry.role});
    return result;
}

void RecipientSelectionModel::applyOverrides(const QList<RecipientOverride> &overrides)
{
    QHash<QString, RecipientOverride> byPerson;
    for (const RecipientOverride &overrideValue : overrides)
        byPerson.insert(overrideValue.personId, overrideValue);
    bool changed = false;
    for (Entry &entry : m_entries) {
        const auto found = byPerson.constFind(entry.person.id);
        if (entry.manual || found == byPerson.cend())
            continue;
        if (entry.selected != found->selected || entry.role != found->role) {
            entry.selected = found->selected;
            entry.role = found->role;
            changed = true;
        }
    }
    if (!changed)
        return;
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1), {SelectedRole, RecipientRoleRole});
    emit selectionStateChanged();
}

void RecipientSelectionModel::reset()
{
    beginResetModel();
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                   [](const Entry &entry) { return entry.manual; }),
                    m_entries.end());
    for (Entry &entry : m_entries) { entry.selected = true; entry.role = RecipientRole::To; }
    const bool addressLaterChanged = m_addressLaterExplicitlyChosen;
    m_addressLaterExplicitlyChosen = false;
    endResetModel();
    if (addressLaterChanged)
        emit addressLaterExplicitlyChosenChanged();
    emit selectionStateChanged();
}

RecipientResolution RecipientSelectionModel::resolve() const
{
    AudienceResolution audience;
    QList<RecipientOverride> overrides;
    QList<Entry> orderedEntries = m_entries;
    std::stable_sort(orderedEntries.begin(), orderedEntries.end(),
                     [](const Entry &left, const Entry &right) { return left.sourceOrder < right.sourceOrder; });
    for (const Entry &entry : orderedEntries) {
        audience.people.append(entry.person);
        overrides.append({entry.person.id, entry.selected, entry.role});
    }
    RecipientResolution result = applyRecipientOverrides(audience, overrides);
    result.validation.issues = m_audienceValidation.issues + result.validation.issues;
    result.addressLaterExplicitlyChosen = m_addressLaterExplicitlyChosen;
    return result;
}

} // namespace PN::Comm
