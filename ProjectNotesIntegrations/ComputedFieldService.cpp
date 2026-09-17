// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ComputedFieldService.h"

#include <QRunnable>

namespace PN::Comm {

ComputedFieldService::ComputedFieldService(QObject *parent) : QObject(parent)
{
    m_pool.setMaxThreadCount(1);
}

ComputedFieldService::~ComputedFieldService()
{
    for (Operation &operation : m_operations)
        operation.cancelled->store(true);
    m_pool.waitForDone();
}

bool ComputedFieldService::evaluate(QUuid operationId, QList<ComputedFieldDefinition> definitions,
                                    TemplateContext context, Completion completion)
{
    if (operationId.isNull() || !completion || m_operations.contains(operationId))
        return false;
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    m_operations.insert(operationId, {cancelled, std::move(completion)});
    m_pool.start(QRunnable::create([this, operationId, definitions = std::move(definitions),
                                    context = std::move(context), cancelled] () mutable {
        ComputedFieldRunner runner;
        ComputedFieldResult result = runner.evaluate(definitions, context, cancelled.get());
        QMetaObject::invokeMethod(this, [this, operationId, result = std::move(result)]() mutable {
            const auto found = m_operations.find(operationId);
            if (found == m_operations.end())
                return;
            Operation operation = std::move(found.value());
            m_operations.erase(found);
            if (operation.cancelled->load())
                result.cancelled = true;
            operation.completion(operationId, std::move(result));
        }, Qt::QueuedConnection);
    }));
    return true;
}

void ComputedFieldService::cancel(const QUuid &operationId)
{
    const auto found = m_operations.constFind(operationId);
    if (found != m_operations.cend())
        found->cancelled->store(true);
}

} // namespace PN::Comm
