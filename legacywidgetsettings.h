// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QStringList>
class QSettings;

namespace PN::LegacyWidgetSettings {
QStringList filterKeys();
QStringList databaseKeys();
void cleanupLocal(QSettings &settings);
}
