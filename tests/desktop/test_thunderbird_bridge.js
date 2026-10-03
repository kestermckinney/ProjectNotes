// SPDX-License-Identifier: GPL-3.0-only
// End-to-end native framing + local socket test with Thunderbird's API mocked.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const net = require("node:net");
const vm = require("node:vm");
const { spawn, execFileSync } = require("node:child_process");
const { once } = require("node:events");

async function main() {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), "pn-tb-"));
  const home = path.join(directory, "home"), runtime = path.join(directory, "runtime");
  const temporary = path.join(directory, "thunderbird-tmp");
  for (const folder of [home, runtime, temporary]) fs.mkdirSync(folder, { mode: 0o700 });
  const suffix = require("node:crypto").createHash("sha256").update(home).digest("hex").slice(0, 20);
  const socketPath = process.platform === "linux"
    ? path.join(runtime, "app/com.projectnotespro.ProjectNotes/contacts", suffix)
    : path.join(directory, "bridge.sock");
  const hostEnvironment = { ...process.env, HOME: home, XDG_RUNTIME_DIR: runtime, TMPDIR: temporary };
  const hostArguments = process.platform === "linux" ? [] : ["--test-socket", socketPath];
  // Optional real Flatpak smoke test. Permissions apply to this invocation only;
  // no user's Thunderbird settings or persistent overrides are changed.
  let installedLauncher;
  if (process.argv[4]) {
    const registration = JSON.parse(execFileSync("flatpak", ["run", "--persist=.mozilla",
      "--command=/usr/bin/cat", process.argv[4],
      path.join(os.homedir(), ".mozilla/native-messaging-hosts/org.projectnotes.contacts.json")], { encoding: "utf8" }));
    assert.equal(registration.name, "org.projectnotes.contacts");
    assert.ok(registration.path.includes(`/.var/app/${process.argv[4]}/`), "Thunderbird must discover its sandbox launcher, not the desktop helper path.");
    installedLauncher = registration.path;
  }
  function launchHost(environment) {
    if (installedLauncher) return spawn("flatpak", ["run", "--persist=.mozilla",
      "--talk-name=org.freedesktop.Flatpak", `--command=${installedLauncher}`,
      process.argv[4], "--test-socket", socketPath], { stdio: ["pipe", "pipe", "inherit"] });
    return spawn(process.argv[2], hostArguments, { env: environment, stdio: ["pipe", "pipe", "inherit"] });
  }
  const host = launchHost(hostEnvironment);
  const exited = once(host, "exit");
  let finishedTesting = false;
  host.on("exit", code => { if (!finishedTesting) { console.error(`Native host exited before protocol checks (code ${code}).`); process.exitCode = 1; } });
  let listener, buffer = Buffer.alloc(0), profile = "fixture-profile", created;
  const existing = { id: "alice", properties: { vCard: "BEGIN:VCARD\r\nVERSION:3.0\r\nUID:alice-uid\r\nFN:Alice Smith\r\nEMAIL;TYPE=WORK:alice@example.test\r\nEMAIL;TYPE=HOME:alice.home@example.test\r\nNOTE:Keep this\r\nEND:VCARD\r\n" } };
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
      async list() { return [{ id: "book", name: "Contacts", contacts: [existing] }]; },
      async get(id) { assert.equal(id, "book"); return { id, readOnly: false, remote: false }; }
    },
    contacts: {
      async create(book, properties) { assert.equal(book, "book"); assert.ok(properties.vCard.includes("UID:fixture-intent")); created = { id: "new-contact", properties }; return created.id; },
      async get(id) { if (id === existing.id) return existing; assert.equal(id, created.id); return created; },
      async update(id, properties) { assert.equal(id, existing.id); assert.ok(properties.vCard.includes("NOTE:Keep this")); assert.ok(properties.vCard.includes("UID:alice-uid")); existing.properties = properties; }
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
    const updated = await request({ action: "update", book: "book", existing: { id: "alice", book: "book", fields: { email: "alice@example.test" } }, contact: { name: "Alice Jones", company: "Acme", fields: { email: "alice.jones@example.test" } } });
    assert.ok(updated.contact, updated.error);
    assert.equal(updated.contact.id, "alice"); assert.equal(updated.contact.name, "Alice Jones"); assert.ok(existing.properties.vCard.includes("NOTE:Keep this"));
    assert.ok(existing.properties.vCard.includes("alice.home@example.test")); assert.ok(!existing.properties.vCard.includes("EMAIL;TYPE=WORK:alice@example.test"));
    const invalid = await request({ action: "delete" }); assert.ok(invalid.error);
    // A second host launched with another TMPDIR must neither steal the socket
    // nor remove it. This reproduces Thunderbird/Project Notes sandbox isolation.
    const second = launchHost({ ...hostEnvironment, TMPDIR: directory });
    const [secondCode] = await once(second, "exit");
    assert.equal(secondCode, 1);
    assert.equal((await request({ action: "snapshot" })).account, profile);
    console.log(installedLauncher
      ? "Thunderbird Flatpak bridge: registration mapping, installed launcher, protocol, and exclusive socket ownership passed."
      : "Thunderbird native bridge: protocol, separate TMPDIR, and exclusive socket ownership passed.");
  } finally {
    finishedTesting = true;
    host.stdin.end();
    await exited;
    fs.rmSync(directory, { recursive: true, force: true });
  }
}
main().catch(error => { console.error(error); process.exit(1); });
