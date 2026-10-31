// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Round trips through the platform credential store under a throwaway service
// name. Windows Credential Manager holds 2560 bytes per entry, so the long
// secret, the size of an OAuth token response, is stored in parts.

#include "credentialstore.h"

#include <QUuid>
#include <QtTest>

class CredentialStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void roundTrips_data();
    void roundTrips();
    void shorterSecretReplacesLonger();

private:
    QString m_service;
};

void CredentialStoreTest::init()
{
    m_service = QStringLiteral("ProjectNotes test ") + QUuid::createUuid().toString(QUuid::Id128);
}

void CredentialStoreTest::cleanup()
{
    CredentialStore::remove(m_service, QStringLiteral("token"));
}

void CredentialStoreTest::roundTrips_data()
{
    QTest::addColumn<QString>("secret");
    QTest::newRow("short") << QStringLiteral("hunter2");
    QTest::newRow("one part exactly") << QString(2560, QLatin1Char('a'));
    QTest::newRow("token response") << QString(6000, QLatin1Char('t')) + QStringLiteral("é end");
}

void CredentialStoreTest::roundTrips()
{
    QFETCH(QString, secret);
    QString error;
    if (!CredentialStore::write(m_service, QStringLiteral("token"), secret, &error))
        QSKIP(qPrintable(QStringLiteral("No credential store here: ") + error));
    QCOMPARE(CredentialStore::read(m_service, QStringLiteral("token"), &error), secret);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QVERIFY(CredentialStore::remove(m_service, QStringLiteral("token"), &error));
    QCOMPARE(CredentialStore::read(m_service, QStringLiteral("token"), &error), QString());
    QVERIFY2(error.isEmpty(), qPrintable(error));
}

void CredentialStoreTest::shorterSecretReplacesLonger()
{
    QString error;
    if (!CredentialStore::write(m_service, QStringLiteral("token"), QString(8000, QLatin1Char('x')), &error))
        QSKIP(qPrintable(QStringLiteral("No credential store here: ") + error));
    QVERIFY2(CredentialStore::write(m_service, QStringLiteral("token"), QStringLiteral("short"), &error), qPrintable(error));
    QCOMPARE(CredentialStore::read(m_service, QStringLiteral("token"), &error), QStringLiteral("short"));
}

QTEST_GUILESS_MAIN(CredentialStoreTest)
#include "tst_credentialstore.moc"
