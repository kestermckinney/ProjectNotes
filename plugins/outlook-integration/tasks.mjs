// Open tracker items assigned to the project manager, mirrored as tasks in
// the Microsoft To Do "Tasks" list. Replaces GraphAPITools.sync_tracker_to_tasks.
import { app, data, log, office365, storage } from "projectnotes";

const importance = { High: "high", Low: "low" };

function localMidnightUtc(isoDate) {
    const parts = isoDate.split("-").map(Number);
    const d = new Date(parts[0], parts[1] - 1, parts[2]);
    return d.toISOString().replace("Z", "");
}

function* allPages(path) {
    const out = [];
    yield office365.graphPages(path).forEach(function (item) { out.push(item); });
    return out;
}

// `all`: every item; otherwise the next 20 (the five-minute background slice).
// Returns how many tracker items were looked at.
export function* syncTrackerToTasks(cfg, all) {
    const defaults = yield app.defaults();
    const pm = defaults.projectManagerId;
    if (!pm) {
        log.warn("Choose the project manager in Settings > Preferences to sync tracker items.");
        return 0;
    }

    const state = (yield storage.readJson("state.json")) || {};
    const top = 20;
    const skip = all ? 0 : ((state.tracker_export && state.tracker_export.skip) || 0);
    const query = { where: { assigned_to: pm }, orderBy: ["project_number", "item_number"] };
    if (!all) { query.limit = top; query.offset = skip; }
    const items = yield data.list("item_tracker", query);

    const lists = yield office365.graph("GET", "me/todo/lists", {});
    if (lists.status !== 200) {
        log.warn("Could not read Microsoft To Do lists: " + lists.status);
        return 0;
    }
    const tasksList = (lists.body.value || []).find(l => l.displayName === "Tasks");
    if (!tasksList) {
        log.warn("Microsoft To Do has no \"Tasks\" list.");
        return 0;
    }
    const base = "me/todo/lists/" + tasksList.id + "/tasks";
    const todos = yield allPages(base);

    for (const item of items) {
        const prefix = item.project_number + "-" + item.item_number;
        const existing = todos.find(t => String(t.title || "").indexOf(prefix) === 0);
        const due = item.date_due;
        const open = (item.status === "New" || item.status === "Assigned")
                     && item.assigned_to && item.assigned_to.id === pm
                     && ((due && cfg.syncWithDue) || (!due && cfg.syncWithoutDue));

        if (open) {
            const task = {
                title: prefix + " " + (item.project_name || "") + " " + (item.item_name || ""),
                importance: importance[item.priority] || "normal",
                body: { content: (item.description || "") + "\n" + (item.comments || ""), contentType: "text" },
            };
            const changed = !existing || existing.title !== task.title || existing.importance !== task.importance
                            || !existing.body || existing.body.content !== task.body.content;
            if (due) {
                const when = { dateTime: localMidnightUtc(due), timeZone: "UTC" };
                task.isReminderOn = true;
                task.dueDateTime = when;
                task.reminderDateTime = when;
            }
            if (!existing) {
                const r = yield office365.graph("POST", base, { body: task });
                if (r.status !== 201) log.warn("Adding task " + prefix + " failed: " + r.status);
            } else if (changed) {
                const r = yield office365.graph("PATCH", base + "/" + existing.id, { body: task });
                if (r.status !== 200) log.warn("Updating task " + prefix + " failed: " + r.status);
            }
        } else if (existing) {
            const r = yield office365.graph("DELETE", base + "/" + existing.id, {});
            if (r.status !== 204) log.warn("Removing task " + prefix + " failed: " + r.status);
        }
    }

    if (!all) {
        state.tracker_export = { skip: items.length < top ? 0 : skip + top };
        yield storage.writeJson("state.json", state);
    }
    return items.length;
}
