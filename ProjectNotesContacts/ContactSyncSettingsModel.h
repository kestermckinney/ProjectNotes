// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactProvider.h"
#include "ProjectNotesIntegrations/ICloudCredentials.h"
#include <QThread>
class DatabaseObjects;
namespace PN::Contacts {
class ContactSyncWorker;
// UI-thread facade; the worker exclusively owns reconciliation, timers and SQL.
class ContactSyncSettingsModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap configuration READ configuration NOTIFY changed)
    Q_PROPERTY(QVariantList books READ books NOTIFY changed)
    Q_PROPERTY(QVariantList reviews READ reviews NOTIFY changed)
    Q_PROPERTY(QVariantList exportReviews READ exportReviews NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString lastSuccess READ lastSuccess NOTIFY changed)
    Q_PROPERTY(bool thunderbirdFlatpakInstalled READ thunderbirdFlatpakInstalled CONSTANT)
    Q_PROPERTY(bool thunderbirdSetupBusy READ thunderbirdSetupBusy NOTIFY changed)
public:
    ContactSyncSettingsModel(DatabaseObjects *database,PN::Comm::Office365Service *office,
                             QString organization,QString dataRoot,QObject *parent=nullptr);
    ~ContactSyncSettingsModel() override;
    QVariantMap configuration() const {return m_config;}
    QVariantList books() const {return m_books;}
    QVariantList reviews() const {return m_reviews;}
    QVariantList exportReviews() const {return m_exportReviews;}
    bool busy() const {return m_busy;}
    QString status() const {return m_status;}
    QString lastSuccess() const {return m_lastSuccess;}
    bool thunderbirdFlatpakInstalled() const;
    bool thunderbirdSetupBusy() const {return m_thunderbirdSetupBusy;}
    PN::Comm::ICloudCredentials iCloudCredentials() const;
    void setDatabase(const QString &path);
    void setIntegrationBackend(const QString &backend);
    Q_INVOKABLE void setOption(const QString &key,const QVariant &value);
    Q_INVOKABLE void savePassword(const QString &password);
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void fullExport();
    Q_INVOKABLE void testConnection();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void cancelSync();
    Q_INVOKABLE void resolveExport(const QString &personId,const QString &action,const QString &externalId);
    Q_INVOKABLE void resolve(const QString &key,const QString &action,const QString &personId,
                             const QString &clientId,const QString &displayName);
    Q_INVOKABLE void rejectCandidate(const QString &key,const QString &personId);
    Q_INVOKABLE void installThunderbirdHost();
    void setProviderForTesting(ContactProvider *provider);
signals:
    void changed();
    void recordsChanged();
    void thunderbirdSetupFinished(const QString &message, bool success);
    void fullExportFinished(const QString &message, bool success);
    void exportReviewReady();
private:
    void post(std::function<void(ContactSyncWorker &)> action);
    void finishThunderbirdSetup(const QString &message,bool success);
    void enableThunderbirdFlatpak(QStringList ids,const QString &instructions);
    QThread m_thread;
    ContactSyncWorker *m_worker = nullptr;
    DatabaseObjects *m_database;
    PN::Comm::Office365Service *m_office;
    quint64 m_revision = 0;
    QVariantMap m_config;
    QVariantList m_books,m_reviews,m_exportReviews;
    bool m_busy = false, m_refreshQueued = false;
    bool m_thunderbirdSetupBusy = false;
    QString m_status,m_lastSuccess,m_databasePath,m_root,m_integrationProvider;
};
}
