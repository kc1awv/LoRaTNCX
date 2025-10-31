import { initThemeSync, bindThemeControl, currentThemeState } from './theme.js';
import { createRealtimeConnection, normalizeMessage } from './websocket.js';
import { getStatus } from './api.js';

const REALTIME_EVENTS = Object.freeze({
    TELEMETRY: 'lora:telemetry',
    CSRF_TOKEN: 'lora:csrf-token',
    ACTIVITY: 'lora:activity',
    CONNECTION: 'lora:connection'
});

const MAX_ACTIVITY_ENTRIES = 50;
const POLL_INTERVAL_MS = 15000;
const POLL_RETRY_DELAY_MS = 5000;

let realtimeClient = null;
let realtimeInitialised = false;
let telemetryState = null;
let telemetryMeta = { source: null, timestamp: null };
let hasSnapshot = false;
let latestCsrfToken = null;
const activityLog = [];
let pollingEnabled = false;
let pollTimer = null;
let pollInFlight = false;

function isObject(value) {
    return value != null && typeof value === 'object' && !Array.isArray(value);
}

function cloneData(value) {
    if (value == null) {
        return value;
    }
    if (typeof structuredClone === 'function') {
        try {
            return structuredClone(value);
        } catch (error) {
            // Fall through to JSON cloning.
        }
    }
    try {
        return JSON.parse(JSON.stringify(value));
    } catch (error) {
        return value;
    }
}

function mergeDeep(target, source) {
    if (!isObject(source)) {
        return source;
    }
    const base = isObject(target) ? { ...target } : {};
    for (const [key, value] of Object.entries(source)) {
        if (isObject(value)) {
            base[key] = mergeDeep(base[key], value);
        } else {
            base[key] = value;
        }
    }
    return base;
}

function dispatchDocumentEvent(name, detail) {
    document.dispatchEvent(new CustomEvent(name, { detail }));
}

function dispatchTelemetry(source, timestamp) {
    if (!telemetryState) {
        return;
    }
    telemetryMeta = { source, timestamp };
    dispatchDocumentEvent(REALTIME_EVENTS.TELEMETRY, {
        telemetry: cloneData(telemetryState),
        meta: { ...telemetryMeta }
    });
}

function updateCsrfToken(token, source, timestamp) {
    if (!token || token === latestCsrfToken) {
        return;
    }
    latestCsrfToken = token;
    dispatchDocumentEvent(REALTIME_EVENTS.CSRF_TOKEN, {
        token,
        source,
        timestamp
    });
}

function recordActivityEntry(type, payload, source, timestamp) {
    const entry = {
        type,
        payload: cloneData(payload),
        source,
        timestamp
    };
    activityLog.unshift(entry);
    if (activityLog.length > MAX_ACTIVITY_ENTRIES) {
        activityLog.pop();
    }
    dispatchDocumentEvent(REALTIME_EVENTS.ACTIVITY, cloneData(entry));
}

function applySnapshot(payload, source, timestamp) {
    if (!payload) {
        return;
    }
    const next = telemetryState ? cloneData(telemetryState) : {};
    if (payload.wifi) {
        next.wifi = cloneData(payload.wifi);
    }
    if (payload.tnc) {
        next.tnc = cloneData(payload.tnc);
    }
    if (payload.ui) {
        next.ui = mergeDeep(next.ui, payload.ui);
    }
    if (payload.clientId !== undefined) {
        next.clientId = payload.clientId;
    }
    telemetryState = next;
    hasSnapshot = true;
    dispatchTelemetry(source, timestamp);
}

function applyHello(payload, source, timestamp) {
    if (!payload) {
        return;
    }
    const next = telemetryState ? cloneData(telemetryState) : {};
    const wifiUpdate = {};
    if (payload.apSsid !== undefined) {
        wifiUpdate.apSsid = payload.apSsid;
    }
    if (payload.staConnected !== undefined) {
        wifiUpdate.staConnected = payload.staConnected;
    }
    if (payload.pairingRequired !== undefined) {
        wifiUpdate.pairingRequired = payload.pairingRequired;
    }
    if (Object.keys(wifiUpdate).length > 0) {
        next.wifi = mergeDeep(next.wifi, wifiUpdate);
    }
    const tncUpdate = {};
    if (payload.tncAvailable !== undefined) {
        tncUpdate.available = payload.tncAvailable;
    }
    if (payload.tncStatus !== undefined) {
        tncUpdate.statusText = payload.tncStatus;
    }
    if (Object.keys(tncUpdate).length > 0) {
        next.tnc = mergeDeep(next.tnc, tncUpdate);
    }
    if (payload.clientId !== undefined) {
        next.clientId = payload.clientId;
    }
    telemetryState = next;
    dispatchTelemetry(source, timestamp);
}

function applyStatusUpdate(payload, source, timestamp) {
    if (!payload || !telemetryState) {
        if (!hasSnapshot) {
            startPolling(true);
        }
        return;
    }
    const next = cloneData(telemetryState);
    if (payload.clientCount !== undefined) {
        next.clientCount = payload.clientCount;
    }
    if (payload.display) {
        next.tnc = next.tnc ? { ...next.tnc } : {};
        next.tnc.display = mergeDeep(next.tnc.display, payload.display);
    }
    telemetryState = next;
    dispatchTelemetry(source, timestamp);
}

function applyUiTheme(payload, source, timestamp) {
    if (!payload) {
        return;
    }
    const next = telemetryState ? cloneData(telemetryState) : {};
    next.ui = mergeDeep(next.ui, payload);
    telemetryState = next;
    dispatchTelemetry(source, timestamp);
}

function dispatchConnectionState(state, extra = {}) {
    dispatchDocumentEvent(REALTIME_EVENTS.CONNECTION, {
        state,
        timestamp: Date.now(),
        ...extra
    });
}

async function runPoll(delayNext = POLL_INTERVAL_MS) {
    if (!pollingEnabled || pollInFlight) {
        return;
    }
    pollInFlight = true;
    try {
        const data = await getStatus();
        const normalized = normalizeMessage({
            ...data,
            type: 'status_snapshot',
            timestamp: Date.now()
        });
        if (normalized) {
            applySnapshot(normalized.payload, 'poll', normalized.timestamp);
            if (normalized.payload?.ui?.csrfToken) {
                updateCsrfToken(normalized.payload.ui.csrfToken, 'poll', normalized.timestamp);
            }
        }
    } catch (error) {
        dispatchConnectionState('error', { error, source: 'poll' });
        delayNext = POLL_RETRY_DELAY_MS;
    } finally {
        pollInFlight = false;
        if (pollingEnabled) {
            pollTimer = window.setTimeout(() => runPoll(), delayNext);
        }
    }
}

function startPolling(immediate = false) {
    if (pollingEnabled) {
        if (immediate) {
            if (pollTimer) {
                window.clearTimeout(pollTimer);
            }
            pollTimer = window.setTimeout(() => runPoll(), 0);
        }
        return;
    }
    pollingEnabled = true;
    if (pollTimer) {
        window.clearTimeout(pollTimer);
    }
    pollTimer = window.setTimeout(() => runPoll(), immediate ? 0 : POLL_INTERVAL_MS);
}

function stopPolling() {
    if (!pollingEnabled) {
        return;
    }
    pollingEnabled = false;
    if (pollTimer) {
        window.clearTimeout(pollTimer);
        pollTimer = null;
    }
}

function handleRealtimeMessage(message) {
    const { type, payload, timestamp } = message;
    switch (type) {
        case 'hello':
            applyHello(payload, 'websocket', timestamp);
            if (payload?.csrfToken) {
                updateCsrfToken(payload.csrfToken, 'websocket', timestamp);
            }
            break;
        case 'status_snapshot':
            applySnapshot(payload, 'websocket', timestamp);
            if (payload?.ui?.csrfToken) {
                updateCsrfToken(payload.ui.csrfToken, 'websocket', timestamp);
            }
            stopPolling();
            break;
        case 'status':
            applyStatusUpdate(payload, 'websocket', timestamp);
            break;
        case 'command_result':
        case 'config_result':
        case 'alert':
        case 'packet':
        case 'error':
        case 'client_disconnected':
        case 'pong':
            recordActivityEntry(type, payload, 'websocket', timestamp);
            break;
        case 'csrf_token':
            if (payload?.token) {
                updateCsrfToken(payload.token, 'websocket', timestamp);
            }
            break;
        case 'ui_theme':
            applyUiTheme(payload, 'websocket', timestamp);
            if (payload?.csrfToken) {
                updateCsrfToken(payload.csrfToken, 'websocket', timestamp);
            }
            break;
        default:
            break;
    }
}

function initialiseRealtime() {
    if (realtimeInitialised) {
        return;
    }
    realtimeInitialised = true;
    realtimeClient = createRealtimeConnection();

    realtimeClient.addEventListener('open', (event) => {
        dispatchConnectionState('open', { source: 'websocket', event: event.detail?.event });
        if (!hasSnapshot) {
            startPolling(true);
        }
    });

    realtimeClient.addEventListener('close', (event) => {
        dispatchConnectionState('closed', { source: 'websocket', event: event.detail?.event });
        startPolling(true);
    });

    realtimeClient.addEventListener('error', (event) => {
        dispatchConnectionState('error', { source: 'websocket', event: event.detail?.event, error: event.detail?.error });
        startPolling(true);
    });

    realtimeClient.addEventListener('message', (event) => {
        handleRealtimeMessage(event.detail.message);
    });

    startPolling(true);
}

function setActiveNavLink(pathname) {
    document.querySelectorAll('[data-nav]').forEach((link) => {
        if (link.dataset.nav === pathname) {
            link.classList.add('active');
            link.setAttribute('aria-current', 'page');
        } else {
            link.classList.remove('active');
            link.removeAttribute('aria-current');
        }
    });
}

function wireThemeIndicator() {
    const indicator = document.querySelector('[data-theme-indicator]');
    if (!indicator) {
        return;
    }

    const updateIndicator = ({ theme, override }) => {
        const label = override ? `${theme} (override)` : `${theme} (system)`;
        indicator.textContent = label;
    };

    const initial = currentThemeState();
    updateIndicator({ theme: initial.theme, override: Boolean(initial.override) });

    document.documentElement.addEventListener('themechange', (event) => {
        updateIndicator(event.detail);
    });
}

export function initPage({ activeNav } = {}) {
    initThemeSync();

    const themeSelector = document.querySelector('[data-theme-toggle]');
    if (themeSelector) {
        bindThemeControl(themeSelector);
    }

    wireThemeIndicator();

    if (activeNav) {
        setActiveNavLink(activeNav);
    }

    initialiseRealtime();
}

export function renderAlert(target, message, type = 'danger') {
    if (!target) {
        return;
    }
    target.innerHTML = `
        <div class="alert alert-${type} alert-dismissible fade show" role="alert">
            ${message}
            <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
        </div>`;
}

export function getLatestTelemetry() {
    return telemetryState ? cloneData(telemetryState) : null;
}

export function getActivityHistory() {
    return activityLog.map((entry) => cloneData(entry));
}

export function getLatestCsrfToken() {
    return latestCsrfToken;
}

export { REALTIME_EVENTS };

