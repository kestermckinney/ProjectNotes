// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
//
// Plugin API prelude. Evaluated once per plugin engine before the plugin's own
// module loads. It builds the "projectnotes" and "projectnotes/common" modules
// on top of a single native bridge (`host`) that never becomes reachable from
// plugin code: it is captured in this closure and not stored on any object the
// plugin can see.
//
// The API surface here must stay in step with projectnotes.d.ts and the plugin
// API reference in docs/QJSEnginePluginsPlan.md.

(function (host, info) {
    "use strict";

    // Qt's JavaScript engine (V4, Qt 6.11) implements roughly ES2016 plus
    // optional chaining and nullish coalescing. It has no async/await, so
    // plugins write coroutines as generator functions and `yield` a promise
    // where they would `await` it; the host drives them (see drive()). This
    // file itself sticks to that subset.

    // ── Polyfills for common newer built-ins (each plugin has its own engine) ─
    if (typeof Promise.prototype.finally !== "function") {
        Object.defineProperty(Promise.prototype, "finally", { configurable: true, writable: true, value: function (fn) {
            return this.then(function (v) { return Promise.resolve(fn()).then(function () { return v; }); },
                             function (e) { return Promise.resolve(fn()).then(function () { throw e; }); });
        } });
    }
    if (typeof Object.fromEntries !== "function") {
        Object.defineProperty(Object, "fromEntries", { configurable: true, writable: true, value: function (entries) {
            const o = {};
            for (const pair of entries) o[pair[0]] = pair[1];
            return o;
        } });
    }
    if (typeof Array.prototype.flat !== "function") {
        Object.defineProperty(Array.prototype, "flat", { configurable: true, writable: true, value: function (depth) {
            const d = depth === undefined ? 1 : depth;
            return this.reduce(function (acc, v) {
                return acc.concat(Array.isArray(v) && d > 0 ? v.flat(d - 1) : [v]);
            }, []);
        } });
        Object.defineProperty(Array.prototype, "flatMap", { configurable: true, writable: true, value: function (fn, self) {
            return this.map(fn, self).flat(1);
        } });
    }

    // ── Coroutines ─────────────────────────────────────────────────────────
    // A plugin function may return a value, a promise, or a generator. A
    // generator is run as a coroutine: each `yield` waits for the yielded
    // promise (or array of promises, or nested generator) and resumes with its
    // value, or throws its error into the generator.
    function isIterator(r) {
        return !!r && typeof r.next === "function" && typeof r.throw === "function";
    }
    function settle(r) {
        if (isIterator(r)) return drive(r);
        if (Array.isArray(r)) return Promise.all(r.map(settle));
        return Promise.resolve(r);
    }
    function drive(gen) {
        return new Promise(function (resolve, reject) {
            function step(method, arg) {
                let r;
                try { r = gen[method](arg); } catch (e) { reject(e); return; }
                if (r.done) { resolve(r.value); return; }
                settle(r.value).then(function (v) { step("next", v); }, function (e) { step("throw", e); });
            }
            step("next", undefined);
        });
    }

    // ── Errors ─────────────────────────────────────────────────────────────
    class PluginError extends Error {
        constructor(code, message, permission, details) {
            super(message || code);
            this.name = "PluginError";
            this.code = code;
            if (permission) this.permission = permission;
            if (details) this.details = details;
        }
    }
    function toError(e) {
        if (e instanceof PluginError) return e;
        if (e && typeof e === "object" && e.code)
            return new PluginError(e.code, e.message, e.permission, e.details);
        return new PluginError("plugin-error", String(e && e.message ? e.message : e));
    }

    // ── Bridge ─────────────────────────────────────────────────────────────
    const pending = new Map();      // request id -> {resolve, reject}

    function call(method, args) {
        return new Promise(function (resolve, reject) {
            const id = host.call(method, args || []);
            pending.set(id, { resolve: resolve, reject: reject });
        });
    }
    function callSync(method, args) {
        const r = host.callSync(method, args || []);
        if (r && r.error) throw toError(r.error);
        return r ? r.value : undefined;
    }
    function freeze(o) { return Object.freeze(o); }

    // ── Timers ─────────────────────────────────────────────────────────────
    let nextTimer = 1;
    const timerCallbacks = new Map();
    function startTimer(fn, ms, repeat, args) {
        if (typeof fn !== "function") throw new PluginError("validation-failed", "Timer callback must be a function.");
        const id = nextTimer++;
        timerCallbacks.set(id, { fn: fn, args: args || [], repeat: repeat });
        callSync("timer.start", [id, Math.max(0, Number(ms) || 0), !!repeat]);
        return id;
    }
    function stopTimer(id) {
        if (timerCallbacks.delete(id)) callSync("timer.stop", [id]);
    }

    // ── Logging ────────────────────────────────────────────────────────────
    function format(args) {
        return Array.prototype.map.call(args, function (a) {
            if (typeof a === "string") return a;
            if (a instanceof Error) return a.stack ? a.message + "\n" + a.stack : a.message;
            try { return JSON.stringify(a); } catch (e) { return String(a); }
        }).join(" ");
    }
    const log = freeze({
        debug: function () { host.log("debug", format(arguments)); },
        info: function () { host.log("info", format(arguments)); },
        warn: function () { host.log("warn", format(arguments)); },
        error: function () { host.log("error", format(arguments)); },
    });

    // ── Events from the host ───────────────────────────────────────────────
    const subscriptions = { "data.changed": [], "app.viewChanged": [], "settings.changed": [] };
    function subscribe(kind, entry) {
        subscriptions[kind].push(entry);
        return freeze({ unsubscribe: function () {
            const list = subscriptions[kind];
            const i = list.indexOf(entry);
            if (i >= 0) list.splice(i, 1);
        } });
    }
    function report(where, e) {
        const err = toError(e);
        host.reportError(where, err.message, err.stack || "");
    }
    function safely(where, fn, args) {
        try {
            settle(fn.apply(null, args || [])).then(undefined, function (e) { report(where, e); });
        } catch (e) {
            report(where, e);
        }
    }

    // ── plugin ─────────────────────────────────────────────────────────────
    const plugin = freeze({
        id: info.id, name: info.name, version: info.version, apiVersion: info.apiVersion,
        bundled: info.bundled, packagePath: info.packagePath, dataPath: info.dataPath,
    });

    // ── data ───────────────────────────────────────────────────────────────
    function makeDataOps(prefix) {
        return {
            get: function (entity, keyOrWhere, options) { return call(prefix + "get", [entity, keyOrWhere, options || {}]); },
            list: function (entity, query) { return call(prefix + "list", [entity, query || {}]); },
            count: function (entity, where) { return call(prefix + "count", [entity, where || {}]); },
            save: function (entity, record) { return call(prefix + "save", [entity, record]); },
            saveAll: function (entity, records) { return call(prefix + "saveAll", [entity, records]); },
            remove: function (entity, keyOrRecord) { return call(prefix + "remove", [entity, keyOrRecord]); },
        };
    }
    const dataOps = makeDataOps("data.");
    let transactionOpen = false;
    const data = freeze(Object.assign({
        entities: function () { return call("data.entities", []); },
        schema: function (entity) { return call("data.schema", [entity]); },
        search: function (text, options) { return call("data.search", [text, options || {}]); },
        transaction: function (work) {
            if (typeof work !== "function") return Promise.reject(new PluginError("validation-failed", "transaction() needs a function."));
            if (transactionOpen) return Promise.reject(new PluginError("transaction-active", "A transaction is already open."));
            const tx = freeze(Object.assign({}, dataOps));
            return call("data.begin", []).then(function () {
                transactionOpen = true;
                let result;
                try { result = settle(work(tx)); } catch (e) { result = Promise.reject(e); }
                return result.then(function (value) {
                    transactionOpen = false;
                    return call("data.commit", []).then(function () { return value; });
                }, function (e) {
                    transactionOpen = false;
                    return call("data.rollback", []).then(function () { throw toError(e); });
                });
            });
        },
        onChanged: function (entities, handler) {
            if (typeof handler !== "function") throw new PluginError("validation-failed", "onChanged() needs a handler.");
            const list = Array.isArray(entities) ? entities.slice() : [entities];
            return subscribe("data.changed", { entities: list, handler: handler });
        },
        exportXml: function (entity, key, options) { return call("data.exportXml", [entity, key, options || {}]); },
        importXml: function (xml) { return call("data.importXml", [xml]); },
    }, dataOps));

    // ── app ────────────────────────────────────────────────────────────────
    const capabilitySet = new Set(info.capabilities || []);
    const app = freeze({
        version: info.appVersion,
        apiVersion: info.apiVersion,
        platform: info.platform,
        buildFlavor: info.buildFlavor,
        profile: info.profile,
        locale: info.locale,
        capabilities: freeze({
            has: function (name) { return capabilitySet.has(name); },
            list: function () { return Array.from(capabilitySet); },
        }),
        currentView: function () { return callSync("app.currentView", []); },
        currentRecord: function (options) {
            const view = callSync("app.currentView", []) || {};
            if (!view.entity || !view.key) return Promise.resolve(null);
            return dataOps.get(view.entity, view.key, { include: (options && options.include) || [] });
        },
        navigate: function (entity, key, options) { return call("app.navigate", [entity, key, options || {}]); },
        projectFolder: function (projectKey) { return call("app.projectFolder", [projectKey]); },
        defaults: function () { return call("app.defaults", []); },
        refresh: function () { callSync("app.refresh", []); },
        onViewChanged: function (handler) { return subscribe("app.viewChanged", { handler: handler }); },
    });

    // ── menus ──────────────────────────────────────────────────────────────
    const menuHandlers = new Map();
    const menus = freeze({
        register: function (items) {
            const list = Array.isArray(items) ? items : [items];
            const plain = list.map(function (item) {
                if (!item || !item.id || !item.title) throw new PluginError("validation-failed", "A menu needs an id and a title.");
                if (typeof item.handler === "function") menuHandlers.set(item.id, item.handler);
                const copy = Object.assign({}, item);
                delete copy.handler;
                return copy;
            });
            callSync("menus.register", [plain]);
        },
        unregister: function (ids) {
            const list = Array.isArray(ids) ? ids : [ids];
            list.forEach(function (id) { menuHandlers.delete(id); });
            callSync("menus.unregister", [list]);
        },
        clear: function () { menuHandlers.clear(); callSync("menus.clear", []); },
        list: function () { return callSync("menus.list", []); },
    });

    // ── ui ─────────────────────────────────────────────────────────────────
    let nextDialog = 1;
    const dialogHandlers = new Map();   // dialog token -> handlers object
    const ui = freeze({
        message: function (text, options) { return call("ui.message", [String(text), options || {}]); },
        confirm: function (text, options) { return call("ui.confirm", [String(text), options || {}]); },
        prompt: function (label, options) { return call("ui.prompt", [String(label), options || {}]); },
        choose: function (label, choices, options) { return call("ui.choose", [String(label), choices || [], options || {}]); },
        showForm: function (form, values, options) { return call("ui.showForm", [form, values || {}, options || {}]); },
        showDialog: function (qmlFile, dialogData, options) {
            const opts = Object.assign({}, options || {});
            const token = "dialog-" + (nextDialog++);
            if (opts.handlers) dialogHandlers.set(token, opts.handlers);
            delete opts.handlers;
            opts.token = token;
            return call("ui.showDialog", [qmlFile, dialogData === undefined ? {} : dialogData, opts])
                .then(function (v) { dialogHandlers.delete(token); return v; },
                      function (e) { dialogHandlers.delete(token); throw e; });
        },
        pickFile: function (options) { return call("ui.pickFile", [options || {}]); },
        pickFolder: function (options) { return call("ui.pickFolder", [options || {}]); },
        saveFile: function (options) { return call("ui.saveFile", [options || {}]); },
        notify: function (text, options) { callSync("ui.notify", [String(text), options || {}]); },
        progress: function (title, task, options) {
            if (typeof task !== "function") return Promise.reject(new PluginError("validation-failed", "progress() needs a task."));
            return call("ui.progressBegin", [String(title), options || {}]).then(function (token) {
                let cancelled = false;
                const p = freeze({
                    update: function (fraction, text) {
                        cancelled = !!callSync("ui.progressUpdate", [token, fraction === null ? -1 : Number(fraction), text || ""]);
                    },
                    get cancelled() { return cancelled || !!callSync("ui.progressCancelled", [token]); },
                });
                let result;
                try { result = settle(task(p)); } catch (e) { result = Promise.reject(e); }
                return result.then(function (v) { callSync("ui.progressEnd", [token]); return v; },
                                   function (e) { callSync("ui.progressEnd", [token]); throw e; });
            });
        },
        openUrl: function (url) { return call("ui.openUrl", [String(url)]); },
        clipboard: freeze({
            setText: function (text) { callSync("ui.clipboardSetText", [String(text)]); },
            text: function () { return call("ui.clipboardText", []); },
        }),
    });

    // ── settings / secrets / storage ───────────────────────────────────────
    const settings = freeze({
        get: function (key, defaultValue) {
            const v = callSync("settings.get", [String(key)]);
            return v === undefined || v === null ? (defaultValue === undefined ? null : defaultValue) : v;
        },
        set: function (key, value) { callSync("settings.set", [String(key), value === undefined ? null : value]); },
        remove: function (key) { callSync("settings.remove", [String(key)]); },
        keys: function () { return callSync("settings.keys", []); },
        all: function () { return callSync("settings.all", []); },
        onChanged: function (handler) { return subscribe("settings.changed", { handler: handler }); },
    });
    // My Shortcuts' table, so a plugin can offer its menus there.
    const shortcuts = freeze({
        list: function () { return callSync("shortcuts.list", []) || []; },
        add: function (rows) {
            const plain = (rows || []).map(function (r) {
                return { menu: String(r.menu || ""), submenu: String(r.submenu || ""),
                         entity: String(r.entity || ""), url: String(r.url || "") };
            });
            return callSync("shortcuts.add", [plain]);
        },
    });
    const secrets = freeze({
        get: function (account) { return call("secrets.get", [String(account)]); },
        set: function (account, secret) { return call("secrets.set", [String(account), String(secret)]); },
        remove: function (account) { return call("secrets.remove", [String(account)]); },
    });
    const storage = freeze({
        path: info.dataPath,
        readText: function (path) { return call("storage.readText", [String(path)]); },
        writeText: function (path, text) { return call("storage.writeText", [String(path), String(text)]); },
        readJson: function (path) {
            return call("storage.readText", [String(path)]).then(function (t) { return t === null ? null : JSON.parse(t); });
        },
        writeJson: function (path, value) { return call("storage.writeText", [String(path), JSON.stringify(value, null, 2)]); },
        remove: function (path) { return call("storage.remove", [String(path)]); },
        list: function (dir) { return call("storage.list", [dir === undefined ? "" : String(dir)]); },
    });

    // ── files ──────────────────────────────────────────────────────────────
    const files = freeze({
        exists: function (path) { return call("files.exists", [String(path)]); },
        stat: function (path) { return call("files.stat", [String(path)]); },
        list: function (dir, options) { return call("files.list", [String(dir), options || {}]); },
        readText: function (path) { return call("files.readText", [String(path)]); },
        writeText: function (path, text) { return call("files.writeText", [String(path), String(text)]); },
        readBytes: function (path) { return call("files.readBytes", [String(path)]); },
        writeBytes: function (path, bytes) { return call("files.writeBytes", [String(path), bytes]); },
        copy: function (from, to, options) { return call("files.copy", [String(from), String(to), options || {}]); },
        move: function (from, to, options) { return call("files.move", [String(from), String(to), options || {}]); },
        mkdir: function (path) { return call("files.mkdir", [String(path)]); },
        remove: function (path) { return call("files.remove", [String(path)]); },
        packagePath: function (relative) { return callSync("files.packagePath", [String(relative)]); },
        tempPath: function (name) { return callSync("files.tempPath", [String(name)]); },
        openWithSystem: function (path) { return call("files.openWithSystem", [String(path)]); },
        reveal: function (path) { return call("files.reveal", [String(path)]); },
    });

    // ── projectFiles: the project's primary project folder ──────────────────
    // Paths are relative to the project folder, which is either local or a
    // Microsoft Teams channel folder; the host writes to Teams through Graph.
    const projectFiles = freeze({
        location: function (projectKey, options) { return call("projectFiles.location", [String(projectKey), options || {}]); },
        list: function (projectKey, folder) {
            return call("projectFiles.list", [String(projectKey), folder === undefined || folder === null ? "" : String(folder)]);
        },
        exists: function (projectKey, path) { return call("projectFiles.exists", [String(projectKey), String(path)]); },
        mkdir: function (projectKey, path) { return call("projectFiles.mkdir", [String(projectKey), String(path)]); },
        readText: function (projectKey, path) { return call("projectFiles.readText", [String(projectKey), String(path)]); },
        writeText: function (projectKey, path, text, options) {
            return call("projectFiles.writeText", [String(projectKey), String(path), String(text), options || {}]);
        },
        upload: function (projectKey, localPath, path, options) {
            return call("projectFiles.upload", [String(projectKey), String(localPath), String(path), options || {}]);
        },
        open: function (projectKey, path) { return call("projectFiles.open", [String(projectKey), String(path)]); },
    });

    // ── integrations ───────────────────────────────────────────────────────
    const email = freeze({
        backends: function () { return call("email.backends", []); },
        compose: function (message) { return call("email.compose", [message || {}]); },
        audiences: function (projectKey, workflow) { return call("email.audiences", [String(projectKey), workflow || ""]); },
        resolveAudience: function (projectKey, audienceId) { return call("email.resolveAudience", [String(projectKey), String(audienceId)]); },
        chooseRecipients: function (projectKey, options) { return call("email.chooseRecipients", [String(projectKey), options || {}]); },
        templates: function () { return call("email.templates", []); },
        renderTemplate: function (templateId, context) { return call("email.renderTemplate", [String(templateId), context || {}]); },
    });
    const calendar = freeze({
        backends: function () { return call("calendar.backends", []); },
        draftMeeting: function (meeting) { return call("calendar.draftMeeting", [meeting || {}]); },
        writeIcs: function (meeting, path) { return call("calendar.writeIcs", [meeting || {}, String(path)]); },
    });
    const contacts = freeze({
        sources: function () { return call("contacts.sources", []); },
        syncNow: function (sourceId) { return call("contacts.syncNow", [sourceId || ""]); },
        pick: function (options) { return call("contacts.pick", [options || {}]); },
        toVCard: function (contactKey) { return call("contacts.toVCard", [String(contactKey)]); },
        fromVCard: function (text) { return call("contacts.fromVCard", [String(text)]); },
    });
    const office365 = freeze({
        status: function () { return call("office365.status", []); },
        signIn: function () { return call("office365.signIn", []); },
        graph: function (method, path, options) { return call("office365.graph", [String(method), String(path), options || {}]); },
        // Pages through a Graph collection by following @odata.nextLink.
        // next() resolves {done, value} one item at a time; forEach(fn)
        // visits every item (fn may return a promise or generator).
        graphPages: function (path, options) {
            let next = null;
            let started = false;
            let buffer = [];
            function fill() {
                if (buffer.length > 0 || (started && !next)) return Promise.resolve();
                const request = started ? office365.graph("GET", next, {}) : office365.graph("GET", path, options || {});
                started = true;
                return request.then(function (response) {
                    const body = response.body || {};
                    buffer = Array.isArray(body.value) ? body.value.slice() : [];
                    next = body["@odata.nextLink"] || null;
                    return fill();
                });
            }
            const pages = {
                next: function () {
                    return fill().then(function () {
                        return buffer.length ? { done: false, value: buffer.shift() } : { done: true, value: undefined };
                    });
                },
                forEach: function (fn) {
                    return pages.next().then(function (r) {
                        if (r.done) return undefined;
                        return settle(fn(r.value)).then(function () { return pages.forEach(fn); });
                    });
                },
            };
            if (typeof Symbol.asyncIterator !== "undefined")
                pages[Symbol.asyncIterator] = function () { return pages; };
            return pages;
        },
    });
    function makeResponse(r) {
        return freeze({
            status: r.status, ok: r.status >= 200 && r.status < 300, headers: freeze(r.headers || {}),
            text: function () { return Promise.resolve(r.body); },
            json: function () { return Promise.resolve().then(function () { return JSON.parse(r.body); }); },
            arrayBuffer: function () { return call("http.bodyBytes", [r.bodyId]); },
        });
    }
    // auth: the browser step of an OAuth authorization-code sign-in. Resolves
    // {url} with the loopback callback URL; the plugin exchanges the code.
    const auth = freeze({
        browserSignIn: function (options) { return call("auth.browserSignIn", [options || {}]); },
    });
    const http = freeze({
        fetch: function (url, options) {
            const opts = Object.assign({}, options || {});
            if (opts.body !== undefined && typeof opts.body === "object" && !(opts.body instanceof ArrayBuffer)) {
                opts.body = JSON.stringify(opts.body);
                opts.headers = Object.assign({ "Content-Type": "application/json" }, opts.headers || {});
            }
            return call("http.fetch", [String(url), opts]).then(makeResponse);
        },
    });
    const timers = freeze({
        every: function (minutes, job, options) {
            let running = false;
            const run = function () {
                if (running) { log.warn("timer job still running; skipped this tick"); return; }
                running = true;
                Promise.resolve().then(function () { return settle(job()); })
                    .then(function () { running = false; }, function (e) { running = false; report("timers.every", e); });
            };
            const id = startTimer(run, Math.max(1, Number(minutes) || 1) * 60000, true);
            if (options && options.runAtStart) startTimer(run, 0, false);
            return freeze({ cancel: function () { stopTimer(id); } });
        },
        after: function (milliseconds, job) {
            const id = startTimer(function () {
                Promise.resolve().then(function () { return settle(job()); })
                    .then(undefined, function (e) { report("timers.after", e); });
            }, milliseconds, false);
            return freeze({ cancel: function () { stopTimer(id); } });
        },
    });

    // com: a Proxy over native dispatch handles (Windows only). Reading a
    // member does a property get; when the native side reports that the member
    // is a method, the read returns a function that invokes it. Calling an
    // object itself invokes its default member (e.g. collection(1)).
    function comWrap(handle) {
        const target = function () {};
        return new Proxy(target, {
            get: function (t, name) {
                if (name === "$handle") return handle;
                if (name === "$call") return function (member) {
                    return comResult(callSync("com.call", [handle, String(member), Array.prototype.slice.call(arguments, 1).map(comArg)]));
                };
                if (name === "$get") return function (member) {
                    return comResult(callSync("com.get", [handle, String(member), Array.prototype.slice.call(arguments, 1).map(comArg)]));
                };
                if (name === "$set") return function (member, value) {
                    callSync("com.set", [handle, String(member), comArg(value), Array.prototype.slice.call(arguments, 2).map(comArg)]);
                };
                if (name === Symbol.iterator) return function () {
                    return callSync("com.enumerate", [handle]).map(comResult)[Symbol.iterator]();
                };
                if (typeof name === "symbol" || name === "then" || name === "toJSON") return undefined;
                const member = String(name);
                const r = callSync("com.get", [handle, member, []]);
                if (r && typeof r === "object" && r.$method)
                    return function () {
                        return comResult(callSync("com.call", [handle, member, Array.prototype.slice.call(arguments).map(comArg)]));
                    };
                return comResult(r);
            },
            set: function (t, name, value) { callSync("com.set", [handle, String(name), comArg(value), []]); return true; },
            apply: function (t, self, args) { return comResult(callSync("com.call", [handle, "", args.map(comArg)])); },
        });
    }
    function comArg(v) { return v && v.$handle ? { $handle: v.$handle } : v; }
    function comResult(v) { return v && typeof v === "object" && v.$handle ? comWrap(v.$handle) : v; }
    const com = freeze({
        createObject: function (progId) { return comResult(callSync("com.createObject", [String(progId)])); },
        getActiveObject: function (progId) { return comResult(callSync("com.getActiveObject", [String(progId)])); },
        getObject: function (moniker) { return comResult(callSync("com.getObject", [String(moniker)])); },
        release: function (object) { if (object && object.$handle) callSync("com.release", [object.$handle]); },
    });
    const processApi = freeze({
        launch: function (program, args, options) { return call("process.launch", [String(program), args || [], options || {}]); },
    });
    const reports = freeze({
        generate: function (workflow, projectKey, options) { return call("reports.generate", [String(workflow), String(projectKey), options || {}]); },
    });

    // ── projectnotes/common ────────────────────────────────────────────────
    function escapeHtml(text) {
        return String(text === null || text === undefined ? "" : text)
            .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
    }
    function pad(n, width) { let s = String(n); while (s.length < width) s = "0" + s; return s; }
    function parseIso(iso) {
        const m = /^(\d{4})-(\d{2})-(\d{2})(?:[T ](\d{2}):(\d{2})(?::(\d{2}))?)?/.exec(String(iso || ""));
        if (!m) return null;
        return { y: +m[1], M: +m[2], d: +m[3], h: +(m[4] || 0), m: +(m[5] || 0), s: +(m[6] || 0) };
    }
    function formatDate(isoDate, pattern) {
        const t = parseIso(isoDate);
        if (!t) return "";
        const months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
        return String(pattern).replace(/yyyy|yy|MMMM|MMM|MM|M|dd|d|HH|H|hh|h|mm|ss|AP|ap/g, function (token) {
            switch (token) {
            case "yyyy": return pad(t.y, 4);
            case "yy": return pad(t.y % 100, 2);
            case "MMMM": return months[t.M - 1];
            case "MMM": return months[t.M - 1].slice(0, 3);
            case "MM": return pad(t.M, 2);
            case "M": return String(t.M);
            case "dd": return pad(t.d, 2);
            case "d": return String(t.d);
            case "HH": return pad(t.h, 2);
            case "H": return String(t.h);
            case "hh": return pad(((t.h + 11) % 12) + 1, 2);
            case "h": return String(((t.h + 11) % 12) + 1);
            case "mm": return pad(t.m, 2);
            case "ss": return pad(t.s, 2);
            case "AP": return t.h < 12 ? "AM" : "PM";
            case "ap": return t.h < 12 ? "am" : "pm";
            }
            return token;
        });
    }
    function parseDate(text, pattern) {
        const s = String(text || "").trim();
        if (!pattern) {
            if (/^\d{4}-\d{2}-\d{2}/.test(s)) return s.slice(0, 10);
            const us = /^(\d{1,2})\/(\d{1,2})\/(\d{2,4})$/.exec(s);
            if (!us) return null;
            let y = +us[3];
            if (y < 100) y += 2000;
            return pad(y, 4) + "-" + pad(+us[1], 2) + "-" + pad(+us[2], 2);
        }
        const order = [];
        const regex = new RegExp("^" + String(pattern).replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
            .replace(/yyyy|MM|dd|M|d/g, function (tok) { order.push(tok); return tok === "yyyy" ? "(\\d{4})" : "(\\d{1,2})"; }) + "$");
        const m = regex.exec(s);
        if (!m) return null;
        const parts = {};
        order.forEach(function (tok, i) { parts[tok[0]] = +m[i + 1]; });
        if (!parts.y || !parts.M || !parts.d) return null;
        return pad(parts.y, 4) + "-" + pad(parts.M, 2) + "-" + pad(parts.d, 2);
    }
    // "{{ dotted.path }}" tokens, the same grammar the email and meeting
    // templates use (ProjectNotesIntegrations/TemplateParser.cpp): whitespace
    // inside the braces is trimmed, "\{{" writes a literal "{{", and each
    // segment is a letter or "_" followed by letters, digits or "_". Digit-only
    // segments and "*" are also allowed here, as row numbers. `values` is an object
    // keyed by the full path, or a function(path) returning the value. A token
    // that is invalid or has no value renders as nothing; an unclosed one is
    // kept as written. Options: html escapes each value.
    function render(template, values, options) {
        const input = String(template);
        const html = !!(options && options.html);
        const lookup = typeof values === "function" ? values : function (path) {
            return values && Object.prototype.hasOwnProperty.call(values, path) ? values[path] : undefined;
        };
        let out = "";
        for (let i = 0; i < input.length;) {
            if (input.substr(i, 3) === "\\{{") { out += "{{"; i += 3; continue; }
            if (input.substr(i, 2) !== "{{") { out += input[i++]; continue; }
            const end = input.indexOf("}}", i + 2);
            if (end < 0) { out += input.slice(i); break; }
            const key = input.slice(i + 2, end).trim();
            const valid = key !== "" && key.split(".").every(function (part) {
                return /^(?:[A-Za-z_][A-Za-z0-9_]*|\d+|\*)$/.test(part) && part !== "__proto__" && part !== "prototype" && part !== "constructor";
            });
            let value = valid ? lookup(key) : undefined;
            if (value && typeof value === "object" && "id" in value) value = value.lookup !== null ? value.lookup : value.id;
            if (value !== null && value !== undefined) out += html ? escapeHtml(String(value)) : String(value);
            i = end + 2;
        }
        return out;
    }
    // Rewrite the "[$entity.column.row]" and "[$name]" spelling earlier
    // versions saved into "{{ entity.row.column }}" and "{{ name }}".
    function upgradeVariables(template) {
        return String(template)
            .replace(/\[\$([A-Za-z0-9_]+)\.([A-Za-z0-9_]+)\.(\d+)\]/g, "{{ $1.$3.$2 }}")
            .replace(/\[\$([A-Za-z0-9_]+)\]/g, "{{ $1 }}");
    }
    // Fill a template from a record and optional named extras. Paths:
    //   entity.column        the record (or its first child row) for that entity
    //   entity.N.column      row N (1-based) of a child array, or the record when N is 1
    //   entity.*.column      that column of every row, empty ones skipped, joined with ", "
    //   name                 any key of `extras`, dots included ("report.date")
    // Ref values substitute their display value. Old "[$...]" spellings still work.
    function cell(value) {
        if (value && typeof value === "object" && "id" in value) value = value.lookup !== null ? value.lookup : value.id;
        return value === null || value === undefined ? "" : String(value);
    }
    function replaceVariables(template, record, extras) {
        function fromRow(entity, row, column) {
            const index = Math.max(1, +row) - 1;
            let source = null;
            if (record && record.$entity === entity && index === 0) source = record;
            else if (record && Array.isArray(record[entity])) source = record[entity][index] || null;
            return source ? source[column] : undefined;
        }
        function fromAllRows(entity, column) {
            let rows = [];
            if (record && Array.isArray(record[entity])) rows = record[entity];
            else if (record && record.$entity === entity) rows = [record];
            return rows.map(function (row) { return cell(row[column]); })
                       .filter(function (v) { return v !== ""; }).join(", ");
        }
        return render(upgradeVariables(template), function (path) {
            if (extras && Object.prototype.hasOwnProperty.call(extras, path)) return extras[path];
            const parts = path.split(".");
            if (parts.length === 2) return fromRow(parts[0], 1, parts[1]);
            if (parts.length === 3 && parts[1] === "*") return fromAllRows(parts[0], parts[2]);
            if (parts.length === 3 && /^\d+$/.test(parts[1])) return fromRow(parts[0], parts[1], parts[2]);
            return undefined;
        });
    }
    const common = freeze({
        // task(function* (a, b) { const x = yield promise; ... }) returns a
        // function that runs the generator as a coroutine and returns a Promise.
        task: function (generatorFunction) {
            if (typeof generatorFunction !== "function") throw new PluginError("validation-failed", "task() needs a generator function.");
            return function () { return settle(generatorFunction.apply(this, arguments)); };
        },
        all: function (items) { return settle(items); },
        render: render,
        replaceVariables: replaceVariables,
        upgradeVariables: upgradeVariables,
        escapeHtml: escapeHtml,
        textToHtml: function (text) { return escapeHtml(text).replace(/\r\n|\r|\n/g, "<br>"); },
        validFilename: function (name) { return String(name).replace(/[\\/:*?"<>|]/g, "_").replace(/\s+$/g, "").trim(); },
        formatDate: formatDate,
        parseDate: parseDate,
    });

    const api = {
        plugin: plugin, data: data, app: app, menus: menus, ui: ui, settings: settings, shortcuts: shortcuts, secrets: secrets,
        storage: storage, files: files, projectFiles: projectFiles, email: email, calendar: calendar, contacts: contacts,
        office365: office365, http: http, auth: auth, timers: timers, log: log, reports: reports, PluginError: PluginError,
    };
    if (capabilitySet.has("com")) api.com = com;
    if (capabilitySet.has("process.launch")) api.process = processApi;
    freeze(api);

    // Globals every plugin expects.
    const console = freeze({ log: log.info, info: log.info, debug: log.debug, warn: log.warn, error: log.error });

    // ── Hooks the host calls ───────────────────────────────────────────────
    let moduleNamespace = null;
    const hooks = {
        api: api,
        common: common,
        globals: {
            console: console,
            setTimeout: function (fn, ms) { return startTimer(fn, ms, false, Array.prototype.slice.call(arguments, 2)); },
            setInterval: function (fn, ms) { return startTimer(fn, ms, true, Array.prototype.slice.call(arguments, 2)); },
            clearTimeout: stopTimer,
            clearInterval: stopTimer,
        },
        setModule: function (ns) { moduleNamespace = ns; },
        resolve: function (id, value) {
            const p = pending.get(id);
            if (!p) return;
            pending.delete(id);
            p.resolve(value);
        },
        reject: function (id, error) {
            const p = pending.get(id);
            if (!p) return;
            pending.delete(id);
            p.reject(toError(error));
        },
        fireTimer: function (id) {
            const t = timerCallbacks.get(id);
            if (!t) return;
            if (!t.repeat) timerCallbacks.delete(id);
            safely("timer", t.fn, t.args);
        },
        hasExport: function (name) {
            return !!(moduleNamespace && typeof moduleNamespace[name] === "function");
        },
        // Lifecycle exports. Returns a promise so the host can wait for onStop.
        callExport: function (name, args) {
            if (!moduleNamespace || typeof moduleNamespace[name] !== "function") return Promise.resolve();
            try {
                return settle(moduleNamespace[name].apply(null, args || [])).then(undefined, function (e) { report(name, e); });
            } catch (e) {
                report(name, e);
                return Promise.resolve();
            }
        },
        menu: function (menuId, context) {
            const handler = menuHandlers.get(menuId);
            if (handler) {
                host.log("info", "menu " + menuId + " goes to its registered handler");
                return safely("menu " + menuId, handler, [context]);
            }
            if (moduleNamespace && typeof moduleNamespace.onMenu === "function") {
                host.log("info", "menu " + menuId + " goes to onMenu");
                return safely("onMenu", moduleNamespace.onMenu, [menuId, context]);
            }
            report("menu", new PluginError("not-found", "No handler for menu \"" + menuId + "\"."));
        },
        // QML -> plugin calls (pluginDialog.invoke / pluginSettings.invoke).
        invoke: function (replyId, token, name, args) {
            let fn = null;
            const handlers = token ? dialogHandlers.get(token) : null;
            if (handlers && typeof handlers[name] === "function") fn = handlers[name];
            else if (moduleNamespace && typeof moduleNamespace[name] === "function") fn = moduleNamespace[name];
            if (!fn) {
                host.reply(replyId, false, { code: "not-found", message: "No handler named \"" + name + "\"." });
                return;
            }
            Promise.resolve().then(function () { return settle(fn.apply(null, Array.isArray(args) ? args : [args])); })
                .then(function (value) { host.reply(replyId, true, value === undefined ? null : value); },
                      function (e) { const err = toError(e); host.reply(replyId, false, { code: err.code, message: err.message }); });
        },
        deliver: function (kind, payload) {
            const list = (subscriptions[kind] || []).slice();
            if (kind === "data.changed") {
                list.forEach(function (s) {
                    payload.forEach(function (change) {
                        if (s.entities.indexOf(change.entity) >= 0) safely("data.onChanged", s.handler, [change]);
                    });
                });
                return;
            }
            list.forEach(function (s) { safely(kind, s.handler, [payload]); });
            if (kind === "settings.changed" && moduleNamespace && typeof moduleNamespace.onSettingsChanged === "function")
                safely("onSettingsChanged", moduleNamespace.onSettingsChanged, [payload]);
        },
    };
    return hooks;
})
