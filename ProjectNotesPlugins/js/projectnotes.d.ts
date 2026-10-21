// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
//
// Type declarations for Project Notes plugins (apiVersion 1). Reference this
// file from a plugin folder (for example with a jsconfig.json) to get editor
// completion and type checking:
//
//   { "compilerOptions": { "checkJs": true }, "include": ["*.mjs", "projectnotes.d.ts"] }
//
// The implementation is ProjectNotesPlugins/js/prelude.js.

declare module "projectnotes" {
    // ── Shared types ───────────────────────────────────────────────────────
    export interface Ref { id: string | null; lookup: string | null; }

    export class PluginError extends Error {
        code: string;            // "permission-denied", "not-found", "validation-failed",
                                 // "reference-blocked", "deleted", "save-failed", "cancelled",
                                 // "unavailable", "timeout", "network", "com-failed", ...
        permission?: string;     // the missing permission for "permission-denied"
        details?: { [key: string]: any };
    }

    export type Value = string | number | boolean | null;

    export interface DataRecord {
        readonly $entity: string;
        readonly $key: string;
        [column: string]: Value | Ref | DataRecord[];
    }

    export interface Operator {
        $ne?: Value; $lt?: Value; $lte?: Value; $gt?: Value; $gte?: Value;
        $like?: string; $null?: boolean; $between?: [Value, Value];
    }
    export type Where = { [column: string]: Value | Value[] | Ref | { lookup: string } | Operator };
    export type OrderBy = (string | { column: string; descending?: boolean })[];
    export interface IncludeSpec { entity: string; where?: Where; include?: (string | IncludeSpec)[]; orderBy?: OrderBy; }
    export interface Query {
        where?: Where;
        include?: (string | IncludeSpec)[];
        fields?: string[];
        orderBy?: OrderBy;
        limit?: number;
        offset?: number;
    }

    export interface ColumnInfo {
        name: string; displayName: string;
        type: "string" | "text" | "date" | "datetime" | "bool" | "number" | "currency" | "percent" | "ref";
        required: boolean; readOnly: boolean; unique: boolean; searchable: boolean;
        ref?: { entity: string };
        choices?: string[];
    }
    export interface EntityInfo {
        name: string; displayName: string; key: string;
        uniqueKeys: string[][];
        columns: ColumnInfo[];
        children: { entity: string; column: string; owned: boolean }[];
    }

    export interface SaveResult { record: DataRecord; operation: "insert" | "update"; }
    export interface RemoveResult { removed: { entity: string; key: string }[]; }
    export interface ImportResult { inserted: number; updated: number; removed: number; }
    export interface ChangeEvent {
        entity: string; key: string; operation: "insert" | "update" | "delete";
        source: "app" | "plugin";      // "app" covers user edits and sync
        pluginId?: string;             // set when source is "plugin"
    }
    export interface Subscription { unsubscribe(): void; }

    export interface MenuContext {
        menuId: string;
        kind: "record" | "global";
        entity: string | null;
        key: string | null;
        record: DataRecord | null;     // loaded with the menu's "include"
        parameter: string;
    }

    // ── plugin ─────────────────────────────────────────────────────────────
    export const plugin: {
        readonly id: string; readonly name: string; readonly version: string; readonly apiVersion: number;
        readonly bundled: boolean; readonly packagePath: string; readonly dataPath: string;
    };

    // ── data ───────────────────────────────────────────────────────────────
    export interface DataOps {
        get(entity: string, keyOrWhere: string | Where, options?: Omit<Query, "where">): Promise<DataRecord | null>;
        list(entity: string, query?: Query): Promise<DataRecord[]>;
        count(entity: string, where?: Where): Promise<number>;
        save(entity: string, record: object): Promise<SaveResult>;
        saveAll(entity: string, records: object[]): Promise<SaveResult[]>;
        remove(entity: string, keyOrRecord: string | DataRecord): Promise<RemoveResult>;
    }
    export const data: DataOps & {
        entities(): Promise<EntityInfo[]>;
        schema(entity: string): Promise<EntityInfo>;
        search(text: string, options?: { entities?: string[]; limit?: number })
            : Promise<{ entity: string; key: string; title: string; snippet: string }[]>;
        transaction<T>(work: (tx: DataOps) => Promise<T>): Promise<T>;
        onChanged(entities: string | string[], handler: (e: ChangeEvent) => void): Subscription;
        exportXml(entity: string, key: string, options?: { include?: string[] }): Promise<string>;
        importXml(xml: string): Promise<ImportResult>;
    };

    // ── app ────────────────────────────────────────────────────────────────
    export interface ViewInfo { section: string; entity: string | null; key: string | null; tab: string | null; }
    export const app: {
        readonly version: string; readonly apiVersion: number;
        readonly platform: "windows" | "macos" | "linux";
        readonly buildFlavor: "standard" | "appstore";
        readonly profile: string; readonly locale: string;
        readonly capabilities: { has(name: string): boolean; list(): string[] };
        currentView(): ViewInfo;
        currentRecord(options?: { include?: string[] }): Promise<DataRecord | null>;
        navigate(entity: string, key: string, options?: { tab?: string }): Promise<void>;
        /** The project's primary project folder path, or null when it has none. */
        projectFolder(projectKey: string): Promise<string | null>;
        defaults(): Promise<{ managingCompanyId: string | null; managingCompanyName: string | null;
                              projectManagerId: string | null; projectManagerName: string | null }>;
        refresh(): void;
        onViewChanged(handler: (view: ViewInfo) => void): Subscription;
    };

    // ── menus ──────────────────────────────────────────────────────────────
    export interface MenuItem {
        id: string; title: string;
        context: "record" | "global";
        entity?: string; submenu?: string; include?: string[]; parameter?: string;
        platforms?: ("windows" | "macos" | "linux")[];
        handler?: (context: MenuContext) => void | Promise<void>;
    }
    export const menus: {
        register(items: MenuItem | MenuItem[]): void;
        unregister(ids: string | string[]): void;
        clear(): void;
        list(): MenuItem[];
    };

    // ── ui ─────────────────────────────────────────────────────────────────
    export interface FormField {
        key: string; label: string;
        type: "text" | "multiline" | "password" | "number" | "date" | "checkbox" | "choice" | "combo"
            | "file" | "folder" | "contact" | "table" | "section" | "note";
        required?: boolean; placeholder?: string; help?: string;
        choices?: string[]; /* a "choice" table column is a drop-down with a blank entry */ filters?: string[]; columns?: FormField[];
        visibleWhen?: { [key: string]: any };
        /** Text cells in a table: `{{ path }}` placeholders listed under Insert Field in the cell's right-click menu. */
        variables?: string[];
        /** Also list `variablesByValue[row[variablesBy]]`, so the choices follow another column of the same row. */
        variablesBy?: string; variablesByValue?: { [value: string]: string[] };
    }
    export interface FormSchema { fields: FormField[]; }
    export interface Progress { update(fraction: number | null, text?: string): void; readonly cancelled: boolean; }
    export const ui: {
        message(text: string, options?: { title?: string; kind?: "info" | "warning" | "error" }): Promise<void>;
        confirm(text: string, options?: { title?: string; okText?: string; cancelText?: string; destructive?: boolean }): Promise<boolean>;
        prompt(label: string, options?: { title?: string; value?: string; placeholder?: string; multiline?: boolean; password?: boolean }): Promise<string | null>;
        choose<T extends string>(label: string, choices: T[], options?: { title?: string; multiple?: boolean }): Promise<T | T[] | null>;
        showForm(form: FormSchema, values?: object, options?: { title?: string; okText?: string; width?: number }): Promise<object | null>;
        showDialog(qmlFile: string, data?: object, options?: {
            title?: string; width?: number; height?: number;
            handlers?: { [name: string]: (...args: any[]) => any };
        }): Promise<any | null>;
        pickFile(options?: { title?: string; filters?: string[]; startIn?: string; multiple?: boolean }): Promise<string | string[] | null>;
        pickFolder(options?: { title?: string; startIn?: string }): Promise<string | null>;
        saveFile(options?: { title?: string; suggestedName?: string; filters?: string[] }): Promise<string | null>;
        notify(text: string, options?: { kind?: "info" | "success" | "warning" | "error"; seconds?: number }): void;
        progress<T>(title: string, task: (p: Progress) => Promise<T>, options?: { cancellable?: boolean }): Promise<T>;
        openUrl(url: string): Promise<boolean>;
        clipboard: { setText(text: string): void; text(): Promise<string> };
    };

    // ── settings / secrets / storage / files ───────────────────────────────
    export const settings: {
        get<T = any>(key: string, defaultValue?: T): T;
        set(key: string, value: any): void;
        remove(key: string): void;
        keys(): string[];
        all(): { [key: string]: any };
        onChanged(handler: (keys: string[]) => void): Subscription;
    };
    export const secrets: {
        get(account: string): Promise<string | null>;
        set(account: string, secret: string): Promise<void>;
        remove(account: string): Promise<void>;
    };
    export const storage: {
        readonly path: string;
        readText(path: string): Promise<string | null>;
        writeText(path: string, text: string): Promise<void>;
        readJson<T = any>(path: string): Promise<T | null>;
        writeJson(path: string, value: any): Promise<void>;
        remove(path: string): Promise<void>;
        list(dir?: string): Promise<string[]>;
    };
    export interface FileInfo { path: string; name: string; isDir: boolean; size: number; modified: string; }
    export const files: {
        exists(path: string): Promise<boolean>;
        stat(path: string): Promise<FileInfo | null>;
        list(dir: string, options?: { filter?: string[]; recursive?: boolean }): Promise<FileInfo[]>;
        readText(path: string): Promise<string>;
        writeText(path: string, text: string): Promise<void>;
        readBytes(path: string): Promise<ArrayBuffer>;
        writeBytes(path: string, bytes: ArrayBuffer): Promise<void>;
        copy(from: string, to: string, options?: { overwrite?: boolean }): Promise<void>;
        move(from: string, to: string, options?: { overwrite?: boolean }): Promise<void>;
        mkdir(path: string): Promise<void>;
        remove(path: string): Promise<void>;
        packagePath(relative: string): string;
        tempPath(name: string): string;
        openWithSystem(path: string): Promise<boolean>;
        reveal(path: string): Promise<void>;
    };

    // ── integrations ───────────────────────────────────────────────────────
    export type Recipient = string | { name?: string; email: string } | { contact: string };
    export interface EmailMessage {
        to?: Recipient[]; cc?: Recipient[]; bcc?: Recipient[];
        subject: string; html?: string; text?: string; attachments?: string[]; projectKey?: string;
    }
    export const email: {
        backends(): Promise<{ id: string; name: string; available: boolean; preferred: boolean; supportsHtml: boolean; supportsAttachments: boolean }[]>;
        compose(message: EmailMessage): Promise<{ status: "handed-off" | "cancelled"; backend?: string }>;
        audiences(projectKey: string, workflow?: string): Promise<{ id: string; name: string; workflow: string; isDefault: boolean }[]>;
        resolveAudience(projectKey: string, audienceId: string): Promise<{ to: Recipient[]; cc: Recipient[]; bcc: Recipient[] }>;
        templates(): Promise<{ id: string; name: string; workflow: string }[]>;
        renderTemplate(templateId: string, context: { projectKey?: string; noteKey?: string }): Promise<{ subject: string; html: string }>;
    };
    export interface Meeting {
        subject: string; start: string; end: string; allDay?: boolean; location?: string;
        attendees?: Recipient[]; html?: string; projectKey?: string; noteKey?: string;
    }
    export const calendar: {
        backends(): Promise<{ id: string; name: string; available: boolean; preferred: boolean }[]>;
        draftMeeting(meeting: Meeting): Promise<{ status: "handed-off" | "cancelled"; backend?: string }>;
        writeIcs(meeting: Meeting, path: string): Promise<string>;
    };
    export const contacts: {
        sources(): Promise<{ id: string; name: string; kind: string; enabled: boolean; lastSync: string | null }[]>;
        syncNow(sourceId?: string): Promise<{ added: number; updated: number; conflicts: number }>;
        pick(options?: { multiple?: boolean; projectKey?: string }): Promise<string | string[] | null>;
        toVCard(contactKey: string): Promise<string>;
        fromVCard(text: string): Promise<object[]>;
    };
    export interface GraphResponse { status: number; headers: { [name: string]: string }; body: any; }
    export const office365: {
        status(): Promise<{ signedIn: boolean; account: string | null; scopes: string[] }>;
        signIn(): Promise<boolean>;
        graph(method: "GET" | "POST" | "PATCH" | "PUT" | "DELETE", path: string, options?: {
            body?: any; query?: { [k: string]: string }; headers?: { [k: string]: string };
            responseType?: "json" | "text" | "arraybuffer" }): Promise<GraphResponse>;
        graphPages(path: string, options?: { query?: { [k: string]: string } }): AsyncIterable<any>;
    };
    export interface HttpResponse {
        status: number; ok: boolean; headers: { [name: string]: string };
        text(): Promise<string>; json(): Promise<any>; arrayBuffer(): Promise<ArrayBuffer>;
    }
    export const http: {
        fetch(url: string, options?: {
            method?: string; headers?: { [k: string]: string };
            body?: string | ArrayBuffer | object;
            auth?: { type: "basic"; username: string; password: string } | { type: "bearer"; token: string };
            timeoutSeconds?: number }): Promise<HttpResponse>;
    };
    // Needs the "auth.browser" permission; the url's host must be in "http".
    export const auth: {
        browserSignIn(options: { url: string; redirectUri: string; timeoutSeconds?: number }): Promise<{ url: string }>;
    };
    export interface Timer { cancel(): void; }
    export const timers: {
        every(minutes: number, job: () => void | Promise<void>, options?: { runAtStart?: boolean }): Timer;
        after(milliseconds: number, job: () => void | Promise<void>): Timer;
    };
    export const log: {
        debug(...args: any[]): void; info(...args: any[]): void; warn(...args: any[]): void; error(...args: any[]): void;
    };

    // Windows only: present when app.capabilities.has("com").
    export interface ComObject {
        [member: string]: any;
        $call(name: string, ...args: any[]): any;
        $get(name: string, ...index: any[]): any;
        $set(name: string, value: any, ...index: any[]): void;
        [Symbol.iterator](): Iterator<any>;
    }
    export const com: {
        createObject(progId: string): ComObject;
        getActiveObject(progId: string): ComObject | null;
        getObject(moniker: string): ComObject;
        release(object: ComObject): void;
    } | undefined;

    // Present when app.capabilities.has("process.launch") (not in App Store builds).
    export const process: {
        launch(program: string, args?: string[], options?: { workingDirectory?: string }): Promise<{ pid: number }>;
    } | undefined;

    export const reports: {
        generate(workflow: "tracker-items" | "status" | "meeting-notes", projectKey: string, options?: object)
            : Promise<{ pdf?: string; html?: string }>;
    };
}

declare module "projectnotes/common" {
    import { DataRecord } from "projectnotes";
    /** Fill "{{ dotted.path }}" tokens, the grammar email templates use. `values` is keyed by full path or is a lookup function. Missing values render as nothing; `\{{` writes a literal `{{`. */
    export function render(template: string, values: { [path: string]: any } | ((path: string) => any),
                           options?: { html?: boolean }): string;
    /** Fill a template from a record: `{{ projects.project_number }}`, `{{ project_team.2.role }}` (row 2), `{{ project_team.*.email }}` (every row, joined with ", "), plus named `extras` such as `{{ report.date }}`. Old `[$entity.column.row]` spellings still work. */
    export function replaceVariables(template: string, record: DataRecord, extras?: { [name: string]: any }): string;
    /** Rewrite the older `[$entity.column.row]` and `[$name]` spellings to `{{ }}` tokens. */
    export function upgradeVariables(template: string): string;
    export function escapeHtml(text: string): string;
    export function textToHtml(text: string): string;
    export function validFilename(name: string): string;
    export function formatDate(isoDate: string, pattern: string): string;
    export function parseDate(text: string, pattern?: string): string | null;
}

// Plugin entry points (export any of these from main.mjs).
declare function onStart(): void | Promise<void>;
declare function onStop(): void | Promise<void>;
declare function onMenu(menuId: string, context: import("projectnotes").MenuContext): void | Promise<void>;
declare function onSettingsChanged(keys: string[]): void | Promise<void>;
