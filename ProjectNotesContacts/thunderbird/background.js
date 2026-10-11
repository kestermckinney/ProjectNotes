// SPDX-License-Identifier: GPL-3.0-only
/* Thunderbird is the only owner of its address-book databases. */
let port;
const escapeCard = value => String(value || "").replace(/\\/g, "\\\\").replace(/\r?\n/g, "\\n").replace(/;/g, "\\;").replace(/,/g, "\\,");
function property(card, name) {
  const lines = String(card || "").replace(/\r?\n[ \t]/g, "").split(/\r?\n/);
  return lines.filter(line => line.split(/[;:]/, 1)[0].split(".").pop().toUpperCase() === name).map(line => {
    const split = line.indexOf(":");
    return { type: line.slice(0, split).toUpperCase(), value: line.slice(split + 1).replace(/\\([nN,;\\])/g, (_, c) => /n/i.test(c) ? " " : c) };
  });
}
function contact(node, book) {
  const p = node.properties || {}, card = p.vCard || node.vCard || "";
  const emails = property(card, "EMAIL"), phones = property(card, "TEL");
  const name = p.DisplayName || property(card, "FN")[0]?.value || [p.FirstName, p.LastName].filter(Boolean).join(" ");
  const company = p.Company || property(card, "ORG")[0]?.value || "";
  const email = emails.find(e => e.type.includes("WORK"))?.value || p.PrimaryEmail || emails[0]?.value || "";
  const office = phones.find(e => e.type.includes("WORK"))?.value || p.WorkPhone || "";
  const cell = phones.find(e => /CELL|MOBILE/.test(e.type))?.value || p.CellularNumber || "";
  const fields = { name };
  if (Object.hasOwn(p, "Company") || property(card, "ORG").length) fields.company = company;
  if (Object.hasOwn(p, "PrimaryEmail") || emails.length) fields.email = email;
  if (Object.hasOwn(p, "WorkPhone") || phones.some(e => e.type.includes("WORK"))) fields.office_phone = office;
  if (Object.hasOwn(p, "CellularNumber") || phones.some(e => /CELL|MOBILE/.test(e.type))) fields.cell_phone = cell;
  if (Object.hasOwn(p, "JobTitle") || property(card, "TITLE").length) fields.role = p.JobTitle || property(card, "TITLE")[0]?.value || "";
  return { id: node.id, book, name, company, emails: [...new Set([email, p.SecondEmail, ...emails.map(e => e.value)].filter(Boolean))],
    phones: [...new Set([office, cell, ...phones.map(e => e.value)].filter(Boolean))],
    fields };
}
function mergeCard(card, c, previous) {
  const f = c.fields || {}, unfolded = String(card || "").replace(/\r?\n[ \t]/g, "");
  const old = previous.fields || {}, digits = value => String(value || "").replace(/[^0-9]/g, "");
  const kept = unfolded.split(/\r?\n/).filter(line => {
    const colon = line.indexOf(":"), left = line.slice(0, Math.max(0, colon)).toUpperCase();
    const key = left.split(";", 1)[0].split(".").pop();
    const value = colon < 0 ? "" : line.slice(colon + 1).replace(/\\([nN,;\\])/g, (_, ch) => /n/i.test(ch) ? " " : ch);
    if (["BEGIN", "END", "FN", "N", "ORG", "TITLE"].includes(key)) return false;
    if (key === "EMAIL" && left.includes("WORK") && old.email && value.toLowerCase() === String(old.email).trim().toLowerCase()) return false;
    if (key === "TEL" && left.includes("WORK") && digits(old.office_phone) && digits(value) === digits(old.office_phone)) return false;
    if (key === "TEL" && /CELL|MOBILE/.test(left) && digits(old.cell_phone) && digits(value) === digits(old.cell_phone)) return false;
    return !!line;
  });
  if (!kept.some(line => /^VERSION:/i.test(line))) kept.unshift("VERSION:3.0");
  const managed = ["FN:" + escapeCard(c.name), "N:;" + escapeCard(c.name) + ";;;",
    "ORG:" + escapeCard(c.company), "TITLE:" + escapeCard(f.role), "EMAIL;TYPE=WORK:" + escapeCard(f.email),
    "TEL;TYPE=WORK:" + escapeCard(f.office_phone), "TEL;TYPE=CELL:" + escapeCard(f.cell_phone)].filter(line => !line.endsWith(":"));
  return ["BEGIN:VCARD", ...kept, ...managed, "END:VCARD", ""].join("\r\n");
}
async function handle(request) {
  if (request.version !== 1) throw new Error("Incompatible Project Notes bridge version.");
  let { profileId } = await messenger.storage.local.get("profileId");
  if (!profileId) { profileId = crypto.randomUUID(); await messenger.storage.local.set({ profileId }); }
  if (request.action === "snapshot") {
    const books = await messenger.addressBooks.list(true), contacts = [];
    for (const book of books) for (const node of book.contacts || []) contacts.push(contact(node, book.id));
    return { account: profileId, books: books.map(b => ({ id: b.id, name: b.name, writable: !b.readOnly && !b.remote })), contacts };
  }
  if (request.action === "create") {
    const book = await messenger.addressBooks.get(request.book);
    if (book.readOnly || book.remote) throw new Error("Choose a writable local Thunderbird address book.");
    const c = request.contact, f = c.fields;
    const lines = ["BEGIN:VCARD", "VERSION:3.0", "UID:" + escapeCard(request.intentId), "FN:" + escapeCard(c.name), "N:;" + escapeCard(c.name) + ";;;",
      "ORG:" + escapeCard(c.company), "TITLE:" + escapeCard(f.role), "EMAIL;TYPE=WORK:" + escapeCard(f.email),
      "TEL;TYPE=WORK:" + escapeCard(f.office_phone), "TEL;TYPE=CELL:" + escapeCard(f.cell_phone), "END:VCARD", ""];
    const id = await messenger.contacts.create(request.book, { vCard: lines.join("\r\n") });
    return { contact: contact(await messenger.contacts.get(id), request.book) };
  }
  if (request.action === "update") {
    const book = await messenger.addressBooks.get(request.book);
    if (book.readOnly || book.remote) throw new Error("The selected Thunderbird address book is not writable.");
    const node = await messenger.contacts.get(request.existing.id);
    await messenger.contacts.update(request.existing.id, { vCard: mergeCard(node.properties?.vCard || node.vCard, request.contact, request.existing) });
    return { contact: contact(await messenger.contacts.get(request.existing.id), request.book) };
  }
  throw new Error("Unsupported contact bridge action.");
}
function connect() {
  port = messenger.runtime.connectNative("org.projectnotes.contacts");
  port.onMessage.addListener(async request => {
    let response;
    try { response = await handle(request); } catch (error) { response = { error: String(error.message || error) }; }
    try { port.postMessage({ ...response, version: 1, requestId: request.requestId }); } catch (_) { /* Host disconnected. */ }
  });
  port.onDisconnect.addListener(() => { setTimeout(connect, 15000); });
}
connect();
