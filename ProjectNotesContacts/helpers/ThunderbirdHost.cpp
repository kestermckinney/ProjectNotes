// SPDX-License-Identifier: GPL-3.0-only
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <cstdio>
#include <thread>
#include "../ThunderbirdBridge.h"
#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif
// Thunderbird owns this process and speaks native-messaging frames on stdio.
// Project Notes speaks one bounded JSON line over a user-only local socket.
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);
#ifdef Q_OS_WIN
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    auto send=[](const QJsonObject&o){QByteArray bytes=QJsonDocument(o).toJson(QJsonDocument::Compact);quint32 n=quint32(bytes.size());std::fwrite(&n,4,1,stdout);std::fwrite(bytes.constData(),1,n,stdout);std::fflush(stdout);};
    QLocalServer server;server.setSocketOptions(QLocalServer::UserAccessOption);
    QString name=PN::Contacts::thunderbirdBridgeSocket();
    if(app.arguments().size()==3&&app.arguments()[1]=="--test-socket")name=app.arguments()[2];
    if(QDir::isAbsolutePath(name)&&!QDir().mkpath(QFileInfo(name).absolutePath()))return 1;
    QLockFile lock(QDir::isAbsolutePath(name)?name+".lock":QDir::tempPath()+"/projectnotes-contacts-host-"+QString::fromLatin1(QCryptographicHash::hash(name.toUtf8(),QCryptographicHash::Sha256).toHex())+".lock");
    lock.setStaleLockTime(0);if(!lock.tryLock())return 1;
    // Only the lock owner can remove the stale socket after a host crash.
    QLocalServer::removeServer(name);
    if(!server.listen(name))return 1;
    QHash<QString,QPointer<QLocalSocket>> pending;
    QObject::connect(&server,&QLocalServer::newConnection,&app,[&]{
        auto socket=server.nextPendingConnection();auto buffer=std::make_shared<QByteArray>();auto used=std::make_shared<bool>(false);
        QObject::connect(socket,&QLocalSocket::readyRead,&app,[&,socket,buffer,used]{
            *buffer+=socket->readAll();if(buffer->size()>1024*1024){socket->abort();return;}int end=buffer->indexOf('\n');if(end<0||*used)return;*used=true;
            auto o=QJsonDocument::fromJson(buffer->left(end)).object();QString action=o["action"].toString();
            if(o["version"].toInt()!=1 || (action!="snapshot"&&action!="create")){socket->write("{\"version\":1,\"error\":\"Invalid bridge request\"}\n");socket->disconnectFromServer();return;}
            QString id=QUuid::createUuid().toString();o["requestId"]=id;pending[id]=socket;send(o);
            QTimer::singleShot(60000,socket,[&,id,socket]{pending.remove(id);socket->abort();socket->deleteLater();});
        });
        QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
    });
    std::thread reader([&]{
        for(;;){quint32 size=0;if(std::fread(&size,4,1,stdin)!=1)break;if(size==0||size>64*1024*1024)break;
            QByteArray bytes(int(size),Qt::Uninitialized);if(std::fread(bytes.data(),1,size,stdin)!=size)break;
            auto object=QJsonDocument::fromJson(bytes).object();
            QMetaObject::invokeMethod(&app,[&,object]{auto socket=pending.take(object["requestId"].toString());if(socket){socket->write(QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n');socket->disconnectFromServer();}},Qt::QueuedConnection);
        }QMetaObject::invokeMethod(&app,&QCoreApplication::quit,Qt::QueuedConnection);
    });
    int result=app.exec();reader.join();return result;
}
