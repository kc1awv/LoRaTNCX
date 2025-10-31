import { initPage, renderAlert } from './common.js';
import { getStatus } from './api.js';

function renderDisplayStatus(display) {
    if (!display) {
        return '<p class="text-muted mb-0">Display information unavailable.</p>';
    }

    const rows = [
        ['Mode', display.mode],
        ['TX Count', display.tx_count],
        ['RX Count', display.rx_count],
        ['Last Packet (ms)', display.last_packet_millis],
        ['Recent Packet', display.has_recent_packet ? 'Yes' : 'No'],
        ['Last RSSI', display.last_rssi],
        ['Last SNR', display.last_snr],
        ['Frequency (MHz)', display.frequency_mhz],
        ['Bandwidth (kHz)', display.bandwidth_khz],
        ['Spreading Factor', display.spreading_factor],
        ['Coding Rate', display.coding_rate],
        ['TX Power (dBm)', display.tx_power_dbm],
        ['Uptime (ms)', display.uptime_ms]
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
            <dd class="col-sm-7">${gnss.has_fix ? (gnss.is_3d_fix ? '3D' : '2D') : 'No'}</dd>
            <dt class="col-sm-5">Coordinates</dt>
            <dd class="col-sm-7">${coordinates}</dd>
            <dt class="col-sm-5">Altitude (m)</dt>
            <dd class="col-sm-7">${gnss.altitude_m ?? 'N/A'}</dd>
            <dt class="col-sm-5">Satellites</dt>
            <dd class="col-sm-7">${gnss.satellites ?? 'N/A'}</dd>
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
                    <div class="card-body">
                        <dl class="row mb-0">
                            <dt class="col-sm-5">Active</dt>
                            <dd class="col-sm-7">${display.power_off?.active ? 'Yes' : 'No'}</dd>
                            <dt class="col-sm-5">Progress</dt>
                            <dd class="col-sm-7">${display.power_off?.progress ?? 'N/A'}</dd>
                            <dt class="col-sm-5">Complete</dt>
                            <dd class="col-sm-7">${display.power_off?.complete ? 'Yes' : 'No'}</dd>
                        </dl>
                    </div>
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

async function loadStatus() {
    const alertHost = document.querySelector('[data-alert-host]');
    try {
        const data = await getStatus();
        renderStatus(data);
        if (alertHost) {
            alertHost.innerHTML = '';
        }
    } catch (error) {
        renderStatus(null);
        renderAlert(alertHost, `Unable to load status: ${error.message}`);
    }
}

initPage({ activeNav: 'status' });
loadStatus();

