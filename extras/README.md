# Separately distributed plugins

Plugins in this folder do not ship with Project Notes. Each one is packed into a `.pnplugin` file that users install from **Settings > Plugins**, which copies it into their own plugins folder.

```bash
packaging/plugins/build-pnplugin.sh extras/ifs-cloud dist
```

On Windows, use the PowerShell version. It needs nothing beyond Windows PowerShell 5.1:

```powershell
powershell -ExecutionPolicy Bypass -File packaging\plugins\build-pnplugin.ps1 extras\ifs-cloud dist
```

Both scripts check the `.mjs` files' syntax when `node` is on the PATH and skip that check otherwise.

| Folder | Plugin |
| --- | --- |
| `ifs-cloud` | IFS Cloud: project, budget and team import, tracker item sync with the ISSUES activity, SSRS status reports, and IFS shortcut menus. |

## IFS Cloud

The manifest's `http` array lists the IFS Cloud host and the report server. Add your own hosts there before packing if they differ; Project Notes asks the user to approve the plugin again whenever that list changes.

Sign-in uses Keycloak through the browser and a redirect to `http://localhost:8080/callback`. The token and refresh token, and the report server password, are kept in the operating system's credential store. Settings saved by the Python IFS plugins carry over the first time the plugin starts.

Unlike the Python version, connections verify the server's TLS certificate. A server whose certificate is not trusted by the operating system has to be fixed on the server or added to the system's trusted certificates.
