# Copyright (C) 2025, 2026 Paul McKinney
from includes.common import ProjectNotesCommon
from PyQt6 import QtCore
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo, QElapsedTimer, QThread
from urllib3.exceptions import InsecureRequestWarning

import os
import base64
import time
import requests
import inspect
import json
import threading
import webbrowser
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
from requests_oauthlib import OAuth2Session
from urllib3.exceptions import InsecureRequestWarning

# Allow OAuth2 over plain HTTP for the localhost callback server.
# requests_oauthlib enforces HTTPS by default; this override is safe
# because only the local redirect uses HTTP — the token exchange itself
# still goes to the Keycloak HTTPS endpoint.
os.environ["OAUTHLIB_INSECURE_TRANSPORT"] = "1"
os.environ["OAUTHLIB_RELAX_TOKEN_SCOPE"] = "1"  # Keycloak expands scopes server-side; allow the mismatch

import projectnotes

requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)


class _SSOCallbackHandler(BaseHTTPRequestHandler):
    """Local HTTP handler that captures the OAuth2 redirect from Keycloak."""

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path != "/callback":
            self.send_response(204)
            self.end_headers()
            return

        self.server.auth_response_path = self.path

        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(b"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Project Notes &mdash; IFS Login</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(160deg, #1a2744 0%, #2a3f6f 60%, #1e3a5f 100%);
    font-family: 'Segoe UI', Arial, sans-serif;
  }
  .card {
    background: rgba(255,255,255,0.07);
    border: 1px solid rgba(255,255,255,0.15);
    border-radius: 12px;
    padding: 48px 56px;
    max-width: 460px;
    width: 90%;
    text-align: center;
    backdrop-filter: blur(4px);
  }
  .logo {
    margin-bottom: 28px;
  }
  .logo svg {
    width: 64px;
    height: 64px;
    filter: drop-shadow(0 2px 8px rgba(0,0,0,0.4));
  }
  .app-name {
    color: #ffffff;
    font-size: 22px;
    font-weight: 600;
    letter-spacing: 0.5px;
    margin-bottom: 6px;
  }
  .app-sub {
    color: rgba(255,255,255,0.55);
    font-size: 13px;
    letter-spacing: 1.5px;
    text-transform: uppercase;
    margin-bottom: 36px;
  }
  .check-circle {
    width: 64px;
    height: 64px;
    background: linear-gradient(135deg, #2ecc71, #27ae60);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 0 auto 24px;
    box-shadow: 0 4px 16px rgba(46,204,113,0.4);
    animation: pop 0.4s cubic-bezier(0.175,0.885,0.32,1.275);
  }
  @keyframes pop {
    from { transform: scale(0); opacity: 0; }
    to   { transform: scale(1); opacity: 1; }
  }
  .check-circle svg { width: 32px; height: 32px; }
  h1 {
    color: #ffffff;
    font-size: 20px;
    font-weight: 600;
    margin-bottom: 12px;
  }
  p {
    color: rgba(255,255,255,0.7);
    font-size: 14px;
    line-height: 1.6;
  }
  .divider {
    border: none;
    border-top: 1px solid rgba(255,255,255,0.12);
    margin: 32px 0 24px;
  }
  .footer {
    color: rgba(255,255,255,0.3);
    font-size: 11px;
  }
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <svg viewBox="0 0 64 64" fill="none" xmlns="http://www.w3.org/2000/svg">
      <!-- clipboard body -->
      <rect x="10" y="14" width="44" height="46" rx="4" fill="rgba(255,255,255,0.15)" stroke="rgba(255,255,255,0.6)" stroke-width="2"/>
      <!-- clip at top -->
      <rect x="22" y="8" width="20" height="12" rx="3" fill="rgba(255,255,255,0.25)" stroke="rgba(255,255,255,0.7)" stroke-width="2"/>
      <!-- lines -->
      <line x1="18" y1="30" x2="46" y2="30" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
      <line x1="18" y1="38" x2="46" y2="38" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
      <line x1="18" y1="46" x2="36" y2="46" stroke="rgba(255,255,255,0.6)" stroke-width="2" stroke-linecap="round"/>
    </svg>
  </div>
  <div class="app-name">Project Notes</div>
  <div class="app-sub">IFS Integration</div>
  <div class="check-circle">
    <svg viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
      <polyline points="6,17 13,24 26,9" stroke="white" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
    </svg>
  </div>
  <h1>Login Successful</h1>
  <p>You are now authenticated with IFS.<br>You can close this window and return to Project Notes.</p>
  <hr class="divider">
  <div class="footer">This window was opened automatically by Project Notes.</div>
</div>
</body>
</html>""")

        self.server.auth_code_event.set()

    def log_message(self, format, *args):
        return  # Suppress log spam


class SSOAuthAPI:
    """Handles Keycloak SSO authentication for IFS Cloud REST calls.

    Mirrors the structure of TokenAPI in graphapi_tools.py but uses the
    OAuth2 browser-redirect flow demonstrated in sso_example.py.
    """

    CLIENT_ID = "CCI_public"
    REDIRECT_URI = "http://localhost:8080/callback"
    SCOPE = ["openid"]  # CCI_public only exposes the openid scope

    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "IFS Cloud"

        self.ifs_url = self.pnc.get_plugin_setting("URL", self.settings_pluginname) or ""

        self.auth_url = None
        self.token_url = None

        self.temporary_folder = self.pnc.get_temporary_folder()
        self.token_cache_file = self.temporary_folder + "/ifs_sso_token_cache.json"

        self.token_response = None
        self.access_token = None
        self.current_user = None

    def _discover_endpoints(self):
        """Discover the Keycloak auth and token URLs from the IFS base URL.

        Makes an unauthenticated request to the IFS API. The server either
        returns a 401 with a WWW-Authenticate header pointing at the Keycloak
        realm, or redirects to the Keycloak login page (whose URL contains the
        realm path). Either way the OpenID Connect discovery document is then
        fetched to get the exact auth/token endpoint URLs.
        """
        import re

        probe_url = self.ifs_url.rstrip("/") + "/main/ifsapplications/projection/v1/"
        try:
            # allow_redirects=False so we can inspect the first response;
            # some IFS deployments return a redirect rather than a bare 401.
            response = requests.get(probe_url, verify=False, timeout=(10, 30), allow_redirects=False)
        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Could not reach IFS URL: {e}")
            return False

        realm_url = None

        if response.status_code == 401:
            # Standard case: 401 with WWW-Authenticate: Bearer realm="<url or name>"
            www_auth = response.headers.get("WWW-Authenticate", "")
            match = re.search(r'realm="([^"]+)"', www_auth)
            if match:
                realm_value = match.group(1)
                if realm_value.startswith("http"):
                    # Full URL realm — use directly
                    realm_url = realm_value
                elif "@" in realm_value:
                    # Format: "client_id@host/auth/realms/name/" — take the part after @
                    realm_url = "https://" + realm_value.split("@", 1)[1]
                else:
                    # Short realm name — construct the standard Keycloak path
                    realm_url = self.ifs_url.rstrip("/") + "/auth/realms/" + realm_value

        elif response.status_code in (301, 302, 307, 308):
            # Redirect case: Location header points at the Keycloak login page
            # whose path contains /auth/realms/<name>/
            location = response.headers.get("Location", "")
            match = re.search(r'(/auth/realms/[^/?#]+)', location)
            if match:
                parsed = urlparse(location)
                realm_url = f"{parsed.scheme}://{parsed.netloc}{match.group(1)}"

        if not realm_url:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Could not determine Keycloak realm URL. "
                  f"status={response.status_code}, "
                  f"WWW-Authenticate={response.headers.get('WWW-Authenticate', 'none')}, "
                  f"Location={response.headers.get('Location', 'none')}")
            return False

        # Fetch the OpenID Connect discovery document
        discovery_url = realm_url.rstrip("/") + "/.well-known/openid-configuration"
        try:
            discovery = requests.get(discovery_url, verify=False, timeout=(10, 30))
            discovery.raise_for_status()
            config = discovery.json()
        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Could not fetch discovery document from {discovery_url}: {e}")
            return False

        self.auth_url = config.get("authorization_endpoint")
        self.token_url = config.get("token_endpoint")

        if not self.auth_url or not self.token_url:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Discovery document missing authorization_endpoint or token_endpoint.")
            return False

        return True

    def _load_cached_token(self):
        """Return the cached token dict if it exists and has not expired, otherwise None."""
        file = QFile(self.token_cache_file)
        if not file.exists() or file.size() == 0:
            return None
        if not file.open(QIODevice.OpenModeFlag.ReadOnly):
            print(f"Function '{inspect.currentframe().f_code.co_name}': Could not open {self.token_cache_file}.")
            return None
        try:
            token = json.loads(file.readAll().data().decode("utf-8"))
        except Exception:
            return None
        finally:
            file.close()
        # expires_at is a Unix timestamp written by requests_oauthlib
        if token.get("expires_at", 0) > time.time():
            return token
        return None

    def _save_token(self, token):
        """Persist the token dict to disk for reuse across process restarts."""
        file = QFile(self.token_cache_file)
        if file.open(QIODevice.OpenModeFlag.WriteOnly):
            file.write(json.dumps(token).encode("utf-8"))
            file.close()
        else:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Failed to open {self.token_cache_file} for writing.")

    def _try_refresh_token(self):
        """Use the stored refresh_token to obtain a new access token without a browser.

        Returns the new access token string on success, or None if the refresh
        token is absent, expired, or the Keycloak request fails.
        """
        # Read the raw cache file without checking access-token expiry.
        file = QFile(self.token_cache_file)
        if not file.exists() or file.size() == 0:
            return None
        if not file.open(QIODevice.OpenModeFlag.ReadOnly):
            return None
        try:
            token = json.loads(file.readAll().data().decode("utf-8"))
        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': failed to parse token cache: {e}")
            return None
        finally:
            file.close()

        refresh_token = token.get("refresh_token")
        if not refresh_token:
            return None

        try:
            response = requests.post(
                self.token_url,
                data={
                    "grant_type": "refresh_token",
                    "refresh_token": refresh_token,
                    "client_id": self.CLIENT_ID,
                },
                verify=False,
                timeout=(10, 60),
            )
            if response.status_code != 200:
                print(f"Function '{inspect.currentframe().f_code.co_name}': Token refresh failed {response.status_code}: {response.text}")
                return None

            new_token = response.json()
            # Compute expires_at the same way requests_oauthlib does so that
            # _load_cached_token() can validate it on the next run.
            if "expires_in" in new_token and "expires_at" not in new_token:
                new_token["expires_at"] = time.time() + new_token["expires_in"] - 10

            self.token_response = new_token
            self.access_token = new_token["access_token"]
            self._save_token(new_token)
            self._lookup_current_user()
            return self.access_token

        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Token refresh error: {e}")
            return None

    def _lookup_current_user(self):
        """Populate self.current_user by matching the JWT email against Reference_FndUser."""
        jwt_payload = self.access_token.split(".")[1]
        jwt_payload += "=" * (4 - len(jwt_payload) % 4)
        jwt_claims = json.loads(base64.urlsafe_b64decode(jwt_payload))
        current_email = jwt_claims.get("preferred_username", "")

        fnduser_url = self.ifs_url.rstrip("/") + f"/main/ifsapplications/projection/v1/PrUserHandling.svc/Reference_FndUser?$filter=WebUser eq '{current_email.upper()}'&$select=Identity,WebUser"
        try:
            result = requests.get(fnduser_url, verify=False, timeout=(10, 60),
                                  headers={"Authorization": f"Bearer {self.access_token}", "Content-Type": "application/json"})
            if result.status_code == 200:
                users = result.json().get("value", [])
                self.current_user = users[0].get("Identity") if users else None
            else:
                print(f"Function '{inspect.currentframe().f_code.co_name}': Reference_FndUser lookup failed {result.status_code}: {result.text}")
                self.current_user = None
        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Reference_FndUser lookup error: {e}")
            self.current_user = None

    def authenticate(self):
        """Return a valid Bearer access token, triggering a browser login if needed."""

        if not self.ifs_url:
            return None

        # Discover the Keycloak endpoints on first use
        if not self.auth_url or not self.token_url:
            if not self._discover_endpoints():
                return None

        # Return the in-process cached token if still valid
        if self.access_token is not None:
            cached = self._load_cached_token()
            if cached:
                return self.access_token

        # Try the on-disk token cache before opening the browser
        cached = self._load_cached_token()
        if cached:
            self.access_token = cached["access_token"]
            self._lookup_current_user()
            return self.access_token

        # Access token expired — try the refresh token before opening the browser
        token = self._try_refresh_token()
        if token:
            return token

        # No valid cached token and refresh failed — do the full browser flow
        callback_server = HTTPServer(("localhost", 8080), _SSOCallbackHandler)
        callback_server.auth_code_event = threading.Event()
        callback_server.auth_response_path = None

        server_thread = threading.Thread(target=callback_server.serve_forever, daemon=True)
        server_thread.start()

        try:
            oauth = OAuth2Session(
                client_id=self.CLIENT_ID,
                redirect_uri=self.REDIRECT_URI,
                scope=self.SCOPE,
            )

            authorization_url, _ = oauth.authorization_url(
                self.auth_url,
                access_type="offline",
                prompt="consent",
            )

            webbrowser.open(authorization_url)

            received = callback_server.auth_code_event.wait(timeout=120)

            if not received or callback_server.auth_response_path is None:
                print(f"Function '{inspect.currentframe().f_code.co_name}': SSO login timed out or was cancelled.")
                return None

            full_url = f"http://localhost:8080{callback_server.auth_response_path}"

            self.token_response = oauth.fetch_token(
                self.token_url,
                authorization_response=full_url,
                client_secret=False,
            )

        except Exception as e:
            print(f"Function '{inspect.currentframe().f_code.co_name}': Error during SSO authentication: {e}")
            return None

        finally:
            callback_server.shutdown()

        if "access_token" not in self.token_response:
            print(f"Function '{inspect.currentframe().f_code.co_name}': No access_token in SSO response.")
            return None

        self.access_token = self.token_response["access_token"]
        self._save_token(self.token_response)
        self._lookup_current_user()

        return self.access_token


class IFSCommon:
    def __init__(self):
        self.settings_pluginname = "IFS Cloud"

        self.pnc = ProjectNotesCommon()

        self.ifs_url = self.pnc.get_plugin_setting("URL", self.settings_pluginname)
        self.ifs_person_id = None
        self.report_server = self.pnc.get_plugin_setting("ReportServer", self.settings_pluginname)
        self.domain_user = self.pnc.get_plugin_setting("DomainUser", self.settings_pluginname)
        self.domain_password = self.pnc.get_plugin_setting("DomainPassword", self.settings_pluginname)
        sync_val = self.pnc.get_plugin_setting("SyncTrackerItems", self.settings_pluginname)
        self.sync_tracker_items = sync_val is not None and sync_val.lower() == "true"

        self.sso_auth = SSOAuthAPI()

    def _get_token(self):
        """Authenticate via SSO and return a valid Bearer token, or None on failure.

        Also syncs self.ifs_person_id from the authenticated user identity.
        """
        token = self.sso_auth.authenticate()
        if token:
            if self.sso_auth.current_user:
                self.ifs_person_id = self.sso_auth.current_user
            else:
                print(f"Function '_get_token': authenticated but current_user is None — Reference_FndUser lookup may have failed.")
        return token

    def url_is_available(self):
        try:
            response = requests.head(self.ifs_url, timeout=10, verify=False)
            return response.status_code < 400
        except (requests.ConnectionError, requests.Timeout):
            return False

    def get_has_settings(self):
        if self.ifs_url is None or self.ifs_url == '':
            return False

        return True

    def get_sync_tracker_items(self):
        return self.sync_tracker_items

    def get_earned_value_metrics(self, projectid):
            token = self._get_token()
            if not token:
                print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
                return ""

            request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectAnalysisArray?$apply=aggregate(Bcws%20with%20sum%20as%20Bcws_aggr_,Bcwp%20with%20sum%20as%20Bcwp_aggr_,Acwp%20with%20sum%20as%20Acwp_aggr_,Bac%20with%20sum%20as%20Bac_aggr_,Etc%20with%20sum%20as%20Etc_aggr_,Eac%20with%20sum%20as%20Eac_aggr_,Vac%20with%20sum%20as%20Vac_aggr_,Cv%20with%20sum%20as%20Cv_aggr_,Sv%20with%20sum%20as%20Sv_aggr_)'

            result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

            if (result.status_code != 200):
                print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
                return ""

            json_result = result.json()

            return(json_result['value'][0])

    def get_cost_metrics(self, projectid):
        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return ""

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectMonitoringHandling.svc/Projects(ProjectId=%27' + projectid + '%27)/ProjectCostArray?$apply=aggregate(Estimated%20with%20sum%20as%20Estimated_aggr_,Planned%20with%20sum%20as%20Planned_aggr_,Baseline%20with%20sum%20as%20Baseline_aggr_,EarnedValue%20with%20sum%20as%20EarnedValue_aggr_,ScheduledWork%20with%20sum%20as%20ScheduledWork_aggr_,PlannedCommitted%20with%20sum%20as%20PlannedCommitted_aggr_,Committed%20with%20sum%20as%20Committed_aggr_,Used%20with%20sum%20as%20Used_aggr_,Actual%20with%20sum%20as%20Actual_aggr_)'

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result['value'][0])

    def get_open_activities(self, projectid):
        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return ""

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ActivityListHandling.svc/Activities?$filter=((((Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Planned%27%20or%20Objstate%20eq%20IfsApp.ActivityListHandling.ActivityState%27Released%27))%20and%20(ProjectId%20eq%20%27' + projectid + '%27)))&$orderby=ShortName,ActivityNo&$select=ProjectId,Description,EarlyStartDate,EarlyFinishDate,Manager,Company,CCusPoSeq,ActivitySeq,Objstate,Objgrants,ProgressCost,ProgressHours,ActivityNo,ProgressTemplate,SubProjectId,AccessOn,EarlyStart,ActualStart,EarlyFinish,ActualFinish,TotalWorkDays,ShortName,ProgressMethod,ExcludeResourceProgress,ManualProgressLevel,ManualProgressCost,ManualProgressHours,EstimatedProgress,ProgressTemplateStep,PlannedCostDriver,Note,LateStart,LateFinish,luname,keyref&$expand=ProjectRef($select=Name,Objgrants,luname,keyref),SubProjectRef($select=Description,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result)

    def get_activity_tasks(self, activityseq):
        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return ""

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectScopeAndScheduleHandling.svc/Activities(ActivitySeq=' + str(activityseq) + ')/ActivityTasks?$orderby=TaskId&$select=TaskId,Name,Info,Completed,Objgrants,CompletedDate,luname,keyref'

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        return(json_result)

    def get_status_items(self, project_num, json_data, dayspan):
        xml = '<table name="status_report_items">\n'

        if 'value' in json_data:
            for j in json_data['value']:
                if 'luname' in j:
                    if j['luname'] == 'Activity' and j['ProgressMethod'] == 'Manual' and j['EarlyStart'] is not None and j['EarlyFinish'] is not None: 

                        earlystart = QDateTime.fromString(j['EarlyStart'][0:10], 'yyyy-MM-dd')
                        earlyfinish = QDateTime.fromString(j['EarlyFinish'][0:10], 'yyyy-MM-dd')
                        
                        finishrange = QDateTime.currentDateTime().daysTo(earlyfinish)
                        startrange = QDateTime.currentDateTime().daysTo(earlystart)

                        status = j['Objstate']

                        showitem = False
                        inprogress = False
                        willstart = False
                        hascompleted = False

                        if (startrange < 0 and status != 'Completed' and status != 'Closed'):
                            # show if started and not complete
                            inprogress = True
                            showitem = True

                        if (finishrange >= -dayspan and (status == 'Completed' or status == 'Closed')):
                            # show item if closed in the past day range
                            hascompleted = True
                            showitem = True

                        if (startrange <= dayspan and startrange > 0 and status != 'Completed' and status != 'Closed'):
                            # show item if starting in the next day range
                            willstart = True
                            showitem = True

                        if showitem:
                            xml = xml + '<row earlystart="' + j['EarlyStart'] +'" EarlyFinish="' + j['EarlyFinish'] + '" startrange="' + str(startrange) + '" finishrange="' + str(finishrange) + '" objstate="' + status + '">\n'

                            xml = xml + '<column name="id" lookupvalue="' + project_num + '"></column>\n'

                            progress = str(round(float(0 if j['EstimatedProgress'] is None else j['EstimatedProgress']) * 100.0, 0)) + '%'

                            if hascompleted:
                                xml = xml + '<column name="task_category">Completed</column>\n'
                            elif willstart:
                                xml = xml + '<column name="task_category">Starting</column>\n'
                            else: 
                                xml = xml + '<column name="task_category">In Progress</column>\n'

                            xml = xml + '<column name="task_description">' + j['Description'] + ' (' + progress + ')'

                            if (j['Note'] is not None):
                                xml = xml + ' - ' + str(j['Note'])

                            xml = xml +  '</column>\n'

                            xml = xml + '</row>\n'

        xml = xml + '</table>\n'

        return(xml)

    def create_activity_task(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
        docdata = {
            "TaskId": taskid,
            "Name": name,
            "Info": description,
            "Completed": False,
            "Offset": 0,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            "Cf_Assigned_To" : assignedto,
            "Cf_Identified_By" : identifiedby,
            }

        if datedue.isValid():
            docdata["Cf_Datedue"] = datedue.toString("yyyy-MM-dd")

        if dateupdated.isValid():
            docdata["Cf_Date_Updated"] = dateupdated.toString("yyyy-MM-dd")

        if dateresolved.isValid():
            docdata["Cf_Dateresolved"] = dateresolved.toString("yyyy-MM-dd")

        if priority is not None and priority != '':
            docdata["Cf_Priority"] = "CfEnum_" + priority.upper()

        if status is not None and status != '':
            docdata["Cf_Status"] = "CfEnum_" + status.upper()

        # print("sending doc:")
        # print(json.dumps(docdata, indent=4))

        request_url = self.ifs_url + "/main/ifsapplications/projection/v1/create_activity_task.svc/TaskSet"

        # print(f"Creating Activity in IFS, makeing url request: {request_url}")

        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return False

        result = requests.post(request_url, verify=False, json=docdata, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"})

        if (result.status_code != 201):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return False

        return True

    def update_activity_task(self, activityseq, projectid, taskid, name, description, assignedto, dateupdated, datedue, dateresolved, identifiedby, priority, status):
        docdata = {
            "Name": name,
            "Info": description,
            "Completed": False,
            "Offset": 0,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            "Cf_Assigned_To" : assignedto,
            "Cf_Identified_By" : identifiedby,
            }

        if datedue.isValid():
            docdata["Cf_Datedue"] = datedue.toString("yyyy-MM-dd")

        if dateupdated.isValid():
            docdata["Cf_Date_Updated"] = dateupdated.toString("yyyy-MM-dd")

        if dateresolved.isValid():
            docdata["Cf_Dateresolved"] = dateresolved.toString("yyyy-MM-dd")

        if priority is not None and priority != '':
            docdata["Cf_Priority"] = "CfEnum_" + priority.upper()

        if status is not None and status != '':
            docdata["Cf_Status"] = "CfEnum_" + status.upper()

        request_url = self.ifs_url + "/int/ifsapplications/entity/v1/ActivityTaskEntity.svc/ActivityTaskSet(TaskId='" + taskid + "')"

        #print(f"Updating Activity in IFS, makeing url request: {request_url}")

        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return False

        result = requests.patch(request_url, verify=False, json=docdata, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return False

        return True

    def delete_activity_task(self, activityseq, projectid, taskid):
        docdata = {
            "Name": "Works like a champ",
            "Info": "It is a Text",
            "HoursPlanned": 1,
            "Completed": False,
            "Offset": 1,
            "ActivitySeq": activityseq,
            "ProjectCostElement": "LABOR-INT",
            }

        request_url = self.ifs_url + "/int/ifsapplications/entity/v1/ActivityTaskEntity.svc/ActivityTaskSet(TaskId='" + taskid + "')"

        #print(f"Deleting Activity in IFS, makeing url request: {request_url}")

        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return False

        result = requests.delete(request_url, verify=False, json=docdata, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json", "If-Match": "*"})
        
        if (result.status_code != 204):
            #print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return False

        return True

    def get_issues_activity(self, json_data):
        if 'value' in json_data:
            for j in json_data['value']:
                if 'ActivitySeq' in j:
                    if j['ActivityNo'] == 'ISSUES':
                        return j['ActivitySeq']

        return None

    def get_projects_xml(self, rgroups, clientsdict, rd, parameter):

        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return

        if not self.ifs_person_id:
            print(f"Function 'get_projects_xml': ifs_person_id is not set — user lookup failed after authentication.")
            return

        saved_state = None
        statename = "ifs_projects_import"
        skip = 0
        top = 8

        projectcount = 0

        segment = ""

        if parameter == "all":
            top = 300
        else:
            skip = self.pnc.get_save_state(statename)

            if skip is None:
                print("Failed to get save state.  Will try to get projects list again.")
                return ""

        if (skip > 0):
            segment = segment + f"$skip={skip}"

        if (skip > 0 and top > 0):
            segment = segment + "&"

        if (top > 0):
            segment = segment + f"$top={top}"

        if (skip > 0 or top > 0):
         segment = segment + "&"

        segment = segment + "$orderby=ProjectId&"

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ProjectsHandling.svc/Projects?' + segment + '$filter=(Manager%20eq%20%27' + self.ifs_person_id + '%27)&$select=BudgetControlOn,ControlAsBudgeted,ControlOnTotalBudget,ProjUniquePurchase,ProjUniqueSale,State,ProjectId,Objstate,Objgrants,Name,Company,CustomerCategory,CustomerId,FinancialProjectExist,History,DefaultSite,ProjectPngExists,CheckForecast,Description,CompanyName,Manager,AccessOnOff,PlanStart,PlanFinish,ActualStart,ActualFinish,ApprovedDate,CloseDate,CancelDate,FrozenDate,EarnedValueMethod,BaselineRevisionNumber,Cf_Lastinvoiced,luname,keyref&$expand=AccountingProjectRef($select=ProjectGroup,Objgrants,luname,keyref),ManagerRef($select=Name,luname,keyref),CustomerIdRef($select=Name,Objgrants,luname,keyref)'

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        for rowval in json_result['value']:

            # === CHECK FOR SHUTDOWN REQUEST ===
            if QThread.currentThread().isInterruptionRequested():
                print("Shutdown requested - ifs_tools.py exiting gracefully")
                break

            projectcount = projectcount + 1

            rd['companyname'] = rowval['CompanyName']

            rd['projectsxmlrows'] +=  "  <row>\n"

            rd['projectsxmlrows'] +=  "    <column name=\"project_number\">" + self.pnc.to_xml(rowval['ProjectId']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"project_name\">" + self.pnc.to_xml(rowval['Description']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"last_invoice_date\">" + self.pnc.to_xml(rowval['Cf_Lastinvoiced']) + "</column>\n"

            if rowval['CustomerIdRef'] is not None and rowval['CustomerIdRef']['Name']:
                rd['projectsxmlrows'] +=  "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rowval['CustomerIdRef']['Name']) + "\"></column>\n"

                # only add the company name once to the client list
                clientsdict[rowval['CustomerIdRef']['Name']] = True

            if self.pnc.to_xml(rowval['Objstate']) in ('Initialized', 'Started', 'Approved'):
                rd['projectsxmlrows'] +=  "    <column name=\"project_status\">Active</column>\n"
            else:
                rd['projectsxmlrows'] +=  "    <column name=\"project_status\">Closed</column>\n"


            metrics = self.get_earned_value_metrics(rowval['ProjectId'])
            costmetrics = self.get_cost_metrics(rowval['ProjectId'])

            rd['projectsxmlrows'] +=  "    <column name=\"budget\">" + str(costmetrics['Baseline_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"actual\">" + str(costmetrics['Used_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bcwp\">" + str(costmetrics['EarnedValue_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bcws\">" + str(costmetrics['ScheduledWork_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "    <column name=\"bac\">" + str(metrics['Bac_aggr_']) + "</column>\n"
            rd['projectsxmlrows'] +=  "  </row>\n"

            self.get_team_members_xml(rgroups, clientsdict, rowval['ProjectId'], rd )

        if self.pnc.set_save_state(statename, skip, top, projectcount) is None:
            print("Failed to set save state. Will retreive the same projects list again.")


    def get_resource_groups(self, rgroups):
        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return ""

        request_url = self.ifs_url + '/main/ifsapplications/projection/v1/ResourceGroupsHandling.svc/ResourceSet?$select=ResourceId,Description'

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        for value in json_result['value']:
            rgroups[value['ResourceId']] = value['Description']        


    def get_team_members_xml(self, rgroups, clientsdict, projectid, rd):
        token = self._get_token()
        if not token:
            print(f"No token found! '{inspect.currentframe().f_code.co_name}'")
            return ""

        request_url = self.ifs_url + "/main/ifsapplications/projection/v1/ProjectResourceAllocationsHandling.svc/ProjResourceAllocations?$filter=(ProjectId%20eq%20%27" + projectid + "%27)&$expand=EmployeeIdRef($select=EmployeeName)"

        result = requests.get(request_url, verify=False, timeout=(10, 120), headers={"Authorization": f"Bearer {token}", "Prefer": "odata.maxpagesize=500,odata.track-changes"})

        if (result.status_code != 200):
            print(f"Function '{inspect.currentframe().f_code.co_name}', ODATA Request Failed {result.reason}: {result.text} url: {request_url}")
            return ""

        json_result = result.json()

        for rowval in json_result['value']:

            if rowval['EmployeeIdRef'] is not None:  # some older projects or templates may not have employee asignments
                name = rowval['EmployeeIdRef']['EmployeeName']

                project_person_key = (projectid, name)
                if project_person_key not in rd['projectpeopleadded']:
                    rd['projectpeopleadded'].add(project_person_key)
                    rd['projectpeoplexmlrows'] += "  <row>\n"
                    rd['projectpeoplexmlrows'] += "    <column name=\"project_id\" lookupvalue=\"" + self.pnc.to_xml(projectid) + "\"></column>\n"
                    rd['projectpeoplexmlrows'] += "    <column name=\"people_id\" lookupvalue=\"" + self.pnc.to_xml(name) + "\"></column>\n"
                    rd['projectpeoplexmlrows'] += "    <column name=\"role\">" + self.pnc.to_xml(rgroups[rowval['ResourceParentId']]) + "</column>\n"
                    rd['projectpeoplexmlrows'] += "  </row>\n"

                if name not in rd['peopleadded']:
                    rd['peopleadded'].add(name)
                    rd['peoplexmlrows'] += "  <row>\n"
                    rd['peoplexmlrows'] += "    <column name=\"name\">" + self.pnc.to_xml(name) + "</column>\n"
                    if rd['companyname']:
                        rd['peoplexmlrows'] += "    <column name=\"client_id\" lookupvalue=\"" + self.pnc.to_xml(rd['companyname']) + "\"></column>\n"
                    rd['peoplexmlrows'] += "  </row>\n"

                # only add the company name once to the client list
                if rd['companyname']:
                    clientsdict[rd['companyname']] = True

    def import_ifs_projects(self, parameter):
        timer = QElapsedTimer()
        timer.start()

        clientsdict = {}
        rgroups = {}

        rd = {}
        rd['companyname'] = ''
        rd['clientsxmlrows'] = ''
        rd['projectsxmlrows'] = ''
        rd['projectlocationsxmlrows'] = ''
        rd['peoplexmlrows'] = ''
        rd['projectpeoplexmlrows'] = ''
        rd['peopleadded'] = set()          # tracks names already in peoplexmlrows
        rd['projectpeopleadded'] = set()   # tracks (projectid, name) already in projectpeoplexmlrows

        docxml = "<projectnotes>\n"

        self.get_resource_groups(rgroups)

        self.get_projects_xml(rgroups, clientsdict, rd, parameter)

        docxml += "<table name=\"clients\">\n"
        for k in clientsdict:
            cn = self.pnc.to_xml(k)
            if cn is not None and cn != '':
                docxml +=  f"  <row>\n   <column name=\"client_name\">{cn}</column>\n </row>\n"
        docxml +=  "</table>\n"

        docxml += "<table name=\"people\">\n"
        docxml += rd['peoplexmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"projects\">\n"
        docxml += rd['projectsxmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"project_people\">\n"
        docxml += rd['projectpeoplexmlrows']
        docxml += "</table>\n"

        docxml += "<table name=\"project_locations\">\n"
        docxml += rd['projectlocationsxmlrows']
        docxml += "</table>\n"

        docxml += "</projectnotes>\n"

        # === CHECK FOR SHUTDOWN REQUEST ===
        if QThread.currentThread().isInterruptionRequested():
            print("Shutdown requested - ifs_tools.py exiting gracefully")
            return

        projectnotes.update_data(docxml)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

    def export_ifs_project_tracker_items(self, project_id, project_number):
        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table filter_field_1="project_id" filter_value_1="{project_id}" name="item_tracker" />\n</projectnotes>\n'
        xmlresult = projectnotes.get_data(xmldoc)

        xmlval = QDomDocument()
        xmlval.setContent(xmlresult)
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

        # show all tracker items
        trackeritems = self.pnc.find_node(xmlroot, "table", "name", "item_tracker")

        isinternal = 0
        itemcount = 0
        itemstatus = ""
        itemtype = ""

        if not trackeritems is None:

            # === CHECK FOR SHUTDOWN REQUEST ===
            if QThread.currentThread().isInterruptionRequested():
                print("Shutdown requested - ifs_tools.py exiting gracefully")
                return

            itemrow = trackeritems.firstChild()

            # find the ISSUES Activity
            jsact = self.get_open_activities(project_number)
            issuesseq = self.get_issues_activity(jsact)

            if not issuesseq is None:           
                # === CHECK FOR SHUTDOWN REQUEST ===
                if QThread.currentThread().isInterruptionRequested():
                    print("Shutdown requested - ifs_tools.py exiting gracefully")
                    return

                itemrow = trackeritems.firstChild()

                while not itemrow.isNull():
                    # === CHECK FOR SHUTDOWN REQUEST ===
                    if QThread.currentThread().isInterruptionRequested():
                        print("Shutdown requested - ifs_tools.py exiting gracefully")
                        return

                    isinternal = self.pnc.get_column_value(itemrow, "internal_item")
                    itemstatus = self.pnc.get_column_value(itemrow, "status")
                    itemtype = self.pnc.get_column_value(itemrow, "item_type")
                    ifsitemid = project_number + self.pnc.get_column_value(itemrow, "item_number")

                    #print(f"Identified Issue {ifsitemid} in project {project_number}")

                    duedate = QDateTime()
                    assignedto = ''
                    dateupdated = QDateTime()
                    dateresolved = QDateTime()
                    identifiedby = ''
                    priority = ''
                    status = ''

                    if self.pnc.get_column_value(itemrow, "date_due") is not None:
                        duedate = QDateTime.fromString(self.pnc.get_column_value(itemrow, "date_due"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "last_update") is not None:
                        dateupdated = QDateTime.fromString(self.pnc.get_column_value(itemrow, "last_update"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "date_resolved") is not None:
                        dateresolved = QDateTime.fromString(self.pnc.get_column_value(itemrow, "date_resolved"),'MM/dd/yyyy')

                    if self.pnc.get_column_value(itemrow, "assigned_to") is not None:
                        colnode = self.pnc.find_node(itemrow, "column", "name", "assigned_to")
                        if colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                            assignedto = colnode.attributes().namedItem("lookupvalue").nodeValue()

                    if self.pnc.get_column_value(itemrow, "identified_by") is not None:
                        colnode = self.pnc.find_node(itemrow, "column", "name", "identified_by")
                        if colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                            identifiedby = colnode.attributes().namedItem("lookupvalue").nodeValue()

                    priority = self.pnc.get_column_value(itemrow, "priority")
                    desc = self.pnc.get_column_value(itemrow, "description")
                    status = self.pnc.get_column_value(itemrow, "status")

                    item_name = self.pnc.get_column_value(itemrow, "item_name")

                    if isinternal != "1":  
                        if not self.update_activity_task(issuesseq, project_number, ifsitemid, item_name, desc, assignedto, dateupdated, duedate, dateresolved, identifiedby, priority, status):
                            self.create_activity_task(issuesseq, project_number, ifsitemid, item_name, desc, assignedto, dateupdated, duedate, dateresolved, identifiedby, priority, status)
                    else:
                        self.delete_activity_task(issuesseq, project_number, ifsitemid)

                    itemcount = itemcount + 1

                    itemrow = itemrow.nextSibling()

    # IMPORTANT:  This plugin relys on custom fields on the TASK Entity.  You'll need to evaluate the REST calls to determine the field info
    def export_ifs_tracker_items(self, parameter):
        timer = QElapsedTimer()
        timer.start()

        saved_state = None
        statename = "ifs_tracker_export"
        skip = 0
        top = 30

        projectcount = 0

        if parameter == "all":
            top = 300
        else:
            skip = self.pnc.get_save_state(statename)

            if skip is None:
                print("Failed to get save state.  Will try to export tracker items again.")
                return

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table filter_field_1="project_status" filter_value_1="Active" name="projects" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'
        xmlresult = projectnotes.get_data(xmldoc)
        
        xmlval = QDomDocument()
        xmlval.setContent(xmlresult)
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node  

        # find all projects
        projects = self.pnc.find_node(xmlroot, "table", "name", "projects")

        if not projects is None:
            # === CHECK FOR SHUTDOWN REQUEST ===
            if QThread.currentThread().isInterruptionRequested():
                print("Shutdown requested - ifs_tools.py exiting gracefully")
                return

            projectrow = projects.firstChild()

            while not projectrow.isNull():
                # === CHECK FOR SHUTDOWN REQUEST ===
                if QThread.currentThread().isInterruptionRequested():
                    print("Shutdown requested - ifs_tools.py exiting gracefully")
                    break

                project_number = self.pnc.get_column_value(projectrow, "project_number")
                project_id = self.pnc.get_column_value(projectrow, "id")

                projectcount += 1

                self.export_ifs_project_tracker_items(project_id, project_number)

                projectrow = projectrow.nextSibling()

        if self.pnc.set_save_state(statename, skip, top, projectcount) is None:
            print("Failed to save state.  Will export the same tracker items again.")

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

#setup test data
if __name__ == '__main__':
    import os
    import sys


    os.chdir("..")

    ifs = IFSCommon()

    clientsdict = {}
    rgroups = {}

    rd = {}

    #ifs.get_resource_groups(rgroups)
    #ifs.get_team_members_xml(rgroups, clientsdict, 'P3114M', rd)

