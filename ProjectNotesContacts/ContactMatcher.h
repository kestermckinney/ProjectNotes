// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ContactTypes.h"
namespace PN::Contacts {
class ContactMatcher {
public:
    static QString normalize(QString value, bool company = false);
    static QString phone(QString value);
    // Comparison only: never use these keys as replacement display values.
    static bool equivalent(const QString &field, const QString &a, const QString &b);
    static bool equivalentFields(const QVariantMap &a, const QVariantMap &b);
    static double similarity(const QString &a, const QString &b);
    static double nameSimilarity(const QString &a, const QString &b);
    static Match company(const QString &name, const QList<Client> &clients);
    static Match person(const Contact &contact, const QString &clientId,
                        const QList<Person> &people, const QStringList &rejected = {});
};
}
