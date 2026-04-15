# Error Log

Project Notes maintains an `error.log` file that captures errors from the embedded Python plugin system and other application-level errors. This log is the primary place to look when a plugin is not behaving as expected or when the application encounters a problem at runtime.

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

- **Python plugin exceptions** — When a plugin raises an unhandled exception, the full Python traceback (exception type, message, file name, and line number) is written to the error log. The entry identifies the plugin module by name so you know exactly which plugin caused the problem.
- **Python stderr output** — Any text written to `sys.stderr` inside a plugin (for example, from `print(..., file=sys.stderr)`) is captured and forwarded to the error log.
- **Application errors** — Internal application errors that are logged at the Error level also appear here.

A typical Python plugin error entry looks like:

```
[2025-04-15 10:22:03] [ERROR] Python error in plugin my_plugin:
Traceback (most recent call last):
  File "/Users/you/Project Notes/plugins/my_plugin.py", line 42, in event_timer
    result = fetch_data()
NameError: name 'fetch_data' is not defined
```

## Related Log Files

Two additional log files are written to the same `logs/` folder:

| File | Level | Contents |
| :--- | :--- | :--- |
| `error.log` | Error | Plugin exceptions, stderr output, application errors |
| `console.log` | Info | General informational messages and routine activity |
| `debugging.log` | Debug | Verbose diagnostic output — written only in Debug builds |

## Viewing Logs on Desktop

The built-in **Log Viewer** (available from the **Help** menu) lets you inspect all three log files without leaving the application.

## Troubleshooting Plugins

If a plugin fails silently or produces unexpected results:

1. Open `error.log` (or use the Log Viewer).
2. Search for your plugin name.
3. The full traceback will show the file, line number, and exception that caused the problem.
4. Fix the issue in your plugin, save the file, and the plugin will reload automatically.
