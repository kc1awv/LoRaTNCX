import { initPage, REALTIME_EVENTS, getLatestTelemetry, getActivityHistory } from './common.js';

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

function renderStatus(telemetry) {
    const container = document.querySelector('[data-status-container]');
    if (!container) {
        return;
    }

    if (!telemetry) {
        container.innerHTML = '<p class="text-muted">Status unavailable.</p>';
        return;
    }

    const wifi = telemetry.wifi || {};
    const tnc = telemetry.tnc || {};

    container.innerHTML = `
        <div class="row g-3">
            <div class="col-md-6">
                <div class="card h-100">
                    <div class="card-header">Wi-Fi</div>
                    <div class="card-body">
                        <dl class="row mb-0">
                            <dt class="col-sm-5">AP SSID</dt>
                            <dd class="col-sm-7">${escapeHtml(wifi.apSsid) || 'N/A'}</dd>
                            <dt class="col-sm-5">STA Connected</dt>
                            <dd class="col-sm-7">${wifi.staConnected ? 'Yes' : 'No'}</dd>
                            <dt class="col-sm-5">Pairing Required</dt>
                            <dd class="col-sm-7">${wifi.pairingRequired ? 'Yes' : 'No'}</dd>
                        </dl>
                    </div>
                </div>
            </div>
            <div class="col-md-6">
                <div class="card h-100">
                    <div class="card-header">TNC</div>
                    <div class="card-body">
                        <p class="mb-1"><strong>Available:</strong> ${tnc.available ? 'Yes' : 'No'}</p>
                        <p class="mb-0"><strong>Status:</strong> ${escapeHtml(tnc.statusText) || 'Unknown'}</p>
                        ${telemetry.clientCount != null
                            ? `<p class="mb-0 mt-2 small text-muted">Active clients: ${telemetry.clientCount}</p>`
                            : ''}
                    </div>
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

function initialiseEventSubscriptions() {
    document.addEventListener(REALTIME_EVENTS.TELEMETRY, (event) => {
        renderStatus(event.detail?.telemetry || null);
    });

    document.addEventListener(REALTIME_EVENTS.ACTIVITY, () => {
        renderActivity(getActivityHistory());
    });
}

initPage({ activeNav: 'index' });
renderStatus(getLatestTelemetry());
renderActivity(getActivityHistory());
initialiseEventSubscriptions();

