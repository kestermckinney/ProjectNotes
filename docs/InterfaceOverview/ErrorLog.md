# Error Log

Project Notes maintains an `error.log` file that captures errors from plugins and from the application itself. This log is the primary place to look when a plugin is not behaving as expected or when the application encounters a problem at runtime.

## Log File Location

Log files are stored in the application data folder for your platform.

**Desktop:**

| Platform | Path |
| :--- | :--- |
| Windows | `%APPDATA%\ProjectNotes\logs\error.log` |
| macOS | `~/Library/Application Support/ProjectNotes/logs/error.log` |
| Linux | `~/.local/share/ProjectNotes/logs/error.log` |

**Project Notes Mobile:**

| Platform | Path |
| :--- | :--- |
| iOS | Inside the app's sandboxed data container under `logs/error.log` |

The `logs/` folder is created automatically on first launch if it does not exist.

## What Gets Logged

`error.log` records **Error** level messages only. This includes:

- **Plugin errors** — When a plugin throws an error it does not handle, the message and the JavaScript stack (file and line) are written here, starting with the plugin's id and where the error happened, such as `onMenu` or a timer.
- **Plugin log messages** — `log.error()` and `console.error()` in a plugin write here; `log.info()`, `log.warn()` and `console.log()` go to `console.log`.
- **Stopped plugins** — A plugin call that runs too long is stopped and noted here.
- **Application errors** — Internal application errors that are logged at the Error level also appear here.

A typical plugin error entry looks like:

```
[2026-10-13 10:22:03] [ERROR] [com.example.my-plugin] onMenu: fetchData is not defined
fetchData@file:///home/you/.local/share/projectnotes/plugins/my-plugin/main.mjs:42
```

## Related Log Files

Several additional log files are written to the same `logs/` folder:

| File | Level | Contents |
| :--- | :--- | :--- |
| `error.log` | Error | Plugin errors, application errors |
| `console.log` | Info | General informational messages and routine activity |
| `syncerrors.log` | Warning | Cloud sync failures (see below) |
| `debugging.log` | Debug | Verbose diagnostic output — written only in Debug builds |

### Sync Error Log

`syncerrors.log` is a dedicated log for **cloud sync** problems. Failures raised deep inside the synchronization engine are routed to this file so they are never lost or silently swallowed. If your data does not appear to be syncing correctly, this is the first log to check.

Typical entries include connectivity failures, authentication problems, and records the server rejected. Each entry is written at the **Warning** level with a timestamp.

## Viewing Logs on Desktop

The built-in **Log Viewer** lets you inspect all of these log files without leaving the application. Open it by choosing **Log Viewer** from the app menu (the menu icon at the top of the icon rail). It opens as its own window with one tab per log file; select **Clear Log** in its footer to delete the file backing the current tab, or **Close** to hide the window (it keeps tailing the logs in the background so reopening it is instant).

## Sending Logs to Support

If you need help diagnosing a problem, Project Notes can package your log files and email them to support for you. From the app menu choose **Send Logs to Support…** — Project Notes zips `error.log`, `console.log`, and `syncerrors.log` and attaches the archive to a new email addressed to support.

## Troubleshooting Plugins

If a plugin fails silently or produces unexpected results:

1. Open `error.log` (or use the Log Viewer).
2. Search for your plugin name.
3. The full traceback will show the file, line number, and exception that caused the problem.
4. Fix the issue in your plugin, save the file, and the plugin will reload automatically.
