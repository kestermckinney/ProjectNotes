// SPDX-License-Identifier: GPL-3.0-only
#include "ContactSyncSettingsModel.h"
#include "ContactSyncWorker.h"
#include "databaseobjects.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QTimer>
namespace PN::Contacts {
// Test providers retain their caller's affinity; callbacks return to the worker.
class QueuedContactProvider final : public ContactProvider {
public:
    QueuedContactProvider(ContactProvider *target,QObject *parent):ContactProvider(parent),m_target(target){}
    void fetch(const QVariantMap &config,const QJsonObject &state,Completion done) override {
        auto target=m_target;QPointer<QueuedContactProvider> self(this);
        if(!target){Snapshot error;error.error="Contact provider is unavailable.";done(error);return;}
        QMetaObject::invokeMethod(target,[target,self,config,state,done=std::move(done)]()mutable{
            if(!target||!self)return;
            target->fetch(config,state,[self,done=std::move(done)](Snapshot result)mutable{
                if(self)QMetaObject::invokeMethod(self,[self,done=std::move(done),result=std::move(result)]()mutable{if(self)done(std::move(result));},Qt::QueuedConnection);
            });
        },Qt::QueuedConnection);
    }
    void create(const QVariantMap &config,const Contact &contact,const QString &intent,Created done) override {
        auto target=m_target;QPointer<QueuedContactProvider> self(this);
        if(!target){done({},"Contact provider is unavailable.");return;}
        QMetaObject::invokeMethod(target,[target,self,config,contact,intent,done=std::move(done)]()mutable{
            if(!target||!self)return;
            target->create(config,contact,intent,[self,done=std::move(done)](Contact result,QString error)mutable{
                if(self)QMetaObject::invokeMethod(self,[self,done=std::move(done),result=std::move(result),error=std::move(error)]()mutable{if(self)done(std::move(result),std::move(error));},Qt::QueuedConnection);
            });
        },Qt::QueuedConnection);
    }
    void cancel() override {
        auto target=m_target;if(target)QMetaObject::invokeMethod(target,[target]{if(target)target->cancel();},Qt::QueuedConnection);
    }
private: QPointer<ContactProvider> m_target;
};
ContactSyncSettingsModel::ContactSyncSettingsModel(DatabaseObjects *database,PN::Comm::Office365Service *office,QString organization,QString root,QObject *parent)
    :QObject(parent),m_database(database),m_office(office),m_root(std::move(root)) {
    m_worker=new ContactSyncWorker(office,std::move(organization),m_root);
    m_worker->moveToThread(&m_thread);m_thread.setObjectName("ContactSync");
    connect(&m_thread,&QThread::finished,m_worker,&QObject::deleteLater);
    connect(m_worker,&ContactSyncWorker::changed,m_worker,[this]{
        auto revision=m_worker->revision;auto config=m_worker->configuration();auto books=m_worker->books();auto reviews=m_worker->reviews();
        auto busy=m_worker->busy();auto status=m_worker->status();auto success=m_worker->lastSuccess();
        QMetaObject::invokeMethod(this,[this,revision,config,books,reviews,busy,status,success]{
            if(revision!=m_revision)return;
            m_config=config;m_books=books;m_reviews=reviews;m_busy=busy;m_status=status;m_lastSuccess=success;emit changed();
        },Qt::QueuedConnection);
    });
    connect(m_worker,&ContactSyncWorker::recordsChanged,this,[this](const QString &path){
        if(path!=m_databasePath||!m_database->isOpen()||m_refreshQueued)return;
        m_refreshQueued=true;
        QTimer::singleShot(50,this,[this,path]{
            m_refreshQueued=false;if(path!=m_databasePath||!m_database->isOpen())return;
            for(auto *model:m_database->getOpenModels())if(model->tablename()=="people"||model->tablename()=="clients")model->refresh();
        });
    },Qt::QueuedConnection);
    m_thread.start();
    if(office){
        auto update=[this]{auto account=m_office->account();bool progress=m_office->authenticationInProgress();post([account,progress](auto &worker){worker.setOfficeAccount(account,progress);});};
        connect(office,&PN::Comm::Office365Service::accountChanged,this,update);update();
    }
}
ContactSyncSettingsModel::~ContactSyncSettingsModel(){
    ++m_revision;
    QMetaObject::invokeMethod(m_worker,[this]{m_worker->setDatabase({});},Qt::BlockingQueuedConnection);
    m_thread.quit();m_thread.wait();
}
void ContactSyncSettingsModel::post(std::function<void(ContactSyncWorker &)> action){
    auto revision=++m_revision;auto *worker=m_worker;
    QMetaObject::invokeMethod(worker,[worker,revision,action=std::move(action)]{worker->revision=revision;action(*worker);},Qt::QueuedConnection);
}
void ContactSyncSettingsModel::setDatabase(const QString &path){
    m_databasePath=path;auto revision=++m_revision;
    QVariantMap config;QVariantList books,reviews;QString status,success;bool busy=false;
    QMetaObject::invokeMethod(m_worker,[&]{
        m_worker->revision=revision;m_worker->setDatabase(path);
        if(!m_integrationProvider.isNull())m_worker->setOption("provider",m_integrationProvider);
        config=m_worker->configuration();books=m_worker->books();reviews=m_worker->reviews();busy=m_worker->busy();status=m_worker->status();success=m_worker->lastSuccess();
    },Qt::BlockingQueuedConnection);
    m_config=config;m_books=books;m_reviews=reviews;m_busy=busy;m_status=status;m_lastSuccess=success;emit changed();
}
void ContactSyncSettingsModel::setIntegrationBackend(const QString &backend){
    QString provider="none";
    if(backend=="graph"||backend=="outlook")provider="office365";
    else if(backend=="outlook-classic")provider="outlook";
    else if(backend=="thunderbird")provider="thunderbird";
    else if(backend=="mac-mail")provider="icloud";
    m_integrationProvider=provider;
    if(m_databasePath.isEmpty()){m_config["provider"]=provider;emit changed();}
    else if(m_config.value("provider")!=provider)setOption("provider",provider);
}
void ContactSyncSettingsModel::setOption(const QString &key,const QVariant &value){
    if(!QStringList{"provider","enabled","startup","interval","export","account","sourceBooks"}.contains(key))return;
    if(key=="provider"&&!QStringList{"outlook","office365","thunderbird","icloud","none"}.contains(value.toString()))return;
    if((key=="provider"||key=="account")&&value!=m_config.value(key)){m_config["sourceBooks"]=QStringList{};m_books.clear();}
    m_config[key]=key=="interval"?QVariant(qBound(1,value.toInt(),1440)):value;m_busy=false;emit changed();
    post([key,value](auto &worker){worker.setOption(key,value);});
}
void ContactSyncSettingsModel::syncNow(){if(m_busy)return;m_busy=true;emit changed();post([](auto &worker){worker.syncNow();});}
void ContactSyncSettingsModel::testConnection(){if(m_busy)return;m_busy=true;emit changed();post([](auto &worker){worker.testConnection();});}
void ContactSyncSettingsModel::cancel(){m_busy=false;emit changed();post([](auto &worker){worker.cancel();});}
void ContactSyncSettingsModel::cancelSync(){m_busy=false;emit changed();post([](auto &worker){worker.cancelSync();});}
void ContactSyncSettingsModel::savePassword(const QString &password){post([password](auto &worker){worker.savePassword(password);});}
void ContactSyncSettingsModel::resolve(const QString &key,const QString &action,const QString &person,const QString &client,const QString &name){
    post([key,action,person,client,name](auto &worker){worker.resolve(key,action,person,client,name);});
}
void ContactSyncSettingsModel::rejectCandidate(const QString &key,const QString &person){post([key,person](auto &worker){worker.rejectCandidate(key,person);});}
void ContactSyncSettingsModel::setProviderForTesting(ContactProvider *provider){
    QMetaObject::invokeMethod(m_worker,[this,provider]{m_worker->setProviderForTesting(new QueuedContactProvider(provider,m_worker));},Qt::BlockingQueuedConnection);
}
void ContactSyncSettingsModel::installThunderbirdHost(){
    QString executable=QCoreApplication::applicationDirPath()+"/ProjectNotesThunderbirdContacts";
#ifdef Q_OS_WIN
    executable+=".exe";
#endif
    if(!QFileInfo::exists(executable)){m_status="The Thunderbird native host is missing from this installation.";emit changed();return;}
    QJsonObject manifest{{"name","org.projectnotes.contacts"},{"description","Project Notes contact bridge"},{"path",executable},{"type","stdio"},
        {"allowed_extensions",QJsonArray{"contacts@projectnotes.org"}}};
    QString directory;
#ifdef Q_OS_WIN
    directory=m_root+"/contacts";
#elif defined(Q_OS_MACOS)
    directory=QDir::homePath()+"/Library/Mozilla/NativeMessagingHosts";
#else
    directory=QDir::homePath()+"/.mozilla/native-messaging-hosts";
#endif
    QDir().mkpath(directory);QString path=directory+"/org.projectnotes.contacts.json";QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(QJsonDocument(manifest).toJson())<0||!file.commit()){m_status="Cannot install Thunderbird native-host manifest.";emit changed();return;}
#ifdef Q_OS_WIN
    QSettings registry("HKEY_CURRENT_USER\\Software\\Mozilla\\NativeMessagingHosts\\org.projectnotes.contacts",QSettings::NativeFormat);registry.setValue(".",path);registry.sync();
#endif
    QString addon=QCoreApplication::applicationDirPath()+"/projectnotes-contacts.xpi";
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(addon).absolutePath()));
    m_status="Native host installed. In Thunderbird Add-ons, choose Install Add-on From File and select projectnotes-contacts.xpi beside the application. Restart Thunderbird. Sandboxed Thunderbird requires a host accessible inside its sandbox.";emit changed();
}
}
