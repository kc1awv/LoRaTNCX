const DEFAULT_HEADERS = {
    'Content-Type': 'application/json'
};

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

export async function postConfigCommand(command) {
    return handleResponse(
        await fetch('/api/config', {
            method: 'POST',
            headers: DEFAULT_HEADERS,
            body: JSON.stringify({ command })
        })
    );
}

export async function postCommand(command) {
    return handleResponse(
        await fetch('/api/command', {
            method: 'POST',
            headers: DEFAULT_HEADERS,
            body: JSON.stringify({ command })
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
            headers: DEFAULT_HEADERS,
            body: JSON.stringify({ theme, source })
        })
    );
}
