const DEFAULT_RECONNECT_DELAY = 1500;
const MAX_RECONNECT_DELAY = 15000;

function isObject(value) {
    return value != null && typeof value === 'object' && !Array.isArray(value);
}

function assignIfPresent(target, key, value, transform) {
    if (value === undefined) {
        return;
    }
    target[key] = transform ? transform(value) : value;
}

function normalizeWifi(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const wifi = {};
    if ('ap_ssid' in raw) {
        assignIfPresent(wifi, 'apSsid', raw.ap_ssid);
    }
    if ('sta_connected' in raw) {
        assignIfPresent(wifi, 'staConnected', Boolean(raw.sta_connected));
    }
    if ('pairing_required' in raw) {
        assignIfPresent(wifi, 'pairingRequired', Boolean(raw.pairing_required));
    }
    return Object.keys(wifi).length ? wifi : null;
}

function normalizeBattery(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const battery = {};
    if ('voltage' in raw) {
        assignIfPresent(battery, 'voltage', raw.voltage);
    }
    if ('percent' in raw) {
        assignIfPresent(battery, 'percent', raw.percent);
    }
    return Object.keys(battery).length ? battery : null;
}

function normalizePowerOff(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const powerOff = {};
    if ('active' in raw) {
        assignIfPresent(powerOff, 'active', Boolean(raw.active));
    }
    if ('progress' in raw) {
        assignIfPresent(powerOff, 'progress', raw.progress);
    }
    if ('complete' in raw) {
        assignIfPresent(powerOff, 'complete', Boolean(raw.complete));
    }
    return Object.keys(powerOff).length ? powerOff : null;
}

function normalizeGnss(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const gnss = {};
    const fields = [
        ['enabled', 'enabled', (value) => Boolean(value)],
        ['has_fix', 'hasFix', (value) => Boolean(value)],
        ['is_3d_fix', 'is3dFix', (value) => Boolean(value)],
        ['latitude', 'latitude'],
        ['longitude', 'longitude'],
        ['altitude_m', 'altitudeMeters'],
        ['speed_knots', 'speedKnots'],
        ['course_degrees', 'courseDegrees'],
        ['hdop', 'hdop'],
        ['satellites', 'satellites'],
        ['time_valid', 'timeValid', (value) => Boolean(value)],
        ['time_synced', 'timeSynced', (value) => Boolean(value)],
        ['year', 'year'],
        ['month', 'month'],
        ['day', 'day'],
        ['hour', 'hour'],
        ['minute', 'minute'],
        ['second', 'second'],
        ['pps_available', 'ppsAvailable', (value) => Boolean(value)],
        ['pps_last_millis', 'ppsLastMillis'],
        ['pps_count', 'ppsCount']
    ];

    for (const [rawKey, targetKey, transform] of fields) {
        if (rawKey in raw) {
            assignIfPresent(gnss, targetKey, raw[rawKey], transform);
        }
    }

    return Object.keys(gnss).length ? gnss : null;
}

function normalizeDisplay(raw) {
    if (!isObject(raw)) {
        return null;
    }

    const display = {};
    const fields = [
        ['mode', 'mode'],
        ['tx_count', 'txCount'],
        ['rx_count', 'rxCount'],
        ['last_packet_millis', 'lastPacketMillis'],
        ['has_recent_packet', 'hasRecentPacket', (value) => Boolean(value)],
        ['last_rssi', 'lastRssi'],
        ['last_snr', 'lastSnr'],
        ['frequency_mhz', 'frequencyMHz'],
        ['bandwidth_khz', 'bandwidthKHz'],
        ['spreading_factor', 'spreadingFactor'],
        ['coding_rate', 'codingRate'],
        ['tx_power_dbm', 'txPowerDbm'],
        ['uptime_ms', 'uptimeMs']
    ];

    for (const [rawKey, targetKey, transform] of fields) {
        if (rawKey in raw) {
            assignIfPresent(display, targetKey, raw[rawKey], transform);
        }
    }

    if ('battery' in raw) {
        const battery = normalizeBattery(raw.battery);
        if (battery) {
            display.battery = battery;
        }
    }

    if ('power_off' in raw) {
        const powerOff = normalizePowerOff(raw.power_off);
        if (powerOff) {
            display.powerOff = powerOff;
        }
    }

    if ('gnss' in raw) {
        const gnss = normalizeGnss(raw.gnss);
        if (gnss) {
            display.gnss = gnss;
        }
    }

    return Object.keys(display).length ? display : null;
}

function normalizeTnc(raw) {
    if (!isObject(raw)) {
        return null;
    }

    const tnc = {};
    if ('available' in raw) {
        assignIfPresent(tnc, 'available', Boolean(raw.available));
    }
    if ('status_text' in raw) {
        assignIfPresent(tnc, 'statusText', raw.status_text);
    }
    if ('display' in raw) {
        const display = normalizeDisplay(raw.display);
        if (display) {
            tnc.display = display;
        }
    }
    return Object.keys(tnc).length ? tnc : null;
}

function normalizeUi(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const ui = {};
    if ('theme' in raw) {
        assignIfPresent(ui, 'theme', raw.theme);
    }
    if ('override' in raw) {
        assignIfPresent(ui, 'override', Boolean(raw.override));
    }
    if ('csrf_token' in raw) {
        assignIfPresent(ui, 'csrfToken', raw.csrf_token);
    }
    return Object.keys(ui).length ? ui : null;
}

function normalizeHello(raw, timestamp) {
    const payload = {};
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    if ('ap_ssid' in raw) {
        assignIfPresent(payload, 'apSsid', raw.ap_ssid);
    }
    if ('sta_connected' in raw) {
        assignIfPresent(payload, 'staConnected', Boolean(raw.sta_connected));
    }
    if ('pairing_required' in raw) {
        assignIfPresent(payload, 'pairingRequired', Boolean(raw.pairing_required));
    }
    if ('ui_theme' in raw) {
        assignIfPresent(payload, 'uiTheme', raw.ui_theme);
    }
    if ('ui_theme_override' in raw) {
        assignIfPresent(payload, 'uiThemeOverride', Boolean(raw.ui_theme_override));
    }
    if ('csrf_token' in raw) {
        assignIfPresent(payload, 'csrfToken', raw.csrf_token);
    }
    if ('tnc_available' in raw) {
        assignIfPresent(payload, 'tncAvailable', Boolean(raw.tnc_available));
    }
    if ('tnc_status' in raw) {
        assignIfPresent(payload, 'tncStatus', raw.tnc_status);
    }
    return {
        type: 'hello',
        timestamp,
        payload
    };
}

function normalizeStatusSnapshot(raw, timestamp) {
    const payload = {};
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    const wifi = normalizeWifi(raw.wifi);
    if (wifi) {
        payload.wifi = wifi;
    }
    const tnc = normalizeTnc(raw.tnc);
    if (tnc) {
        payload.tnc = tnc;
    }
    const ui = normalizeUi(raw.ui);
    if (ui) {
        payload.ui = ui;
    }
    return {
        type: 'status_snapshot',
        timestamp,
        payload
    };
}

function normalizeStatus(raw, timestamp) {
    const payload = {};
    if ('client_count' in raw) {
        assignIfPresent(payload, 'clientCount', raw.client_count);
    }
    if ('display' in raw) {
        const display = normalizeDisplay(raw.display);
        if (display) {
            payload.display = display;
        }
    }
    return {
        type: 'status',
        timestamp,
        payload
    };
}

function normalizeCommandResult(raw, timestamp) {
    const payload = {};
    if ('command' in raw) {
        assignIfPresent(payload, 'command', raw.command);
    }
    if ('success' in raw) {
        assignIfPresent(payload, 'success', Boolean(raw.success));
    }
    if ('message' in raw) {
        assignIfPresent(payload, 'message', raw.message);
    }
    if ('result' in raw) {
        assignIfPresent(payload, 'result', raw.result);
    }
    if ('source' in raw) {
        assignIfPresent(payload, 'source', raw.source);
    }
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    return {
        type: 'command_result',
        timestamp,
        payload
    };
}

function normalizeConfigResult(raw, timestamp) {
    const payload = {};
    if ('command' in raw) {
        assignIfPresent(payload, 'command', raw.command);
    }
    if ('success' in raw) {
        assignIfPresent(payload, 'success', Boolean(raw.success));
    }
    if ('message' in raw) {
        assignIfPresent(payload, 'message', raw.message);
    }
    if ('source' in raw) {
        assignIfPresent(payload, 'source', raw.source);
    }
    if ('status_text' in raw) {
        assignIfPresent(payload, 'statusText', raw.status_text);
    }
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    return {
        type: 'config_result',
        timestamp,
        payload
    };
}

function normalizeAlert(raw, timestamp) {
    const payload = {};
    if ('category' in raw) {
        assignIfPresent(payload, 'category', raw.category);
    }
    if ('message' in raw) {
        assignIfPresent(payload, 'message', raw.message);
    }
    if ('state' in raw) {
        assignIfPresent(payload, 'state', Boolean(raw.state));
    }
    return {
        type: 'alert',
        timestamp,
        payload
    };
}

function normalizePacket(raw, timestamp) {
    const payload = {};
    if ('length' in raw) {
        assignIfPresent(payload, 'length', raw.length);
    }
    if ('rssi' in raw) {
        assignIfPresent(payload, 'rssi', raw.rssi);
    }
    if ('snr' in raw) {
        assignIfPresent(payload, 'snr', raw.snr);
    }
    if ('preview' in raw) {
        assignIfPresent(payload, 'preview', raw.preview);
    }
    return {
        type: 'packet',
        timestamp,
        payload
    };
}

function normalizeCsrf(raw, timestamp) {
    const payload = {};
    if ('token' in raw) {
        assignIfPresent(payload, 'token', raw.token);
    }
    return {
        type: 'csrf_token',
        timestamp,
        payload
    };
}

function normalizeUiTheme(raw, timestamp) {
    const payload = {};
    if ('theme' in raw) {
        assignIfPresent(payload, 'theme', raw.theme);
    }
    if ('override' in raw) {
        assignIfPresent(payload, 'override', Boolean(raw.override));
    }
    if ('source' in raw) {
        assignIfPresent(payload, 'source', raw.source);
    }
    if ('csrf_token' in raw) {
        assignIfPresent(payload, 'csrfToken', raw.csrf_token);
    }
    return {
        type: 'ui_theme',
        timestamp,
        payload
    };
}

function normalizeError(raw, timestamp) {
    const payload = {};
    if ('message' in raw) {
        assignIfPresent(payload, 'message', raw.message);
    }
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    return {
        type: 'error',
        timestamp,
        payload
    };
}

function normalizeClientDisconnected(raw, timestamp) {
    const payload = {};
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    return {
        type: 'client_disconnected',
        timestamp,
        payload
    };
}

function normalizePong(raw, timestamp) {
    const payload = {};
    if ('client_id' in raw) {
        assignIfPresent(payload, 'clientId', raw.client_id);
    }
    if ('timestamp' in raw) {
        assignIfPresent(payload, 'echoTimestamp', raw.timestamp);
    }
    return {
        type: 'pong',
        timestamp,
        payload
    };
}

const MESSAGE_NORMALIZERS = {
    hello: normalizeHello,
    status_snapshot: normalizeStatusSnapshot,
    status: normalizeStatus,
    command_result: normalizeCommandResult,
    config_result: normalizeConfigResult,
    alert: normalizeAlert,
    packet: normalizePacket,
    csrf_token: normalizeCsrf,
    ui_theme: normalizeUiTheme,
    error: normalizeError,
    client_disconnected: normalizeClientDisconnected,
    pong: normalizePong
};

export function normalizeMessage(raw) {
    if (!isObject(raw)) {
        return null;
    }
    const rawType = raw.type;
    if (!rawType) {
        return null;
    }
    const type = String(rawType).toLowerCase();
    const timestamp = 'timestamp' in raw ? raw.timestamp : Date.now();
    const normalizer = MESSAGE_NORMALIZERS[type];
    if (!normalizer) {
        return null;
    }
    return normalizer(raw, timestamp);
}

export function createRealtimeConnection({
    url,
    reconnectDelay = DEFAULT_RECONNECT_DELAY,
    maxReconnectDelay = MAX_RECONNECT_DELAY
} = {}) {
    const target = new EventTarget();
    let socket = null;
    let shouldReconnect = true;
    let reconnectTimer = null;
    let attempt = 0;

    const resolvedUrl = url || (() => {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        return `${protocol}//${window.location.host}/ws`;
    })();

    const cleanupSocket = () => {
        if (!socket) {
            return;
        }
        socket.onopen = null;
        socket.onclose = null;
        socket.onerror = null;
        socket.onmessage = null;
        socket = null;
    };

    const scheduleReconnect = () => {
        if (!shouldReconnect) {
            return;
        }
        const delay = Math.min(maxReconnectDelay, reconnectDelay * Math.pow(2, attempt));
        reconnectTimer = window.setTimeout(() => {
            reconnectTimer = null;
            connect();
        }, delay);
        attempt += 1;
    };

    const connect = () => {
        cleanupSocket();
        if (!shouldReconnect) {
            return;
        }
        try {
            socket = new WebSocket(resolvedUrl);
        } catch (error) {
            target.dispatchEvent(new CustomEvent('error', { detail: { error } }));
            scheduleReconnect();
            return;
        }

        socket.onopen = (event) => {
            attempt = 0;
            target.dispatchEvent(new CustomEvent('open', { detail: { event } }));
        };

        socket.onclose = (event) => {
            target.dispatchEvent(new CustomEvent('close', { detail: { event } }));
            cleanupSocket();
            if (shouldReconnect) {
                scheduleReconnect();
            }
        };

        socket.onerror = (event) => {
            target.dispatchEvent(new CustomEvent('error', { detail: { event } }));
        };

        socket.onmessage = (event) => {
            let parsed;
            try {
                parsed = JSON.parse(event.data);
            } catch (error) {
                target.dispatchEvent(new CustomEvent('error', { detail: { error } }));
                return;
            }
            const message = normalizeMessage(parsed);
            if (!message) {
                return;
            }
            target.dispatchEvent(new CustomEvent('message', { detail: { message, raw: parsed } }));
        };
    };

    const start = () => {
        if (socket || reconnectTimer) {
            return;
        }
        attempt = 0;
        shouldReconnect = true;
        connect();
    };

    const stop = () => {
        shouldReconnect = false;
        if (reconnectTimer) {
            window.clearTimeout(reconnectTimer);
            reconnectTimer = null;
        }
        if (socket && socket.readyState === WebSocket.OPEN) {
            socket.close();
        }
        cleanupSocket();
    };

    start();

    return {
        addEventListener: (...args) => target.addEventListener(...args),
        removeEventListener: (...args) => target.removeEventListener(...args),
        start,
        stop,
        send(data) {
            if (!socket || socket.readyState !== WebSocket.OPEN) {
                return false;
            }
            socket.send(data);
            return true;
        },
        isConnected() {
            return socket != null && socket.readyState === WebSocket.OPEN;
        }
    };
}

