// SPDX-License-Identifier: GPL-3.0-only
#include "MacContactsProvider.h"

#import <Contacts/Contacts.h>

namespace PN::Contacts {
namespace {

QString ns2qs(NSString *s) { return s ? QString::fromNSString(s) : QString(); }

QString containerName(CNContainer *container)
{
    if (container.name.length) return ns2qs(container.name);
    switch (container.type) {
    case CNContainerTypeLocal: return QStringLiteral("On My Mac");
    case CNContainerTypeExchange: return QStringLiteral("Exchange");
    case CNContainerTypeCardDAV: return QStringLiteral("CardDAV");
    default: return QStringLiteral("Contacts");
    }
}

NSArray<id<CNKeyDescriptor>> *fetchKeys()
{
    return @[ CNContactIdentifierKey, CNContactGivenNameKey, CNContactFamilyNameKey, CNContactOrganizationNameKey,
              CNContactJobTitleKey, CNContactEmailAddressesKey, CNContactPhoneNumbersKey ];
}

// requestAccessForEntityType: answers on an arbitrary queue; this runs on the
// dedicated ContactSync worker thread (never the UI thread), so blocking here
// until the user responds to the one-time system prompt is safe.
CNAuthorizationStatus ensureAuthorized()
{
    CNAuthorizationStatus status = [CNContactStore authorizationStatusForEntityType:CNEntityTypeContacts];
    if (status != CNAuthorizationStatusNotDetermined) return status;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    CNContactStore *authStore = [CNContactStore new];
    [authStore requestAccessForEntityType:CNEntityTypeContacts
                         completionHandler:^(BOOL, NSError *) {
                             dispatch_semaphore_signal(sem);
                         }];
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    (void)authStore;
    return [CNContactStore authorizationStatusForEntityType:CNEntityTypeContacts];
}

Contact decodeContact(CNContact *contact, const QString &book)
{
    Contact c;
    c.id = ns2qs(contact.identifier);
    c.book = book;
    const QString given = ns2qs(contact.givenName), family = ns2qs(contact.familyName);
    c.name = (given + " " + family).trimmed();
    if (c.name.isEmpty()) c.name = ns2qs(contact.organizationName);
    c.company = ns2qs(contact.organizationName);
    if (!ns2qs(contact.jobTitle).isEmpty()) c.fields["role"] = ns2qs(contact.jobTitle);
    for (CNLabeledValue<NSString *> *email in contact.emailAddresses) c.emails << ns2qs(email.value);
    if (!c.emails.isEmpty()) c.fields["email"] = c.emails.first();
    for (CNLabeledValue<CNPhoneNumber *> *phone in contact.phoneNumbers) {
        const QString number = ns2qs(phone.value.stringValue);
        c.phones << number;
        if ([phone.label isEqualToString:CNLabelWork] && !c.fields.contains("office_phone"))
            c.fields["office_phone"] = number;
        else if (([phone.label isEqualToString:CNLabelPhoneNumberMobile]
                   || [phone.label isEqualToString:CNLabelPhoneNumberiPhone])
                 && !c.fields.contains("cell_phone"))
            c.fields["cell_phone"] = number;
    }
    return c;
}

// Mirrors the level of fidelity the Office 365 and iCloud backends use: the
// managed "name" field maps to a single Contacts property rather than being
// split into given/family name, so re-saving never mangles a name Project
// Notes did not itself choose to split.
void applyFields(CNMutableContact *mutableContact, const Contact &contact)
{
    mutableContact.givenName = contact.name.toNSString();
    mutableContact.organizationName = contact.company.toNSString();
    mutableContact.jobTitle = contact.fields.value("role").toString().toNSString();

    NSMutableArray<CNLabeledValue<NSString *> *> *emails = [NSMutableArray array];
    const QString email = contact.fields.value("email").toString();
    if (!email.trimmed().isEmpty())
        [emails addObject:[CNLabeledValue labeledValueWithLabel:CNLabelWork value:email.toNSString()]];
    mutableContact.emailAddresses = emails;

    NSMutableArray<CNLabeledValue<CNPhoneNumber *> *> *phones = [NSMutableArray array];
    const QString office = contact.fields.value("office_phone").toString();
    const QString cell = contact.fields.value("cell_phone").toString();
    if (!office.trimmed().isEmpty())
        [phones addObject:[CNLabeledValue labeledValueWithLabel:CNLabelWork
                                                            value:[CNPhoneNumber phoneNumberWithStringValue:office.toNSString()]]];
    if (!cell.trimmed().isEmpty())
        [phones addObject:[CNLabeledValue labeledValueWithLabel:CNLabelPhoneNumberMobile
                                                            value:[CNPhoneNumber phoneNumberWithStringValue:cell.toNSString()]]];
    mutableContact.phoneNumbers = phones;
}

} // namespace

void MacContactsProvider::fetch(const QVariantMap &, const QJsonObject &, Completion done)
{
    const auto gen = ++m_generation;
    Snapshot snapshot;
    @autoreleasepool {
        const CNAuthorizationStatus status = ensureAuthorized();
        if (gen != m_generation) return;
        if (status == CNAuthorizationStatusDenied || status == CNAuthorizationStatusRestricted) {
            snapshot.error = QStringLiteral(
                "Project Notes does not have access to your contacts. Enable it in System Settings > "
                "Privacy & Security > Contacts, then try again.");
            snapshot.authenticationError = true;
            done(snapshot);
            return;
        }

        CNContactStore *store = [CNContactStore new];
        NSError *error = nil;
        NSArray<CNContainer *> *containers = [store containersMatchingPredicate:nil error:&error];
        if (!containers) {
            snapshot.error = QStringLiteral("Cannot list macOS address books: %1").arg(ns2qs(error.localizedDescription));
            done(snapshot);
            return;
        }

        snapshot.account = ns2qs(NSUserName());
        QString defaultBook;
        for (CNContainer *container in containers) {
            const QString bookId = ns2qs(container.identifier);
            snapshot.books << QVariantMap{{"id", bookId}, {"name", containerName(container)}, {"writable", true}};
            if (container.type == CNContainerTypeLocal) defaultBook = bookId;

            NSPredicate *predicate = [CNContact predicateForContactsInContainerWithIdentifier:container.identifier];
            NSError *fetchError = nil;
            NSArray<CNContact *> *contacts = [store unifiedContactsMatchingPredicate:predicate
                                                                         keysToFetch:fetchKeys()
                                                                               error:&fetchError];
            if (!contacts) {
                snapshot.error = QStringLiteral("Cannot read macOS contacts: %1").arg(ns2qs(fetchError.localizedDescription));
                done(snapshot);
                return;
            }
            for (CNContact *contact in contacts) {
                if (gen != m_generation) return;
                const Contact c = decodeContact(contact, bookId);
                if (c.id.isEmpty() || c.name.trimmed().isEmpty()) continue;
                snapshot.contacts << c;
            }
        }
        if (defaultBook.isEmpty() && !snapshot.books.isEmpty())
            defaultBook = snapshot.books.first().toMap().value("id").toString();
        snapshot.defaultExportBookId = defaultBook;
    }
    done(snapshot);
}

void MacContactsProvider::create(const QVariantMap &config, const Contact &contact, const QString &, Created done)
{
    const QString book = config.value("exportBookId").toString();
    if (book.isEmpty()) {
        done({}, "The contact application has no available writable address book.");
        return;
    }
    @autoreleasepool {
        CNContactStore *store = [CNContactStore new];
        CNMutableContact *mutableContact = [CNMutableContact new];
        applyFields(mutableContact, contact);

        CNSaveRequest *request = [CNSaveRequest new];
        [request addContact:mutableContact toContainerWithIdentifier:book.toNSString()];
        NSError *error = nil;
        if (![store executeSaveRequest:request error:&error]) {
            done({}, QStringLiteral("Cannot create macOS contact: %1").arg(ns2qs(error.localizedDescription)));
            return;
        }
        auto c = contact;
        c.id = ns2qs(mutableContact.identifier);
        c.book = book;
        done(c, {});
    }
}

void MacContactsProvider::update(const QVariantMap &, const Contact &existing, const Contact &contact, Updated done)
{
    if (existing.id.isEmpty()) {
        done({}, "The existing contact has no stable identity.");
        return;
    }
    @autoreleasepool {
        CNContactStore *store = [CNContactStore new];
        NSError *error = nil;
        CNContact *found = [store unifiedContactWithIdentifier:existing.id.toNSString() keysToFetch:fetchKeys() error:&error];
        if (!found) {
            done({}, QStringLiteral("Cannot load macOS contact to update: %1").arg(ns2qs(error.localizedDescription)));
            return;
        }
        CNMutableContact *mutableContact = [found mutableCopy];
        applyFields(mutableContact, contact);

        CNSaveRequest *request = [CNSaveRequest new];
        [request updateContact:mutableContact];
        if (![store executeSaveRequest:request error:&error]) {
            done({}, QStringLiteral("Cannot update macOS contact: %1").arg(ns2qs(error.localizedDescription)));
            return;
        }
        auto c = contact;
        c.id = existing.id;
        c.book = existing.book;
        done(c, {});
    }
}

void MacContactsProvider::cancel()
{
    ++m_generation;
}

ContactProvider *createMacContactProvider(QObject *parent)
{
    return new MacContactsProvider(parent);
}

}
