// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ComputedFieldRunner.h"

#include <QJSEngine>
#include <QJSValue>
#include <QRegularExpression>
#include <QSet>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <thread>

namespace PN::Comm {
namespace {

bool isSafePath(const QString &path)
{
    if (path.isEmpty()) return false;
    for (const QString &segment : path.split('.')) {
        if (segment.isEmpty() || segment == QStringLiteral("__proto__")
            || segment == QStringLiteral("prototype") || segment == QStringLiteral("constructor")
            || (!segment.front().isLetter() && segment.front() != '_')) return false;
        for (qsizetype i = 1; i < segment.size(); ++i)
            if (!segment.at(i).isLetterOrNumber() && segment.at(i) != '_') return false;
    }
    return true;
}

bool isScalar(const QJSValue &value)
{
    return value.isString() || value.isNumber() || value.isBool() || value.isNull();
}

bool installReadOnlyContext(QJSEngine &engine, const TemplateContext &context)
{
    qsizetype contextCharacters = 0;
    QJSValue jsContext = engine.newObject();
    for (auto value = context.values.cbegin(); value != context.values.cend(); ++value) {
        if (!isSafePath(value.key()))
            return false;
        contextCharacters += value.key().toUtf8().size() + value.value().toUtf8().size();
        if (contextCharacters > ComputedFieldRunner::maximumContextCharacters)
            return false;

        // Retain the literal-token spelling for existing expressions while
        // also publishing the documented structured `context.project.name`
        // form.  Both are engine-local copies, never application objects.
        jsContext.setProperty(value.key(), value.value());
        QJSValue destination = jsContext;
        const QStringList segments = value.key().split(u'.');
        for (qsizetype index = 0; index + 1 < segments.size(); ++index) {
            QJSValue child = destination.property(segments.at(index));
            if (!child.isObject()) {
                child = engine.newObject();
                destination.setProperty(segments.at(index), child);
            }
            destination = child;
        }
        destination.setProperty(segments.constLast(), value.value());
    }
    engine.globalObject().setProperty(QStringLiteral("context"), jsContext);
    // The data is already copied into this fresh engine. Freeze the complete
    // object graph as well so one computed expression cannot mutate values it
    // observes while evaluating.
    const QJSValue frozen = engine.evaluate(QStringLiteral(
        "(function freeze(value) {"
        " if (!value || typeof value !== 'object' || Object.isFrozen(value)) return value;"
        " Object.getOwnPropertyNames(value).forEach(function(key) { freeze(value[key]); });"
        " return Object.freeze(value);"
        "})(context);"));
    return !frozen.isError();
}

QString evaluationSource(const QString &definition)
{
    static const QRegularExpression computeFunction(
        QStringLiteral("^\\s*function\\s+compute\\s*\\("));
    if (computeFunction.match(definition).hasMatch()) {
        // Helpers intentionally start empty: this runner does not grant a
        // hidden application API merely because a template accepts the richer
        // documented function form. Future pure helpers can be added here as
        // explicit scalar functions with their own contracts.
        return QStringLiteral("'use strict'; (function(context) {"
                              "const helpers = Object.freeze({}); %1;"
                              "if (typeof compute !== 'function') throw new Error('compute missing');"
                              "return compute(context, helpers); })(context)")
            .arg(definition);
    }
    return QStringLiteral("'use strict'; (function(context) { return (%1); })(context)")
        .arg(definition);
}

} // namespace

ComputedFieldResult ComputedFieldRunner::evaluate(const QList<ComputedFieldDefinition> &definitions,
                                                   const TemplateContext &context,
                                                   const std::atomic_bool *cancelled) const
{
    ComputedFieldResult result;
    const auto totalDeadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(maximumTotalMilliseconds);
    if (definitions.size() > maximumDefinitions) {
        result.validation.addError("computed-field-limit", "definitions");
        return result;
    }

    // A field is a named output contract. Letting a later definition silently
    // replace an earlier one makes saved templates order-dependent, so reject
    // duplicate enabled names before executing any untrusted expression.
    QSet<QString> enabledPaths;
    for (const ComputedFieldDefinition &definition : definitions) {
        if (!definition.enabled) continue;
        if (cancelled && cancelled->load()) { result.cancelled = true; return result; }
        if (std::chrono::steady_clock::now() >= totalDeadline) {
            result.validation.addError("computed-field-total-timeout", definition.path);
            return result;
        }
        if (!isSafePath(definition.path) || definition.expression.size() > maximumExpressionCharacters) {
            result.validation.addError("computed-field-invalid", definition.path);
            continue;
        }
        if (enabledPaths.contains(definition.path)) {
            result.validation.addError("computed-field-duplicate", definition.path);
            continue;
        }
        enabledPaths.insert(definition.path);

        QJSEngine engine;
        if (!installReadOnlyContext(engine, context)) {
            result.validation.addError("computed-field-context-invalid", definition.path);
            continue;
        }
        // No QObject, network object, filesystem API, or host callback is exposed.
        engine.globalObject().setProperty(QStringLiteral("console"), QJSValue::UndefinedValue);

        std::atomic_bool finished = false;
        std::thread interrupter([&engine, &finished, cancelled, totalDeadline] {
            const auto fieldDeadline = std::chrono::steady_clock::now()
                + std::chrono::milliseconds(ComputedFieldRunner::maximumFieldMilliseconds);
            const auto deadline = std::min(fieldDeadline, totalDeadline);
            while (!finished.load() && std::chrono::steady_clock::now() < deadline) {
                if (cancelled && cancelled->load()) { engine.setInterrupted(true); return; }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            if (!finished.load()) engine.setInterrupted(true);
        });

        const QJSValue value = engine.evaluate(evaluationSource(definition.expression), definition.path);
        finished.store(true);
        interrupter.join();

        if (cancelled && cancelled->load()) { result.cancelled = true; return result; }
        if (engine.isInterrupted()) {
            result.validation.addError("computed-field-timeout", definition.path);
            continue;
        }
        if (value.isError()) {
            result.validation.addError("computed-field-runtime", definition.path,
                                       value.toString());
            continue;
        }
        if (!isScalar(value)) {
            result.validation.addError("computed-field-non-scalar", definition.path);
            continue;
        }
        const QString output = value.isNull() ? QString() : value.toString();
        if (output.toUtf8().size() > maximumOutputCharacters) {
            result.validation.addError("computed-field-output-limit", definition.path);
            continue;
        }
        result.values.insert(definition.path, output);
    }
    return result;
}

} // namespace PN::Comm
