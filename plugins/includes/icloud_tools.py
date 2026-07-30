# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only

"""CardDAV and vCard support for the Project Notes iCloud importer."""

from __future__ import annotations

import base64
import hashlib
import json
import os
import quopri
import re
import tempfile
import uuid
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from urllib.parse import urljoin, urlparse

import requests


DAV = "DAV:"
CARD = "urn:ietf:params:xml:ns:carddav"
APPLE_SERVICE = "Project Notes iCloud Contacts"
DEFAULT_ENDPOINT = "https://contacts.icloud.com/"
UUID_NAMESPACE = uuid.UUID("42b5bfbc-0ee7-4df2-a1a5-f379de3fe950")


class ICloudError(RuntimeError):
    pass


class ICloudAuthenticationError(ICloudError):
    pass


@dataclass
class Contact:
    external_id: str
    href: str
    etag: str = ""
    name: str = ""
    company: str = ""
    email: str = ""
    office_phone: str = ""
    cell_phone: str = ""
    role: str = ""


@dataclass
class ImportStats:
    created: int = 0
    updated: int = 0
    unchanged: int = 0
    skipped: int = 0
    conflicted: int = 0
    exported: int = 0
    errors: list[str] = field(default_factory=list)

    def summary(self):
        return (f"Imported {self.created} new, updated {self.updated}, unchanged {self.unchanged}, "
                f"skipped {self.skipped}, conflicts {self.conflicted}, exported {self.exported} new")


def _unescape(value):
    out = []
    escaped = False
    for char in value:
        if escaped:
            out.append(" " if char in "nN" else char)
            escaped = False
        elif char == "\\":
            escaped = True
        else:
            out.append(char)
    if escaped:
        out.append("\\")
    return "".join(out).strip()


def _split_unescaped(value, separator=";"):
    result, part, escaped = [], [], False
    for char in value:
        if escaped:
            part.extend(("\\", char))
            escaped = False
        elif char == "\\":
            escaped = True
        elif char == separator:
            result.append(_unescape("".join(part)))
            part = []
        else:
            part.append(char)
    if escaped:
        part.append("\\")
    result.append(_unescape("".join(part)))
    return result


def _unfold_vcard(text):
    physical = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    logical = []
    for line in physical:
        if line.startswith((" ", "\t")) and logical:
            if logical[-1].endswith("="):
                logical[-1] = logical[-1][:-1]
            logical[-1] += line[1:]
        elif logical and logical[-1].endswith("="):
            logical[-1] += line
        else:
            logical.append(line)
    return logical


def parse_vcard(text, href="", etag=""):
    """Parse the subset of vCard 3/4 represented by PN's people model."""
    properties = []
    for line in _unfold_vcard(text):
        if ":" not in line:
            continue
        left, value = line.split(":", 1)
        parts = left.split(";")
        name = parts[0].split(".")[-1].upper()
        params = {}
        bare_types = []
        for item in parts[1:]:
            if "=" in item:
                key, param_value = item.split("=", 1)
                params[key.upper()] = param_value.strip('"')
            else:
                bare_types.append(item)
        types = set(bare_types)
        types.update(re.split(r"[,]", params.get("TYPE", "")))
        types = {item.strip().upper() for item in types if item.strip()}
        encoding = params.get("ENCODING", "").upper()
        charset = params.get("CHARSET", "utf-8")
        try:
            if encoding in ("QUOTED-PRINTABLE", "QP"):
                value = quopri.decodestring(value).decode(charset, errors="replace")
            elif encoding in ("B", "BASE64") and name not in ("PHOTO", "LOGO", "SOUND"):
                value = base64.b64decode(value).decode(charset, errors="replace")
        except (LookupError, ValueError):
            pass
        properties.append((name, types, _unescape(value)))

    def values(prop):
        return [(types, value) for name, types, value in properties if name == prop and value]

    uid_values = values("UID")
    fn_values = values("FN")
    name = fn_values[0][1].strip() if fn_values else ""
    if not name:
        structured = values("N")
        if structured:
            bits = _split_unescaped(structured[0][1])
            name = " ".join(bit for bit in (bits[1] if len(bits) > 1 else "", bits[0]) if bit).strip()

    def preferred(prop, wanted=(), fallback=True):
        entries = values(prop)
        for label in wanted:
            for types, value in entries:
                if label in types:
                    return value
        return entries[0][1] if entries and fallback else ""

    org = values("ORG")
    title = values("TITLE")
    external_id = uid_values[0][1] if uid_values else href
    return Contact(
        external_id=external_id,
        href=href,
        etag=etag,
        name=name,
        company=_split_unescaped(org[0][1])[0] if org else "",
        email=preferred("EMAIL", ("WORK", "PREF")),
        office_phone=preferred("TEL", ("WORK",)),
        cell_phone=preferred("TEL", ("CELL", "MOBILE"), fallback=False),
        role=title[0][1] if title else "",
    )


class CardDAVClient:
    def __init__(self, username, password, session=None, endpoint=DEFAULT_ENDPOINT,
                 timeout=(10, 45)):
        self.username = username.strip()
        self.endpoint = endpoint
        self.timeout = timeout
        self.session = session or requests.Session()
        self.session.auth = (self.username, password)
        self.session.headers.update({"User-Agent": "ProjectNotes-iCloudContacts/1.0"})

    def _request(self, method, url, body=None, depth=None):
        headers = {"Content-Type": "application/xml; charset=utf-8"}
        if depth is not None:
            headers["Depth"] = str(depth)
        try:
            response = None
            request_url = url
            for _ in range(5):
                response = self.session.request(method, request_url, data=body, headers=headers,
                                                timeout=self.timeout, allow_redirects=False)
                if response.status_code not in (301, 302, 307, 308):
                    break
                location = response.headers.get("Location", "")
                redirected = urljoin(request_url, location)
                old_host = (urlparse(request_url).hostname or "").lower()
                new_parts = urlparse(redirected)
                new_host = (new_parts.hostname or "").lower()
                trusted = (new_parts.scheme == "https" and
                           (new_host == old_host or
                            (old_host.endswith(".icloud.com") and new_host.endswith(".icloud.com"))))
                if not location or not trusted:
                    raise ICloudError("iCloud returned an unsafe CardDAV redirect.")
                request_url = redirected
            else:
                raise ICloudError("iCloud returned too many CardDAV redirects.")
        except requests.RequestException as exc:
            raise ICloudError(f"Unable to reach iCloud Contacts: {exc}") from exc
        if response.status_code == 401:
            raise ICloudAuthenticationError(
                "iCloud rejected the Apple Account or app-specific password.")
        if response.status_code not in (200, 207):
            raise ICloudError(f"iCloud CardDAV returned HTTP {response.status_code}.")
        try:
            return response, ET.fromstring(response.content)
        except ET.ParseError as exc:
            raise ICloudError("iCloud returned malformed CardDAV XML.") from exc

    @staticmethod
    def _href(root, path):
        node = root.find(path)
        return (node.text or "").strip() if node is not None else ""

    def discover(self):
        body = (f'<d:propfind xmlns:d="{DAV}"><d:prop><d:current-user-principal/>'
                '</d:prop></d:propfind>')
        _, root = self._request("PROPFIND", self.endpoint, body, 0)
        principal = self._href(root, f".//{{{DAV}}}current-user-principal/{{{DAV}}}href")
        if not principal:
            raise ICloudError("iCloud did not return a CardDAV principal.")
        principal_url = urljoin(self.endpoint, principal)
        body = (f'<d:propfind xmlns:d="{DAV}" xmlns:c="{CARD}"><d:prop>'
                '<c:addressbook-home-set/></d:prop></d:propfind>')
        _, root = self._request("PROPFIND", principal_url, body, 0)
        home = self._href(root, f".//{{{CARD}}}addressbook-home-set/{{{DAV}}}href")
        if not home:
            raise ICloudError("iCloud did not return an address-book home.")
        home_url = urljoin(principal_url, home)
        body = (f'<d:propfind xmlns:d="{DAV}"><d:prop><d:resourcetype/>'
                '<d:displayname/><d:sync-token/></d:prop></d:propfind>')
        _, root = self._request("PROPFIND", home_url, body, 1)
        books = []
        for response in root.findall(f"{{{DAV}}}response"):
            resource = response.find(f".//{{{DAV}}}resourcetype")
            if resource is None or resource.find(f"{{{CARD}}}addressbook") is None:
                continue
            href = self._href(response, f"{{{DAV}}}href")
            token = self._href(response, f".//{{{DAV}}}sync-token")
            display = self._href(response, f".//{{{DAV}}}displayname")
            books.append({"url": urljoin(home_url, href), "name": display or href, "sync_token": token})
        if not books:
            raise ICloudError("No iCloud CardDAV address books were found.")
        return books

    def _contacts_from_multistatus(self, root, book_url):
        contacts, deleted = [], []
        for response in root.findall(f"{{{DAV}}}response"):
            href = self._href(response, f"{{{DAV}}}href")
            status = self._href(response, f"{{{DAV}}}status")
            if " 404 " in status:
                deleted.append(urljoin(book_url, href))
                continue
            card = response.find(f".//{{{CARD}}}address-data")
            if card is None or not card.text:
                continue
            etag = self._href(response, f".//{{{DAV}}}getetag")
            contact = parse_vcard(card.text, urljoin(book_url, href), etag)
            if contact.external_id:
                contacts.append(contact)
        token = self._href(root, f"{{{DAV}}}sync-token")
        return contacts, deleted, token

    def fetch_addressbook(self, book_url, sync_token=""):
        if sync_token:
            body = (f'<d:sync-collection xmlns:d="{DAV}" xmlns:c="{CARD}"><d:sync-token>'
                    f'{_xml(sync_token)}</d:sync-token><d:sync-level>1</d:sync-level><d:prop>'
                    '<d:getetag/><c:address-data/></d:prop></d:sync-collection>')
            try:
                _, root = self._request("REPORT", book_url, body, 1)
                return self._contacts_from_multistatus(root, book_url)
            except ICloudAuthenticationError:
                raise
            except ICloudError:
                pass
        body = (f'<c:addressbook-query xmlns:d="{DAV}" xmlns:c="{CARD}"><d:prop>'
                '<d:getetag/><c:address-data/></d:prop><c:filter>'
                '<c:prop-filter name="UID"/></c:filter></c:addressbook-query>')
        _, root = self._request("REPORT", book_url, body, 1)
        return self._contacts_from_multistatus(root, book_url)

    def test_connection(self):
        return len(self.discover())

    def create_contact(self, book_url, person):
        person_id = str(person.get("id") or uuid.uuid4())
        href = urljoin(book_url.rstrip("/") + "/", f"{uuid.uuid5(UUID_NAMESPACE, person_id)}.vcf")
        card = serialize_vcard(person, person_id)
        headers = {"Content-Type": "text/vcard; charset=utf-8", "If-None-Match": "*"}
        try:
            response = self.session.request("PUT", href, data=card.encode("utf-8"), headers=headers,
                                            timeout=self.timeout, allow_redirects=False)
        except requests.RequestException as exc:
            raise ICloudError(f"Unable to export a contact to iCloud: {exc}") from exc
        if response.status_code == 401:
            raise ICloudAuthenticationError(
                "iCloud rejected the Apple Account or app-specific password.")
        if response.status_code not in (201, 204):
            raise ICloudError(f"iCloud rejected contact '{person.get('name', '')}' "
                              f"with HTTP {response.status_code}.")
        return Contact(person_id, href, response.headers.get("ETag", ""),
                       person.get("name", ""), person.get("client_id", ""),
                       person.get("email", ""), person.get("office_phone", ""),
                       person.get("cell_phone", ""), person.get("role", ""))


def _xml(value):
    return (str(value or "").replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;").replace("'", "&apos;"))


def _vcard(value):
    return (str(value or "").replace("\\", "\\\\").replace("\n", "\\n")
            .replace(";", "\\;").replace(",", "\\,"))


def serialize_vcard(person, uid):
    name = _vcard(person.get("name", ""))
    lines = ["BEGIN:VCARD", "VERSION:3.0", f"UID:{_vcard(uid)}", f"FN:{name}", f"N:;{name};;;" ]
    fields = (("ORG", person.get("client_id", "")),
              ("TITLE", person.get("role", "")),
              ("EMAIL;TYPE=WORK", person.get("email", "")),
              ("TEL;TYPE=WORK", person.get("office_phone", "")),
              ("TEL;TYPE=CELL", person.get("cell_phone", "")))
    lines.extend(f"{field}:{_vcard(value)}" for field, value in fields if value)
    lines.extend(("PRODID:-//Project Notes//iCloud Contacts//EN", "END:VCARD", ""))
    return "\r\n".join(lines)


class StateStore:
    def __init__(self, cache_dir, database_path, account):
        identity = hashlib.sha256(f"{os.path.abspath(database_path)}\0{account.lower()}".encode()).hexdigest()
        self.path = os.path.join(cache_dir, f"icloud_contacts_{identity}.json")

    def load(self):
        try:
            with open(self.path, "r", encoding="utf-8") as stream:
                value = json.load(stream)
                return value if isinstance(value, dict) else {}
        except (OSError, ValueError):
            return {}

    def save(self, state):
        os.makedirs(os.path.dirname(self.path), exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix="icloud_contacts_", suffix=".tmp",
                                         dir=os.path.dirname(self.path))
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as stream:
                json.dump(state, stream, ensure_ascii=False, indent=2, sort_keys=True)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


class ProjectNotesContactImporter:
    def __init__(self, projectnotes_module, source_namespace=""):
        self.projectnotes = projectnotes_module
        self.source_namespace = source_namespace

    def current_people(self):
        request = '<?xml version="1.0"?><projectnotes><table name="people" top="100000" /></projectnotes>'
        xml = self.projectnotes.get_data(request)
        root = ET.fromstring(xml)
        people = {}
        for row in root.findall(".//table[@name='people']/row"):
            values = {}
            for column in row.findall("column"):
                values[column.get("name")] = column.get("lookupvalue") or (column.text or "")
            if values.get("name"):
                people[values["name"].strip()] = values
        return people, root.get("filepath", "")

    @staticmethod
    def _record(contact, person_id):
        return {"id": person_id, "name": contact.name.strip(), "email": contact.email,
                "office_phone": contact.office_phone, "cell_phone": contact.cell_phone,
                "client_id": contact.company, "role": contact.role}

    def import_contacts(self, contacts, links):
        stats = ImportStats()
        people, _ = self.current_people()
        ids = {record.get("id"): name for name, record in people.items()}
        incoming_names = {}
        for contact in contacts:
            incoming_names.setdefault(contact.name.strip(), []).append(contact)

        client_rows, person_rows = {}, []
        pending_links = {}
        for contact in contacts:
            name = contact.name.strip()
            if not name:
                stats.skipped += 1
                continue
            if len(incoming_names.get(name, [])) > 1:
                stats.conflicted += 1
                continue
            source_key = hashlib.sha256(contact.href.encode("utf-8")).hexdigest()
            person_id = links.get(source_key)
            if person_id and person_id not in ids:
                # PN does not export soft-deleted people. Never send their known id
                # back through the importer, which could otherwise look like a create.
                stats.skipped += 1
                continue
            if person_id and person_id in ids and name in people and people[name].get("id") != person_id:
                stats.conflicted += 1
                continue
            if not person_id:
                existing = people.get(name)
                identity = f"{self.source_namespace}\0{contact.href}\0{contact.external_id}"
                person_id = existing.get("id") if existing else str(uuid.uuid5(UUID_NAMESPACE, identity))
            elif name in people and people[name].get("id") != person_id:
                stats.conflicted += 1
                continue

            record = self._record(contact, person_id)
            existing = next((value for value in people.values() if value.get("id") == person_id), None)
            if existing and all((existing.get(key, "") or "") == (value or "")
                                for key, value in record.items() if key != "id"):
                stats.unchanged += 1
                pending_links[source_key] = person_id
                continue
            if contact.company:
                client_rows[contact.company] = contact.company
            person_rows.append(record)
            stats.updated += 1 if existing else 0
            stats.created += 0 if existing else 1
            pending_links[source_key] = person_id

        if person_rows:
            clients_xml = "".join(
                f'<row><column name="client_name">{_xml(name)}</column></row>' for name in client_rows)
            people_xml = "".join(
                '<row>'
                f'<column name="id">{_xml(row["id"])}</column>'
                f'<column name="name">{_xml(row["name"])}</column>'
                f'<column name="email">{_xml(row["email"])}</column>'
                f'<column name="office_phone">{_xml(row["office_phone"])}</column>'
                f'<column name="cell_phone">{_xml(row["cell_phone"])}</column>'
                + (f'<column name="client_id" lookupvalue="{_xml(row["client_id"])}"></column>'
                   if row["client_id"] else '<column name="client_id"></column>')
                + f'<column name="role">{_xml(row["role"])}</column></row>' for row in person_rows)
            document = ('<?xml version="1.0" encoding="UTF-8"?><projectnotes>'
                        + (f'<table name="clients">{clients_xml}</table>' if clients_xml else "")
                        + f'<table name="people">{people_xml}</table></projectnotes>')
            if not self.projectnotes.update_data(document):
                raise ICloudError("Project Notes rejected the imported contact data.")
        links.update(pending_links)
        return stats


class ICloudContactSync:
    def __init__(self, projectnotes_module, cache_dir, username, password, session=None):
        self.projectnotes = projectnotes_module
        self.cache_dir = cache_dir
        self.client = CardDAVClient(username, password, session=session)
        self.username = username

    def run(self, export_new=False):
        importer = ProjectNotesContactImporter(self.projectnotes, self.username.lower())
        _, database_path = importer.current_people()
        if not database_path:
            raise ICloudError("Open a Project Notes database before importing contacts.")
        state_store = StateStore(self.cache_dir, database_path, self.username)
        state = state_store.load()
        state.setdefault("books", {})
        state.setdefault("links", {})
        contacts = []
        books = self.client.discover()
        for book in books:
            book_state = state["books"].setdefault(book["url"], {})
            changed, _deleted, token = self.client.fetch_addressbook(
                book["url"], "" if export_new else book_state.get("sync_token", ""))
            contacts.extend(changed)
            book_state["sync_token"] = token or book.get("sync_token", "")
        stats = importer.import_contacts(contacts, state["links"])
        if export_new:
            people, _ = importer.current_people()
            remote_names = {contact.name.strip().casefold() for contact in contacts if contact.name.strip()}
            target = next((book for book in books if book["name"].strip().casefold() == "contacts"),
                          books[0])
            for name, person in people.items():
                normalized = name.strip().casefold()
                if not normalized or normalized in remote_names:
                    continue
                exported = self.client.create_contact(target["url"], person)
                state["links"][hashlib.sha256(exported.href.encode("utf-8")).hexdigest()] = person.get("id", "")
                remote_names.add(normalized)
                stats.exported += 1
        state_store.save(state)
        return stats
