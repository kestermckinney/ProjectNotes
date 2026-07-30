# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "plugins"))

from includes.icloud_tools import (CardDAVClient, Contact, ICloudAuthenticationError,
                                   ICloudContactSync, ProjectNotesContactImporter,
                                   parse_vcard, serialize_vcard)


class FakeResponse:
    def __init__(self, status, body, headers=None):
        self.status_code = status
        self.content = body.encode()
        self.headers = headers or {}


class FakeSession:
    def __init__(self, responses):
        self.responses = list(responses)
        self.headers = {}
        self.auth = None
        self.requests = []

    def request(self, method, url, **kwargs):
        self.requests.append((method, url, kwargs))
        return self.responses.pop(0)


class FakeProjectNotes:
    def __init__(self, people=()):
        rows = []
        for person in people:
            columns = "".join(
                f'<column name="{key}" lookupvalue="{value}"></column>' if key == "client_id"
                else f'<column name="{key}">{value}</column>' for key, value in person.items())
            rows.append(f"<row>{columns}</row>")
        self.export = (f'<projectnotes filepath="/tmp/test.pnd"><table name="people">'
                       f'{"".join(rows)}</table></projectnotes>')
        self.updates = []

    def get_data(self, request):
        return self.export

    def update_data(self, document):
        self.updates.append(document)
        return True


class VCardTests(unittest.TestCase):
    def test_maps_preferred_work_and_mobile_fields(self):
        card = parse_vcard("""BEGIN:VCARD
VERSION:3.0
UID:abc-123
N:Doe;Jane;;;
FN:Jane Doe
ORG:Acme Corp;Engineering
TITLE:Lead Engineer
EMAIL;TYPE=HOME:home@example.com
EMAIL;TYPE=WORK:jane@example.com
TEL;TYPE=WORK:+1 555 0100
TEL;TYPE=CELL:+1 555 0101
END:VCARD
""", "/card/1.vcf", '"etag"')
        self.assertEqual(card.name, "Jane Doe")
        self.assertEqual(card.company, "Acme Corp")
        self.assertEqual(card.email, "jane@example.com")
        self.assertEqual(card.office_phone, "+1 555 0100")
        self.assertEqual(card.cell_phone, "+1 555 0101")
        self.assertEqual(card.role, "Lead Engineer")

    def test_unfolds_and_decodes_quoted_printable(self):
        card = parse_vcard("BEGIN:VCARD\nFN;ENCODING=QUOTED-PRINTABLE:J=C3=B6rg=20=\n =C3=85ngstr=C3=B6m\nUID:2\nEND:VCARD")
        self.assertEqual(card.name, "Jörg Ångström")

    def test_serializes_project_notes_contact(self):
        card = serialize_vcard({"name": "Jane Doe", "email": "jane@example.com",
                                "client_id": "Acme", "cell_phone": "+1 555"}, "person-1")
        self.assertIn("UID:person-1\r\n", card)
        self.assertIn("FN:Jane Doe\r\n", card)
        self.assertIn("ORG:Acme\r\n", card)
        self.assertIn("TEL;TYPE=CELL:+1 555\r\n", card)


class CardDAVTests(unittest.TestCase):
    def test_discovery_finds_all_address_books(self):
        principal = '<d:multistatus xmlns:d="DAV:"><d:response><d:propstat><d:prop><d:current-user-principal><d:href>/p/1/</d:href></d:current-user-principal></d:prop></d:propstat></d:response></d:multistatus>'
        home = '<d:multistatus xmlns:d="DAV:" xmlns:c="urn:ietf:params:xml:ns:carddav"><d:response><d:propstat><d:prop><c:addressbook-home-set><d:href>/a/1/</d:href></c:addressbook-home-set></d:prop></d:propstat></d:response></d:multistatus>'
        books = '<d:multistatus xmlns:d="DAV:" xmlns:c="urn:ietf:params:xml:ns:carddav"><d:response><d:href>/a/1/book/</d:href><d:propstat><d:prop><d:resourcetype><c:addressbook/></d:resourcetype><d:displayname>Contacts</d:displayname><d:sync-token>token-1</d:sync-token></d:prop></d:propstat></d:response></d:multistatus>'
        session = FakeSession([FakeResponse(207, principal), FakeResponse(207, home), FakeResponse(207, books)])
        result = CardDAVClient("user", "secret", session=session).discover()
        self.assertEqual(result[0]["name"], "Contacts")
        self.assertEqual(result[0]["sync_token"], "token-1")

    def test_raises_specific_authentication_error(self):
        session = FakeSession([FakeResponse(401, "")])
        with self.assertRaises(ICloudAuthenticationError):
            CardDAVClient("user", "bad", session=session).discover()

    def test_reauthenticates_only_for_trusted_icloud_redirects(self):
        principal = '<d:multistatus xmlns:d="DAV:"><d:response><d:propstat><d:prop><d:current-user-principal><d:href>/p/1/</d:href></d:current-user-principal></d:prop></d:propstat></d:response></d:multistatus>'
        home = '<d:multistatus xmlns:d="DAV:" xmlns:c="urn:ietf:params:xml:ns:carddav"><d:response><d:propstat><d:prop><c:addressbook-home-set><d:href>/a/1/</d:href></c:addressbook-home-set></d:prop></d:propstat></d:response></d:multistatus>'
        books = '<d:multistatus xmlns:d="DAV:" xmlns:c="urn:ietf:params:xml:ns:carddav"><d:response><d:href>/a/1/book/</d:href><d:propstat><d:prop><d:resourcetype><c:addressbook/></d:resourcetype></d:prop></d:propstat></d:response></d:multistatus>'
        session = FakeSession([FakeResponse(301, "", {"Location": "https://p01-contacts.icloud.com/"}),
                               FakeResponse(207, principal), FakeResponse(207, home), FakeResponse(207, books)])
        CardDAVClient("user", "secret", session=session).discover()
        self.assertEqual(session.requests[1][1], "https://p01-contacts.icloud.com/")

    def test_creates_contact_with_create_only_put(self):
        session = FakeSession([FakeResponse(201, "", {"ETag": '"new"'})])
        created = CardDAVClient("user", "secret", session=session).create_contact(
            "https://p01-contacts.icloud.com/book/",
            {"id": "person-1", "name": "Jane Doe", "email": "jane@example.com"})
        method, url, options = session.requests[0]
        self.assertEqual(method, "PUT")
        self.assertTrue(url.endswith(".vcf"))
        self.assertEqual(options["headers"]["If-None-Match"], "*")
        self.assertIn(b"FN:Jane Doe", options["data"])
        self.assertEqual(created.etag, '"new"')


class ImportTests(unittest.TestCase):
    def test_existing_person_is_overwritten_by_id(self):
        pn = FakeProjectNotes([{"id": "person-1", "name": "Jane Doe", "email": "old@example.com",
                                "office_phone": "", "cell_phone": "", "client_id": "", "role": ""}])
        importer = ProjectNotesContactImporter(pn, "user")
        contact = Contact("uid", "/1.vcf", name="Jane Doe", email="new@example.com")
        stats = importer.import_contacts([contact], {})
        self.assertEqual(stats.updated, 1)
        self.assertIn('<column name="id">person-1</column>', pn.updates[0])
        self.assertIn("new@example.com", pn.updates[0])

    def test_duplicate_names_are_skipped(self):
        pn = FakeProjectNotes()
        importer = ProjectNotesContactImporter(pn, "user")
        contacts = [Contact("1", "/1", name="Same Name"), Contact("2", "/2", name="Same Name")]
        stats = importer.import_contacts(contacts, {})
        self.assertEqual(stats.conflicted, 2)
        self.assertFalse(pn.updates)


class SyncTests(unittest.TestCase):
    def test_exports_only_people_not_found_by_name(self):
        pn = FakeProjectNotes([
            {"id": "person-1", "name": "Jane Doe", "email": "jane@example.com"},
            {"id": "person-2", "name": "New Person", "email": "new@example.com"},
        ])

        class FakeClient:
            def __init__(self):
                self.created = []

            def discover(self):
                return [{"url": "https://icloud.test/book/", "name": "Contacts", "sync_token": "t"}]

            def fetch_addressbook(self, url, token):
                return [Contact("uid-1", url + "jane.vcf", name="Jane Doe",
                                email="jane@example.com")], [], "t2"

            def create_contact(self, url, person):
                self.created.append(person)
                return Contact(person["id"], url + person["id"] + ".vcf", name=person["name"])

        with tempfile.TemporaryDirectory() as folder:
            sync = ICloudContactSync(pn, folder, "user", "secret")
            sync.client = FakeClient()
            stats = sync.run(export_new=True)
        self.assertEqual([person["name"] for person in sync.client.created], ["New Person"])
        self.assertEqual(stats.exported, 1)

    def test_missing_mapped_person_is_not_resurrected(self):
        pn = FakeProjectNotes()
        importer = ProjectNotesContactImporter(pn, "user")
        stats = importer.import_contacts([Contact("1", "/1", name="Deleted Person")],
                                         {__import__("hashlib").sha256(b"/1").hexdigest(): "deleted-id"})
        self.assertEqual(stats.skipped, 1)
        self.assertFalse(pn.updates)


if __name__ == "__main__":
    unittest.main()
