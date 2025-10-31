import { initPage } from './common.js';
import { getStatus } from './api.js';

function renderStatus(status) {
    const container = document.querySelector('[data-status-container]');
    if (!container) {
        return;
    }

    if (!status) {
        container.innerHTML = '<p class="text-muted">Status unavailable.</p>';
        return;
    }

    const wifi = status.wifi || {};
    const tnc = status.tnc || {};

    container.innerHTML = `
        <div class="row g-3">
            <div class="col-md-6">
                <div class="card h-100">
                    <div class="card-header">Wi-Fi</div>
                    <div class="card-body">
                        <dl class="row mb-0">
                            <dt class="col-sm-5">AP SSID</dt>
                            <dd class="col-sm-7">${wifi.ap_ssid || 'N/A'}</dd>
                            <dt class="col-sm-5">STA Connected</dt>
                            <dd class="col-sm-7">${wifi.sta_connected ? 'Yes' : 'No'}</dd>
                        </dl>
                    </div>
                </div>
            </div>
            <div class="col-md-6">
                <div class="card h-100">
                    <div class="card-header">TNC</div>
                    <div class="card-body">
                        <p class="mb-1"><strong>Available:</strong> ${tnc.available ? 'Yes' : 'No'}</p>
                        <p class="mb-0"><strong>Status:</strong> ${tnc.status_text || 'Unknown'}</p>
                    </div>
                </div>
            </div>
        </div>`;
}

async function loadStatus() {
    try {
        const data = await getStatus();
        renderStatus(data);
    } catch (error) {
        renderStatus(null);
        console.error('Unable to load status:', error);
    }
}

initPage({ activeNav: 'index' });
loadStatus();
