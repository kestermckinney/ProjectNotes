#include "ReportFormatters.h"
#include <QRegularExpression>
#include <optional>
namespace PN::Comm {
static std::optional<double> number(const QString &text) { QString value=text; value.remove(QRegularExpression("[^0-9.\\-]")); bool ok=false; const double parsed=value.toDouble(&ok); return ok?std::optional<double>(parsed):std::nullopt; }
static QString display(const std::optional<double> &value,bool percent=false) { return value?QString::number(*value,'f',2)+(percent?QStringLiteral("%"):QString()):QStringLiteral("No Schedule"); }
QPair<QString,QString> reviewPeriodDates(const QString &period,const QDate &end) { if(!end.isValid())return{}; QDate start; if(period=="Monthly")start=end.addMonths(-1); else if(period=="Weekly")start=end.addDays(-7); else if(period=="Bi-Weekly")start=end.addDays(-14); else return{}; return {start.toString("MM/dd/yyyy"),end.toString("MM/dd/yyyy")}; }
EarnedValueMetrics earnedValueMetrics(const QString &,const QString &actual,const QString &bcwp,const QString &bcws,const QString &bac) { auto a=number(actual),ev=number(bcwp),pv=number(bcws),b=number(bac); std::optional<double> e,cv,sv,pc,cpi; if(a&&*a>0&&ev&&*ev!=0&&pv&&*pv>0&&b)e=*a+(*b-*ev)/((*ev / *a)*(*ev / *pv)); if(a&&ev&&*ev>0)cv=(*a-*ev)/ *ev*100; if(ev&&pv&&*pv>0)sv=(*ev-*pv)/ *pv*100; if(ev&&b&&*b>0)pc=*ev/ *b*100; if(ev&&a&&*a>0)cpi=*ev/ *a; return {bcwp.isEmpty()?"No Schedule":bcwp,bcws.isEmpty()?"No Schedule":bcws,bac.isEmpty()?"No Schedule":bac,actual.isEmpty()?"No Schedule":actual,display(e),display(cv,true),display(sv,true),display(pc,true),display(cpi)}; }
}
