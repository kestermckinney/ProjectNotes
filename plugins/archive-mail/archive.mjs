// Project email archiving: Microsoft Graph (as .eml) and Outlook classic over
// COM (as .msg). File names and the per-folder download_state.json match the
// Python plugin, so mail it already archived is not downloaded again.
import * as pn from "projectnotes";

const { app, data, files, log, office365, storage } = pn;

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

function* readState(path) {
    if (!(yield files.exists(path))) return {};
    try { return JSON.parse(yield files.readText(path)); } catch (e) { return {}; }
}

function* downloadBox(projectNumber, box, top, folder, all) {
    const statePath = folder + "/download_state.json";
    const stateName = projectNumber + "_" + box + "_downloadtracker";
    const state = yield readState(statePath);
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
            const base = folder + "/" + makeFilename(m.sentDateTime, m.subject);
            if ((yield files.exists(base + ".msg")) || (yield files.exists(base + ".eml"))) continue;
            const full = yield office365.graph("GET", "me/messages/" + m.id, { query: { "$expand": "attachments" } });
            if (full.status !== 200) {
                log.warn("Downloading a message failed: " + full.status);
                continue;
            }
            yield files.writeText(base + ".eml", toEml(full.body));
            saved++;
        }
        skip += messages.length;
        if (!all || messages.length < top) break;
    }
    state[stateName] = { skip: skip };
    yield files.writeText(statePath, JSON.stringify(state, null, 4));
    return saved;
}

function* archiveProject(cfg, projectKey, projectNumber, all) {
    const folder = yield app.projectFolder(projectKey);
    if (!folder || !(yield files.exists(folder))) return 0;
    const received = folder + "/" + cfg.inboxFolder;
    const sent = folder + "/" + cfg.sentFolder;
    yield files.mkdir(received);
    yield files.mkdir(sent);
    let saved = yield downloadBox(projectNumber, "inbox", 20, received, all);
    saved += yield downloadBox(projectNumber, "sentitems", 20, sent, all);
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
    const folders = yield data.list("project_locations", {
        where: { location_description: "Project Folder" }, orderBy: ["project_number"], limit: top, offset: skip });
    let saved = 0;
    for (const location of folders) {
        if (location.project_id) saved += yield archiveProject(cfg, location.project_id.id, location.project_number, false);
    }
    state.project_emails_export = { skip: folders.length < top ? 0 : skip + top };
    yield storage.writeJson("state.json", state);
    return saved;
}

// Outlook classic over COM: saves matching messages as .msg (Windows only).
export function* archiveFromOutlook(cfg, projectNumber, folder, progress) {
    const com = pn.com;
    const outlook = com.createObject("Outlook.Application");
    let saved = 0;
    try {
        const mapi = outlook.GetNamespace("MAPI");
        const boxes = [[mapi.GetDefaultFolder(6), folder + "/" + cfg.inboxFolder, "Inbox items"],   // olFolderInbox
                       [mapi.GetDefaultFolder(5), folder + "/" + cfg.sentFolder, "Sent items"]];   // olFolderSentMail
        // Restrict to matching subjects instead of walking the whole mailbox.
        const filter = "@SQL=\"urn:schemas:httpmail:subject\" LIKE '%" + projectNumber.replace(/'/g, "''") + "%'";
        for (let b = 0; b < boxes.length; b++) {
            yield files.mkdir(boxes[b][1]);
            const items = boxes[b][0].Items.Restrict(filter);
            const total = items.Count || 0;
            let index = 0;
            for (const message of items) {
                index++;
                progress.update((b + index / Math.max(total, 1)) / 2, boxes[b][2]);
                const sentOn = message.SentOn;
                if (sentOn) {
                    const base = boxes[b][1] + "/" + makeFilename(sentOn, message.Subject);
                    if (!(yield files.exists(base + ".msg")) && !(yield files.exists(base + ".eml"))) {
                        message.SaveAs(base.replace(/\//g, "\\") + ".msg", 3);   // olMSG
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
