import { initPage, renderAlert } from './common.js';
import { getConfig, getConfigPresets, postConfigCommand } from './api.js';

function escapeHtml(value) {
    if (value == null) {
        return '';
    }
    return String(value).replace(/[&<>"']/g, (char) => {
        switch (char) {
        case '&':
            return '&amp;';
        case '<':
            return '&lt;';
        case '>':
            return '&gt;';
        case '"':
            return '&quot;';
        case '\'':
            return '&#39;';
        default:
            return char;
        }
    });
}

function formatFrequency(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
        return '—';
    }
    return `${number.toLocaleString(undefined, {
        minimumFractionDigits: 1,
        maximumFractionDigits: 3
    })} MHz`;
}

function formatBandwidth(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
        return '—';
    }
    return `${number.toLocaleString(undefined, {
        minimumFractionDigits: 0,
        maximumFractionDigits: 1
    })} kHz`;
}

function formatMaxPayload(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
        return '—';
    }
    return `${number.toLocaleString()} bytes`;
}

function formatModulation(spreadingFactor, codingRate) {
    const sfValue = Number(spreadingFactor);
    const crValue = Number(codingRate);
    const parts = [];
    if (Number.isFinite(sfValue)) {
        parts.push(`SF${sfValue}`);
    }
    if (Number.isFinite(crValue)) {
        parts.push(`4/${crValue}`);
    }
    if (parts.length === 0) {
        return '—';
    }
    return parts.join(' • ');
}

function createPresetCard(preset) {
    const index = Number.parseInt(preset?.index, 10);
    const hasIndex = Number.isInteger(index);
    const badgeLabel = hasIndex ? `#${index}` : 'Preset';
    const commandHint = hasIndex ? `SETCONFIG ${index}` : null;
    const useCase = preset?.use_case ? escapeHtml(preset.use_case) : '—';
    const throughput = preset?.throughput ? escapeHtml(preset.throughput) : '—';
    const range = preset?.range ? escapeHtml(preset.range) : '—';
    const applyAttr = hasIndex ? `data-preset-apply="${index}"` : '';
    const disabledAttr = hasIndex ? '' : 'disabled';
    const hintMarkup = commandHint
        ? `<span class="text-muted small">Runs <code>${escapeHtml(commandHint)}</code></span>`
        : '';

    return `
        <div class="col-12 col-lg-6">
            <div class="card h-100 shadow-sm">
                <div class="card-body d-flex flex-column gap-3">
                    <div class="d-flex justify-content-between align-items-start">
                        <div>
                            <h3 class="h6 mb-1">${escapeHtml(preset?.name || 'Preset')}</h3>
                            <p class="text-muted small mb-0">${useCase}</p>
                        </div>
                        <span class="badge text-bg-secondary">${escapeHtml(badgeLabel)}</span>
                    </div>
                    <ul class="list-unstyled small mb-0 flex-grow-1">
                        <li><strong>Frequency:</strong> ${escapeHtml(formatFrequency(preset?.frequency_mhz))}</li>
                        <li><strong>Bandwidth:</strong> ${escapeHtml(formatBandwidth(preset?.bandwidth_khz))}</li>
                        <li><strong>Modulation:</strong> ${escapeHtml(formatModulation(preset?.spreading_factor, preset?.coding_rate))}</li>
                        <li><strong>Max payload:</strong> ${escapeHtml(formatMaxPayload(preset?.max_payload_bytes))}</li>
                        <li><strong>Expected range:</strong> ${range}</li>
                        <li><strong>Throughput:</strong> ${throughput}</li>
                    </ul>
                    <div class="d-flex flex-column gap-1">
                        <button type="button" class="btn btn-primary" ${applyAttr} ${disabledAttr}>Apply preset</button>
                        ${hintMarkup}
                    </div>
                </div>
            </div>
        </div>`;
}

function renderPresetList(data) {
    const container = document.querySelector('[data-preset-list]');
    if (!container) {
        return;
    }

    const presets = Array.isArray(data?.presets) ? data.presets : [];
    if (presets.length === 0) {
        container.innerHTML = '<p class="text-muted mb-0">No presets available.</p>';
        return;
    }

    const cards = presets.map((preset) => createPresetCard(preset)).join('');
    container.innerHTML = `<div class="row g-3">${cards}</div>`;
}

function showPresetLoadingState() {
    const container = document.querySelector('[data-preset-list]');
    if (!container) {
        return;
    }
    container.innerHTML = `
        <div class="text-center py-4 text-muted">
            <div class="spinner-border" role="status" aria-hidden="true"></div>
            <p class="mt-3 mb-0">Loading presets…</p>
        </div>`;
}

async function loadPresets() {
    const alertHost = document.querySelector('[data-preset-feedback]');
    showPresetLoadingState();
    try {
        const data = await getConfigPresets();
        renderPresetList(data);
        if (alertHost) {
            alertHost.innerHTML = '';
        }
    } catch (error) {
        const container = document.querySelector('[data-preset-list]');
        if (container) {
            container.innerHTML = '<p class="text-muted mb-0">Preset information unavailable.</p>';
        }
        renderAlert(alertHost, `Unable to load presets: ${error.message}`);
    }
}

function renderConfigStatus(data) {
    const container = document.querySelector('[data-config-status]');
    if (!container) {
        return;
    }

    if (!data?.config) {
        container.innerHTML = '<p class="text-muted mb-0">Configuration data unavailable.</p>';
        return;
    }

    const config = data.config;

    container.innerHTML = `
        <dl class="row mb-0">
            <dt class="col-sm-4">Available</dt>
            <dd class="col-sm-8">${config.available ? 'Yes' : 'No'}</dd>
            <dt class="col-sm-4">Status</dt>
            <dd class="col-sm-8">${config.status_text || 'Unknown'}</dd>
        </dl>`;
}

async function loadConfig() {
    const alertHost = document.querySelector('[data-alert-host]');
    try {
        const data = await getConfig();
        renderConfigStatus(data);
        if (alertHost) {
            alertHost.innerHTML = '';
        }
    } catch (error) {
        renderConfigStatus(null);
        renderAlert(alertHost, `Unable to load configuration: ${error.message}`);
    }
}

function wirePresetActions() {
    const container = document.querySelector('[data-preset-list]');
    if (!container) {
        return;
    }

    const alertHost = document.querySelector('[data-preset-feedback]');

    container.addEventListener('click', async (event) => {
        const button = event.target.closest('[data-preset-apply]');
        if (!button || button.disabled) {
            return;
        }

        const presetIndex = button.getAttribute('data-preset-apply');
        if (presetIndex == null || presetIndex === '') {
            return;
        }

        if (alertHost) {
            alertHost.innerHTML = '';
        }

        const command = `SETCONFIG ${presetIndex}`;
        const originalText = button.textContent;
        button.disabled = true;
        button.textContent = 'Applying…';

        try {
            const response = await postConfigCommand(command);
            if (alertHost) {
                const message = response?.message || 'Preset applied successfully.';
                renderAlert(alertHost, `Preset applied: ${message}`, 'success');
            }
            await loadConfig();
        } catch (error) {
            if (alertHost) {
                renderAlert(alertHost, `Failed to apply preset: ${error.message}`);
            }
        } finally {
            button.disabled = false;
            button.textContent = originalText;
        }
    });
}

function wireConfigForm() {
    const form = document.querySelector('[data-config-form]');
    if (!form) {
        return;
    }

    const commandInput = form.querySelector('[name="command"]');
    const resultHost = document.querySelector('[data-config-result]');

    form.addEventListener('submit', async (event) => {
        event.preventDefault();
        if (!commandInput.value.trim()) {
            renderAlert(resultHost, 'Please enter a configuration command.', 'warning');
            return;
        }

        try {
            const response = await postConfigCommand(commandInput.value.trim());
            renderAlert(
                resultHost,
                `Command processed: ${response.message || 'Success.'}`,
                response.success ? 'success' : 'warning'
            );
            commandInput.value = '';
            await loadConfig();
        } catch (error) {
            renderAlert(resultHost, `Command failed: ${error.message}`);
        }
    });
}

initPage({ activeNav: 'config' });
loadConfig();
loadPresets();
wireConfigForm();
wirePresetActions();
