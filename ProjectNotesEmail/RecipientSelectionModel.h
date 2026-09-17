// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "RecipientAudienceResolver.h"

#include <QAbstractListModel>
#include <QVariantList>

namespace PN::Comm {

class RecipientSelectionModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool addressLaterExplicitlyChosen READ addressLaterExplicitlyChosen WRITE setAddressLaterExplicitlyChosen NOTIFY addressLaterExplicitlyChosenChanged)
    Q_PROPERTY(int recipientCount READ recipientCount NOTIFY selectionStateChanged)
    Q_PROPERTY(int selectedRecipientCount READ selectedRecipientCount NOTIFY selectionStateChanged)
    Q_PROPERTY(int toRecipientCount READ toRecipientCount NOTIFY selectionStateChanged)
    Q_PROPERTY(int ccRecipientCount READ ccRecipientCount NOTIFY selectionStateChanged)
    Q_PROPERTY(int bccRecipientCount READ bccRecipientCount NOTIFY selectionStateChanged)
    Q_PROPERTY(QVariantList excludedRecipients READ excludedRecipients NOTIFY selectionStateChanged)
    Q_PROPERTY(QString internalAudienceWarning READ internalAudienceWarning NOTIFY selectionStateChanged)
    Q_PROPERTY(QString audienceDiagnostic READ audienceDiagnostic NOTIFY selectionStateChanged)
public:
    enum Role {
        PersonIdRole = Qt::UserRole + 1,
        NameRole,
        AddressRole,
        CompanyNameRole,
        SelectedRole,
        RecipientRoleRole,
        ManualRole,
        CompanyGroupRole,
        SourceReasonRole
    };

    explicit RecipientSelectionModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setAudience(AudienceResolution audience);
    // The controller supplies only report classification and the snapshot's
    // managing-company ID. The model derives the warning from its selected
    // entries; QML never receives or compares company IDs.
    void setInternalReportContext(bool internalReport, QString managingCompanyId);
    Q_INVOKABLE bool setSelected(const QString &personId, bool selected);
    Q_INVOKABLE bool setRecipientRole(const QString &personId, RecipientRole role);
    Q_INVOKABLE bool setRecipientRoleValue(const QString &personId, int role);
    Q_INVOKABLE bool addManual(QString displayName, QString address, RecipientRole role = RecipientRole::To);
    Q_INVOKABLE bool removeManual(const QString &personId);
    Q_INVOKABLE void setAddressLaterExplicitlyChosen(bool chosen);
    [[nodiscard]] bool addressLaterExplicitlyChosen() const;
    Q_INVOKABLE void reset();
    [[nodiscard]] int recipientCount() const;
    [[nodiscard]] int selectedRecipientCount() const;
    [[nodiscard]] int toRecipientCount() const;
    [[nodiscard]] int ccRecipientCount() const;
    [[nodiscard]] int bccRecipientCount() const;
    [[nodiscard]] QVariantList excludedRecipients() const;
    [[nodiscard]] QString internalAudienceWarning() const;
    [[nodiscard]] QString audienceDiagnostic() const;
    // Presets contain only persisted snapshot-person choices. Manual entries
    // and the address-later escape hatch deliberately remain review-local.
    [[nodiscard]] QList<RecipientOverride> overrides() const;
    void applyOverrides(const QList<RecipientOverride> &overrides);
    RecipientResolution resolve() const;

signals:
    void addressLaterExplicitlyChosenChanged();
    void selectionStateChanged();

private:
    struct Entry {
        SnapshotPerson person;
        bool selected = true;
        RecipientRole role = RecipientRole::To;
        bool manual = false;
        int sourceOrder = 0;
    };
    int indexOf(const QString &personId) const;
    int recipientRoleCount(RecipientRole role) const;
    QString sourceReason(const SnapshotPerson &person) const;
    QList<Entry> m_entries;
    QList<AudienceExclusion> m_exclusions;
    ValidationResult m_audienceValidation;
    bool m_addressLaterExplicitlyChosen = false;
    bool m_internalReport = false;
    QString m_managingCompanyId;
};

} // namespace PN::Comm
