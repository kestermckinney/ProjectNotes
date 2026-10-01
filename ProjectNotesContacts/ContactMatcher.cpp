// SPDX-License-Identifier: GPL-3.0-only
#include "ContactMatcher.h"
#include <QRegularExpression>
#include <algorithm>
namespace PN::Contacts {
QString ContactMatcher::normalize(QString s, bool company) {
    s=s.normalized(QString::NormalizationForm_KC).toCaseFolded();
    if (company) s.replace('&', " and ");
    s.replace(QRegularExpression("[^\\p{L}\\p{N}]+"), " "); s=s.simplified();
    if (company) {
        static const QStringList suffixes={"inc","incorporated","llc","ltd","limited","corp","corporation","plc","gmbh"};
        auto words=s.split(' ',Qt::SkipEmptyParts);
        while (words.size()>1 && suffixes.contains(words.last())) words.removeLast();
        s=words.join(' ');
    }
    return s;
}
QString ContactMatcher::phone(QString s) {
    s.remove(QRegularExpression("[^0-9]")); return s.size()>=7?s:QString();
}
static QString folded(QString s) {
    s=s.normalized(QString::NormalizationForm_D);
    s.remove(QRegularExpression("\\p{M}")); return s;
}
double ContactMatcher::similarity(const QString &a, const QString &b) {
    if (a.isEmpty() || b.isEmpty()) return 0;
    if (a==b) return 1;
    QVector<int> row(b.size()+1); for(int j=0;j<=b.size();++j) row[j]=j;
    for(int i=1;i<=a.size();++i) {
        int prev=row[0]; row[0]=i;
        for(int j=1;j<=b.size();++j) {
            int old=row[j]; row[j]=std::min({row[j]+1,row[j-1]+1,prev+(a[i-1]!=b[j-1])}); prev=old;
        }
    }
    return 1.0-double(row.last())/std::max(a.size(),b.size());
}
static double nameScore(const QString &a,const QString &b) {
    auto x=ContactMatcher::normalize(a), y=ContactMatcher::normalize(b);
    auto xs=x.split(' ',Qt::SkipEmptyParts), ys=y.split(' ',Qt::SkipEmptyParts);
    std::sort(xs.begin(),xs.end()); std::sort(ys.begin(),ys.end());
    return std::max({ContactMatcher::similarity(x,y),ContactMatcher::similarity(xs.join(' '),ys.join(' ')),
                     ContactMatcher::similarity(folded(xs.join(' ')),folded(ys.join(' ')))});
}
static Match finish(QList<Candidate> cs) {
    std::sort(cs.begin(),cs.end(),[](const Candidate&a,const Candidate&b){return a.score==b.score?a.id<b.id:a.score>b.score;});
    Match m; m.candidates=cs;
    if (!cs.isEmpty()) {
        if(cs.first().strong && (cs.size()==1 || cs.first().score-cs[1].score>=.08)) m.id=cs.first().id;
        else m.review=true;
    }
    return m;
}
// Keep the suffix in the full name: a matching base identifies a review
// candidate, not proof that two branches are the same company record.
static QString companyBase(QString name) {
    name=name.normalized(QString::NormalizationForm_KC);
    static const QRegularExpression separator("\\s+[-\\x{2013}\\x{2014}]\\s+");
    const auto match=separator.match(name);
    if(!match.hasMatch() || name.mid(match.capturedEnd()).trimmed().isEmpty()) return {};
    const auto base=ContactMatcher::normalize(name.left(match.capturedStart()),true);
    return base.size()>=8 && base.split(' ',Qt::SkipEmptyParts).size()>=2?base:QString();
}
Match ContactMatcher::company(const QString &name,const QList<Client>& clients) {
    QList<Candidate> cs; const auto n=normalize(name,true); if(n.isEmpty()) return {};
    const auto base=companyBase(name);
    for(const auto &c:clients) {
        if(c.deleted) continue;
        auto other=normalize(c.name,true); double score=similarity(n,other);
        const auto otherBase=companyBase(c.name);
        const bool branchMatch=n!=other && ((!base.isEmpty() && base==other) || (!otherBase.isEmpty() && otherBase==n));
        if(branchMatch) score=std::max(score,.90);
        if(score>=.85) cs.append({c.id,c.name,branchMatch?"Company base matches; branch/location differs":"Company name similarity",score,n==other && n.size()>=4});
    }
    return finish(cs);
}
Match ContactMatcher::person(const Contact &c,const QString&clientId,const QList<Person>& people,const QStringList&rejected) {
    QList<Candidate> cs;
    for(const auto &p:people) {
        if(p.deleted || rejected.contains(p.id)) continue;
        double score=nameScore(c.name,p.name); bool email=false,telephone=false;
        QString pe=p.fields.value("email").toString().trimmed().toCaseFolded();
        for(const auto&e:c.emails) if(!pe.isEmpty() && e.trimmed().toCaseFolded()==pe) email=true;
        for(const auto &v:c.phones) {
            auto ph=phone(v); if(ph.isEmpty()) continue;
            if(ph==phone(p.fields.value("office_phone").toString()) || ph==phone(p.fields.value("cell_phone").toString())) telephone=true;
        }
        bool differentEmail=!pe.isEmpty() && !c.emails.isEmpty() && !email;
        bool differentCompany=(!clientId.isEmpty() && !p.clientId.isEmpty() && clientId!=p.clientId) ||
            (clientId.isEmpty() && !c.company.isEmpty() && !p.company.isEmpty() && normalize(c.company,true)!=normalize(p.company,true));
        bool shortName=normalize(c.name).size()<6 || normalize(p.name).split(' ').size()<2;
        // A complete Thunderbird card can have no usable email or phone. Treat
        // identical visible fields as stronger evidence than a fuzzy candidate.
        const auto sameText=[](const QString &a,const QString &b){return a.simplified().toCaseFolded()==b.simplified().toCaseFolded();};
        bool exactProfile=sameText(c.name,p.name) && !shortName && !differentCompany && !differentEmail &&
            !c.company.trimmed().isEmpty() && sameText(c.company,p.company) &&
            c.fields.contains("email") && c.fields.contains("office_phone") &&
            c.fields.contains("cell_phone") && c.fields.contains("role") &&
            sameText(c.fields.value("email").toString(),p.fields.value("email").toString()) &&
            sameText(c.fields.value("office_phone").toString(),p.fields.value("office_phone").toString()) &&
            sameText(c.fields.value("cell_phone").toString(),p.fields.value("cell_phone").toString()) &&
            sameText(c.fields.value("role").toString(),p.fields.value("role").toString());
        bool strong=!differentCompany && !shortName && ((email && score>=.80) ||
                    (!differentEmail && score>=.92 && telephone && !clientId.isEmpty() && clientId==p.clientId) || exactProfile);
        if(score>=.80 || email || telephone) {
            QString reason=exactProfile?"Exact contact fields":email?"Exact email":telephone?"Exact phone":"Name similarity";
            if(differentCompany || differentEmail) reason+="; conflicting identity fields";
            cs.append({p.id,p.name+" — "+p.company+" — "+pe,reason,exactProfile?1.1:(strong?1.0:score),strong});
        }
    }
    return finish(cs);
}
}
