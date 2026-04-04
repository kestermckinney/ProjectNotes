# Developer Profile

The `--developer-profile` command-line argument lets you run a completely isolated instance of Project Notes alongside your production installation. It is intended for plugin developers and testers who need to experiment without touching their real data.

## What It Isolates

When `--developer-profile PROFILENAME` is specified, Project Notes changes three things:

| Resource | Production | Developer profile |
|---|---|---|
| Application settings | `ProjectNotes` organization key | `ProjectNotesPROFILENAME` organization key |
| Database file | `<AppData>/ProjectNotes.db` | `<AppData>/PROFILENAME/ProjectNotes.db` |
| Log files | `<AppData>/logs/` | `<AppData>/PROFILENAME/logs/` |
| Organization domain | `projectnotes.com` | `PROFILENAME.projectnotes.com` |

Both the settings and the database are stored in your platform's standard app-data location. The developer profile places its database and logs in a named subfolder, so the two instances never interfere with each other.

## Usage

```bash
# macOS / Linux
ProjectNotes --developer-profile dev

# Windows
ProjectNotes.exe --developer-profile dev
```

Replace `dev` with any short alphanumeric name. The same name must be used each time you launch that profile so the settings and database persist between sessions.

## Typical Workflow

1. Launch with `--developer-profile dev` (or whatever name you chose).
2. A fresh, empty database is created in the `dev/` subfolder of the app-data directory on first run.
3. Populate it with test data — imports, plugin runs, XML files, etc. — without touching the production database.
4. When you are done, simply close the window. All test data stays in the profile-specific files.

## Notes for Plugin Developers

The profile name is also exposed to Python plugins via the `projectnotes.developer_profile` attribute. You can use this to enable extra logging or bypass certain guards during development:

```python
import projectnotes as pn

if pn.developer_profile:
    print(f"Running under developer profile: {pn.developer_profile}")
```

The attribute is an empty string when Project Notes is running in production mode.
