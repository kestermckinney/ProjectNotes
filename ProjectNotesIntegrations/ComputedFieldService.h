// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ComputedFieldRunner.h"

#include <QHash>
#include <QObject>
#include <QThreadPool>
#include <QUuid>

#include <atomic>
#include <functional>
#include <memory>

namespace PN::Comm {

// Owner-thread async boundary for the bounded runner. Results are always
// delivered on this QObject's thread; cancellation is safe while evaluation is
// active and destruction waits for active worker jobs to observe interruption.
class ComputedFieldService final : public QObject {
    Q_OBJECT
public:
    using Completion = std::function<void(QUuid, ComputedFieldResult)>;

    explicit ComputedFieldService(QObject *parent = nullptr);
    ~ComputedFieldService() override;

    bool evaluate(QUuid operationId, QList<ComputedFieldDefinition> definitions,
                  TemplateContext context, Completion completion);
    void cancel(const QUuid &operationId);

private:
    struct Operation { std::shared_ptr<std::atomic_bool> cancelled; Completion completion; };
    QHash<QUuid, Operation> m_operations;
    QThreadPool m_pool;
};

} // namespace PN::Comm
