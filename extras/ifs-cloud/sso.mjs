// Keycloak sign-in for the IFS Cloud REST API: the authorization-code flow in
// the browser, with the token and refresh token kept in the credential store.
// Replaces SSOAuthAPI in includes/ifs_tools.py.
import { auth, http, log, secrets, settings } from "projectnotes";

const CLIENT_ID = "CCI_public";
const REDIRECT_URI = "http://localhost:8080/callback";
const TOKEN_ACCOUNT = "sso-token";
const COOLDOWN_MS = 3600 * 1000;   // after a failed browser sign-in, wait an hour before opening another

let endpoints = null;
let accessToken = null;
let expiresAt = 0;
let refreshToken = null;
let currentUser = null;
let cooldownUntil = 0;

export function ifsUrl() { return String(settings.get("URL", "") || "").trim().replace(/\/+$/, ""); }
function realm() { return String(settings.get("Realm", "") || "").trim() || "cciprod"; }
function now() { return Date.now() / 1000; }

export function user() { return currentUser; }

// Troubleshooting trace, written to the console log. Never pass it a token or
// password; describe them with tokenInfo() instead.
export function trace(area, message) { console.log("[trace " + area + "] " + message); }
function tokenInfo(value) { return value ? "present (" + String(value).length + " chars)" : "missing"; }
function secondsLeft(at) { return Math.round((Number(at) || 0) - now()) + "s"; }

export function reset() {
    endpoints = null;
    accessToken = null;
    expiresAt = 0;
    refreshToken = null;
    currentUser = null;
    cooldownUntil = 0;
}

function formEncode(values) {
    return Object.keys(values).map(k => encodeURIComponent(k) + "=" + encodeURIComponent(values[k])).join("&");
}

function queryParams(url) {
    const out = {};
    const query = String(url).split("#")[0].split("?")[1] || "";
    query.split("&").forEach(function (pair) {
        if (!pair) return;
        const i = pair.indexOf("=");
        const key = decodeURIComponent((i < 0 ? pair : pair.slice(0, i)).replace(/\+/g, " "));
        out[key] = i < 0 ? "" : decodeURIComponent(pair.slice(i + 1).replace(/\+/g, " "));
    });
    return out;
}

function base64UrlDecode(text) {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const clean = String(text).replace(/-/g, "+").replace(/_/g, "/").replace(/[^A-Za-z0-9+/]/g, "");
    let bits = 0, value = 0, bytes = [];
    for (let i = 0; i < clean.length; ++i) {
        value = (value << 6) | alphabet.indexOf(clean[i]);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes.push((value >> bits) & 0xff);
        }
    }
    // UTF-8 bytes to a string.
    return decodeURIComponent(bytes.map(b => "%" + ("0" + b.toString(16)).slice(-2)).join(""));
}

function randomState() {
    let s = "";
    for (let i = 0; i < 32; ++i) s += Math.floor(Math.random() * 16).toString(16);
    return s;
}

function* discover() {
    const url = ifsUrl() + "/auth/realms/" + encodeURIComponent(realm()) + "/.well-known/openid-configuration";
    trace("sso", "discovering sign-in endpoints at " + url);
    try {
        const response = yield http.fetch(url, { timeoutSeconds: 30 });
        trace("sso", "discovery answered " + response.status);
        if (!response.ok) {
            log.warn("Could not read the IFS sign-in configuration (" + response.status + ") from " + url);
            return false;
        }
        const config = yield response.json();
        if (!config.authorization_endpoint || !config.token_endpoint) {
            log.warn("The IFS sign-in configuration has no authorization or token endpoint.");
            return false;
        }
        endpoints = { authorize: config.authorization_endpoint, token: config.token_endpoint };
        trace("sso", "token endpoint " + endpoints.token);
        return true;
    } catch (e) {
        log.warn("Cannot reach IFS Cloud at " + ifsUrl() + ": " + e.message);
        return false;
    }
}

function* requestToken(form) {
    trace("sso", "token request: " + form.grant_type);
    const response = yield http.fetch(endpoints.token, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: formEncode(form),
        timeoutSeconds: 60,
    });
    trace("sso", "token request answered " + response.status);
    if (!response.ok) {
        log.warn("IFS token request failed (" + response.status + "): " + (yield response.text()));
        return null;
    }
    const token = yield response.json();
    trace("sso", "received access token " + tokenInfo(token.access_token) + ", expires_in " + token.expires_in
          + "s; refresh token " + tokenInfo(token.refresh_token) + ", refresh_expires_in " + token.refresh_expires_in
          + "s; id token " + tokenInfo(token.id_token));
    token.expires_at = now() + (Number(token.expires_in) || 300) - 10;
    // A refresh that does not rotate the refresh token keeps the old one.
    if (!token.refresh_token && form.refresh_token) {
        trace("sso", "no new refresh token; keeping the old one");
        token.refresh_token = form.refresh_token;
    }
    accessToken = token.access_token;
    expiresAt = token.expires_at;
    refreshToken = token.refresh_token || null;
    // Only what token() reads back; the id token would only make it bigger.
    const saved = { access_token: token.access_token, expires_at: token.expires_at, refresh_token: token.refresh_token };
    const text = JSON.stringify(saved);
    try {
        yield secrets.set(TOKEN_ACCOUNT, text);
        trace("sso", "saved the sign-in to the credential store (" + text.length + " chars)");
        const check = yield secrets.get(TOKEN_ACCOUNT);
        trace("sso", "read-back " + (check === text ? "matches" : "DOES NOT MATCH (" + tokenInfo(check) + ")"));
    } catch (e) {
        log.warn("Could not save the IFS sign-in; it lasts until Project Notes closes: " + e.message);
    }
    return accessToken;
}

// The IFS identity of the signed-in user, from the token's e-mail address.
function* lookupUser() {
    let email = "";
    try {
        email = String(JSON.parse(base64UrlDecode(accessToken.split(".")[1])).preferred_username || "");
    } catch (e) {
        log.warn("Could not read the IFS access token: " + e.message);
    }
    const url = ifsUrl() + "/main/ifsapplications/projection/v1/UserRelatedData.svc/Reference_FndUser?$filter="
        + encodeURIComponent("WebUser eq '" + email.toUpperCase().replace(/'/g, "''") + "'") + "&$select=Identity,WebUser";
    try {
        const response = yield http.fetch(url, { auth: { type: "bearer", token: accessToken }, timeoutSeconds: 60 });
        trace("sso", "user lookup for " + email + " answered " + response.status);
        if (response.ok) {
            const users = (yield response.json()).value || [];
            currentUser = users.length ? users[0].Identity : null;
            trace("sso", "IFS user is " + currentUser);
        } else {
            log.info("IFS user lookup failed (" + response.status + "); using the sign-in name.");
            currentUser = email ? email.toUpperCase() : null;
        }
    } catch (e) {
        log.warn("IFS user lookup failed: " + e.message);
        currentUser = null;
    }
}

// A valid access token, signing in through the browser when the saved token
// and its refresh token no longer work. Null when IFS cannot be reached.
export function* token() {
    if (!ifsUrl()) return null;
    if (accessToken && expiresAt > now()) return accessToken;
    trace("sso", "need a token: in-memory access token " + tokenInfo(accessToken)
          + (accessToken ? " (expired " + secondsLeft(expiresAt) + ")" : "") + ", in-memory refresh token " + tokenInfo(refreshToken));
    if (!endpoints && !(yield discover())) return null;

    let saved = null;
    try {
        const text = yield secrets.get(TOKEN_ACCOUNT);
        trace("sso", "credential store sign-in " + tokenInfo(text));
        saved = JSON.parse(text || "null");
    } catch (e) {
        trace("sso", "could not read the saved sign-in: " + e.message);
        saved = null;
    }
    if (saved)
        trace("sso", "saved access token " + tokenInfo(saved.access_token) + ", expires in " + secondsLeft(saved.expires_at)
              + "; saved refresh token " + tokenInfo(saved.refresh_token));
    if (saved && saved.access_token && saved.expires_at > now()) {
        trace("sso", "using the saved access token");
        accessToken = saved.access_token;
        expiresAt = saved.expires_at;
        refreshToken = saved.refresh_token || refreshToken;
        if (!currentUser) yield lookupUser();
        return accessToken;
    }
    // The refresh token this session already holds, then the saved one.
    const candidates = [refreshToken, saved && saved.refresh_token].filter((t, i, all) => t && all.indexOf(t) === i);
    trace("sso", candidates.length + " refresh token(s) to try");
    for (const candidate of candidates) {
        const refreshed = yield requestToken({ grant_type: "refresh_token", refresh_token: candidate, client_id: CLIENT_ID });
        if (refreshed) {
            if (!currentUser) yield lookupUser();
            return refreshed;
        }
    }
    refreshToken = null;

    if (Date.now() < cooldownUntil) {
        log.info("IFS browser sign-in is paused until " + new Date(cooldownUntil).toLocaleTimeString() + ".");
        return null;
    }
    trace("sso", "opening the browser sign-in");
    const state = randomState();
    const authorizeUrl = endpoints.authorize + "?" + formEncode({
        response_type: "code", client_id: CLIENT_ID, redirect_uri: REDIRECT_URI, scope: "openid", state: state });
    let callback;
    try {
        callback = (yield auth.browserSignIn({ url: authorizeUrl, redirectUri: REDIRECT_URI, timeoutSeconds: 120,
                                             serviceName: "IFS" })).url;
    } catch (e) {
        cooldownUntil = Date.now() + COOLDOWN_MS;
        log.warn("IFS browser sign-in did not finish: " + e.message);
        return null;
    }
    const params = queryParams(callback);
    trace("sso", "browser sign-in returned; code " + tokenInfo(params.code) + ", state " + (params.state === state ? "matches" : "differs"));
    if (params.state !== state || !params.code) {
        cooldownUntil = Date.now() + COOLDOWN_MS;
        log.warn("IFS sign-in returned an unexpected response" + (params.error ? ": " + params.error : "."));
        return null;
    }
    let signedIn = null;
    try {
        signedIn = yield requestToken({ grant_type: "authorization_code", code: params.code,
                                        redirect_uri: REDIRECT_URI, client_id: CLIENT_ID });
    } catch (e) {
        log.warn("IFS token request failed: " + e.message);
    }
    if (!signedIn) {
        cooldownUntil = Date.now() + COOLDOWN_MS;
        return null;
    }
    cooldownUntil = 0;
    yield lookupUser();
    return signedIn;
}
