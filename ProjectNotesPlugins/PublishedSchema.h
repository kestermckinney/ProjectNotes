// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace PN::Plugins {

// Maps the names plugins use (the Contact/Company vocabulary of the UI) to the
// stored table and column names, in both directions. Plugins never see the
// stored people/clients names; the database and the XML file format keep them.
//
// Frozen with apiVersion 1. See docs/QJSEnginePluginsPlan.md, "Published
// schema: Contact and Company names".
class PublishedSchema final {
public:
    static const PublishedSchema& instance();

    // Entity (table) names. Unknown names map to themselves.
    QString storedTable(const QString& entity) const;
    QString publishedEntity(const QString& table) const;
    // True when `entity` is a published entity a plugin may name.
    bool isEntity(const QString& entity) const;
    // True when `name` is a stored name that has a different published name
    // (used to reject it with a helpful message).
    bool isRetiredName(const QString& name) const;
    QStringList entities() const;

    // Column names. The mapping is the same for every table.
    QString storedColumn(const QString& column) const;
    QString publishedColumn(const QString& column) const;

    // Values of columns whose stored values use the old vocabulary.
    QString storedValue(const QString& storedColumn, const QString& value) const;
    QString publishedValue(const QString& storedColumn, const QString& value) const;

private:
    PublishedSchema();
    QHash<QString, QString> m_tableToEntity, m_entityToTable;
    QHash<QString, QString> m_storedToColumn, m_columnToStored;
    QHash<QString, QHash<QString, QString>> m_storedToValue, m_valueToStored;
    QStringList m_entities;
};

} // namespace PN::Plugins
