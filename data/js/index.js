import { initPage, REALTIME_EVENTS, getLatestTelemetry, getActivityHistory } from './common.js';

// Chart instance for real-time data
let statusChart = null;
let chartData = {
    signal: [],
    packets: [],
    errors: [],
    timestamps: []
};
const MAX_CHART_POINTS = 50;

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

function initChart() {
    const ctx = document.getElementById('statusChart');
    if (!ctx) return;

    statusChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: chartData.timestamps,
            datasets: [{
                label: 'Signal Strength (dBm)',
                data: chartData.signal,
                borderColor: '#2563eb',
                backgroundColor: 'rgba(37, 99, 235, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: false
                }
            },
            scales: {
                x: {
                    display: false
                },
                y: {
                    beginAtZero: false,
                    grid: {
                        color: 'rgba(148, 163, 184, 0.1)'
                    }
                }
            },
            elements: {
                point: {
                    radius: 0,
                    hoverRadius: 6
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            }
        }
    });
}

function updateChart(telemetry) {
    if (!statusChart || !telemetry) return;

    const now = formatTimestamp(new Date());
    chartData.timestamps.push(now);
    
    // Add data points
    chartData.signal.push(telemetry.radio?.rssi || 0);
    chartData.packets.push((telemetry.packets_sent || 0) + (telemetry.packets_received || 0));
    chartData.errors.push(telemetry.errors || 0);

    // Keep only recent data
    if (chartData.timestamps.length > MAX_CHART_POINTS) {
        chartData.timestamps.shift();
        chartData.signal.shift();
        chartData.packets.shift();
        chartData.errors.shift();
    }

    // Update chart based on selected type
    const selectedChart = document.querySelector('input[name="chartType"]:checked')?.id || 'chart-signal';
    updateChartDataset(selectedChart);
}

function updateChartDataset(chartType) {
    if (!statusChart) return;

    let data, label, color, backgroundColor;
    
    switch (chartType) {
        case 'chart-packets':
            data = chartData.packets;
            label = 'Total Packets';
            color = '#10b981';
            backgroundColor = 'rgba(16, 185, 129, 0.1)';
            break;
        case 'chart-errors':
            data = chartData.errors;
            label = 'Error Count';
            color = '#ef4444';
            backgroundColor = 'rgba(239, 68, 68, 0.1)';
            break;
        default:
            data = chartData.signal;
            label = 'Signal Strength (dBm)';
            color = '#2563eb';
            backgroundColor = 'rgba(37, 99, 235, 0.1)';
    }

    statusChart.data.datasets[0].data = data;
    statusChart.data.datasets[0].label = label;
    statusChart.data.datasets[0].borderColor = color;
    statusChart.data.datasets[0].backgroundColor = backgroundColor;
    statusChart.update('none');
}

function renderStatusCards(telemetry) {
    if (!telemetry) return;

    // Signal Strength
    const signalElement = document.querySelector('[data-signal-strength]');
    if (signalElement) {
        const rssi = telemetry.radio?.rssi || 0;
        signalElement.textContent = `${rssi} dBm`;
        
        const trendElement = document.querySelector('[data-signal-trend]');
        if (trendElement) {
            const quality = rssi > -70 ? 'Excellent' : rssi > -85 ? 'Good' : rssi > -100 ? 'Fair' : 'Poor';
            trendElement.textContent = quality;
        }
    }

    // Packets Sent
    const packetsSentElement = document.querySelector('[data-packets-sent]');
    if (packetsSentElement) {
        packetsSentElement.textContent = telemetry.packets_sent || 0;
    }

    // Packets Received
    const packetsReceivedElement = document.querySelector('[data-packets-received]');
    if (packetsReceivedElement) {
        packetsReceivedElement.textContent = telemetry.packets_received || 0;
    }

    // Uptime
    const uptimeElement = document.querySelector('[data-uptime]');
    if (uptimeElement) {
        const uptime = telemetry.uptime || 0;
        uptimeElement.textContent = formatUptime(uptime);
        
        const uptimeDetailElement = document.querySelector('[data-uptime-detail]');
        if (uptimeDetailElement) {
            uptimeDetailElement.textContent = `${uptime}s`;
        }
    }
}

function renderDeviceInfo(telemetry) {
    const container = document.querySelector('[data-device-info]');
    if (!container || !telemetry) return;

    const info = [
        { label: 'Operating Mode', value: telemetry.mode || 'Unknown', icon: 'bi-gear' },
        { label: 'Frequency', value: telemetry.radio?.frequency ? `${telemetry.radio.frequency} MHz` : 'Unknown', icon: 'bi-broadcast' },
        { label: 'Bandwidth', value: telemetry.radio?.bandwidth ? `${telemetry.radio.bandwidth} kHz` : 'Unknown', icon: 'bi-speedometer' },
        { label: 'Spreading Factor', value: telemetry.radio?.spreading_factor || 'Unknown', icon: 'bi-layers' },
        { label: 'Coding Rate', value: telemetry.radio?.coding_rate || 'Unknown', icon: 'bi-code-slash' },
        { label: 'TX Power', value: telemetry.radio?.tx_power ? `${telemetry.radio.tx_power} dBm` : 'Unknown', icon: 'bi-lightning' }
    ];

    const html = info.map(item => `
        <div class="d-flex justify-content-between align-items-center py-2 border-bottom">
            <div class="d-flex align-items-center">
                <i class="bi ${item.icon} me-2 text-primary"></i>
                <span class="small">${item.label}</span>
            </div>
            <span class="fw-medium">${item.value}</span>
        </div>
    `).join('');

    container.innerHTML = html;
}

function renderConfigSummary(telemetry) {
    const container = document.querySelector('[data-config-summary]');
    if (!container || !telemetry) return;

    const configs = [
        { label: 'Call Sign', value: telemetry.callsign || 'Not Set', status: telemetry.callsign ? 'success' : 'warning' },
        { label: 'APRS Beacon', value: telemetry.beacon_enabled ? 'Enabled' : 'Disabled', status: telemetry.beacon_enabled ? 'success' : 'secondary' },
        { label: 'GPS Status', value: telemetry.gps?.valid ? 'Valid' : 'No Fix', status: telemetry.gps?.valid ? 'success' : 'warning' }
    ];

    const html = configs.map(item => `
        <div class="d-flex justify-content-between align-items-center py-2">
            <span class="small">${item.label}</span>
            <span class="badge bg-${item.status}">${item.value}</span>
        </div>
    `).join('');

    container.innerHTML = html;
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) return `${days}d ${hours}h`;
    if (hours > 0) return `${hours}h ${minutes}m`;
    return `${minutes}m`;
}

function renderStatus(telemetry) {
    renderStatusCards(telemetry);
    renderDeviceInfo(telemetry);
    renderConfigSummary(telemetry);
    updateChart(telemetry);
    
    // Update last update time
    const lastUpdateElement = document.querySelector('[data-last-update]');
    if (lastUpdateElement) {
        lastUpdateElement.textContent = new Date().toLocaleTimeString();
    }
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

