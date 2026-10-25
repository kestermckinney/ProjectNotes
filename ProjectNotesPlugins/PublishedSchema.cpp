// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PublishedSchema.h"

namespace PN::Plugins {

const PublishedSchema& PublishedSchema::instance()
{
    static const PublishedSchema schema;
    return schema;
}

PublishedSchema::PublishedSchema()
{
    const QList<QPair<QString, QString>> tables {
        {QStringLiteral("people"), QStringLiteral("contacts")},
        {QStringLiteral("clients"), QStringLiteral("companies")},
        {QStringLiteral("project_people"), QStringLiteral("project_team")},
    };
    for (const auto& [stored, published] : tables) {
        m_tableToEntity.insert(stored, published);
        m_entityToTable.insert(published, stored);
    }

    // Every stored name containing people/person/client, from the Phase 1
    // audit of databasestructure.cpp, databaseviews.cpp and the models.
    const QList<QPair<QString, QString>> columns {
        {QStringLiteral("people_id"), QStringLiteral("contact_id")},
        {QStringLiteral("person_id"), QStringLiteral("contact_id")},
        {QStringLiteral("client_id"), QStringLiteral("company_id")},
        {QStringLiteral("client_name"), QStringLiteral("company_name")},
        {QStringLiteral("selected_person_ids_json"), QStringLiteral("selected_contact_ids_json")},
        {QStringLiteral("people_source"), QStringLiteral("contact_source")},
    };
    for (const auto& [stored, published] : columns) {
        m_storedToColumn.insert(stored, published);
        // people_id and person_id never appear in the same table, so the
        // reverse map is resolved per table by the caller when both exist.
        if (!m_columnToStored.contains(published))
            m_columnToStored.insert(published, stored);
    }

    // project_email_audiences.people_source values.
    m_storedToValue[QStringLiteral("people_source")].insert(QStringLiteral("chosen-people"), QStringLiteral("chosen-contacts"));
    m_valueToStored[QStringLiteral("people_source")].insert(QStringLiteral("chosen-contacts"), QStringLiteral("chosen-people"));

    m_entities = {
        QStringLiteral("companies"), QStringLiteral("contacts"), QStringLiteral("projects"),
        QStringLiteral("project_team"), QStringLiteral("project_locations"), QStringLiteral("project_notes"),
        QStringLiteral("meeting_attendees"), QStringLiteral("item_tracker"), QStringLiteral("item_tracker_updates"),
        QStringLiteral("status_report_items"), QStringLiteral("project_email_audiences"),
    };
}

QString PublishedSchema::storedTable(const QString& entity) const
{
    return m_entityToTable.value(entity, entity);
}

QString PublishedSchema::publishedEntity(const QString& table) const
{
    return m_tableToEntity.value(table, table);
}

bool PublishedSchema::isEntity(const QString& entity) const
{
    return m_entities.contains(entity);
}

bool PublishedSchema::isRetiredName(const QString& name) const
{
    return m_tableToEntity.contains(name) || m_storedToColumn.contains(name);
}

QStringList PublishedSchema::entities() const
{
    return m_entities;
}

QString PublishedSchema::storedColumn(const QString& column) const
{
    return m_columnToStored.value(column, column);
}

QString PublishedSchema::publishedColumn(const QString& column) const
{
    return m_storedToColumn.value(column, column);
}

QString PublishedSchema::storedValue(const QString& storedColumn, const QString& value) const
{
    const auto it = m_valueToStored.constFind(storedColumn);
    return it == m_valueToStored.constEnd() ? value : it->value(value, value);
}

QString PublishedSchema::publishedValue(const QString& storedColumn, const QString& value) const
{
    const auto it = m_storedToValue.constFind(storedColumn);
    return it == m_storedToValue.constEnd() ? value : it->value(value, value);
}

} // namespace PN::Plugins
