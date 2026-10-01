// SPDX-License-Identifier: GPL-3.0-only
#include "ContactSyncSettingsModel.h"
#include "ContactSyncWorker.h"
#include "ThunderbirdBridge.h"
#include "databaseobjects.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QProcess>
#include <QStandardPaths>
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
    connect(m_worker,&ContactSyncWorker::fullExportFinished,m_worker,[this](const QString &message,bool success){
        const auto revision=m_worker->revision;
        QMetaObject::invokeMethod(this,[this,revision,message,success]{
            if(revision==m_revision)emit fullExportFinished(message,success);
        },Qt::QueuedConnection);
    });
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
            emit recordsChanged();
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
    if(!QStringList{"provider","enabled","startup","interval","account","sourceBooks"}.contains(key))return;
    if(key=="provider"&&!QStringList{"outlook","office365","thunderbird","icloud","none"}.contains(value.toString()))return;
    if((key=="provider"||key=="account")&&value!=m_config.value(key)){m_config["sourceBooks"]=QStringList{};m_books.clear();}
    m_config[key]=key=="interval"?QVariant(qBound(1,value.toInt(),1440)):value;m_busy=false;emit changed();
    post([key,value](auto &worker){worker.setOption(key,value);});
}
void ContactSyncSettingsModel::syncNow(){if(m_busy)return;m_busy=true;emit changed();post([](auto &worker){worker.syncNow();});}
void ContactSyncSettingsModel::fullExport(){if(m_busy)return;m_busy=true;emit changed();post([](auto &worker){worker.fullExport();});}
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
bool ContactSyncSettingsModel::thunderbirdFlatpakInstalled() const {
    return !thunderbirdFlatpakIds(QDir::homePath()).isEmpty();
}
void ContactSyncSettingsModel::finishThunderbirdSetup(const QString &message,bool success){
    m_thunderbirdSetupBusy=false;m_status=message;emit changed();emit thunderbirdSetupFinished(message,success);
}
void ContactSyncSettingsModel::enableThunderbirdFlatpak(QStringList ids,const QString &instructions){
    if(ids.isEmpty()){finishThunderbirdSetup(instructions,true);return;}
    const QString id=ids.takeFirst();
    auto *process=new QProcess(this);
    QStringList arguments{"override","--user","--persist=.mozilla","--talk-name=org.freedesktop.Flatpak",id};
    if(!qEnvironmentVariable("FLATPAK_ID").isEmpty()){
        arguments.prepend("flatpak");arguments.prepend("--host");
        process->setProgram("flatpak-spawn");
    }else process->setProgram("flatpak");
    process->setArguments(arguments);
    auto completed=std::make_shared<bool>(false);
    auto fail=[this,process,completed,id,ids,instructions]{
        if(*completed)return;*completed=true;
        QString detail=QString::fromUtf8(process->readAllStandardError()).trimmed();
        QStringList commands;
        for(const auto &app:QStringList{id}+ids)
            commands << "flatpak override --user --persist=.mozilla --talk-name=org.freedesktop.Flatpak "+app;
        finishThunderbirdSetup(tr("Bridge files installed, but Flatpak permission could not be enabled automatically.\n\nRun these commands in a terminal, then follow the add-on installation steps below:\n%1\n\n%2\n\n%3").arg(commands.join('\n'),instructions,detail),false);
        process->deleteLater();
    };
    connect(process,&QProcess::errorOccurred,this,[fail](auto){fail();});
    connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
        [this,process,completed,ids,instructions,fail](int code,QProcess::ExitStatus status){
            if(*completed)return;
            if(code!=0||status!=QProcess::NormalExit){fail();return;}
            *completed=true;process->deleteLater();enableThunderbirdFlatpak(ids,instructions);
        });
    QTimer::singleShot(15000,process,[process,completed,fail]{if(!*completed){process->kill();fail();}});
    process->start();
}
void ContactSyncSettingsModel::installThunderbirdHost(){
    if(m_thunderbirdSetupBusy)return;
    m_thunderbirdSetupBusy=true;emit changed();
    QString directory;
#ifdef Q_OS_WIN
    directory=m_root+"/contacts";
#elif defined(Q_OS_MACOS)
    directory=QDir::homePath()+"/Library/Mozilla/NativeMessagingHosts";
#else
    directory=QDir::homePath()+"/.mozilla/native-messaging-hosts";
#endif
    const auto result=installThunderbirdBridge(QDir::homePath(),QCoreApplication::applicationDirPath(),
                                              directory,qEnvironmentVariable("FLATPAK_ID"));
    if(!result.error.isEmpty()){finishThunderbirdSetup(result.error,false);return;}
#ifdef Q_OS_WIN
    QSettings registry("HKEY_CURRENT_USER\\Software\\Mozilla\\NativeMessagingHosts\\org.projectnotes.contacts",QSettings::NativeFormat);
    registry.setValue(".",directory+"/org.projectnotes.contacts.json");registry.sync();
    if(registry.status()!=QSettings::NoError){finishThunderbirdSetup(tr("Cannot register the Thunderbird native host in the Windows registry."),false);return;}
#endif
    const QString instructions=tr("Bridge helper registered. Complete these steps in Thunderbird 128 or newer:\n\n1. Open Add-ons and Themes.\n2. Click the gear menu, then Install Add-on From File.\n3. Select this file:\n%1\n\n4. Accept installation of Project Notes Contacts.\n5. Quit Thunderbird completely and reopen it.\n6. Leave Thunderbird running, return here and click Test Connection.\n\nSetup does not install the add-on automatically.").arg(result.addonPath);
    enableThunderbirdFlatpak(result.flatpakIds,instructions);
}
}
