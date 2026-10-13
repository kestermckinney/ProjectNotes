// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PluginManifest.h"
#include "PluginTypes.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QVariant>

#include <functional>
#include <memory>

class DatabaseObjects;
class SqlQueryModel;
struct RecordTable;

namespace PN::Plugins {

// The plugin `data` API. Lives on the plugin's thread and owns that plugin's
// own database connection for the plugin's lifetime. Every write goes through
// SqlQueryModel::importRecordRow() (the XML import rules) and every read
// through RecordTreeExporter, so plugins see the same normalization as XML.
//
// Values cross the boundary as QVariant (maps, lists, strings, numbers, bools,
// null) using published names; see the plugin API reference.
class DataService final {
public:
    using RowChanged = std::function<void(const QString& storedTable, const QString& key, int operation)>;

    explicit DataService(const PluginManifest& manifest);
    ~DataService();

    bool open(const QString& databasePath, PluginError* error);
    void close();
    DatabaseObjects* database() const { return m_dbo.get(); }

    // Row changes made through this connection, delivered after the write (or
    // after commit for a transaction). Operations use KeyColumnChange values.
    void setRowChangedCallback(RowChanged callback) { m_rowChanged = std::move(callback); }

    QVariant entities(PluginError* error);
    QVariant schema(const QString& entity, PluginError* error);
    QVariant get(const QString& entity, const QVariant& keyOrWhere, const QVariantMap& options, PluginError* error);
    QVariant list(const QString& entity, const QVariantMap& query, PluginError* error);
    QVariant count(const QString& entity, const QVariantMap& where, PluginError* error);
    QVariant search(const QString& text, const QVariantMap& options, PluginError* error);
    QVariant save(const QString& entity, const QVariantMap& record, PluginError* error);
    QVariant saveAll(const QString& entity, const QVariantList& records, PluginError* error);
    QVariant remove(const QString& entity, const QVariant& keyOrRecord, PluginError* error);
    bool begin(PluginError* error);
    bool commit(PluginError* error);
    void rollback();
    bool inTransaction() const;
    QVariant exportXml(const QString& entity, const QString& key, const QVariantMap& options, PluginError* error);
    QVariant importXml(const QString& xml, PluginError* error);

private:
    struct Column;
    struct Entity;

    const Entity* entity(const QString& published, bool write, PluginError* error);
    SqlQueryModel* model(const QString& storedTable);
    bool buildWhere(const Entity& e, const QVariantMap& where, QStringList* clauses,
                    QVariantList* bindings, PluginError* error);
    bool selectRows(const Entity& e, const QVariantMap& query, QList<QVector<QVariant>>* rows,
                    PluginError* error);
    QVariantList toRecords(const RecordTable& table, const QVariantMap& query, PluginError* error);
    QVariant convertOut(const Column& column, const QVariant& value, const QString& lookup) const;
    bool convertIn(const Column& column, const QVariant& value, QVariant* text, PluginError* error);
    QVariant storageValue(const Column& column, const QVariant& value, PluginError* error);
    bool includeSet(const Entity& e, const QVariant& include, QSet<QString>* storedTables,
                    QHash<QString, QVariantMap>* childQueries, PluginError* error);
    QVariant saveOne(const Entity& e, const QVariantMap& record, PluginError* error);
    void deliverChanges();

    PluginManifest m_manifest;
    std::unique_ptr<DatabaseObjects> m_dbo;
    QString m_connectionName;
    QHash<QString, std::shared_ptr<SqlQueryModel>> m_models;
    QHash<QString, std::shared_ptr<Entity>> m_entities;
    RowChanged m_rowChanged;
    struct Change { QString table; QString key; int operation; };
    QList<Change> m_pendingChanges;
};

} // namespace PN::Plugins
