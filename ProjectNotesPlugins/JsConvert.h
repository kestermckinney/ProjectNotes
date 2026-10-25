// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QJSEngine>
#include <QJSValue>
#include <QStringList>
#include <QVariant>

namespace PN::Plugins {

// QVariant -> JavaScript, building real arrays and plain objects all the way
// down. QJSEngine::toScriptValue() turns a QVariantList into an array-like
// wrapper that fails Array.isArray() and Function.prototype.apply(), which
// plugin code (and the prelude) rely on.
inline QJSValue toJsValue(QJSEngine* engine, const QVariant& value)
{
    if (!value.isValid())
        return QJSValue(QJSValue::UndefinedValue);
    switch (value.typeId()) {
    case QMetaType::Nullptr:
        return QJSValue(QJSValue::NullValue);
    case QMetaType::QVariantList:
    case QMetaType::QStringList: {
        const QVariantList list = value.toList();
        QJSValue array = engine->newArray(quint32(list.size()));
        for (qsizetype i = 0; i < list.size(); ++i)
            array.setProperty(quint32(i), toJsValue(engine, list.at(i)));
        return array;
    }
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash: {
        const QVariantMap map = value.toMap();
        QJSValue object = engine->newObject();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            object.setProperty(it.key(), toJsValue(engine, it.value()));
        return object;
    }
    default:
        if (value.isNull() && value.typeId() == QMetaType::QString)
            return QJSValue(QString());
        return engine->toScriptValue(value);
    }
}

} // namespace PN::Plugins
