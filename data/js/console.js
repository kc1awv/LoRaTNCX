import { initPage, REALTIME_EVENTS, getActivityHistory } from './common.js';
import { postCommand } from './api.js';

initPage({ activeNav: 'console' });

const logContainer = document.querySelector('[data-console-log]');
const emptyState = logContainer?.querySelector('[data-console-empty]');
const transportBadge = document.querySelector('[data-console-transport]');
const clearButton = document.querySelector('[data-console-clear]');
const form = document.querySelector('[data-console-form]');
const commandInput = form?.querySelector('[name="command"]');

const COMMAND_HISTORY = [];
let historyIndex = -1;
let realtimeConnected = false;
let realtimeSeen = false;
const seenResults = new Set();

const BADGE_CLASSES = ['text-bg-secondary', 'text-bg-success', 'text-bg-warning', 'text-bg-danger'];

function formatTimestamp(value) {
    const date = new Date(typeof value === 'number' ? value : Date.now());
    if (Number.isNaN(date.getTime())) {
        return new Date().toLocaleTimeString();
    }
    return date.toLocaleTimeString();
}

function hideEmptyState() {
    if (emptyState) {
        emptyState.classList.add('d-none');
    }
}

function showEmptyState() {
    if (emptyState) {
        emptyState.classList.remove('d-none');
    }
}

function scrollLogToBottom() {
    if (logContainer) {
        logContainer.scrollTop = logContainer.scrollHeight;
    }
}

function createLogEntry(timestamp) {
    if (!logContainer) {
        return null;
    }
    hideEmptyState();
    const entry = document.createElement('div');
    entry.className = 'console-entry';
    const timeEl = document.createElement('span');
    timeEl.className = 'console-entry__time text-muted small';
    timeEl.textContent = formatTimestamp(timestamp);
    const messageEl = document.createElement('div');
    messageEl.className = 'console-entry__message flex-grow-1 small';
    entry.append(timeEl, messageEl);
    logContainer.append(entry);
    scrollLogToBottom();
    return messageEl;
}

function appendTextEntry(text, { tone = 'info', timestamp = Date.now() } = {}) {
    const messageEl = createLogEntry(timestamp);
    if (!messageEl) {
        return;
    }
    const classMap = {
        info: 'text-body',
        muted: 'text-secondary',
        success: 'text-success',
        warning: 'text-warning',
        danger: 'text-danger',
        error: 'text-danger'
    };
    const classes = classMap[tone] || classMap.info;
    const lines = String(text).split(/\r?\n/);
    lines.forEach((line, index) => {
        const lineEl = document.createElement('div');
        lineEl.className = classes;
        lineEl.textContent = line;
        if (index > 0) {
            lineEl.classList.add('mt-1');
        }
        messageEl.append(lineEl);
    });
}

function normaliseResultValue(value) {
    if (value == null) {
        return '';
    }
    if (typeof value === 'string') {
        return value;
    }
    try {
        return JSON.stringify(value, null, 2);
    } catch (error) {
        return String(value);
    }
}

function appendCommandEcho(command, timestamp) {
    appendTextEntry(`> ${command}`, { tone: 'muted', timestamp });
}

function appendCommandResult(payload, timestamp) {
    const messageEl = createLogEntry(timestamp);
    if (!messageEl) {
        return;
    }
    const summary = document.createElement('div');
    const success = payload?.success;
    summary.className = `fw-semibold ${success === false ? 'text-danger' : success === true ? 'text-success' : 'text-body'}`;
    const parts = [];
    if (payload?.command) {
        parts.push(`"${payload.command}"`);
    }
    if (success === true) {
        parts.push('succeeded');
    } else if (success === false) {
        parts.push('failed');
    } else {
        parts.push('response');
    }
    if (payload?.message) {
        parts.push(`— ${payload.message}`);
    }
    summary.textContent = parts.join(' ');
    messageEl.append(summary);

    if (payload?.result != null && payload.result !== '') {
        const details = document.createElement('pre');
        details.className = 'console-entry__details small mb-0 mt-2';
        details.textContent = normaliseResultValue(payload.result);
        messageEl.append(details);
    }
}

function makeResultKey(payload, timestamp) {
    const success = payload?.success === true ? '1' : payload?.success === false ? '0' : 'u';
    const result = payload?.result != null ? normaliseResultValue(payload.result) : '';
    const timeKey = typeof timestamp === 'number' ? timestamp : Date.now();
    return [timeKey, payload?.command || '', payload?.message || '', success, result].join('|');
}

function recordCommandResult(payload, timestamp) {
    if (!payload) {
        return;
    }
    const key = makeResultKey(payload, timestamp);
    if (seenResults.has(key)) {
        return;
    }
    seenResults.add(key);
    appendCommandResult(payload, timestamp ?? Date.now());
}

function updateTransportBadge() {
    if (!transportBadge) {
        return;
    }
    transportBadge.classList.remove(...BADGE_CLASSES);
    if (realtimeConnected) {
        transportBadge.classList.add('text-bg-success');
        transportBadge.textContent = 'WebSocket connected';
    } else if (realtimeSeen) {
        transportBadge.classList.add('text-bg-warning');
        transportBadge.textContent = 'WebSocket unavailable – using REST fallback';
    } else {
        transportBadge.classList.add('text-bg-secondary');
        transportBadge.textContent = 'Waiting for WebSocket…';
    }
}

function renderHistory() {
    const history = getActivityHistory()
        .filter((entry) => entry.type === 'command_result')
        .sort((a, b) => {
            const aTime = typeof a.timestamp === 'number' ? a.timestamp : 0;
            const bTime = typeof b.timestamp === 'number' ? b.timestamp : 0;
            return aTime - bTime;
        });
    history.forEach((entry) => {
        recordCommandResult(entry.payload, entry.timestamp);
    });
}

async function sendCommand(command) {
    const trimmed = command.trim();
    if (!trimmed) {
        return;
    }
    const timestamp = Date.now();
    const expectRealtime = realtimeConnected;
    appendCommandEcho(trimmed, timestamp);
    try {
        const response = await postCommand(trimmed);
        if (!expectRealtime) {
            const payload = {
                command: trimmed,
                success: Boolean(response?.success),
                message: response?.message,
                result: response?.result,
                clientId: response?.client_id ?? response?.clientId,
                source: 'rest'
            };
            const responseTimestamp = typeof response?.timestamp === 'number' ? response.timestamp : timestamp;
            recordCommandResult(payload, responseTimestamp);
        }
    } catch (error) {
        appendTextEntry(`Command failed: ${error.message}`, { tone: 'danger', timestamp: Date.now() });
    }
}

function handleFormSubmit(event) {
    event.preventDefault();
    if (!commandInput) {
        return;
    }
    const command = commandInput.value || '';
    const normalized = command.trim();
    if (!normalized) {
        return;
    }
    if (COMMAND_HISTORY.length === 0 || COMMAND_HISTORY[COMMAND_HISTORY.length - 1] !== normalized) {
        COMMAND_HISTORY.push(normalized);
    }
    historyIndex = -1;
    commandInput.value = '';
    sendCommand(command);
}

function recallHistory(direction) {
    if (!commandInput || COMMAND_HISTORY.length === 0) {
        return;
    }
    if (direction < 0) {
        if (historyIndex === -1) {
            historyIndex = COMMAND_HISTORY.length - 1;
        } else if (historyIndex > 0) {
            historyIndex -= 1;
        }
    } else if (direction > 0) {
        if (historyIndex === -1) {
            return;
        }
        historyIndex += 1;
        if (historyIndex >= COMMAND_HISTORY.length) {
            historyIndex = -1;
            commandInput.value = '';
            return;
        }
    }
    if (historyIndex >= 0 && historyIndex < COMMAND_HISTORY.length) {
        commandInput.value = COMMAND_HISTORY[historyIndex];
        commandInput.setSelectionRange(commandInput.value.length, commandInput.value.length);
    }
}

if (commandInput) {
    commandInput.addEventListener('keydown', (event) => {
        if (event.key === 'ArrowUp') {
            event.preventDefault();
            recallHistory(-1);
        } else if (event.key === 'ArrowDown') {
            event.preventDefault();
            recallHistory(1);
        }
    });
}

if (form) {
    form.addEventListener('submit', handleFormSubmit);
}

if (clearButton) {
    clearButton.addEventListener('click', () => {
        if (!logContainer) {
            return;
        }
        logContainer.querySelectorAll('.console-entry').forEach((entry) => entry.remove());
        seenResults.clear();
        showEmptyState();
        appendTextEntry('Log cleared.', { tone: 'muted' });
        commandInput?.focus();
    });
}

document.addEventListener(REALTIME_EVENTS.CONNECTION, (event) => {
    const state = event.detail?.state;
    if (state === 'open') {
        realtimeConnected = true;
        realtimeSeen = true;
    } else if (state === 'closed' || state === 'error') {
        realtimeConnected = false;
        realtimeSeen = true;
    }
    updateTransportBadge();
});

document.addEventListener(REALTIME_EVENTS.ACTIVITY, (event) => {
    if (!event.detail || event.detail.type !== 'command_result') {
        return;
    }
    recordCommandResult(event.detail.payload, event.detail.timestamp);
});

renderHistory();
updateTransportBadge();
appendTextEntry('Console ready. Submit commands to interact with the device.', { tone: 'muted' });
