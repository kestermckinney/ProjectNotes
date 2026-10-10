// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../ContactProvider.h"
namespace PN::Contacts {

// Native macOS contacts, read and written straight through CNContactStore
// (Contacts.framework) instead of CardDAV. There is no incremental sync API
// used here, so every fetch() re-enumerates all containers; local address
// books are small enough that this is not a practical cost.
class MacContactsProvider final : public ContactProvider {
public:
    using ContactProvider::ContactProvider;
    void fetch(const QVariantMap &config, const QJsonObject &state, Completion done) override;
    void create(const QVariantMap &config, const Contact &contact, const QString &intentId, Created done) override;
    void update(const QVariantMap &config, const Contact &existing, const Contact &contact, Updated done) override;
    void cancel() override;

private:
    quint64 m_generation = 0;
};

ContactProvider *createMacContactProvider(QObject *parent);

}
