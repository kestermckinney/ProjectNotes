// SPDX-License-Identifier: GPL-3.0-only
#include "VCard.h"
#include <QRegularExpression>
namespace PN::Contacts {
static QString unescape(QString s) {
    QString out; bool escape=false;
    for(auto ch:s) {if(escape){out+=(ch=='n'||ch=='N')?QChar(' '):ch;escape=false;}else if(ch=='\\')escape=true;else out+=ch;}
    return out.trimmed();
}
static QStringList split(QString s) {
    QStringList out; QString part;bool escape=false;
    for(auto ch:s){if(ch==';'&&!escape){out<<unescape(part);part.clear();}else part+=ch;
        if(ch=='\\'&&!escape)escape=true;else escape=false;}
    out<<unescape(part);return out;
}
Contact parseVCard(const QString &text,const QString&id,const QString&book) {
    Contact c;c.id=id;c.book=book;
    QString unfolded=text;unfolded.replace(QRegularExpression("\\r?\\n[ \\t]"),"");
    QString structured;int emailRank=-1,officeRank=-1;
    for(const auto &line:unfolded.split(QRegularExpression("\\r?\\n"))) {
        int colon=line.indexOf(':');if(colon<0)continue;
        auto left=line.left(colon).toUpper();auto key=left.section(';',0,0).section('.',-1);
        QString raw=line.mid(colon+1),value=unescape(raw);
        if(key=="FN"){c.name=value;c.fields["name"]=value;}
        if(key=="N")structured=raw;
        if(key=="ORG"){c.company=split(raw).value(0);c.fields["company"]=c.company;}
        if(key=="TITLE")c.fields["role"]=value;
        if(key=="EMAIL"){
            if(!value.isEmpty())c.emails<<value;
            int rank=left.contains("WORK")?2:left.contains("PREF")?1:0;
            if(rank>emailRank){emailRank=rank;c.fields["email"]=value;}
        }
        if(key=="TEL"){
            if(!value.isEmpty())c.phones<<value;
            if(left.contains("CELL")||left.contains("MOBILE"))c.fields["cell_phone"]=value;
            else {int rank=left.contains("WORK")?1:0;if(rank>officeRank){officeRank=rank;c.fields["office_phone"]=value;}}
        }
    }
    if(c.name.isEmpty()){auto n=split(structured);c.name=(n.value(1)+" "+n.value(0)).simplified();c.fields["name"]=c.name;}
    return c;
}
static QString escaped(QString s){s.replace("\\","\\\\");s.replace("\r\n","\\n");s.replace('\n',"\\n");s.remove('\r');s.replace(";","\\;");s.replace(",","\\,");return s;}
QByteArray writeVCard(const Contact &c,const QString&uid) {
    QStringList lines={"BEGIN:VCARD","VERSION:3.0","UID:"+escaped(uid),"FN:"+escaped(c.name),"N:;"+escaped(c.name)+";;;"};
    for(const auto &pair:QList<QPair<QString,QString>>{{"ORG",c.company},{"TITLE",c.fields.value("role").toString()},
        {"EMAIL;TYPE=WORK",c.fields.value("email").toString()},{"TEL;TYPE=WORK",c.fields.value("office_phone").toString()},
        {"TEL;TYPE=CELL",c.fields.value("cell_phone").toString()}}) if(!pair.second.isEmpty())lines<<pair.first+":"+escaped(pair.second);
    lines<<"PRODID:-//Project Notes//Native Contacts//EN"<<"END:VCARD";
    QByteArray out;
    for(const auto&line:lines){QByteArray bytes=line.toUtf8();while(bytes.size()>75){int n=75;while(n>0&&(uchar(bytes[n])&0xc0)==0x80)--n;out+=bytes.left(n)+"\r\n ";bytes=bytes.mid(n);}out+=bytes+"\r\n";}
    return out;
}
}
