// Project email archiving: Microsoft Graph (as .eml) and Outlook classic over
// COM (as .msg). File names and the per-folder download_state.json match the
// Python plugin, so mail it already archived is not downloaded again.
import * as pn from "projectnotes";

const { data, files, log, office365, projectFiles, storage } = pn;

// The Python plugin's make_filename(): ISO timestamp without punctuation, a
// dash, the subject with unsafe characters replaced, 70 characters at most.
export function makeFilename(dateTime, subject) {
    const iso = String(dateTime || "").replace(" ", "T");
    const id = iso.replace(/[-`!@#$%^&*()+\\|{}/';:<>,.~?"\][ ]/g, "");
    const name = String(subject || "").replace(/[`!@#$%^&*()+\\|{}/';:<>,.~?"\][]/g, "_");
    return (id + "-" + name).slice(0, 70);
}

function wrap76(text) {
    const lines = [];
    for (let i = 0; i < text.length; i += 76) lines.push(text.slice(i, i + 76));
    return lines.join("\n");
}

function mailDate(iso) {
    const d = new Date(iso);
    if (isNaN(d.getTime())) return "";
    const days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
    const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    const pad = n => ("0" + n).slice(-2);
    return days[d.getUTCDay()] + ", " + pad(d.getUTCDate()) + " " + months[d.getUTCMonth()] + " " + d.getUTCFullYear()
        + " " + pad(d.getUTCHours()) + ":" + pad(d.getUTCMinutes()) + ":" + pad(d.getUTCSeconds()) + " +0000";
}

function addresses(list) {
    return (list || []).map(function (r) {
        const a = r.emailAddress || {};
        return a.address ? '"' + (a.name || "") + '" <' + a.address + ">" : "";
    }).filter(Boolean).join(", ");
}

// A Graph message (with $expand=attachments) as an .eml file.
export function toEml(m) {
    const attachments = m.attachments || [];
    const boundary = String(m.internetMessageId || m.id || "message").replace(/[@.<>-]/g, "").toUpperCase();
    const msgBoundary = "_MSG_" + boundary + "_";
    const attBoundary = "_ATT_" + boundary + "_";
    let eml = "";
    if (m.internetMessageId) eml += "Received: from " + m.internetMessageId + "\n";
    const from = m.from && m.from.emailAddress;
    if (from && from.address) eml += 'From: "' + (from.name || "") + '" <' + from.address + ">\n";
    const to = addresses(m.toRecipients), cc = addresses(m.ccRecipients), bcc = addresses(m.bccRecipients);
    if (to) eml += "To: " + to + "\n";
    if (cc) eml += "CC: " + cc + "\n";
    if (bcc) eml += "BCC: " + bcc + "\n";
    if (m.subject) eml += "Subject: " + m.subject + "\n";
    if (m.internetMessageId) eml += "Message-ID: " + m.internetMessageId + "\n";
    const replyTo = addresses(m.replyTo);
    if (replyTo) eml += "In-Reply-To: " + replyTo + "\n";
    if (m.conversationId) eml += "Thread-Index: " + m.conversationId + "\n";
    eml += "Date: " + mailDate(m.sentDateTime) + "\n";
    if (attachments.length) {
        eml += "X-MS-Has-Attach: yes\nContent-Type: multipart/related;\n\tboundary=\"" + attBoundary + "\";\n"
             + "\ttype=\"multipart/alternative\"\nMIME-Version: 1.0\n\n--" + attBoundary + "\n"
             + "Content-Type: multipart/alternative;\n\tboundary=\"" + msgBoundary + "\"\n\n";
    } else {
        eml += "X-MS-Has-Attach: no\nContent-Type: multipart/mixed;\n\tboundary=\"" + msgBoundary + "\"\nMIME-Version: 1.0\n\n";
    }
    const body = (m.body && m.body.content) || "";
    eml += "--" + msgBoundary + "\nContent-Type: text/html; charset=\"utf-8\"\n\n" + body + "\n\n--" + msgBoundary + "--\n\n";
    attachments.forEach(function (a) {
        eml += "--" + attBoundary + "\n";
        eml += "Content-Type: " + (a.contentType || "application/octet-stream") + "; \n\tname=\"" + (a.name || "") + "\"\n";
        eml += "Content-Description: " + (a.name || "") + "\n";
        eml += "Content-Disposition: " + (a.isInline ? "inline;" : "attachment;") + " filename=\"" + (a.name || "") + "\";\n";
        if (a.size) eml += "\tsize=" + a.size + ";";
        if (a.lastModifiedDateTime) {
            const d = mailDate(a.lastModifiedDateTime);
            eml += " creation-date: \"" + d + "\";\n\tmodification-date: \"" + d + "\"\n";
        }
        if (a.contentId) eml += "Content-ID: <" + a.contentId + ">\n";
        eml += "Content-Transfer-Encoding: base64\n\n" + wrap76(a.contentBytes || "") + "\n\n";
    });
    if (attachments.length) eml += "\n\n--" + attBoundary + "--\n\n";
    return eml;
}

// Where archived mail goes: the project's primary project folder (local or a
// Microsoft Teams folder, through projectFiles) or, with `localFolder`, a
// folder the user picked. Paths given to it are relative to that folder.
export function destination(projectKey, localFolder) {
    if (localFolder) {
        const root = String(localFolder).replace(/[\\/]+$/, "");
        const full = function (relative) { return root + "/" + relative; };
        return {
            mkdir: function* (relative) { yield files.mkdir(full(relative)); },
            names: function* (relative) {
                if (!(yield files.exists(full(relative)))) return new Set();
                return new Set((yield files.list(full(relative))).map(function (f) { return f.name.toLowerCase(); }));
            },
            readText: function* (relative) {
                return (yield files.exists(full(relative))) ? yield files.readText(full(relative)) : null;
            },
            writeText: function* (relative, text) { yield files.writeText(full(relative), text); },
            upload: function* (localPath, relative) { yield files.copy(localPath, full(relative), { overwrite: true }); },
        };
    }
    return {
        mkdir: function* (relative) { yield projectFiles.mkdir(projectKey, relative); },
        names: function* (relative) {
            return new Set((yield projectFiles.list(projectKey, relative)).map(function (f) { return f.name.toLowerCase(); }));
        },
        readText: function* (relative) { return yield projectFiles.readText(projectKey, relative); },
        writeText: function* (relative, text) { yield projectFiles.writeText(projectKey, relative, text); },
        upload: function* (localPath, relative) { yield projectFiles.upload(projectKey, localPath, relative, { overwrite: true }); },
    };
}

// Prefer an available local project folder, including a synced Teams folder.
// The project's folder as a destination, or null when it has none, its
// local folder is gone, or it is a Teams folder Project Notes can't save into.
export function* projectDestination(projectKey) {
    const folder = yield projectFiles.location(projectKey, { preferLocal: true });
    if (!folder || folder.writable === false) return null;
    if (folder.kind === "local" && !(yield files.exists(folder.path))) return null;
    return destination(projectKey, folder.kind === "local" ? folder.path : null);
}

function* readState(target, path) {
    const text = yield target.readText(path);
    if (!text) return {};
    try { return JSON.parse(text); } catch (e) { return {}; }
}

function* downloadBox(projectNumber, box, top, target, folder, all) {
    const statePath = folder + "/download_state.json";
    const stateName = projectNumber + "_" + box + "_downloadtracker";
    const state = yield readState(target, statePath);
    const present = yield target.names(folder);
    let skip = (state[stateName] && state[stateName].skip) || 0;
    let saved = 0;
    for (;;) {
        const response = yield office365.graph("GET", "me/mailFolders/" + box + "/messages", { query: {
            "$filter": "contains(subject,'" + projectNumber.replace(/'/g, "''") + "')",
            "$select": "id,subject,sentDateTime", "$top": String(top), "$skip": String(skip) } });
        if (response.status !== 200) {
            log.warn("Listing " + box + " for " + projectNumber + " failed: " + response.status);
            break;
        }
        const messages = (response.body && response.body.value) || [];
        for (const m of messages) {
            const name = makeFilename(m.sentDateTime, m.subject);
            if (present.has((name + ".msg").toLowerCase()) || present.has((name + ".eml").toLowerCase())) continue;
            const full = yield office365.graph("GET", "me/messages/" + m.id, { query: { "$expand": "attachments" } });
            if (full.status !== 200) {
                log.warn("Downloading a message failed: " + full.status);
                continue;
            }
            yield target.writeText(folder + "/" + name + ".eml", toEml(full.body));
            present.add((name + ".eml").toLowerCase());
            saved++;
        }
        skip += messages.length;
        if (!all || messages.length < top) break;
    }
    state[stateName] = { skip: skip };
    yield target.writeText(statePath, JSON.stringify(state, null, 4));
    return saved;
}

function subfolder(name) { return String(name || "").replace(/\\/g, "/").replace(/^\/+|\/+$/g, ""); }

function* archiveProject(cfg, projectKey, projectNumber, all) {
    const target = yield projectDestination(projectKey);
    if (!target) return 0;
    const received = subfolder(cfg.inboxFolder);
    const sent = subfolder(cfg.sentFolder);
    yield target.mkdir(received);
    yield target.mkdir(sent);
    let saved = yield downloadBox(projectNumber, "inbox", 20, target, received, all);
    saved += yield downloadBox(projectNumber, "sentitems", 20, target, sent, all);
    return saved;
}

// `project` set: every email for that project. Otherwise the next five
// projects with a project folder, 20 emails per mailbox folder each.
export function* archiveFromGraph(cfg, project) {
    if (!cfg.inboxFolder || !cfg.sentFolder) {
        log.warn("Set the inbox and sent items subfolders in Settings > Archive Mail.");
        return 0;
    }
    if (project) return yield archiveProject(cfg, project.$key, project.project_number, true);

    const state = (yield storage.readJson("state.json")) || {};
    const skip = (state.project_emails_export && state.project_emails_export.skip) || 0;
    const top = 5;
    // Projects with a primary project folder; archiveProject skips any whose
    // folder is gone.
    const folders = yield data.list("projects", {
        where: { primary_location_id: { $null: false } }, orderBy: ["project_number"], limit: top, offset: skip });
    let saved = 0;
    for (const project of folders)
        saved += yield archiveProject(cfg, project.$key, project.project_number, false);
    state.project_emails_export = { skip: folders.length < top ? 0 : skip + top };
    yield storage.writeJson("state.json", state);
    return saved;
}

// Outlook classic over COM: saves matching messages as .msg (Windows only).
// Outlook writes each message to a temporary file, which is then copied into
// `target` (see destination()), so a Teams project folder works too.
export function* archiveFromOutlook(cfg, projectNumber, target, progress) {
    const com = pn.com;
    const outlook = com.createObject("Outlook.Application");
    let saved = 0;
    try {
        const mapi = outlook.GetNamespace("MAPI");
        const boxes = [[mapi.GetDefaultFolder(6), subfolder(cfg.inboxFolder), "Inbox items"],   // olFolderInbox
                       [mapi.GetDefaultFolder(5), subfolder(cfg.sentFolder), "Sent items"]];   // olFolderSentMail
        // Restrict to matching subjects instead of walking the whole mailbox.
        const filter = "@SQL=\"urn:schemas:httpmail:subject\" LIKE '%" + projectNumber.replace(/'/g, "''") + "%'";
        for (let b = 0; b < boxes.length; b++) {
            yield target.mkdir(boxes[b][1]);
            const present = yield target.names(boxes[b][1]);
            const items = boxes[b][0].Items.Restrict(filter);
            const total = items.Count || 0;
            let index = 0;
            for (const message of items) {
                index++;
                progress.update((b + index / Math.max(total, 1)) / 2, boxes[b][2]);
                const sentOn = message.SentOn;
                if (sentOn) {
                    const name = makeFilename(sentOn, message.Subject);
                    if (!present.has((name + ".msg").toLowerCase()) && !present.has((name + ".eml").toLowerCase())) {
                        const temporary = files.tempPath(name + ".msg");
                        if (yield files.exists(temporary)) yield files.remove(temporary);
                        message.SaveAs(temporary.replace(/\//g, "\\"), 3);   // olMSG
                        yield target.upload(temporary, boxes[b][1] + "/" + name + ".msg");
                        yield files.remove(temporary);
                        present.add((name + ".msg").toLowerCase());
                        saved++;
                    }
                }
                com.release(message);
            }
        }
    } finally {
        com.release(outlook);
    }
    return saved;
}
