import { initPage, renderAlert, REALTIME_EVENTS, getLatestTelemetry, getActivityHistory } from './common.js';

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

function renderStatus(data) {
    const container = document.querySelector('[data-status-details]');
    if (!container) {
        return;
    }

    const tnc = data?.tnc;
    if (!tnc?.available) {
        container.innerHTML = '<p class="text-danger mb-0">TNC manager is unavailable.</p>';
        return;
    }

    const display = tnc.display || {};

    container.innerHTML = `
        <div class="row g-3">
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">Display</div>
                    <div class="card-body">${renderDisplayStatus(display)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">Battery</div>
                    <div class="card-body">${renderBattery(display.battery)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">Power-Off</div>
                    <div class="card-body">${renderPowerOff(display.powerOff)}</div>
                </div>
            </div>
            <div class="col-lg-6">
                <div class="card h-100">
                    <div class="card-header">GNSS</div>
                    <div class="card-body">${renderGNSS(display.gnss)}</div>
                </div>
            </div>
        </div>`;
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

initPage({ activeNav: 'status' });
renderStatus(getLatestTelemetry());
renderActivity(getActivityHistory());
initialiseEventSubscriptions();

