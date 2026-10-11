// SPDX-License-Identifier: GPL-3.0-only
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <windows.h>
#include <oleauto.h>
#include <wrl/client.h>
#include <vector>
#include <algorithm>
#include <cstdio>
using Microsoft::WRL::ComPtr;
static VARIANT text(const QString&s){VARIANT v;VariantInit(&v);v.vt=VT_BSTR;v.bstrVal=SysAllocStringLen(reinterpret_cast<const OLECHAR*>(s.utf16()),UINT(s.size()));return v;}
static VARIANT number(long n){VARIANT v;VariantInit(&v);v.vt=VT_I4;v.lVal=n;return v;}
static VARIANT call(IDispatch*o,const wchar_t*name,WORD flags,std::vector<VARIANT>args={}){
    VARIANT result;VariantInit(&result);HRESULT hr=E_POINTER;
    if(o){DISPID id;auto member=const_cast<LPOLESTR>(name);hr=o->GetIDsOfNames(IID_NULL,&member,1,LOCALE_USER_DEFAULT,&id);
        if(SUCCEEDED(hr)){std::reverse(args.begin(),args.end());DISPID put=DISPID_PROPERTYPUT;DISPPARAMS p{args.data(),nullptr,UINT(args.size()),0};
            if(flags&DISPATCH_PROPERTYPUT){p.rgdispidNamedArgs=&put;p.cNamedArgs=1;}hr=o->Invoke(id,IID_NULL,LOCALE_USER_DEFAULT,flags,&p,&result,nullptr,nullptr);}}
    for(auto&v:args)VariantClear(&v);
    if(FAILED(hr)) {VariantClear(&result);throw QString("Outlook operation failed (0x%1). Check Outlook security prompts and profile availability.").arg(quint32(hr),8,16,QChar('0'));}return result;
}
static ComPtr<IDispatch> object(IDispatch*o,const wchar_t*name,std::vector<VARIANT>args={}){auto v=call(o,name,DISPATCH_METHOD|DISPATCH_PROPERTYGET,std::move(args));ComPtr<IDispatch>r;if(v.vt==VT_DISPATCH)r=v.pdispVal;VariantClear(&v);return r;}
static QString string(IDispatch*o,const wchar_t*name){auto v=call(o,name,DISPATCH_PROPERTYGET);QString s=v.vt==VT_BSTR?QString::fromWCharArray(v.bstrVal):QString();VariantClear(&v);return s;}
static long integer(IDispatch*o,const wchar_t*name){auto v=call(o,name,DISPATCH_PROPERTYGET);long n=v.vt==VT_I4?v.lVal:0;VariantClear(&v);return n;}
static void set(IDispatch*o,const wchar_t*name,const QString&s){auto v=call(o,name,DISPATCH_PROPERTYPUT,{text(s)});VariantClear(&v);}
static bool writable(IDispatch*folder){
    try {auto accessor=object(folder,L"PropertyAccessor");auto v=call(accessor.Get(),L"GetProperty",DISPATCH_METHOD,{text("http://schemas.microsoft.com/mapi/proptag/0x0FF40003")});
        bool allowed=v.vt==VT_I4&&(v.lVal&0x10);VariantClear(&v);return allowed;
    }catch(const QString&){return false;}
}
static QJsonObject contact(IDispatch*c,const QString&book){
    QString name=string(c,L"FullName"),company=string(c,L"CompanyName"),email=string(c,L"Email1Address"),office=string(c,L"BusinessTelephoneNumber"),cell=string(c,L"MobileTelephoneNumber");
    QJsonArray emails,phones;for(const auto *property:{L"Email1Address",L"Email2Address",L"Email3Address"}){auto s=string(c,property);if(s.contains('@'))emails<<s;}
    if(!email.contains('@'))email=emails.isEmpty()?QString():emails.first().toString();if(!office.isEmpty())phones<<office;if(!cell.isEmpty())phones<<cell;
    return {{"id",string(c,L"EntryID")},{"book",book},{"name",name},{"company",company},{"emails",emails},{"phones",phones},
        {"fields",QJsonObject{{"name",name},{"company",company},{"email",email},{"office_phone",office},{"cell_phone",cell},{"role",string(c,L"JobTitle")}}}};
}
static void applyContact(IDispatch *item,const QJsonObject &c){
    auto f=c["fields"].toObject();set(item,L"FullName",c["name"].toString());set(item,L"CompanyName",c["company"].toString());set(item,L"Email1Address",f["email"].toString());
    set(item,L"BusinessTelephoneNumber",f["office_phone"].toString());set(item,L"MobileTelephoneNumber",f["cell_phone"].toString());set(item,L"JobTitle",f["role"].toString());
}
static void folders(IDispatch*folder,QJsonArray&books,QJsonArray&contacts){
    if(integer(folder,L"DefaultItemType")==2){QString id=string(folder,L"StoreID")+"\n"+string(folder,L"EntryID");books<<QJsonObject{{"id",id},{"name",string(folder,L"FolderPath")},{"writable",writable(folder)}};
        auto items=object(folder,L"Items");int count=integer(items.Get(),L"Count");for(int i=1;i<=count;++i){auto item=object(items.Get(),L"Item",{number(i)});if(integer(item.Get(),L"Class")==40)contacts<<contact(item.Get(),id);}}
    auto children=object(folder,L"Folders");int count=integer(children.Get(),L"Count");for(int i=1;i<=count;++i){auto child=object(children.Get(),L"Item",{number(i)});folders(child.Get(),books,contacts);}
}
int main(int argc,char**argv){QCoreApplication app(argc,argv);QFile input;input.open(stdin,QIODevice::ReadOnly);auto request=QJsonDocument::fromJson(input.readAll()).object();QJsonObject result{{"version",1}};
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    try {
        if(FAILED(initialized))throw QString("Cannot initialize Outlook automation.");
        if(request["version"].toInt()!=1)throw QString("Invalid contact helper version.");
        CLSID clsid;HRESULT hr=CLSIDFromProgID(L"Outlook.Application",&clsid);if(FAILED(hr))throw QString("Classic Outlook is not installed.");
        ComPtr<IDispatch>outlook;hr=CoCreateInstance(clsid,nullptr,CLSCTX_LOCAL_SERVER,IID_IDispatch,reinterpret_cast<void**>(outlook.GetAddressOf()));if(FAILED(hr))throw QString("Cannot open classic Outlook.");
        auto session=object(outlook.Get(),L"GetNamespace",{text("MAPI")});QString action=request["action"].toString();
        if(action=="snapshot"){
            QJsonArray books,contacts;auto roots=object(session.Get(),L"Folders");int count=integer(roots.Get(),L"Count");
            for(int i=1;i<=count;++i){auto root=object(roots.Get(),L"Item",{number(i)});folders(root.Get(),books,contacts);}
            auto defaultFolder=object(session.Get(),L"GetDefaultFolder",{number(10)});
            result["defaultExportBookId"]=string(defaultFolder.Get(),L"StoreID")+"\n"+string(defaultFolder.Get(),L"EntryID");
            result["account"]=string(session.Get(),L"CurrentProfileName");result["books"]=books;result["contacts"]=contacts;
        }
        else if(action=="create"){
            auto ids=request["book"].toString().split('\n');if(ids.size()!=2)throw QString("Invalid Outlook export folder.");
            auto folder=object(session.Get(),L"GetFolderFromID",{text(ids[1]),text(ids[0])});if(integer(folder.Get(),L"DefaultItemType")!=2)throw QString("Choose a contact folder.");
            if(!writable(folder.Get()))throw QString("The selected Outlook folder does not permit creating contacts.");
            auto items=object(folder.Get(),L"Items");auto item=object(items.Get(),L"Add",{text("IPM.Contact")});auto c=request["contact"].toObject();applyContact(item.Get(),c);
            auto v=call(item.Get(),L"Save",DISPATCH_METHOD);VariantClear(&v);result["contact"]=contact(item.Get(),request["book"].toString());
        }else if(action=="update"){
            auto ids=request["book"].toString().split('\n');if(ids.size()!=2)throw QString("Invalid Outlook contact folder.");
            auto folder=object(session.Get(),L"GetFolderFromID",{text(ids[1]),text(ids[0])});
            if(integer(folder.Get(),L"DefaultItemType")!=2||!writable(folder.Get()))throw QString("The selected Outlook contact folder is not writable.");
            auto existing=request["existing"].toObject();auto item=object(session.Get(),L"GetItemFromID",{text(existing["id"].toString()),text(ids[0])});
            if(!item.Get()||integer(item.Get(),L"Class")!=40)throw QString("The selected Outlook contact no longer exists.");
            applyContact(item.Get(),request["contact"].toObject());auto v=call(item.Get(),L"Save",DISPATCH_METHOD);VariantClear(&v);
            result["contact"]=contact(item.Get(),request["book"].toString());
        }else throw QString("Unsupported Outlook contact operation.");
    }catch(const QString&error){result["error"]=error;}
    if(SUCCEEDED(initialized))CoUninitialize();auto bytes=QJsonDocument(result).toJson(QJsonDocument::Compact);std::fwrite(bytes.constData(),1,size_t(bytes.size()),stdout);return 0;
}
