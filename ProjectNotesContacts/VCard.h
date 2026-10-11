// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactTypes.h"
namespace PN::Contacts {
Contact parseVCard(const QString &text,const QString &id,const QString &book);
QByteArray writeVCard(const Contact &contact,const QString &uid);
QByteArray mergeVCard(const QByteArray &existing,const Contact &contact,const Contact &previous);
}
