// SPDX-License-Identifier: GPL-3.0-only
// End-to-end native framing + local socket test with Thunderbird's API mocked.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const net = require("node:net");
const vm = require("node:vm");
const { spawn } = require("node:child_process");
const { once } = require("node:events");

async function main() {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), "projectnotes-contact-bridge-"));
  const socketPath = path.join(directory, "bridge.sock");
  const host = spawn(process.argv[2], ["--test-socket", socketPath], { stdio: ["pipe", "pipe", "inherit"] });
  const exited = once(host, "exit");
  let finishedTesting = false;
  host.on("exit", code => { if (!finishedTesting) { console.error(`Native host exited before protocol checks (code ${code}).`); process.exitCode = 1; } });
  let listener, buffer = Buffer.alloc(0), profile = "fixture-profile", created;
  const port = {
    onMessage: { addListener(fn) { listener = fn; } },
    onDisconnect: { addListener() {} },
    postMessage(object) {
      const bytes = Buffer.from(JSON.stringify(object));
      const header = Buffer.alloc(4); header.writeUInt32LE(bytes.length);
      host.stdin.write(Buffer.concat([header, bytes]));
    }
  };
  const messenger = {
    runtime: { connectNative(name) { assert.equal(name, "org.projectnotes.contacts"); return port; } },
    storage: { local: { async get() { return { profileId: profile }; }, async set(value) { profile = value.profileId; } } },
    addressBooks: {
      async list() { return [{ id: "book", name: "Contacts", contacts: [{ id: "alice", properties: { vCard: "BEGIN:VCARD\r\nFN:Alice Smith\r\nEMAIL;TYPE=WORK:alice@example.test\r\nEND:VCARD\r\n" } }] }]; },
      async get(id) { assert.equal(id, "book"); return { id, readOnly: false, remote: false }; }
    },
    contacts: {
      async create(book, properties) { assert.equal(book, "book"); assert.ok(properties.vCard.includes("UID:fixture-intent")); created = { id: "new-contact", properties }; return created.id; },
      async get(id) { assert.equal(id, created.id); return created; }
    }
  };
  vm.runInNewContext(fs.readFileSync(process.argv[3], "utf8"), { messenger, crypto: require("node:crypto"), setTimeout });
  host.stdout.on("data", chunk => {
    buffer = Buffer.concat([buffer, chunk]);
    while (buffer.length >= 4 && buffer.length >= 4 + buffer.readUInt32LE()) {
      const length = buffer.readUInt32LE();
      const message = JSON.parse(buffer.subarray(4, length + 4));
      buffer = buffer.subarray(length + 4);
      listener(message);
    }
  });
  async function request(object) {
    let socket;
    for (let attempt = 0; attempt < 100; ++attempt) {
      socket = net.createConnection(socketPath);
      try { await once(socket, "connect"); break; } catch (_) { socket.destroy(); await new Promise(resolve => setTimeout(resolve, 20)); }
    }
    if (!socket || socket.destroyed || socket.connecting) throw new Error("Could not connect to the native bridge socket.");
    const response = new Promise((resolve, reject) => {
      let body = "";
      socket.on("data", chunk => { body += chunk; if (body.includes("\n")) resolve(JSON.parse(body.trim())); });
      socket.on("error", reject);
      socket.setTimeout(5000, () => reject(new Error("Native bridge timed out")));
    });
    socket.write(JSON.stringify({ version: 1, ...object }) + "\n");
    try { return await response; } finally { socket.destroy(); }
  }
  try {
    const snapshot = await request({ action: "snapshot" });
    assert.equal(snapshot.version, 1); assert.equal(snapshot.account, profile);
    assert.equal(snapshot.contacts[0].fields.email, "alice@example.test");
    assert.equal(snapshot.contacts[0].fields.company, undefined);
    const exported = await request({ action: "create", book: "book", intentId: "fixture-intent", contact: { name: "Jane; Smith", company: "Acme", fields: { email: "jane@example.test" } } });
    assert.equal(exported.contact.id, "new-contact"); assert.equal(exported.contact.name, "Jane; Smith");
    const invalid = await request({ action: "delete" }); assert.ok(invalid.error);
    console.log("Thunderbird native bridge: snapshot, create, omitted fields, and invalid requests passed.");
  } finally {
    finishedTesting = true;
    host.stdin.end();
    await exited;
    fs.rmSync(directory, { recursive: true, force: true });
  }
}
main().catch(error => { console.error(error); process.exit(1); });
