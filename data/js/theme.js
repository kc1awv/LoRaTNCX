import { postThemePreference } from './api.js';

const STORAGE_KEY = 'loraTncxThemeOverride';
const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
let lastSentTheme = null;
let pendingSync = false;

function getStoredOverride() {
    return localStorage.getItem(STORAGE_KEY);
}

function setStoredOverride(value) {
    if (!value) {
        localStorage.removeItem(STORAGE_KEY);
    } else {
        localStorage.setItem(STORAGE_KEY, value);
    }
}

function systemTheme() {
    return mediaQuery.matches ? 'dark' : 'light';
}

function applyTheme(theme) {
    const resolved = theme || systemTheme();
    document.documentElement.setAttribute('data-bs-theme', resolved);
    document.documentElement.style.colorScheme = resolved;
    return resolved;
}

async function notifyFirmware(theme, source) {
    if (lastSentTheme === theme) {
        return;
    }

    try {
        await postThemePreference(theme, source);
        lastSentTheme = theme;
    } catch (error) {
        console.warn('Unable to update firmware theme preference:', error);
    }
}

function syncFromStorage({ notify = true } = {}) {
    const override = getStoredOverride();
    const resolved = applyTheme(override);
    if (notify && !pendingSync) {
        pendingSync = true;
        queueMicrotask(() => {
            notifyFirmware(override || 'system', override ? 'override' : 'system').finally(() => {
                pendingSync = false;
            });
        });
    }
    return { override, resolved };
}

export function initThemeSync() {
    syncFromStorage();

    mediaQuery.addEventListener('change', () => {
        if (!getStoredOverride()) {
            const resolved = applyTheme();
            notifyFirmware('system', 'system-change').catch(() => {
                /* ignore */
            });
            lastSentTheme = 'system';
            document.documentElement.dispatchEvent(
                new CustomEvent('themechange', { detail: { theme: resolved, override: false } })
            );
        }
    });
}

export function bindThemeControl(control) {
    if (!control) {
        return;
    }

    const applySelection = () => {
        const override = getStoredOverride();
        control.value = override || 'system';
        const resolved = applyTheme(override);
        control.setAttribute('data-active-theme', resolved);
    };

    control.addEventListener('change', () => {
        const value = control.value;
        if (value === 'system') {
            setStoredOverride(null);
        } else {
            setStoredOverride(value);
        }
        const { override, resolved } = syncFromStorage();
        control.setAttribute('data-active-theme', resolved);
        document.documentElement.dispatchEvent(
            new CustomEvent('themechange', { detail: { theme: resolved, override: Boolean(override) } })
        );
    });

    applySelection();
}

export function currentThemeState() {
    const override = getStoredOverride();
    const theme = override || systemTheme();
    return { override, theme };
}
