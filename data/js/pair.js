import { getStatus, postPairingConfiguration } from './api.js';
import { renderAlert, syncCsrfToken, PAIRING_EVENTS } from './common.js';

const THEME_STORAGE_KEY = 'loraTncxThemeOverride';
const STATUS_REFRESH_INTERVAL_MS = 10000;
const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');

const form = document.querySelector('[data-pair-form]');
const feedbackHost = document.querySelector('[data-feedback]');
const apSsidTarget = document.querySelector('[data-ap-ssid]');
const submitButton = form?.querySelector('[data-submit]');
const continueButton = form?.querySelector('[data-continue]');
const themeSelect = form?.querySelector('[name="theme"]');

let pairingFinalised = false;
let statusRefreshTimer = null;

function systemTheme() {
    return mediaQuery.matches ? 'dark' : 'light';
}

function applyThemePreference(theme) {
    const resolved = !theme || theme === 'system' ? systemTheme() : theme;
    document.documentElement.setAttribute('data-bs-theme', resolved);
    document.documentElement.style.colorScheme = resolved;
}

function storeThemePreference(theme) {
    if (!theme || theme === 'system') {
        localStorage.removeItem(THEME_STORAGE_KEY);
    } else {
        localStorage.setItem(THEME_STORAGE_KEY, theme);
    }
    applyThemePreference(theme);
}

function initThemeFromStorage() {
    const stored = localStorage.getItem(THEME_STORAGE_KEY);
    applyThemePreference(stored || 'system');
    if (themeSelect) {
        if (stored && ['light', 'dark'].includes(stored)) {
            themeSelect.value = stored;
        } else {
            themeSelect.value = '';
        }
    }
}

function gotoDashboard() {
    window.location.replace('/index.html');
}

function clearAlert() {
    if (feedbackHost) {
        feedbackHost.innerHTML = '';
    }
}

function showAlert(message, type = 'danger') {
    if (!feedbackHost) {
        return;
    }
    renderAlert(feedbackHost, message, type);
}

function setFormDisabled(disabled) {
    if (!form) {
        return;
    }
    const elements = form.querySelectorAll('input, select, button');
    elements.forEach((element) => {
        element.disabled = disabled;
    });
}

function setSubmitting(submitting) {
    if (!form) {
        return;
    }
    setFormDisabled(submitting);
    if (submitButton) {
        submitButton.disabled = submitting;
        submitButton.innerHTML = submitting
            ? '<span class="spinner-border spinner-border-sm me-2" role="status" aria-hidden="true"></span>Completing…'
            : 'Complete pairing';
    }
}

function markPairingComplete(message, type = 'success') {
    if (pairingFinalised) {
        return;
    }
    pairingFinalised = true;
    if (statusRefreshTimer) {
        window.clearTimeout(statusRefreshTimer);
        statusRefreshTimer = null;
    }
    setFormDisabled(true);
    if (form) {
        form.classList.add('was-validated');
    }
    showAlert(message || 'Pairing complete. Redirecting to the dashboard…', type);
    if (continueButton) {
        continueButton.classList.remove('d-none');
        continueButton.addEventListener('click', gotoDashboard, { once: true });
        continueButton.focus();
    }
    window.setTimeout(() => gotoDashboard(), 2500);
}

async function refreshPairingStatus() {
    if (statusRefreshTimer) {
        window.clearTimeout(statusRefreshTimer);
        statusRefreshTimer = null;
    }

    let pairingRequired = true;
    try {
        const status = await getStatus();
        const apSsid = status?.wifi?.ap_ssid || 'LoRaTNCX';
        if (apSsidTarget) {
            apSsidTarget.textContent = apSsid;
        }
        pairingRequired = Boolean(status?.wifi?.pairing_required);
        if (!pairingRequired) {
            markPairingComplete('Device already paired. Redirecting to the dashboard…', 'info');
            return false;
        }
    } catch (error) {
        showAlert(`Unable to verify pairing status: ${error.message}`, 'warning');
    }
    if (!pairingFinalised && pairingRequired) {
        statusRefreshTimer = window.setTimeout(() => {
            refreshPairingStatus();
        }, STATUS_REFRESH_INTERVAL_MS);
    }
    return !pairingFinalised && pairingRequired;
}

function normaliseThemeSelection(raw) {
    if (!raw) {
        return '';
    }
    const value = String(raw).trim().toLowerCase();
    if (!value) {
        return '';
    }
    return ['system', 'light', 'dark'].includes(value) ? value : null;
}

async function handleSubmit(event) {
    event.preventDefault();
    if (!form || pairingFinalised) {
        return;
    }

    clearAlert();

    const formData = new FormData(form);
    const wifiSsid = String(formData.get('wifi_ssid') || '').trim();
    const wifiPassword = String(formData.get('wifi_password') || '');
    const uiUsername = String(formData.get('ui_username') || '').trim();
    const uiPassword = String(formData.get('ui_password') || '');
    const theme = normaliseThemeSelection(formData.get('theme'));

    const errors = [];
    if (!wifiSsid) {
        errors.push('Wi-Fi SSID is required.');
    }
    if (wifiPassword.length < 8) {
        errors.push('Wi-Fi password must be at least 8 characters long.');
    }
    if (!uiUsername) {
        errors.push('UI username is required.');
    }
    if (uiPassword.length < 8) {
        errors.push('UI password must be at least 8 characters long.');
    }
    if (theme === null) {
        errors.push('Selected theme is not supported.');
    }

    if (errors.length > 0) {
        showAlert(errors.join('<br />'), 'warning');
        return;
    }

    const payload = {
        wifi_ssid: wifiSsid,
        wifi_password: wifiPassword,
        ui_username: uiUsername,
        ui_password: uiPassword
    };

    if (theme) {
        payload.theme = theme;
    }

    try {
        setSubmitting(true);
        const response = await postPairingConfiguration(payload);
        syncCsrfToken(response?.csrf_token);
        storeThemePreference(response?.theme);
        markPairingComplete('Pairing successful! Redirecting to the dashboard…', 'success');
    } catch (error) {
        setSubmitting(false);
        const message = error?.message || 'Pairing failed.';
        if (/already paired/i.test(message)) {
            markPairingComplete(message, 'info');
            return;
        }
        showAlert(`Pairing failed: ${message}`);
    }
}

initThemeFromStorage();

if (form) {
    form.addEventListener('submit', handleSubmit);
}

refreshPairingStatus();

document.addEventListener(PAIRING_EVENTS.STATE, (event) => {
    if (!event?.detail || pairingFinalised) {
        return;
    }
    if (event.detail.required === false) {
        markPairingComplete('Pairing complete. Redirecting to the dashboard…', 'success');
    }
});
