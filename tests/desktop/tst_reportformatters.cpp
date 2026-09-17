#include "ProjectNotesEmail/reports/ReportFormatters.h"
#include <QtTest/QtTest>
using namespace PN::Comm;
class ReportFormattersTest: public QObject {
    Q_OBJECT
private slots:
    void periods();
    void metrics();
    void pythonFloatRoundingBoundaries_data();
    void pythonFloatRoundingBoundaries();
};
void ReportFormattersTest::periods(){
    const auto monthlyMonthEnd = reviewPeriodDates("Monthly", QDate(2026, 3, 31));
    QCOMPARE(monthlyMonthEnd.first, QString("02/28/2026"));
    QCOMPARE(monthlyMonthEnd.second, QString("03/31/2026"));
    QCOMPARE(reviewPeriodDates("Monthly", QDate(2024, 3, 31)).first, QString("02/29/2024"));
    QCOMPARE(reviewPeriodDates("Weekly", QDate(2026, 9, 15)).first, QString("09/08/2026"));
    QCOMPARE(reviewPeriodDates("Bi-Weekly", QDate(2026, 9, 15)).first, QString("09/01/2026"));
    QVERIFY(reviewPeriodDates("None", QDate(2026, 9, 15)).first.isEmpty());
    QVERIFY(reviewPeriodDates("Unexpected", QDate(2026, 9, 15)).first.isEmpty());
    QVERIFY(reviewPeriodDates("Weekly", {}).first.isEmpty());
}
void ReportFormattersTest::metrics(){
    const auto normal = earnedValueMetrics("", "$25.00", "$30.00", "$35.00", "$100.00");
    QCOMPARE(normal.cv, QString("-16.67%"));
    QCOMPARE(normal.sv, QString("-14.29%"));
    QCOMPARE(normal.cpi, QString("1.20"));
    QVERIFY(normal.eac != QString("No Schedule"));

    const auto zeroActual = earnedValueMetrics("", "0", "30", "35", "100");
    QCOMPARE(zeroActual.eac, QString("No Schedule"));
    QCOMPARE(zeroActual.cv, QString("-100.00%"));
    QCOMPARE(zeroActual.sv, QString("-14.29%"));
    QCOMPARE(zeroActual.pctComplete, QString("30.00%"));
    QCOMPARE(zeroActual.cpi, QString("No Schedule"));

    const auto negativeActual = earnedValueMetrics("", "-25", "30", "35", "100");
    QCOMPARE(negativeActual.eac, QString("No Schedule"));
    QCOMPARE(negativeActual.cv, QString("-183.33%"));
    QCOMPARE(negativeActual.cpi, QString("No Schedule"));

    const auto zeroEarnedValue = earnedValueMetrics("", "25", "0", "35", "100");
    QCOMPARE(zeroEarnedValue.eac, QString("No Schedule"));
    QCOMPARE(zeroEarnedValue.cv, QString("No Schedule"));
    QCOMPARE(zeroEarnedValue.sv, QString("-100.00%"));
    QCOMPARE(zeroEarnedValue.pctComplete, QString("0.00%"));
    QCOMPARE(zeroEarnedValue.cpi, QString("0.00"));

    const auto empty = earnedValueMetrics("", "", "", "", " ");
    QCOMPARE(empty.eac, QString("No Schedule"));
    QCOMPARE(empty.actual, QString("No Schedule"));
    QCOMPARE(empty.bac, QString(" "));
    const auto malformed = earnedValueMetrics("", "$--", "not-a-number", "", "");
    QCOMPARE(malformed.eac, QString("No Schedule"));
    QCOMPARE(malformed.cv, QString("No Schedule"));
}
void ReportFormattersTest::pythonFloatRoundingBoundaries_data(){ QTest::addColumn<QString>("earnedValue"); QTest::addColumn<QString>("expected"); QTest::newRow("2.675") << QString("2.675") << QString("2.67"); QTest::newRow("2.685") << QString("2.685") << QString("2.69"); QTest::newRow("2.695") << QString("2.695") << QString("2.69"); QTest::newRow("negative-2.675") << QString("-2.675") << QString("-2.67"); }
void ReportFormattersTest::pythonFloatRoundingBoundaries(){ QFETCH(QString,earnedValue); QFETCH(QString,expected); QCOMPARE(earnedValueMetrics("","1",earnedValue,"","" ).cpi,expected); }
QTEST_GUILESS_MAIN(ReportFormattersTest)
#include "tst_reportformatters.moc"
