# Project Notes File Finder

Native File Finder and Microsoft 365 integration for the QML desktop app.

The service owns a low-priority worker thread. Qt SQL connections are
thread-affine, so the worker opens its own named connection to the same
`ProjectNotes.db` file and coordinates every query/transaction with the shared
Project Notes `QReadWriteLock`. Filesystem and Microsoft Graph traversal happen
outside that lock.

A scan performs these operations:

1. Read active, non-deleted projects in one query.
2. Traverse each configured local root once and compile all regular expressions
   once. Microsoft joined teams and channels are also enumerated once per pass.
3. Preload existing active `project_locations`, reconcile in memory, and commit
   all changed locations in one transaction.
4. Refresh the Locations and search models once after the transaction.

File descriptions use `<classification> : <base filename.ext>` and are deliberately
not source-qualified. Local folders and Microsoft Teams may both be scanned; a
result from either source with the same project, classification, and filename
reconciles into the same location row.
Existing legacy rows are adopted by normalized path or by their description with
an old source prefix removed.
Reconciliation intentionally does not delete locations that disappear from a source.

Folder exclusion expressions are case-insensitive and checked against both a
folder's name and normalized full path, including a directory-form path ending
in `/`. A matching folder is skipped without enumerating its subtree. Exclusions
apply while locating local project folders, while scanning files within a
matched local project folder, and while traversing Microsoft Teams folders.

Microsoft Teams scans persist `lastModifiedDateTime` by Graph drive/item ID for
each visited folder. An unchanged project folder skips the entire remote tree;
when a project changes, unchanged child folders are still pruned individually.
Changing file rules or folder exclusions invalidates this state, as does signing
out or choosing **Reconsider All Files**.

Tenant/client identifiers and finder rules live in `AppSettings`. Microsoft
refresh tokens are stored only through `CredentialStore`; access tokens remain
in memory. **Reconsider All Files** discards the current scan summary and queues
a completely fresh discovery pass, rebuilding the project-folder, timestamp,
and reconciliation hashes without changing saved rules or locations.

New profiles start with `~` as the portable local search root, exclude
`Engineering/.*` folder trees, and receive the maintained default classification
rules. The first-run migration preserves any roots or rules found in the legacy
File Finder settings rather than replacing an existing user's configuration.
