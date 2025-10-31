const DEFAULT_HEADERS = {
    'Content-Type': 'application/json'
};

let latestCsrfToken = null;

function buildHeaders(options = {}) {
    let includeJson = true;
    let includeCsrf = true;

    if (typeof options === 'boolean') {
        includeJson = options;
    } else if (options && typeof options === 'object') {
        if (options.includeJson !== undefined) {
            includeJson = Boolean(options.includeJson);
        }
        if (options.includeCsrf !== undefined) {
            includeCsrf = Boolean(options.includeCsrf);
        }
    }

    const headers = includeJson ? { ...DEFAULT_HEADERS } : {};
    if (includeCsrf && latestCsrfToken) {
        headers['X-CSRF-Token'] = latestCsrfToken;
    }
    return headers;
}

function ensureCsrfHeader(headers) {
    if (!headers || headers['X-CSRF-Token']) {
        return headers;
    }
    if (typeof document !== 'undefined') {
        const meta = document.querySelector('meta[name="csrf-token"]');
        if (meta?.content) {
            headers['X-CSRF-Token'] = meta.content;
        }
    }
    return headers;
}

export function setCsrfToken(token) {
    latestCsrfToken = token || null;
}

async function handleResponse(response) {
    if (!response.ok) {
        const contentType = response.headers.get('content-type') || '';
        let message = `${response.status} ${response.statusText}`;
        if (contentType.includes('application/json')) {
            try {
                const data = await response.json();
                if (data && data.error) {
                    message = data.error;
                }
            } catch (_) {
                // Ignore JSON parsing errors and fall back to status text.
            }
        } else {
            try {
                const text = await response.text();
                if (text) {
                    message = text;
                }
            } catch (_) {
                // Ignore text parsing errors and fall back to status text.
            }
        }
        throw new Error(message);
    }

    const contentType = response.headers.get('content-type') || '';
    if (contentType.includes('application/json')) {
        return response.json();
    }
    return response.text();
}

export async function getStatus() {
    return handleResponse(await fetch('/api/status', { cache: 'no-store' }));
}

export async function getConfig() {
    return handleResponse(await fetch('/api/config', { cache: 'no-store' }));
}

export async function getConfigPresets() {
    return handleResponse(await fetch('/api/config/presets', { cache: 'no-store' }));
}

export async function postConfigCommand(command) {
    return handleResponse(
        await fetch('/api/config', {
            method: 'POST',
            headers: buildHeaders(true),
            body: JSON.stringify({ command })
        })
    );
}

export async function postCommand(command, { signal } = {}) {
    const headers = ensureCsrfHeader(buildHeaders(true));
    return handleResponse(
        await fetch('/api/command', {
            method: 'POST',
            headers,
            body: JSON.stringify({ command }),
            signal
        })
    );
}

export async function getThemePreference() {
    return handleResponse(await fetch('/api/ui/theme', { cache: 'no-store' }));
}

export async function postThemePreference(theme, source = 'user') {
    return handleResponse(
        await fetch('/api/ui/theme', {
            method: 'POST',
            headers: buildHeaders(true),
            body: JSON.stringify({ theme, source })
        })
    );
}

export async function postPairingConfiguration(payload) {
    return handleResponse(
        await fetch('/api/pair', {
            method: 'POST',
            headers: buildHeaders({ includeJson: true, includeCsrf: false }),
            body: JSON.stringify(payload)
        })
    );
}

const PERIPHERAL_ENDPOINTS = Object.freeze({
    gnss: 'gnss',
    oled: 'oled'
});

export async function updatePeripheral(name, enabled) {
    const key = String(name || '').toLowerCase();
    if (!(key in PERIPHERAL_ENDPOINTS)) {
        throw new Error(`Unsupported peripheral: ${name}`);
    }
    if (typeof enabled !== 'boolean') {
        throw new TypeError('Peripheral enabled state must be a boolean.');
    }

    return handleResponse(
        await fetch(`/api/peripherals/${PERIPHERAL_ENDPOINTS[key]}`, {
            method: 'POST',
            headers: buildHeaders(true),
            body: JSON.stringify({ enabled })
        })
    );
}
