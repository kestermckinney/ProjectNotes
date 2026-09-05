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

Descriptions are source-qualified (`File Finder:` and `Office 365:`) to respect
the existing per-project description uniqueness rule. Existing legacy rows are
adopted by matching project and normalized path. Reconciliation intentionally
does not delete locations that disappear from a source.

Tenant/client identifiers and finder rules live in `AppSettings`. Microsoft
refresh tokens are stored only through `CredentialStore`; access tokens remain
in memory.
