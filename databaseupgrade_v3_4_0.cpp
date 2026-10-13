// SPDX-License-Identifier: GPL-3.0-only
#include "databaseupgrade_v3_4_0.h"
#include "databaseobjects.h"
#include "legacywidgetsettings.h"
#include <QSqlQuery>
#include <QSqlError>

bool db_UpgradeStep_v3_4_0()
{
    QWriteLocker lock(&db_rwlock);
    QSqlDatabase &db = global_DBObjects.getDb();
    if (!db.transaction()) {
        global_DBObjects.setLastSaveError(db.lastError().text());
        return false;
    }
    auto fail = [&](const QSqlError &error) {
        global_DBObjects.setLastSaveError(error.text());
        db.rollback();
        return false;
    };
    // The Widgets BasePage was the sole consumer of PageState. All other keys
    // are explicit: current QML UI keys, shared preferences, and plugin settings
    // must never be caught by a broad UI:/UserFilter: prefix deletion.
    const QStringList obsolete = PN::LegacyWidgetSettings::databaseKeys();
    QStringList placeholders;
    for (int i = 0; i < obsolete.size(); ++i) placeholders << "?";
    QSqlQuery cleanup(db);
    if (!cleanup.prepare("UPDATE application_settings SET deleted = 1 "
                         "WHERE deleted = 0 AND (substr(parameter_name, 1, 10) = 'PageState:' "
                         "OR parameter_name IN (" + placeholders.join(',') + "))"))
        return fail(cleanup.lastError());
    for (const QString &key : obsolete) cleanup.addBindValue(key);
    if (!cleanup.exec()) return fail(cleanup.lastError());
    // Risks now live in item_tracker. The retired table has no consumers;
    // 3.4.0 intentionally removes its historical records as well.
    QSqlQuery drop(db);
    if (!drop.exec("DROP TABLE IF EXISTS project_risks")) return fail(drop.lastError());
    if (!db.commit()) return fail(db.lastError());
    return true;
}
