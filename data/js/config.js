import { initPage, renderAlert } from './common.js';
import { getConfig, postConfigCommand } from './api.js';

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
wireConfigForm();

