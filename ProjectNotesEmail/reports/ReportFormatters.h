#pragma once
#include <QDate>
namespace PN::Comm {
struct EarnedValueMetrics { QString bcwp,bcws,bac,actual,eac,cv,sv,pctComplete,cpi; };
QPair<QString,QString> reviewPeriodDates(const QString &,const QDate &);
EarnedValueMetrics earnedValueMetrics(const QString &,const QString &,const QString &,const QString &,const QString &);
}
