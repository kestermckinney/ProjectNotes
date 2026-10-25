// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DataService.h"
#include "PublishedSchema.h"

#include "databaseobjects.h"
#include "recordtree.h"
#include "sqlquerymodel.h"

#include <QDateTime>
#include <QDomDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTimeZone>
#include <QUuid>

#include <algorithm>

namespace PN::Plugins {

struct DataService::Column {
    int index = 0;
    QString stored;
    QString published;
    QString displayName;
    int type = 0;                 // SqlQueryModel::DBColumnType
    bool isKey = false;
    bool required = false;
    bool readOnly = false;
    bool unique = false;
    bool searchable = false;
    QString lookupTable, lookupFk, lookupValue;
    QStringList choices;

    bool isRef() const { return !lookupTable.isEmpty(); }
    QString typeName() const
    {
        if (isRef()) return QStringLiteral("ref");
        switch (type) {
        case SqlQueryModel::DBDate: return QStringLiteral("date");
        case SqlQueryModel::DBDateTime: return QStringLiteral("datetime");
        case SqlQueryModel::DBBool: return QStringLiteral("bool");
        case SqlQueryModel::DBReal:
        case SqlQueryModel::DBInteger: return QStringLiteral("number");
        case SqlQueryModel::DBUSD: return QStringLiteral("currency");
        case SqlQueryModel::DBPercent: return QStringLiteral("percent");
        case SqlQueryModel::DBHtml: return QStringLiteral("text");
        default: return QStringLiteral("string");
        }
    }
};

struct DataService::Entity {
    QString published;
    QString stored;
    QString displayName;
    QList<Column> columns;
    QHash<QString, int> byPublished;    // published column name -> index in columns
    QHash<QString, int> byStored;
    QList<QStringList> uniqueKeys;      // published names
    struct Child { QString entity; QString column; bool owned; };
    QList<Child> children;
    bool deletedFilterInView = false;
    QString orderBy;
    std::shared_ptr<SqlQueryModel> model;

    const Column* column(const QString& name) const
    {
        const auto it = byPublished.constFind(name);
        return it == byPublished.constEnd() ? nullptr : &columns.at(*it);
    }
};

namespace {

PluginError validation(const QString& message, const QString& column = {})
{
    QVariantMap details;
    if (!column.isEmpty())
        details.insert(QStringLiteral("column"), column);
    return PluginError::make(QStringLiteral("validation-failed"), message, details);
}

QString quoteIdentifier(const QString& name)
{
    return QLatin1Char('"') + QString(name).replace(QLatin1Char('"'), QStringLiteral("\"\"")) + QLatin1Char('"');
}

bool isOperatorMap(const QVariant& value)
{
    if (value.typeId() != QMetaType::QVariantMap)
        return false;
    const QVariantMap map = value.toMap();
    return !map.isEmpty() && std::all_of(map.keyBegin(), map.keyEnd(),
                                         [](const QString& k) { return k.startsWith(QLatin1Char('$')); });
}

int compareValues(const QVariant& a, const QVariant& b)
{
    if (a.isNull() || b.isNull())
        return a.isNull() == b.isNull() ? 0 : (a.isNull() ? -1 : 1);
    bool okA = false, okB = false;
    const double da = a.toDouble(&okA), db = b.toDouble(&okB);
    if (okA && okB && a.typeId() != QMetaType::QString && b.typeId() != QMetaType::QString)
        return da < db ? -1 : (da > db ? 1 : 0);
    return QString::compare(a.toString(), b.toString(), Qt::CaseInsensitive);
}

QVariant refId(const QVariant& value)
{
    if (value.typeId() == QMetaType::QVariantMap)
        return value.toMap().value(QStringLiteral("id"));
    return value;
}

// In-memory evaluation of a Where against a converted record, used for the
// `where` of an include (child rows are already loaded in one batched query).
bool matches(const QVariantMap& record, const QVariantMap& where)
{
    for (auto it = where.constBegin(); it != where.constEnd(); ++it) {
        const QVariant actual = refId(record.value(it.key()));
        const QVariant& condition = it.value();
        if (condition.typeId() == QMetaType::QVariantList) {
            const QVariantList options = condition.toList();
            if (std::none_of(options.cbegin(), options.cend(),
                             [&](const QVariant& o) { return compareValues(actual, refId(o)) == 0; }))
                return false;
        } else if (isOperatorMap(condition)) {
            const QVariantMap ops = condition.toMap();
            for (auto op = ops.constBegin(); op != ops.constEnd(); ++op) {
                const int c = compareValues(actual, op.value());
                if ((op.key() == QLatin1String("$ne") && c == 0)
                    || (op.key() == QLatin1String("$lt") && !(c < 0))
                    || (op.key() == QLatin1String("$lte") && !(c <= 0))
                    || (op.key() == QLatin1String("$gt") && !(c > 0))
                    || (op.key() == QLatin1String("$gte") && !(c >= 0))
                    || (op.key() == QLatin1String("$null") && (actual.isNull() != op.value().toBool())))
                    return false;
                if (op.key() == QLatin1String("$like")) {
                    QString pattern = QRegularExpression::escape(op.value().toString());
                    pattern.replace(QStringLiteral("%"), QStringLiteral(".*")).replace(QStringLiteral("_"), QStringLiteral("."));
                    if (!QRegularExpression(QStringLiteral("^%1$").arg(pattern), QRegularExpression::CaseInsensitiveOption)
                             .match(actual.toString()).hasMatch())
                        return false;
                }
                if (op.key() == QLatin1String("$between")) {
                    const QVariantList range = op.value().toList();
                    if (range.size() != 2 || compareValues(actual, range.at(0)) < 0 || compareValues(actual, range.at(1)) > 0)
                        return false;
                }
            }
        } else if (compareValues(actual, refId(condition)) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace

DataService::DataService(const PluginManifest& manifest)
    : m_manifest(manifest)
{
}

DataService::~DataService()
{
    close();
}

bool DataService::open(const QString& databasePath, PluginError* error)
{
    m_dbo = std::make_unique<DatabaseObjects>(nullptr);
    m_dbo->setObjectName(QStringLiteral("plugin:") + m_manifest.id);
    // A connection belongs to the thread that created it, and a plugin gets a
    // new thread every time it starts, so each start uses its own name.
    m_connectionName = QStringLiteral("plugin-%1-%2").arg(m_manifest.id, QUuid::createUuid().toString(QUuid::Id128));
    if (!m_dbo->openDatabase(databasePath, m_connectionName, false)) {
        *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("The database could not be opened."));
        m_dbo.reset();
        return false;
    }
    QObject::connect(m_dbo.get(), &DatabaseObjects::rowChanged,
                     [this](const QString& table, const QVariant& value, int operation) {
                         m_pendingChanges.append({table, value.toString(), operation});
                     });
    return true;
}

void DataService::close()
{
    if (!m_dbo)
        return;
    if (m_dbo->inBatch())
        m_dbo->rollbackBatch();
    m_entities.clear();
    m_models.clear();
    m_dbo->closeDatabase();
    m_dbo.reset();
    QSqlDatabase::removeDatabase(m_connectionName);
}

SqlQueryModel* DataService::model(const QString& storedTable)
{
    auto it = m_models.constFind(storedTable);
    if (it != m_models.constEnd())
        return it->get();
    std::shared_ptr<SqlQueryModel> created(m_dbo->createExportObject(storedTable));
    if (!created)
        return nullptr;
    m_models.insert(storedTable, created);
    return created.get();
}

const DataService::Entity* DataService::entity(const QString& published, bool write, PluginError* error)
{
    const PublishedSchema& schema = PublishedSchema::instance();
    if (!m_dbo) {
        *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("No database is open."));
        return nullptr;
    }
    if (!schema.isEntity(published)) {
        *error = schema.isRetiredName(published)
            ? validation(QStringLiteral("\"%1\" is a stored name; use \"%2\".").arg(published, schema.publishedEntity(published)))
            : PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown entity \"%1\".").arg(published));
        return nullptr;
    }
    if (write ? !m_manifest.canWrite(published) : !m_manifest.canRead(published)) {
        *error = PluginError::denied((write ? QStringLiteral("data.write:") : QStringLiteral("data.read:")) + published);
        return nullptr;
    }

    if (auto cached = m_entities.value(published))
        return cached.get();

    const QString stored = schema.storedTable(published);
    SqlQueryModel* m = model(stored);
    if (!m) {
        *error = PluginError::make(QStringLiteral("not-found"), QStringLiteral("Unknown entity \"%1\".").arg(published));
        return nullptr;
    }

    auto e = std::make_shared<Entity>();
    e->published = published;
    e->stored = stored;
    e->displayName = m->displayname();
    e->deletedFilterInView = m->getDeletedFilterInView();
    e->orderBy = m->orderBy();
    e->model = m_models.value(stored);
    for (int c = 0; c < m->columnCount(); ++c) {
        Column column;
        column.index = c;
        column.stored = m->getColumnName(c);
        column.published = schema.publishedColumn(column.stored);
        column.displayName = m->headerData(c, Qt::Horizontal).toString();
        column.type = m->getType(c);
        column.isKey = (c == 0);
        column.required = m->isRequired(c);
        column.readOnly = !m->isEditable(c);
        column.unique = m->isUniqueColumn(c);
        column.searchable = m->isSearchable(c);
        column.lookupTable = m->getLookupTable(c);
        column.lookupFk = m->getLookupFkColumnName(c);
        column.lookupValue = m->getLookupValueColumnName(c);
        if (QStringList* values = m->lookupValues(c))
            column.choices = *values;
        e->byPublished.insert(column.published, int(e->columns.size()));
        e->byStored.insert(column.stored, int(e->columns.size()));
        e->columns.append(column);
    }
    for (const QStringList& key : m->uniqueKeys()) {
        QStringList names;
        for (const QString& c : key)
            names.append(schema.publishedColumn(c));
        e->uniqueKeys.append(names);
    }
    for (int i = 0; i < m->relatedTableCount(); ++i) {
        const QStringList cols = m->relatedColumns(i);
        e->children.append({schema.publishedEntity(m->relatedTable(i)),
                            cols.isEmpty() ? QString() : schema.publishedColumn(cols.first()),
                            m->isRelationExportable(i)});
    }
    m_entities.insert(published, e);
    return e.get();
}

// ── Value conversion ─────────────────────────────────────────────────────────

QVariant DataService::convertOut(const Column& column, const QVariant& value, const QString& lookup) const
{
    if (column.isRef()) {
        const QString id = value.toString();
        if (value.isNull() || id.isEmpty())
            return QVariant::fromValue(nullptr);
        return QVariantMap {{QStringLiteral("id"), id},
                            {QStringLiteral("lookup"), lookup.isNull() ? QVariant::fromValue(nullptr) : QVariant(lookup)}};
    }
    if (value.isNull())
        return QVariant::fromValue(nullptr);

    switch (column.type) {
    case SqlQueryModel::DBDate:
    case SqlQueryModel::DBDateTime: {
        bool ok = false;
        const qlonglong secs = value.toLongLong(&ok);
        QDateTime moment;
        if (ok)
            moment = QDateTime::fromSecsSinceEpoch(secs);
        else if (!value.toString().isEmpty())
            moment = SqlQueryModel::parseDateTime(value.toString());
        if (!moment.isValid())
            return QVariant::fromValue(nullptr);
        return column.type == SqlQueryModel::DBDate ? moment.date().toString(Qt::ISODate)
                                                    : moment.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    }
    case SqlQueryModel::DBBool: {
        const QString text = value.toString().trimmed().toLower();
        return text == QLatin1String("1") || text == QLatin1String("true") || text == QLatin1String("yes");
    }
    case SqlQueryModel::DBReal:
    case SqlQueryModel::DBInteger:
    case SqlQueryModel::DBUSD:
    case SqlQueryModel::DBPercent: {
        QString text = value.toString();
        text.remove(QLatin1Char('$')).remove(QLatin1Char(',')).remove(QLatin1Char('%'));
        if (text.trimmed().isEmpty())
            return QVariant::fromValue(nullptr);
        bool ok = false;
        const double number = text.toDouble(&ok);
        return ok ? QVariant(number) : QVariant::fromValue(nullptr);
    }
    default:
        return PublishedSchema::instance().publishedValue(column.stored, value.toString());
    }
}

// Script value -> the export text form importRecordRow() parses.
bool DataService::convertIn(const Column& column, const QVariant& value, QVariant* text, PluginError* error)
{
    if (value.isNull() || !value.isValid()) {
        *text = QString();
        return true;
    }
    if (column.isRef()) {
        QString id;
        QString lookup;
        if (value.typeId() == QMetaType::QVariantMap) {
            const QVariantMap ref = value.toMap();
            id = ref.value(QStringLiteral("id")).toString();
            lookup = ref.value(QStringLiteral("lookup")).toString();
        } else {
            id = value.toString();
        }
        if (id.isEmpty() && !lookup.isEmpty()) {
            // Resolve now so an unknown display value is an error rather than
            // a silently cleared foreign key.
            DB_LOCK;
            QSqlQuery query(m_dbo->getDb());
            query.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE %3 = ? AND (deleted IS NULL OR deleted = 0)")
                              .arg(quoteIdentifier(column.lookupFk), quoteIdentifier(column.lookupTable),
                                   quoteIdentifier(column.lookupValue)));
            query.addBindValue(lookup);
            if (query.exec() && query.next())
                id = query.value(0).toString();
            DB_UNLOCK;
            if (id.isEmpty()) {
                *error = validation(QStringLiteral("No %1 matches \"%2\".")
                                        .arg(PublishedSchema::instance().publishedEntity(column.lookupTable), lookup),
                                    column.published);
                return false;
            }
        }
        *text = id;
        return true;
    }

    switch (column.type) {
    case SqlQueryModel::DBDate: {
        const QDate date = QDate::fromString(value.toString().left(10), Qt::ISODate);
        if (!date.isValid()) {
            *error = validation(QStringLiteral("\"%1\" needs a date as YYYY-MM-DD.").arg(column.published), column.published);
            return false;
        }
        *text = date.toString(QStringLiteral("MM/dd/yyyy"));
        return true;
    }
    case SqlQueryModel::DBDateTime: {
        const QDateTime moment = QDateTime::fromString(value.toString(), Qt::ISODate);
        if (!moment.isValid()) {
            *error = validation(QStringLiteral("\"%1\" needs an ISO 8601 date-time.").arg(column.published), column.published);
            return false;
        }
        *text = moment.toLocalTime().toString(QStringLiteral("MM/dd/yyyy HH:mm:ss"));
        return true;
    }
    case SqlQueryModel::DBBool:
        *text = value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
        return true;
    case SqlQueryModel::DBReal:
    case SqlQueryModel::DBInteger:
    case SqlQueryModel::DBUSD:
    case SqlQueryModel::DBPercent: {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok) {
            *error = validation(QStringLiteral("\"%1\" needs a number.").arg(column.published), column.published);
            return false;
        }
        *text = QString::number(number, 'g', 15);
        return true;
    }
    default:
        if (value.typeId() == QMetaType::QVariantMap || value.typeId() == QMetaType::QVariantList) {
            *error = validation(QStringLiteral("\"%1\" needs a string.").arg(column.published), column.published);
            return false;
        }
        *text = PublishedSchema::instance().storedValue(column.stored, value.toString());
        return true;
    }
}

// A script value as it is stored, for WHERE comparisons.
QVariant DataService::storageValue(const Column& column, const QVariant& value, PluginError* error)
{
    if (column.isRef())
        return refId(value).toString();
    switch (column.type) {
    case SqlQueryModel::DBDate: {
        const QDate date = QDate::fromString(value.toString().left(10), Qt::ISODate);
        if (!date.isValid()) {
            *error = validation(QStringLiteral("\"%1\" compares against a date as YYYY-MM-DD.").arg(column.published), column.published);
            return {};
        }
        return QDateTime(date, QTime(0, 0)).toSecsSinceEpoch();
    }
    case SqlQueryModel::DBDateTime: {
        const QDateTime moment = QDateTime::fromString(value.toString(), Qt::ISODate);
        if (!moment.isValid()) {
            *error = validation(QStringLiteral("\"%1\" compares against an ISO 8601 date-time.").arg(column.published), column.published);
            return {};
        }
        return moment.toSecsSinceEpoch();
    }
    case SqlQueryModel::DBReal:
    case SqlQueryModel::DBInteger:
    case SqlQueryModel::DBUSD:
    case SqlQueryModel::DBPercent:
        return value.toDouble();
    default:
        return PublishedSchema::instance().storedValue(column.stored, value.toString());
    }
}

// ── Queries ──────────────────────────────────────────────────────────────────

bool DataService::buildWhere(const Entity& e, const QVariantMap& where, QStringList* clauses,
                             QVariantList* bindings, PluginError* error)
{
    for (auto it = where.constBegin(); it != where.constEnd(); ++it) {
        QString name = it.key();
        if (name == QLatin1String("$key"))
            name = e.columns.first().published;
        const Column* column = e.column(name);
        if (!column) {
            *error = PublishedSchema::instance().isRetiredName(name)
                ? validation(QStringLiteral("\"%1\" is a stored name; use \"%2\".")
                                 .arg(name, PublishedSchema::instance().publishedColumn(name)), name)
                : validation(QStringLiteral("%1 has no column \"%2\".").arg(e.published, name), name);
            return false;
        }
        const QString col = QStringLiteral("pnq.") + quoteIdentifier(column->stored);
        const bool text = column->type == SqlQueryModel::DBString || column->type == SqlQueryModel::DBHtml
                       || column->type == SqlQueryModel::DBPhoneNumber;
        const QString collate = (text && !column->isRef()) ? QStringLiteral(" COLLATE NOCASE") : QString();
        const QVariant& condition = it.value();

        auto bindOne = [&](const QVariant& v, QString* placeholder) -> bool {
            const QVariant stored = storageValue(*column, v, error);
            if (error->isError())
                return false;
            bindings->append(stored);
            *placeholder = QStringLiteral("?");
            return true;
        };

        if (condition.isNull()) {
            clauses->append(QStringLiteral("(%1 IS NULL OR %1 = '')").arg(col));
        } else if (column->type == SqlQueryModel::DBBool && !isOperatorMap(condition)
                   && condition.typeId() != QMetaType::QVariantList) {
            clauses->append(condition.toBool()
                                ? QStringLiteral("(%1 = 1 OR %1 = '1' OR lower(%1) = 'true')").arg(col)
                                : QStringLiteral("(%1 IS NULL OR %1 = 0 OR %1 = '0' OR %1 = '' OR lower(%1) = 'false')").arg(col));
        } else if (condition.typeId() == QMetaType::QVariantList) {
            const QVariantList options = condition.toList();
            if (options.isEmpty()) {
                clauses->append(QStringLiteral("0"));
                continue;
            }
            QStringList placeholders;
            for (const QVariant& option : options) {
                QString p;
                if (!bindOne(option, &p))
                    return false;
                placeholders.append(p);
            }
            clauses->append(QStringLiteral("%1%2 IN (%3)").arg(col, collate, placeholders.join(QLatin1Char(','))));
        } else if (isOperatorMap(condition)) {
            const QVariantMap ops = condition.toMap();
            for (auto op = ops.constBegin(); op != ops.constEnd(); ++op) {
                static const QHash<QString, QString> comparisons {
                    {QStringLiteral("$ne"), QStringLiteral("<>")}, {QStringLiteral("$lt"), QStringLiteral("<")},
                    {QStringLiteral("$lte"), QStringLiteral("<=")}, {QStringLiteral("$gt"), QStringLiteral(">")},
                    {QStringLiteral("$gte"), QStringLiteral(">=")}};
                QString p;
                if (comparisons.contains(op.key())) {
                    if (!bindOne(op.value(), &p))
                        return false;
                    clauses->append(QStringLiteral("%1%2 %3 %4").arg(col, collate, comparisons.value(op.key()), p));
                } else if (op.key() == QLatin1String("$like")) {
                    bindings->append(op.value().toString());
                    clauses->append(QStringLiteral("%1 LIKE ?").arg(col));
                } else if (op.key() == QLatin1String("$null")) {
                    clauses->append(op.value().toBool() ? QStringLiteral("(%1 IS NULL OR %1 = '')").arg(col)
                                                        : QStringLiteral("(%1 IS NOT NULL AND %1 <> '')").arg(col));
                } else if (op.key() == QLatin1String("$between")) {
                    const QVariantList range = op.value().toList();
                    if (range.size() != 2) {
                        *error = validation(QStringLiteral("$between needs [low, high]."), name);
                        return false;
                    }
                    QString lo, hi;
                    if (!bindOne(range.at(0), &lo) || !bindOne(range.at(1), &hi))
                        return false;
                    clauses->append(QStringLiteral("%1%2 BETWEEN %3 AND %4").arg(col, collate, lo, hi));
                } else {
                    *error = validation(QStringLiteral("Unknown operator \"%1\".").arg(op.key()), name);
                    return false;
                }
            }
        } else if (column->isRef() && condition.typeId() == QMetaType::QVariantMap
                   && condition.toMap().value(QStringLiteral("id")).toString().isEmpty()) {
            bindings->append(condition.toMap().value(QStringLiteral("lookup")).toString());
            clauses->append(QStringLiteral("%1 IN (SELECT %2 FROM %3 WHERE %4 = ?)")
                                .arg(col, quoteIdentifier(column->lookupFk), quoteIdentifier(column->lookupTable),
                                     quoteIdentifier(column->lookupValue)));
        } else {
            QString p;
            if (!bindOne(condition, &p))
                return false;
            clauses->append(QStringLiteral("%1%2 = %3").arg(col, collate, p));
        }
    }
    return true;
}

bool DataService::selectRows(const Entity& e, const QVariantMap& query, QList<QVector<QVariant>>* rows,
                             PluginError* error)
{
    SqlQueryModel* m = e.model.get();
    // The model's base SQL plus its deleted-row filter (exactly what the
    // application's lists show), wrapped so filters and ordering can address
    // every column, including computed ones, by name.
    QString inner = m->BaseSQL();
    if (!e.deletedFilterInView)
        inner += QStringLiteral(" WHERE (%1.deleted IS NULL OR %1.deleted = 0)").arg(e.stored);

    QStringList clauses;
    QVariantList bindings;
    if (!buildWhere(e, query.value(QStringLiteral("where")).toMap(), &clauses, &bindings, error))
        return false;

    QString sql = QStringLiteral("SELECT * FROM (%1) AS pnq").arg(inner);
    if (!clauses.isEmpty())
        sql += QStringLiteral(" WHERE ") + clauses.join(QStringLiteral(" AND "));

    const QVariantList orderBy = query.value(QStringLiteral("orderBy")).toList();
    if (!orderBy.isEmpty()) {
        QStringList terms;
        for (const QVariant& term : orderBy) {
            QString name;
            bool descending = false;
            if (term.typeId() == QMetaType::QVariantMap) {
                name = term.toMap().value(QStringLiteral("column")).toString();
                descending = term.toMap().value(QStringLiteral("descending")).toBool();
            } else {
                name = term.toString();
            }
            const Column* column = e.column(name);
            if (!column) {
                *error = validation(QStringLiteral("%1 has no column \"%2\" to order by.").arg(e.published, name), name);
                return false;
            }
            terms.append(QStringLiteral("pnq.%1%2").arg(quoteIdentifier(column->stored),
                                                        descending ? QStringLiteral(" DESC") : QString()));
        }
        sql += QStringLiteral(" ORDER BY ") + terms.join(QStringLiteral(", "));
    } else if (!e.orderBy.isEmpty()) {
        // The model's default order names base-table columns, so order the
        // unfiltered inner query; SQLite keeps that order through the wrapper.
        sql = QStringLiteral("SELECT * FROM (%1 ORDER BY %2) AS pnq").arg(inner, e.orderBy);
        if (!clauses.isEmpty())
            sql += QStringLiteral(" WHERE ") + clauses.join(QStringLiteral(" AND "));
    }

    const int limit = query.value(QStringLiteral("limit"), -1).toInt();
    const int offset = query.value(QStringLiteral("offset"), 0).toInt();
    if (limit >= 0 || offset > 0) {
        sql += QStringLiteral(" LIMIT ? OFFSET ?");
        bindings.append(limit >= 0 ? limit : -1);
        bindings.append(qMax(0, offset));
    }

    const int columns = m->columnCount();
    DB_LOCK;
    QSqlQuery q(m_dbo->getDb());
    q.setForwardOnly(true);
    bool ok = q.prepare(sql);
    for (const QVariant& binding : std::as_const(bindings))
        q.addBindValue(binding);
    ok = ok && q.exec();
    if (ok) {
        while (q.next()) {
            QVector<QVariant> row(columns);
            for (int c = 0; c < columns && c < q.record().count(); ++c)
                row[c] = q.value(c);
            rows->append(row);
        }
    }
    const QString sqlError = q.lastError().text();
    DB_UNLOCK;
    if (!ok) {
        *error = PluginError::make(QStringLiteral("query-failed"), sqlError);
        return false;
    }
    return true;
}

bool DataService::includeSet(const Entity& e, const QVariant& include, QSet<QString>* storedTables,
                             QHash<QString, QVariantMap>* childQueries, PluginError* error)
{
    Q_UNUSED(e);
    QVariantList items;
    if (include.typeId() == QMetaType::QVariantList || include.typeId() == QMetaType::QStringList)
        items = include.toList();
    else if (include.typeId() == QMetaType::QString && !include.toString().isEmpty())
        items = {include};
    else if (include.typeId() == QMetaType::QVariantMap)
        items = {include};
    for (const QVariant& item : items) {
        QString name;
        QVariant nested;
        if (item.typeId() == QMetaType::QVariantMap) {
            const QVariantMap spec = item.toMap();
            name = spec.value(QStringLiteral("entity")).toString();
            nested = spec.value(QStringLiteral("include"));
            QVariantMap childQuery;
            if (spec.contains(QStringLiteral("where")))
                childQuery.insert(QStringLiteral("where"), spec.value(QStringLiteral("where")));
            if (spec.contains(QStringLiteral("orderBy")))
                childQuery.insert(QStringLiteral("orderBy"), spec.value(QStringLiteral("orderBy")));
            if (!childQuery.isEmpty())
                childQueries->insert(name, childQuery);
        } else {
            name = item.toString();
        }
        const Entity* child = entity(name, false, error);
        if (!child)
            return false;
        storedTables->insert(child->stored);
        if (nested.isValid() && !includeSet(*child, nested, storedTables, childQueries, error))
            return false;
    }
    return true;
}

QVariantList DataService::toRecords(const RecordTable& table, const QVariantMap& query, PluginError* error)
{
    QVariantList out;
    const Entity* e = entity(PublishedSchema::instance().publishedEntity(table.name), false, error);
    if (!e)
        return out;

    QHash<QString, QVariantMap> childQueries;
    const QVariantMap rawChildQueries = query.value(QStringLiteral("$childQueries")).toMap();
    for (auto it = rawChildQueries.constBegin(); it != rawChildQueries.constEnd(); ++it)
        childQueries.insert(it.key(), it.value().toMap());

    for (const RecordRow& row : table.rows) {
        QVariantMap record;
        record.insert(QStringLiteral("$entity"), e->published);
        for (const RecordColumn& rc : row.columns) {
            const auto idx = e->byStored.constFind(rc.name);
            if (idx == e->byStored.constEnd())
                continue;
            const Column& column = e->columns.at(*idx);
            if (column.isKey)
                record.insert(QStringLiteral("$key"), rc.value.toString());
            record.insert(column.published, convertOut(column, rc.value, rc.lookup));
        }
        for (const RecordTable& child : row.children) {
            const QString childEntity = PublishedSchema::instance().publishedEntity(child.name);
            QVariantList records = toRecords(child, query, error);
            if (error->isError())
                return {};
            const QVariantMap childQuery = childQueries.value(childEntity);
            const QVariantMap where = childQuery.value(QStringLiteral("where")).toMap();
            if (!where.isEmpty())
                records.erase(std::remove_if(records.begin(), records.end(),
                                             [&](const QVariant& r) { return !matches(r.toMap(), where); }),
                              records.end());
            const QVariantList order = childQuery.value(QStringLiteral("orderBy")).toList();
            if (!order.isEmpty()) {
                std::stable_sort(records.begin(), records.end(), [&](const QVariant& a, const QVariant& b) {
                    for (const QVariant& term : order) {
                        const QString name = term.typeId() == QMetaType::QVariantMap
                            ? term.toMap().value(QStringLiteral("column")).toString() : term.toString();
                        const bool desc = term.typeId() == QMetaType::QVariantMap
                            && term.toMap().value(QStringLiteral("descending")).toBool();
                        const int c = compareValues(refId(a.toMap().value(name)), refId(b.toMap().value(name)));
                        if (c != 0)
                            return desc ? c > 0 : c < 0;
                    }
                    return false;
                });
            }
            QVariantList existing = record.value(childEntity).toList();
            existing.append(records);
            record.insert(childEntity, existing);
        }
        out.append(record);
    }
    return out;
}

// ── Reads ────────────────────────────────────────────────────────────────────

QVariant DataService::entities(PluginError* error)
{
    QVariantList out;
    for (const QString& name : PublishedSchema::instance().entities()) {
        if (!m_manifest.canRead(name))
            continue;
        const QVariant info = schema(name, error);
        if (error->isError())
            return {};
        out.append(info);
    }
    return out;
}

QVariant DataService::schema(const QString& name, PluginError* error)
{
    const Entity* e = entity(name, false, error);
    if (!e)
        return {};
    QVariantList columns;
    for (const Column& c : e->columns) {
        QVariantMap info {
            {QStringLiteral("name"), c.published}, {QStringLiteral("displayName"), c.displayName},
            {QStringLiteral("type"), c.typeName()}, {QStringLiteral("required"), c.required},
            {QStringLiteral("readOnly"), c.readOnly && !c.isKey}, {QStringLiteral("unique"), c.unique},
            {QStringLiteral("searchable"), c.searchable}};
        if (c.isRef())
            info.insert(QStringLiteral("ref"), QVariantMap{{QStringLiteral("entity"),
                                                            PublishedSchema::instance().publishedEntity(c.lookupTable)}});
        if (!c.choices.isEmpty())
            info.insert(QStringLiteral("choices"), c.choices);
        columns.append(info);
    }
    QVariantList uniqueKeys;
    for (const QStringList& k : e->uniqueKeys)
        uniqueKeys.append(k);
    QVariantList children;
    for (const Entity::Child& c : e->children)
        children.append(QVariantMap{{QStringLiteral("entity"), c.entity}, {QStringLiteral("column"), c.column},
                                    {QStringLiteral("owned"), c.owned}});
    return QVariantMap {
        {QStringLiteral("name"), e->published}, {QStringLiteral("displayName"), e->displayName},
        {QStringLiteral("key"), e->columns.first().published}, {QStringLiteral("uniqueKeys"), uniqueKeys},
        {QStringLiteral("columns"), columns}, {QStringLiteral("children"), children}};
}

QVariant DataService::list(const QString& name, const QVariantMap& query, PluginError* error)
{
    const Entity* e = entity(name, false, error);
    if (!e)
        return {};

    QSet<QString> includes;
    QHash<QString, QVariantMap> childQueries;
    if (!includeSet(*e, query.value(QStringLiteral("include")), &includes, &childQueries, error))
        return {};

    QList<QVector<QVariant>> rows;
    if (!selectRows(*e, query, &rows, error))
        return {};

    RecordExportSpec spec;
    spec.includeTable = [includes](const QString& table) { return includes.contains(table); };
    const QStringList fields = query.value(QStringLiteral("fields")).toStringList();
    if (!fields.isEmpty()) {
        QStringList stored;
        for (const QString& f : fields) {
            const Column* c = e->column(f);
            if (!c) {
                *error = validation(QStringLiteral("%1 has no column \"%2\".").arg(e->published, f), f);
                return {};
            }
            stored.append(c->stored);
        }
        spec.fields.insert(e->stored, stored);
    }

    RecordTreeExporter exporter(m_dbo.get());
    const RecordTable table = exporter.exportRows(e->model.get(), rows, spec);

    QVariantMap convertQuery;
    QVariantMap rawChildQueries;
    for (auto it = childQueries.constBegin(); it != childQueries.constEnd(); ++it)
        rawChildQueries.insert(it.key(), it.value());
    convertQuery.insert(QStringLiteral("$childQueries"), rawChildQueries);
    return toRecords(table, convertQuery, error);
}

QVariant DataService::get(const QString& name, const QVariant& keyOrWhere, const QVariantMap& options, PluginError* error)
{
    const Entity* e = entity(name, false, error);
    if (!e)
        return {};
    QVariantMap query = options;
    if (keyOrWhere.typeId() == QMetaType::QVariantMap)
        query.insert(QStringLiteral("where"), keyOrWhere);
    else
        query.insert(QStringLiteral("where"), QVariantMap{{e->columns.first().published, keyOrWhere}});
    query.insert(QStringLiteral("limit"), 1);
    const QVariantList records = list(name, query, error).toList();
    if (error->isError())
        return {};
    return records.isEmpty() ? QVariant::fromValue(nullptr) : records.first();
}

QVariant DataService::count(const QString& name, const QVariantMap& where, PluginError* error)
{
    const Entity* e = entity(name, false, error);
    if (!e)
        return {};
    QList<QVector<QVariant>> rows;
    if (!selectRows(*e, QVariantMap{{QStringLiteral("where"), where}}, &rows, error))
        return {};
    return int(rows.size());
}

QVariant DataService::search(const QString& text, const QVariantMap& options, PluginError* error)
{
    if (!m_dbo) {
        *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("No database is open."));
        return {};
    }
    static const QHash<QString, QString> types {
        {QStringLiteral("Company"), QStringLiteral("companies")}, {QStringLiteral("Contact"), QStringLiteral("contacts")},
        {QStringLiteral("Project"), QStringLiteral("projects")}, {QStringLiteral("Project Team"), QStringLiteral("project_team")},
        {QStringLiteral("Project Locations"), QStringLiteral("project_locations")},
        {QStringLiteral("Project Notes"), QStringLiteral("project_notes")},
        {QStringLiteral("Meeting Attendees"), QStringLiteral("meeting_attendees")},
        {QStringLiteral("Item Tracker"), QStringLiteral("item_tracker")},
        {QStringLiteral("Tracker Update"), QStringLiteral("item_tracker_updates")},
        {QStringLiteral("Status Report Item"), QStringLiteral("status_report_items")}};
    const QStringList wanted = options.value(QStringLiteral("entities")).toStringList();
    const int limit = options.value(QStringLiteral("limit"), 50).toInt();

    QString pattern = text;
    pattern.replace(QStringLiteral("\\"), QStringLiteral("\\\\")).replace(QStringLiteral("%"), QStringLiteral("\\%"))
           .replace(QStringLiteral("_"), QStringLiteral("\\_"));

    QVariantList out;
    DB_LOCK;
    QSqlQuery q(m_dbo->getDb());
    q.prepare(QStringLiteral("SELECT datatype, datakey, dataname, datadescription FROM database_search "
                             "WHERE datadescription LIKE ? ESCAPE '\\'"));
    q.addBindValue(QStringLiteral("%") + pattern + QStringLiteral("%"));
    const bool ok = q.exec();
    while (ok && q.next() && out.size() < limit) {
        const QString entityName = types.value(q.value(0).toString());
        if (entityName.isEmpty() || !m_manifest.canRead(entityName))
            continue;
        if (!wanted.isEmpty() && !wanted.contains(entityName))
            continue;
        out.append(QVariantMap{{QStringLiteral("entity"), entityName}, {QStringLiteral("key"), q.value(1).toString()},
                               {QStringLiteral("title"), q.value(2).toString()},
                               {QStringLiteral("snippet"), q.value(3).toString().left(200)}});
    }
    const QString sqlError = q.lastError().text();
    DB_UNLOCK;
    if (!ok) {
        *error = PluginError::make(QStringLiteral("query-failed"), sqlError);
        return {};
    }
    return out;
}

// ── Writes ───────────────────────────────────────────────────────────────────

QVariant DataService::saveOne(const Entity& e, const QVariantMap& record, PluginError* error)
{
    RecordRow row;
    QString key = record.value(QStringLiteral("$key")).toString();
    for (auto it = record.constBegin(); it != record.constEnd(); ++it) {
        if (it.key().startsWith(QLatin1Char('$')))
            continue;
        const Column* column = e.column(it.key());
        if (!column) {
            if (it.value().typeId() == QMetaType::QVariantList && PublishedSchema::instance().isEntity(it.key())) {
                *error = validation(QStringLiteral("Child records (\"%1\") are not saved with their parent; save them "
                                                   "separately, for example inside data.transaction().").arg(it.key()), it.key());
            } else {
                *error = PublishedSchema::instance().isRetiredName(it.key())
                    ? validation(QStringLiteral("\"%1\" is a stored name; use \"%2\".")
                                     .arg(it.key(), PublishedSchema::instance().publishedColumn(it.key())), it.key())
                    : validation(QStringLiteral("%1 has no column \"%2\".").arg(e.published, it.key()), it.key());
            }
            return {};
        }
        if (column->readOnly && !column->isKey) {
            *error = validation(QStringLiteral("\"%1\" is read-only.").arg(column->published), column->published);
            return {};
        }
        if (!column->choices.isEmpty() && !it.value().isNull()
            && !column->choices.contains(it.value().toString())) {
            *error = validation(QStringLiteral("\"%1\" must be one of: %2.").arg(column->published, column->choices.join(QStringLiteral(", "))),
                                column->published);
            return {};
        }
        QVariant text;
        if (!convertIn(*column, it.value(), &text, error))
            return {};
        if (column->isKey) {
            key = text.toString();
            if (key.isEmpty())
                continue;
        }
        row.columns.append({column->stored, text, {}});
    }
    if (!key.isEmpty() && !row.column(e.columns.first().stored))
        row.columns.prepend({e.columns.first().stored, key, {}});

    SqlQueryModel* m = e.model.get();
    RecordImportOutcome outcome;
    const bool accepted = m->importRecordRow(row, &outcome);
    if (!accepted || outcome.kind == RecordImportOutcome::Rejected) {
        *error = validation(outcome.message.isEmpty() ? QStringLiteral("The record was rejected.") : outcome.message, outcome.column);
        return {};
    }
    if (outcome.kind == RecordImportOutcome::SkippedRequiredEmpty) {
        *error = validation(QStringLiteral("\"%1\" is required.").arg(PublishedSchema::instance().publishedColumn(outcome.column)),
                            PublishedSchema::instance().publishedColumn(outcome.column));
        return {};
    }
    if (outcome.kind == RecordImportOutcome::SkippedPreviouslyDeleted) {
        *error = PluginError::make(QStringLiteral("deleted"), QStringLiteral("This record was deleted and cannot be saved again."));
        return {};
    }
    if (m_dbo->inBatch() && m_dbo->batchFailed()) {
        *error = PluginError::make(QStringLiteral("save-failed"), m_dbo->batchError());
        return {};
    }

    const QVariant saved = get(e.published, outcome.key, {}, error);
    if (error->isError())
        return {};
    if (saved.isNull()) {
        const QString reason = m_dbo->lastSaveError();
        *error = PluginError::make(QStringLiteral("save-failed"),
                                   reason.isEmpty() ? QStringLiteral("The database did not accept the record; "
                                                                     "check that every required column is set.") : reason);
        return {};
    }
    return QVariantMap {{QStringLiteral("record"), saved},
                        {QStringLiteral("operation"), outcome.kind == RecordImportOutcome::Inserted
                                                          ? QStringLiteral("insert") : QStringLiteral("update")}};
}

QVariant DataService::save(const QString& name, const QVariantMap& record, PluginError* error)
{
    const Entity* e = entity(name, true, error);
    if (!e)
        return {};
    const QVariant result = saveOne(*e, record, error);
    deliverChanges();
    return result;
}

QVariant DataService::saveAll(const QString& name, const QVariantList& records, PluginError* error)
{
    const Entity* e = entity(name, true, error);
    if (!e)
        return {};
    const bool ownBatch = !m_dbo->inBatch();
    if (ownBatch && !begin(error))
        return {};
    QVariantList results;
    for (const QVariant& record : records) {
        const QVariant result = saveOne(*e, record.toMap(), error);
        if (error->isError()) {
            if (ownBatch)
                rollback();
            return {};
        }
        results.append(result);
    }
    if (ownBatch && !commit(error))
        return {};
    return results;
}

QVariant DataService::remove(const QString& name, const QVariant& keyOrRecord, PluginError* error)
{
    const Entity* e = entity(name, true, error);
    if (!e)
        return {};
    const QString key = keyOrRecord.typeId() == QMetaType::QVariantMap
        ? keyOrRecord.toMap().value(QStringLiteral("$key"),
                                    keyOrRecord.toMap().value(e->columns.first().published)).toString()
        : keyOrRecord.toString();
    if (key.isEmpty()) {
        *error = validation(QStringLiteral("remove() needs a key or a record."));
        return {};
    }
    // The importer inserts when a row is not found, so check first.
    const QVariant existing = get(name, key, {}, error);
    if (error->isError())
        return {};
    if (existing.isNull()) {
        *error = PluginError::make(QStringLiteral("not-found"), QStringLiteral("No %1 record has key \"%2\".").arg(name, key));
        return {};
    }

    RecordRow row;
    row.deleted = true;
    row.columns.append({e->columns.first().stored, key, {}});
    const qsizetype before = m_pendingChanges.size();
    RecordImportOutcome outcome;
    e->model->importRecordRow(row, &outcome);
    if (outcome.kind == RecordImportOutcome::SkippedReferenced) {
        *error = PluginError::make(QStringLiteral("reference-blocked"),
                                   QStringLiteral("This %1 record is still used by other records.").arg(name));
        return {};
    }
    if (outcome.kind != RecordImportOutcome::Deleted) {
        *error = PluginError::make(QStringLiteral("save-failed"), QStringLiteral("The record could not be deleted."));
        return {};
    }
    QVariantList removed;
    for (qsizetype i = before; i < m_pendingChanges.size(); ++i) {
        const Change& c = m_pendingChanges.at(i);
        if (c.operation == KeyColumnChange::Delete)
            removed.append(QVariantMap{{QStringLiteral("entity"), PublishedSchema::instance().publishedEntity(c.table)},
                                       {QStringLiteral("key"), c.key}});
    }
    deliverChanges();
    return QVariantMap{{QStringLiteral("removed"), removed}};
}

bool DataService::begin(PluginError* error)
{
    if (!m_dbo) {
        *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("No database is open."));
        return false;
    }
    if (m_dbo->inBatch()) {
        *error = PluginError::make(QStringLiteral("transaction-active"), QStringLiteral("A transaction is already open."));
        return false;
    }
    if (!m_dbo->beginBatch()) {
        *error = PluginError::make(QStringLiteral("transaction-failed"), QStringLiteral("A transaction could not be started."));
        return false;
    }
    return true;
}

bool DataService::commit(PluginError* error)
{
    if (!m_dbo || !m_dbo->inBatch()) {
        *error = PluginError::make(QStringLiteral("transaction-failed"), QStringLiteral("No transaction is open."));
        return false;
    }
    if (!m_dbo->commitBatch()) {
        m_pendingChanges.clear();
        *error = PluginError::make(QStringLiteral("transaction-failed"), m_dbo->batchError());
        return false;
    }
    deliverChanges();
    return true;
}

void DataService::rollback()
{
    if (m_dbo && m_dbo->inBatch())
        m_dbo->rollbackBatch();
    m_pendingChanges.clear();
}

bool DataService::inTransaction() const
{
    return m_dbo && m_dbo->inBatch();
}

void DataService::deliverChanges()
{
    if (inTransaction())
        return;
    const QList<Change> changes = std::exchange(m_pendingChanges, {});
    if (!m_rowChanged)
        return;
    for (const Change& c : changes)
        m_rowChanged(c.table, c.key, c.operation);
}

QVariant DataService::exportXml(const QString& name, const QString& key, const QVariantMap& options, PluginError* error)
{
    const Entity* e = entity(name, false, error);
    if (!e)
        return {};
    QStringList filter;
    const QStringList include = options.value(QStringLiteral("include")).toStringList();
    if (!include.isEmpty()) {
        filter.append(e->stored);
        for (const QString& child : include) {
            const Entity* c = entity(child, false, error);
            if (!c)
                return {};
            filter.append(c->stored);
        }
    }
    std::unique_ptr<SqlQueryModel> exportModel(m_dbo->createExportObject(e->stored));
    if (!key.isEmpty())
        exportModel->setFilter(0, key);
    exportModel->refresh();
    std::unique_ptr<QDomDocument> document(m_dbo->createXMLExportDoc(exportModel.get(), filter.join(QLatin1Char('/'))));
    return document ? document->toString(1) : QString();
}

QVariant DataService::importXml(const QString& xml, PluginError* error)
{
    if (!m_dbo) {
        *error = PluginError::make(QStringLiteral("unavailable"), QStringLiteral("No database is open."));
        return {};
    }
    QDomDocument document;
    const auto parsed = document.setContent(xml);
    if (!parsed) {
        *error = validation(QStringLiteral("The XML could not be parsed: %1").arg(parsed.errorMessage));
        return {};
    }
    const RecordTree tree = RecordTreeXml::treeFromDocument(document);
    for (const QString& table : RecordTreeImport::tableOrder()) {
        if (RecordTreeImport::tablesNamed(tree, table).isEmpty())
            continue;
        const QString published = PublishedSchema::instance().publishedEntity(table);
        if (!m_manifest.canWrite(published)) {
            *error = PluginError::denied(QStringLiteral("data.write:") + published);
            return {};
        }
    }
    QList<RecordImportOutcome> outcomes;
    const bool ok = m_dbo->importRecordTree(tree, &outcomes);
    deliverChanges();
    if (!ok) {
        *error = validation(m_dbo->lastSaveError().isEmpty() ? QStringLiteral("The import was rejected.")
                                                                : m_dbo->lastSaveError());
        return {};
    }
    int inserted = 0, updated = 0, removed = 0;
    for (const RecordImportOutcome& o : std::as_const(outcomes)) {
        if (o.kind == RecordImportOutcome::Inserted) ++inserted;
        else if (o.kind == RecordImportOutcome::Updated) ++updated;
        else if (o.kind == RecordImportOutcome::Deleted) ++removed;
    }
    return QVariantMap{{QStringLiteral("inserted"), inserted}, {QStringLiteral("updated"), updated},
                       {QStringLiteral("removed"), removed}};
}

} // namespace PN::Plugins
