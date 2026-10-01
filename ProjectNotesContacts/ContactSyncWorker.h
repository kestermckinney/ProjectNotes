// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactProvider.h"
#include "ContactRepository.h"
#include <QTimer>
#include "ProjectNotesIntegrations/Office365Service.h"
namespace PN::Contacts {
class ContactSyncWorker final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap configuration READ configuration NOTIFY changed)
    Q_PROPERTY(QVariantList books READ books NOTIFY changed)
    Q_PROPERTY(QVariantList reviews READ reviews NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString lastSuccess READ lastSuccess NOTIFY changed)
public:
    ContactSyncWorker(PN::Comm::Office365Service *office,
                             QString organization,QString dataRoot,QObject *parent=nullptr);
    ~ContactSyncWorker() override;
    QVariantMap configuration() const {return m_config;}
    QVariantList books() const {return m_books;}
    QVariantList reviews() const;
    bool busy() const {return m_busy;}
    QString status() const {return m_status;}
    QString lastSuccess() const {return m_journal["lastSuccess"].toString();}
    void setDatabase(const QString &path);
    Q_INVOKABLE void setOption(const QString &key,const QVariant &value);
    Q_INVOKABLE void savePassword(const QString &password);
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void fullExport();
    Q_INVOKABLE void testConnection();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void cancelSync();
    Q_INVOKABLE void resolve(const QString &key,const QString &action,const QString &personId,
                             const QString &clientId,const QString &displayName);
    Q_INVOKABLE void rejectCandidate(const QString &key,const QString &personId);
    // Test seam: injected provider remains owned by its caller.
    void setProviderForTesting(ContactProvider *provider) {m_testProvider=provider;}
    void setOfficeAccount(PN::Comm::Office365Account account, bool inProgress);
    quint64 revision = 0;
signals: void changed();
    void recordsChanged(const QString &databasePath);
    void fullExportFinished(const QString &message, bool success);
private:
    bool persist();
    void schedule();
    void run(bool test);
    bool apply(Contact contact,const QString &key,QString personId,QString clientId,QString displayName,bool review,bool keepBoth=false);
    void receive(Snapshot snapshot,bool test);
    Match companyMatch(const QString &name,const QList<Client> &clients) const;
    bool prepareClientReviews();
    void importNext();
    void finishSync();
    void fullExportNext();
    void complete(const QString &error={}, bool exportSuccess=false);
    void addReview(const QString &key,const Contact &contact,const Match &people,const Match &clients,const QString &reason);
    QString scope(const QString &account) const;
    QString m_databasePath;
    PN::Comm::Office365Account m_account;
    PN::Comm::Office365Service *m_office;
    ContactRepository m_repository;
    QString m_organization,m_root,m_path,m_status="Choose and enable a contact source.";
    QVariantMap m_config;
    QJsonObject m_journal;
    QVariantList m_books;
    QTimer m_timer;
    ContactProvider *m_provider=nullptr,*m_testProvider=nullptr;
    quint64 m_generation=0;
    quint64 m_officeGeneration=0;
    bool m_busy=false,m_authFailed=false,m_runningFullExport=false;
    Snapshot m_snapshot;
    QString m_scope;
    QList<Person> m_people;
    QList<Client> m_clients;
    QStringList m_fullExportIds;
    int m_fullExportIndex=0;
    int m_imported=0,m_exported=0;
    int m_unchanged=0;
    int m_importIndex=0;
};
}
