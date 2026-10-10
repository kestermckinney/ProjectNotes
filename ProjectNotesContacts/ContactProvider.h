// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactTypes.h"
#include <QObject>
#include <QPointer>
#include <QJsonArray>
#include "ProjectNotesIntegrations/HttpTransport.h"
#include "ProjectNotesIntegrations/Office365Service.h"
namespace PN::Comm { class Office365Service; class QtNetworkHttpTransport; }
class QLocalSocket;
class QProcess;
namespace PN::Contacts {
class ContactProvider : public QObject {
public:
    using Completion=std::function<void(Snapshot)>;
    using Created=std::function<void(Contact,QString)>;
    using Updated=std::function<void(Contact,QString)>;
    using QObject::QObject;
    virtual void fetch(const QVariantMap &config,const QJsonObject &state,Completion done)=0;
    virtual void create(const QVariantMap &config,const Contact &contact,const QString &intentId,Created done)=0;
    virtual void update(const QVariantMap &config,const Contact &existing,const Contact &contact,Updated done)=0;
    virtual void cancel()=0;
};
class NativeContactProvider final : public ContactProvider {
public:
    NativeContactProvider(QString provider,PN::Comm::Office365Service *office,QObject *parent=nullptr);
    ~NativeContactProvider() override;
    void fetch(const QVariantMap &config,const QJsonObject &state,Completion done) override;
    void create(const QVariantMap &config,const Contact &contact,const QString &intentId,Created done) override;
    void update(const QVariantMap &config,const Contact &existing,const Contact &contact,Updated done) override;
    void cancel() override;
    void setOfficeAccount(PN::Comm::Office365Account account) {m_officeAccount=std::move(account);}
    void setCardDAVTransportForTesting(PN::Comm::HttpTransport *transport,
                                      std::function<QString()> password) {m_davTransport=transport;m_password=std::move(password);}
private:
    PN::Comm::Office365Account officeAccount() const;
    void sendGraph(PN::Comm::HttpRequest request,PN::Comm::HttpTransport::Completion done);
    void cancelGraph(const QUuid &id);
    void graphPage(QUrl url);
    void graphBook(int index);
    void graphFolders(QUrl url);
    void dav(QByteArray method,QUrl url,QByteArray body,int depth,std::function<void(PN::Comm::HttpResponse)> done,int redirects=0,QByteArray condition={});
    void davDiscover(int step,QUrl url);
    void davBook(int index);
    void local(QJsonObject request,std::function<void(QJsonObject,QString)> done);
    void finish(QString error={},bool authentication=false);
    QString m_provider;
    PN::Comm::Office365Service *m_office;
    PN::Comm::Office365Account m_officeAccount;
    PN::Comm::QtNetworkHttpTransport *m_transport;
    PN::Comm::HttpTransport *m_davTransport=nullptr;
    std::function<QString()> m_password;
    QVariantMap m_config;
    QJsonObject m_state;
    QJsonObject m_remote,m_tokens;
    QStringList m_folderQueue;
    bool m_incremental=false;
    quint64 m_accountGeneration=0;
    Snapshot m_snapshot;
    Completion m_done;
    quint64 m_generation=0;
    QList<QUuid> m_requests;
    QPointer<QProcess> m_process;
    QPointer<QLocalSocket> m_socket;
};
// Picks NativeContactProvider for every remote/bridge provider string, except
// "mac" on Apple builds, which goes straight through CNContactStore.
ContactProvider *createContactProvider(QString provider,PN::Comm::Office365Service *office,QObject *parent);
}
