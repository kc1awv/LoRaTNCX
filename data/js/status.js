import {
    initPage,
    renderAlert,
    REALTIME_EVENTS,
    getLatestTelemetry,
    getActivityHistory,
    syncCsrfToken
} from './common.js';
import { updatePeripheral } from './api.js';

function escapeHtml(value) {
    if (value == null) {
        return '';
    }
    return String(value)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

function formatTimestamp(timestamp) {
    if (!timestamp) {
        return '';
    }
    try {
        const date = new Date(timestamp);
        if (Number.isNaN(date.getTime())) {
            return '';
        }
        return date.toLocaleTimeString();
    } catch (error) {
        return '';
    }
}

function truncate(value, length = 120) {
    if (value == null) {
        return '';
    }
    const text = String(value);
    if (text.length <= length) {
        return text;
    }
    return `${text.slice(0, length - 1)}…`;
}

const PERIPHERAL_LABELS = Object.freeze({
    gnss: 'GNSS receiver',
    oled: 'OLED display'
});

const peripheralState = {
    gnss: { enabled: null, busy: false, available: false },
    oled: { enabled: null, busy: false, available: false }
};

const alertHost = document.querySelector('[data-alert-host]');
const statusContainer = document.querySelector('[data-status-details]');
const statusTemplate = document.getElementById('status-detail-template');

function renderDisplayStatus(display) {
    if (!display) {
        return '<p class="text-muted mb-0">Display information unavailable.</p>';
    }

    const rows = [
        ['Mode', display.mode],
        ['TX Count', display.txCount],
        ['RX Count', display.rxCount],
        ['Last Packet (ms)', display.lastPacketMillis],
        ['Recent Packet', display.hasRecentPacket ? 'Yes' : 'No'],
        ['Last RSSI', display.lastRssi],
        ['Last SNR', display.lastSnr],
        ['Frequency (MHz)', display.frequencyMHz],
        ['Bandwidth (kHz)', display.bandwidthKHz],
        ['Spreading Factor', display.spreadingFactor],
        ['Coding Rate', display.codingRate],
        ['TX Power (dBm)', display.txPowerDbm],
        ['Uptime (ms)', display.uptimeMs]
    ];

    return `
        <dl class="row mb-0">
            ${rows
                .map(([key, value]) => `
                    <dt class="col-sm-5">${key}</dt>
                    <dd class="col-sm-7">${value ?? 'N/A'}</dd>`)
                .join('')}
        </dl>`;
}

function renderBattery(battery) {
    if (!battery) {
        return '<p class="text-muted mb-0">Battery data unavailable.</p>';
    }

    return `
        <dl class="row mb-0">
            <dt class="col-sm-5">Voltage</dt>
            <dd class="col-sm-7">${battery.voltage ?? 'N/A'} V</dd>
            <dt class="col-sm-5">Percent</dt>
            <dd class="col-sm-7">${battery.percent ?? 'N/A'}%</dd>
        </dl>`;
}

function renderPowerOff(powerOff) {
    if (!powerOff) {
        return '<p class="text-muted mb-0">Power-off status unavailable.</p>';
    }

    return `
        <dl class="row mb-0">
            <dt class="col-sm-5">Active</dt>
            <dd class="col-sm-7">${powerOff.active ? 'Yes' : 'No'}</dd>
            <dt class="col-sm-5">Progress</dt>
            <dd class="col-sm-7">${powerOff.progress ?? 'N/A'}</dd>
            <dt class="col-sm-5">Complete</dt>
            <dd class="col-sm-7">${powerOff.complete ? 'Yes' : 'No'}</dd>
        </dl>`;
}

function renderGNSS(gnss) {
    if (!gnss) {
        return '<p class="text-muted mb-0">GNSS data unavailable.</p>';
    }

    const coordinates =
        gnss.latitude != null && gnss.longitude != null
            ? `${gnss.latitude.toFixed ? gnss.latitude.toFixed(6) : gnss.latitude}, ${
                  gnss.longitude.toFixed ? gnss.longitude.toFixed(6) : gnss.longitude
              }`
            : 'Unavailable';

    return `
        <dl class="row mb-0">
            <dt class="col-sm-5">Enabled</dt>
            <dd class="col-sm-7">${gnss.enabled ? 'Yes' : 'No'}</dd>
            <dt class="col-sm-5">Fix</dt>
            <dd class="col-sm-7">${gnss.hasFix ? (gnss.is3dFix ? '3D' : '2D') : 'No'}</dd>
            <dt class="col-sm-5">Coordinates</dt>
            <dd class="col-sm-7">${coordinates}</dd>
            <dt class="col-sm-5">Altitude (m)</dt>
            <dd class="col-sm-7">${gnss.altitudeMeters ?? 'N/A'}</dd>
            <dt class="col-sm-5">Satellites</dt>
            <dd class="col-sm-7">${gnss.satellites ?? 'N/A'}</dd>
            <dt class="col-sm-5">Speed (knots)</dt>
            <dd class="col-sm-7">${gnss.speedKnots ?? 'N/A'}</dd>
            <dt class="col-sm-5">Course (°)</dt>
            <dd class="col-sm-7">${gnss.courseDegrees ?? 'N/A'}</dd>
        </dl>`;
}

function renderPeripheralControl(name) {
    const id = `peripheral-toggle-${name}`;
    return `
        <div class="d-flex align-items-center gap-2 flex-wrap justify-content-end" data-peripheral-group="${name}">
            <span class="badge rounded-pill text-bg-secondary" data-peripheral-status="${name}">Unknown</span>
            <div class="form-check form-switch m-0">
                <input class="form-check-input" type="checkbox" role="switch" id="${id}" data-peripheral-toggle="${name}">
                <label class="form-check-label small" for="${id}" data-peripheral-label="${name}">Enabled</label>
            </div>
        </div>`;
}

function renderStatusLayout(display) {
    return `
        <div class="row g-3">
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header d-flex align-items-center justify-content-between flex-wrap gap-2">
                        <span class="fw-semibold">Display</span>
                        ${renderPeripheralControl('oled')}
                    </div>
                    <div class="card-body">${renderDisplayStatus(display)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">Battery</div>
                    <div class="card-body">${renderBattery(display?.battery)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">Power-Off</div>
                    <div class="card-body">${renderPowerOff(display?.powerOff)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header d-flex align-items-center justify-content-between flex-wrap gap-2">
                        <span class="fw-semibold">GNSS</span>
                        ${renderPeripheralControl('gnss')}
                    </div>
                    <div class="card-body">${renderGNSS(display?.gnss)}</div>
                </div>
            </div>
        </div>`;
}

function updatePeripheralStateFromTelemetry(tnc) {
    if (!tnc || !tnc.available) {
        Object.keys(peripheralState).forEach((key) => {
            const state = peripheralState[key];
            state.available = false;
            state.busy = false;
            if (state.enabled !== null) {
                state.enabled = null;
            }
        });
        return;
    }

    const display = tnc.display;

    const gnss = display?.gnss;
    peripheralState.gnss.available = Boolean(gnss);
    if (gnss && typeof gnss.enabled === 'boolean') {
        peripheralState.gnss.enabled = gnss.enabled;
    } else if (!peripheralState.gnss.available && !peripheralState.gnss.busy) {
        peripheralState.gnss.enabled = null;
    }

    peripheralState.oled.available = Boolean(display);
    const oledEnabled =
        display?.oled?.enabled ?? display?.oledEnabled ?? display?.panelEnabled ?? null;
    if (typeof oledEnabled === 'boolean') {
        peripheralState.oled.enabled = oledEnabled;
    } else if (!peripheralState.oled.available && !peripheralState.oled.busy) {
        peripheralState.oled.enabled = null;
    }
}

function updatePeripheralControls() {
    if (!statusContainer) {
        return;
    }

    Object.entries(peripheralState).forEach(([name, state]) => {
        const input = statusContainer.querySelector(`[data-peripheral-toggle="${name}"]`);
        if (input) {
            const disabled = !state.available || state.busy;
            input.disabled = disabled;
            input.checked = Boolean(state.enabled);
            input.indeterminate = state.enabled == null;
            input.setAttribute('aria-busy', state.busy ? 'true' : 'false');
            input.setAttribute('aria-disabled', disabled ? 'true' : 'false');
        }

        const badge = statusContainer.querySelector(`[data-peripheral-status="${name}"]`);
        if (badge) {
            let text = 'Unknown';
            let badgeClass = 'badge rounded-pill text-bg-secondary';
            if (!state.available) {
                text = 'Unavailable';
            } else if (state.busy) {
                text = 'Updating…';
                badgeClass = 'badge rounded-pill text-bg-info';
            } else if (state.enabled === true) {
                text = 'Enabled';
                badgeClass = 'badge rounded-pill text-bg-success';
            } else if (state.enabled === false) {
                text = 'Disabled';
            }
            badge.textContent = text;
            badge.className = badgeClass;
        }

        const label = statusContainer.querySelector(`[data-peripheral-label="${name}"]`);
        if (label) {
            label.classList.toggle('text-muted', !state.available);
        }
    });
}

function renderStatus(data) {
    if (!statusContainer) {
        return;
    }

    if (!data) {
        return;
    }

    const tnc = data?.tnc;
    updatePeripheralStateFromTelemetry(tnc);

    if (!tnc?.available) {
        statusContainer.innerHTML = '<p class="text-danger mb-0">TNC manager is unavailable.</p>';
        updatePeripheralControls();
        return;
    }

    const display = tnc.display || {};

    if (statusTemplate?.content) {
        const fragment = statusTemplate.content.cloneNode(true);

        const displayBody = fragment.querySelector('[data-display-body]');
        if (displayBody) {
            displayBody.innerHTML = renderDisplayStatus(display);
        }

        const batteryBody = fragment.querySelector('[data-battery-body]');
        if (batteryBody) {
            batteryBody.innerHTML = renderBattery(display.battery);
        }

        const powerBody = fragment.querySelector('[data-power-body]');
        if (powerBody) {
            powerBody.innerHTML = renderPowerOff(display.powerOff);
        }

        const gnssBody = fragment.querySelector('[data-gnss-body]');
        if (gnssBody) {
            gnssBody.innerHTML = renderGNSS(display.gnss);
        }

        fragment.querySelectorAll('[data-peripheral-toggle]').forEach((input) => {
            const name = input.dataset.peripheralToggle;
            if (!name) {
                return;
            }
            const id = `peripheral-toggle-${name}`;
            input.id = id;
            const group = input.closest('[data-peripheral-group]');
            const label = group ? group.querySelector(`[data-peripheral-label="${name}"]`) : null;
            if (label) {
                label.setAttribute('for', id);
            }
        });

        statusContainer.replaceChildren(fragment);
    } else {
        statusContainer.innerHTML = renderStatusLayout(display);
    }

    updatePeripheralControls();
}

async function handlePeripheralToggle(event) {
    const target = event.target;
    if (!(target instanceof HTMLInputElement) || target.type !== 'checkbox') {
        return;
    }

    const name = target.dataset.peripheralToggle;
    if (!name || !Object.prototype.hasOwnProperty.call(peripheralState, name)) {
        return;
    }

    const state = peripheralState[name];
    if (state.busy) {
        event.preventDefault();
        return;
    }

    const previousEnabled = state.enabled;
    const desiredState = target.checked;

    state.enabled = desiredState;
    state.busy = true;
    updatePeripheralControls();

    try {
        const response = await updatePeripheral(name, desiredState);
        if (typeof response?.enabled === 'boolean') {
            state.enabled = response.enabled;
        }
        if (response?.csrf_token) {
            syncCsrfToken(response.csrf_token);
        }
        const friendlyName = PERIPHERAL_LABELS[name] || name;
        const statusText = state.enabled ? 'enabled' : 'disabled';
        renderAlert(alertHost, `${friendlyName} ${statusText}.`, 'success');
    } catch (error) {
        state.enabled = previousEnabled;
        const friendlyName = PERIPHERAL_LABELS[name] || name;
        const message = `${friendlyName} update failed: ${escapeHtml(error?.message || 'Unexpected error')}`;
        renderAlert(alertHost, message, 'danger');
    } finally {
        state.busy = false;
        updatePeripheralControls();
    }
}

function renderActivity(entries) {
    const container = document.querySelector('[data-activity-feed]');
    if (!container) {
        return;
    }

    if (!entries || entries.length === 0) {
        container.innerHTML = '<p class="text-muted mb-0">No recent activity.</p>';
        return;
    }

    const items = entries
        .map((entry) => {
            const time = formatTimestamp(entry.timestamp);
            const { type, payload } = entry;
            let badgeClass = 'text-bg-secondary';
            let summary = '';

            switch (type) {
                case 'command_result':
                    badgeClass = payload?.success ? 'text-bg-success' : 'text-bg-warning';
                    summary = `Command <code>${escapeHtml(payload?.command) || 'N/A'}</code>: ${escapeHtml(payload?.message) || ''}`;
                    break;
                case 'config_result':
                    badgeClass = payload?.success ? 'text-bg-success' : 'text-bg-warning';
                    summary = `Config <code>${escapeHtml(payload?.command) || 'N/A'}</code>: ${escapeHtml(payload?.message) || ''}`;
                    if (payload?.statusText) {
                        summary += ` <span class="text-muted">(${escapeHtml(payload.statusText)})</span>`;
                    }
                    break;
                case 'alert':
                    badgeClass = payload?.state ? 'text-bg-danger' : 'text-bg-secondary';
                    summary = `Alert <strong>${escapeHtml(payload?.category) || 'general'}</strong>: ${escapeHtml(payload?.message) || ''}`;
                    break;
                case 'packet':
                    badgeClass = 'text-bg-primary';
                    summary = `Packet ${payload?.length ?? '?'} bytes`;
                    if (payload?.rssi != null || payload?.snr != null) {
                        const rssi = payload?.rssi != null ? `${payload.rssi} dBm` : 'N/A';
                        const snr = payload?.snr != null ? `${payload.snr} dB` : 'N/A';
                        summary += ` (RSSI ${rssi}, SNR ${snr})`;
                    }
                    if (payload?.preview) {
                        summary += ` – <span class="text-muted">${escapeHtml(truncate(payload.preview, 80))}</span>`;
                    }
                    break;
                case 'error':
                    badgeClass = 'text-bg-danger';
                    summary = `Error: ${escapeHtml(payload?.message) || 'Unknown error'}`;
                    break;
                case 'client_disconnected':
                    summary = `Client ${escapeHtml(payload?.clientId ?? '?')} disconnected.`;
                    break;
                case 'pong':
                    summary = `Pong from client ${escapeHtml(payload?.clientId ?? '?')}`;
                    if (payload?.echoTimestamp != null) {
                        summary += ` <span class="text-muted">(echo ${payload.echoTimestamp})</span>`;
                    }
                    break;
                default:
                    summary = `${escapeHtml(type.replace(/_/g, ' '))} event.`;
                    break;
            }

            return `
                <li class="mb-2">
                    <span class="badge ${badgeClass} me-2 text-uppercase">${escapeHtml(type.replace(/_/g, ' '))}</span>
                    ${summary}
                    ${time ? `<span class="ms-2 text-muted small">${escapeHtml(time)}</span>` : ''}
                </li>`;
        })
        .join('');

    container.innerHTML = `<ul class="list-unstyled mb-0 small">${items}</ul>`;
}

function handleConnectionEvent(detail) {
    const alertHost = document.querySelector('[data-alert-host]');
    if (!alertHost) {
        return;
    }

    if (detail?.state === 'error') {
        renderAlert(alertHost, 'Real-time connection issue detected. Falling back to periodic updates.', 'warning');
    } else if (detail?.state === 'open') {
        alertHost.innerHTML = '';
    }
}

function initialiseEventSubscriptions() {
    document.addEventListener(REALTIME_EVENTS.TELEMETRY, (event) => {
        renderStatus(event.detail?.telemetry || null);
    });

    document.addEventListener(REALTIME_EVENTS.ACTIVITY, () => {
        renderActivity(getActivityHistory());
    });

    document.addEventListener(REALTIME_EVENTS.CONNECTION, (event) => {
        handleConnectionEvent(event.detail);
    });
}

if (statusContainer) {
    statusContainer.addEventListener('change', handlePeripheralToggle);
}

initPage({ activeNav: 'status' });
renderStatus(getLatestTelemetry());
renderActivity(getActivityHistory());
initialiseEventSubscriptions();

