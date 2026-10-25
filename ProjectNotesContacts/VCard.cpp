// SPDX-License-Identifier: GPL-3.0-only
#include "VCard.h"
#include "ContactMatcher.h"
#include "personvcard.h"
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
    QVariantMap fields=c.fields;fields["name"]=c.name;fields["company"]=c.company;
    return writePersonVCard(fields,uid,"-//Project Notes//Native Contacts//EN");
}
QByteArray mergeVCard(const QByteArray &existing,const Contact &c,const Contact &previous) {
    QString text=QString::fromUtf8(existing);text.replace(QRegularExpression("\\r?\\n[ \\t]"),"");
    QStringList kept;QString uid;
    for(const auto &line:text.split(QRegularExpression("\\r?\\n"),Qt::SkipEmptyParts)){
        const int colon=line.indexOf(':');
        const auto left=(colon<0?line:line.left(colon)).toUpper();
        const auto key=left.section(';',0,0).section('.',-1);
        if(key=="UID")uid=line.mid(colon+1);
        if(key=="FN"||key=="N"||key=="ORG"||key=="TITLE")continue;
        const auto value=colon<0?QString():unescape(line.mid(colon+1));
        const auto oldEmail=previous.fields.value("email").toString().trimmed();
        const auto oldOffice=ContactMatcher::phone(previous.fields.value("office_phone").toString()),oldCell=ContactMatcher::phone(previous.fields.value("cell_phone").toString());
        if(key=="EMAIL"&&left.contains("WORK")&&!oldEmail.isEmpty()&&value.compare(oldEmail,Qt::CaseInsensitive)==0)continue;
        if(key=="TEL"&&left.contains("WORK")&&!oldOffice.isEmpty()&&ContactMatcher::phone(value)==oldOffice)continue;
        if(key=="TEL"&&(left.contains("CELL")||left.contains("MOBILE"))&&!oldCell.isEmpty()&&ContactMatcher::phone(value)==oldCell)continue;
        if(key!="BEGIN"&&key!="END")kept<<line;
    }
    QStringList lines={"BEGIN:VCARD"};
    if(!kept.join('\n').contains(QRegularExpression("(^|\\n)VERSION:",QRegularExpression::CaseInsensitiveOption)))lines<<"VERSION:3.0";
    lines<<kept;
    if(!uid.isEmpty()&&!lines.join('\n').contains(QRegularExpression("(^|\\n)UID:",QRegularExpression::CaseInsensitiveOption)))lines<<"UID:"+uid;
    lines<<"FN:"+escaped(c.name)<<"N:;"+escaped(c.name)+";;;";
    for(const auto &pair:QList<QPair<QString,QString>>{{"ORG",c.company},{"TITLE",c.fields.value("role").toString()},
        {"EMAIL;TYPE=WORK",c.fields.value("email").toString()},{"TEL;TYPE=WORK",c.fields.value("office_phone").toString()},
        {"TEL;TYPE=CELL",c.fields.value("cell_phone").toString()}})if(!pair.second.isEmpty())lines<<pair.first+":"+escaped(pair.second);
    lines<<"END:VCARD";return lines.join("\r\n").toUtf8()+"\r\n";
}
}
