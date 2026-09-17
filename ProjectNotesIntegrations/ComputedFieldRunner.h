// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "TemplateTypes.h"

#include <atomic>

namespace PN::Comm {

struct ComputedFieldDefinition {
    QString path;
    QString expression;
    bool enabled = false;
};

struct ComputedFieldResult {
    QHash<QString, QString> values;
    ValidationResult validation;
    bool cancelled = false;
};

// Runs each enabled expression in a fresh engine. The only application data made
// visible to JavaScript is the scalar `context` object. Callers own cancellation.
class ComputedFieldRunner final {
public:
    static constexpr int maximumDefinitions = 64;
    static constexpr int maximumExpressionCharacters = 64 * 1024;
    // Limits are bytes for the serialized UTF-8 context/output contract.
    static constexpr int maximumContextCharacters = 1024 * 1024;
    static constexpr int maximumOutputCharacters = 64 * 1024;
    static constexpr int maximumFieldMilliseconds = 250;
    static constexpr int maximumTotalMilliseconds = 2000;

    ComputedFieldResult evaluate(const QList<ComputedFieldDefinition> &definitions,
                                 const TemplateContext &context,
                                 const std::atomic_bool *cancelled = nullptr) const;
};

} // namespace PN::Comm
