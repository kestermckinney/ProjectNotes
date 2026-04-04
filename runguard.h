// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef RUNGUARD_H
#define RUNGUARD_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>


class RunGuard
{

public:
    RunGuard( const QString& key );
    ~RunGuard();

    bool isAnotherRunning();
    bool tryToRun();
    void release();

private:
    const QString m_key;
    const QString m_memLockKey;
    const QString m_sharedMemKey;

    QSharedMemory m_sharedMem;
    QSystemSemaphore m_memLock;

    Q_DISABLE_COPY( RunGuard )
};


#endif // RUNGUARD_H
