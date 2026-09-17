# Native report-plugin removal manifest

Status: preparation only. Do not delete these files until G6 is explicitly
approved and the optional Widgets scope has been decided.

| Bundled file | Native replacement | Removal condition |
| --- | --- | --- |
| `plugins/exportnotes_plugin.py` | Send Meeting Notes and Meeting Notes Report native preparation/builders | All four native paths, packaged builds, and Widgets compatibility reviewed. |
| `plugins/exportstatusreport_plugin.py` | Native Status Report builder and review route | Same G6 approval; preserve user-modified copies. |
| `plugins/exporttrackeritems_plugin.py` | Native Tracker Items Report builder and review route | Same G6 approval; preserve user-modified copies. |

The following are intentionally **not** in this manifest: generic email and
meeting definitions, shared plugin helpers, user plugins, unrelated report
plugins, the optional Widgets frontend, and user data/settings. Upgrade cleanup
must identify the shipped bundled files exactly and never recursively remove a
plugin directory.
