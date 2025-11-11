let currentTab = 'lora';
let autoRefreshEnabled = true;
let autoRefreshInterval = null;
let toastCounter = 0;
let loadedTabs = {}; // Track which tabs have been loaded
let lastStatusData = null; // Track last status data for change detection
let pageVisible = true; // Track page visibility

// Initialize tooltips
document.addEventListener('DOMContentLoaded', function() {
    tippy('.tooltip-trigger', {
        theme: 'light',
        placement: 'top',
        arrow: true,
        delay: [300, 0]
    });

    // Preload default tab (lora) on page load
    loadBoardInfo();
    loadLoRaConfig();
    loadedTabs['lora'] = true;

    // Add page visibility detection for auto-refresh optimization
    document.addEventListener('visibilitychange', function() {
        pageVisible = !document.hidden;
        if (pageVisible && autoRefreshEnabled) {
            startAutoRefresh(); // Restart with optimized interval when page becomes visible
        } else if (!pageVisible) {
            stopAutoRefresh(); // Stop refreshing when page is hidden
        }
    });
});

// Switch between tabs
function switchTab(tabName) {
    // Update tab buttons
    document.querySelectorAll('.nav-tab').forEach(tab => tab.classList.remove('active'));
    event.target.closest('.nav-tab').classList.add('active');

    // Update tab content
    document.querySelectorAll('.content-section').forEach(content => content.classList.remove('active'));
    document.getElementById(tabName + '-tab').classList.add('active');

    currentTab = tabName;

    // Load data for the tab only if not already loaded (lazy loading)
    if (!loadedTabs[tabName]) {
        if (tabName === 'lora') {
            loadBoardInfo();
            loadLoRaConfig();
        } else if (tabName === 'wifi') {
            loadWiFiConfig();
        } else if (tabName === 'gnss') {
            loadGNSSConfig();
            loadGNSSStatus();
        } else if (tabName === 'system') {
            loadSystemInfo();
        }
        loadedTabs[tabName] = true; // Mark tab as loaded
    }
}

// Show alert message (now uses toast notifications)
function showAlert(type, message, prefix = currentTab) {
    const title = type === 'success' ? 'Success' : type === 'error' ? 'Error' : 'Warning';
    showToast(type, title, message);
}

// Enhanced UX Functions
function showToast(type, title, message, duration = 5000) {
    const toastContainer = document.getElementById('toastContainer');
    const toastId = 'toast-' + (++toastCounter);

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.id = toastId;
    toast.innerHTML = `
        <div class="toast-content">
            <div class="toast-icon">
                <i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'exclamation-triangle'}"></i>
            </div>
            <div class="toast-text">
                <div class="toast-title">${title}</div>
                <div class="toast-message">${message}</div>
            </div>
            <button class="toast-close" onclick="hideToast('${toastId}')">
                <i class="fas fa-times"></i>
            </button>
        </div>
    `;

    toastContainer.appendChild(toast);

    // Trigger animation
    setTimeout(() => toast.classList.add('show'), 10);

    // Auto-hide
    if (duration > 0) {
        setTimeout(() => hideToast(toastId), duration);
    }

    return toastId;
}

function hideToast(toastId) {
    const toast = document.getElementById(toastId);
    if (toast) {
        toast.classList.remove('show');
        setTimeout(() => {
            if (toast.parentNode) {
                toast.parentNode.removeChild(toast);
            }
        }, 300);
    }
}

function showConfirmation(title, message, confirmCallback, confirmText = 'Confirm', danger = false) {
    const dialog = document.getElementById('confirmationDialog');
    const titleEl = document.getElementById('confirmationTitle');
    const messageEl = document.getElementById('confirmationMessage');
    const confirmBtn = document.getElementById('confirmationConfirm');

    titleEl.textContent = title;
    messageEl.textContent = message;
    confirmBtn.textContent = confirmText;
    confirmBtn.className = `btn ${danger ? 'btn-danger' : 'btn-primary'}`;
    confirmBtn.onclick = () => {
        confirmCallback();
        closeConfirmation();
    };

    dialog.classList.add('show');
}

function closeConfirmation() {
    document.getElementById('confirmationDialog').classList.remove('show');
}

function setButtonLoading(buttonId, loading) {
    const button = document.getElementById(buttonId);
    if (button) {
        if (loading) {
            button.classList.add('loading');
            button.disabled = true;
        } else {
            button.classList.remove('loading');
            button.disabled = false;
        }
    }
}

function showProgress(progressId, show) {
    const progress = document.getElementById(progressId);
    if (progress) {
        if (show) {
            progress.classList.add('show');
            const fill = progress.querySelector('.progress-fill');
            fill.style.width = '0%';
            setTimeout(() => fill.style.width = '100%', 10);
        } else {
            progress.classList.remove('show');
        }
    }
}

function validateField(fieldId, validator, errorMessage) {
    const field = document.getElementById(fieldId);
    const group = field.closest('.form-group');
    const validationEl = document.getElementById(fieldId + '-validation');

    if (validator(field.value)) {
        group.classList.remove('error');
        group.classList.add('success');
        if (validationEl) validationEl.textContent = '';
        return true;
    } else {
        group.classList.remove('success');
        group.classList.add('error');
        if (validationEl) validationEl.textContent = errorMessage;
        return false;
    }
}

function clearValidation(fieldId) {
    const field = document.getElementById(fieldId);
    const group = field.closest('.form-group');
    const validationEl = document.getElementById(fieldId + '-validation');

    group.classList.remove('error', 'success');
    if (validationEl) validationEl.textContent = '';
}

function toggleAutoRefresh() {
    autoRefreshEnabled = document.getElementById('autoRefresh').checked;
    if (autoRefreshEnabled) {
        startAutoRefresh();
        showToast('success', 'Auto-refresh Enabled', 'Status will update automatically every 5 seconds');
    } else {
        stopAutoRefresh();
        showToast('warning', 'Auto-refresh Disabled', 'Status will not update automatically');
    }
}

function startAutoRefresh() {
    stopAutoRefresh(); // Clear any existing interval

    // Use different refresh intervals based on page visibility
    // Faster refresh when page is visible (5s), slower when hidden (30s)
    const refreshInterval = pageVisible ? 5000 : 30000;

    autoRefreshInterval = setInterval(() => {
        if (autoRefreshEnabled && pageVisible) {
            loadStatusOptimized();
        }
    }, refreshInterval);
}

function stopAutoRefresh() {
    if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
    }
}

function toggleKeyboardShortcuts() {
    const shortcuts = document.getElementById('keyboardShortcuts');
    shortcuts.classList.toggle('show');
}

function filterNetworks() {
    const searchTerm = document.getElementById('networkSearch').value.toLowerCase();
    const networks = document.querySelectorAll('.network-item');

    networks.forEach(network => {
        const ssid = network.textContent.toLowerCase();
        if (ssid.includes(searchTerm)) {
            network.style.display = 'flex';
        } else {
            network.style.display = 'none';
        }
    });
}

// Keyboard shortcuts
document.addEventListener('keydown', function(e) {
    // Ignore if user is typing in an input field
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.tagName === 'SELECT') {
        return;
    }

    switch(e.key.toLowerCase()) {
        case '1':
            e.preventDefault();
            switchTab('lora');
            break;
        case '2':
            e.preventDefault();
            switchTab('wifi');
            break;
        case '3':
            e.preventDefault();
            switchTab('gnss');
            break;
        case '4':
            e.preventDefault();
            switchTab('system');
            break;
        case 'r':
            e.preventDefault();
            refreshCurrentTab();
            break;
        case 'a':
            e.preventDefault();
            const autoRefresh = document.getElementById('autoRefresh');
            autoRefresh.checked = !autoRefresh.checked;
            toggleAutoRefresh();
            break;
        case '?':
            e.preventDefault();
            toggleKeyboardShortcuts();
            break;
    }
});

function refreshCurrentTab() {
    switch(currentTab) {
        case 'lora':
            loadLoRaConfig();
            break;
        case 'wifi':
            loadWiFiConfig();
            break;
        case 'gnss':
            loadGNSSConfig();
            loadGNSSStatus();
            break;
        case 'system':
            loadSystemInfo();
            break;
    }
    showToast('success', 'Refreshed', `Reloaded ${currentTab} configuration`);
}

// Load status
async function loadStatus() {
    try {
        const data = await apiGet('status');

        // Update WiFi status - handle AP+STA mode
        if (data.wifi.connected && data.wifi.ap_active) {
            document.getElementById('wifiStatus').textContent = 'AP + Station';
            document.getElementById('wifiIcon').className = 'status-icon online';
            document.getElementById('ipAddress').textContent = 'AP: ' + data.wifi.ap_ip + ' | STA: ' + data.wifi.sta_ip;
        } else if (data.wifi.connected) {
            document.getElementById('wifiStatus').textContent = 'Connected';
            document.getElementById('wifiIcon').className = 'status-icon online';
            document.getElementById('ipAddress').textContent = data.wifi.sta_ip;
        } else if (data.wifi.ap_active) {
            document.getElementById('wifiStatus').textContent = 'AP Mode';
            document.getElementById('wifiIcon').className = 'status-icon online';
            document.getElementById('ipAddress').textContent = data.wifi.ap_ip;
        } else {
            document.getElementById('wifiStatus').textContent = 'Offline';
            document.getElementById('wifiIcon').className = 'status-icon offline';
            document.getElementById('ipAddress').textContent = '-';
        }

        // Update battery
        document.getElementById('battery').textContent = data.battery.voltage.toFixed(2) + ' V';

        // Update uptime
        const uptime = data.system.uptime;
        const hours = Math.floor(uptime / 3600);
        const minutes = Math.floor((uptime % 3600) / 60);
        document.getElementById('uptime').textContent = hours + 'h ' + minutes + 'm';

    } catch (error) {
        console.error('Failed to load status:', error);
    }
}

async function loadStatusOptimized() {
    try {
        const data = await apiGet('status');

        // Check if status has changed before updating UI
        if (hasStatusChanged(data)) {
            lastStatusData = data;

            // Update WiFi status - handle AP+STA mode
            if (data.wifi.connected && data.wifi.ap_active) {
                document.getElementById('wifiStatus').textContent = 'AP + Station';
                document.getElementById('wifiIcon').className = 'status-icon online';
                document.getElementById('ipAddress').textContent = 'AP: ' + data.wifi.ap_ip + ' | STA: ' + data.wifi.sta_ip;
            } else if (data.wifi.connected) {
                document.getElementById('wifiStatus').textContent = 'Connected';
                document.getElementById('wifiIcon').className = 'status-icon online';
                document.getElementById('ipAddress').textContent = data.wifi.sta_ip;
            } else if (data.wifi.ap_active) {
                document.getElementById('wifiStatus').textContent = 'AP Mode';
                document.getElementById('wifiIcon').className = 'status-icon online';
                document.getElementById('ipAddress').textContent = data.wifi.ap_ip;
            } else {
                document.getElementById('wifiStatus').textContent = 'Offline';
                document.getElementById('wifiIcon').className = 'status-icon offline';
                document.getElementById('ipAddress').textContent = '-';
            }

            // Update battery
            document.getElementById('battery').textContent = data.battery.voltage.toFixed(2) + ' V';

            // Update uptime
            const uptime = data.system.uptime;
            const hours = Math.floor(uptime / 3600);
            const minutes = Math.floor((uptime % 3600) / 60);
            document.getElementById('uptime').textContent = hours + 'h ' + minutes + 'm';
        }
    } catch (error) {
        console.error('Status update failed:', error);
    }
}

function hasStatusChanged(newData) {
    if (!lastStatusData) return true;

    // Check if WiFi status changed
    if (newData.wifi.connected !== lastStatusData.wifi.connected ||
        newData.wifi.ap_active !== lastStatusData.wifi.ap_active ||
        newData.wifi.ap_ip !== lastStatusData.wifi.ap_ip ||
        newData.wifi.sta_ip !== lastStatusData.wifi.sta_ip) {
        return true;
    }

    // Check if battery voltage changed significantly (>0.1V)
    if (Math.abs(newData.battery.voltage - lastStatusData.battery.voltage) > 0.1) {
        return true;
    }

    // Check if uptime changed (should increment)
    if (newData.system.uptime !== lastStatusData.system.uptime) {
        return true;
    }

    return false;
}

// Board Information and Dynamic Limits
async function loadBoardInfo() {
    try {
        const data = await apiGet('system');
        const boardType = data.board.type;

        // Set power limits based on board type
        const powerInput = document.getElementById('power');
        const powerRange = document.getElementById('power-range');

        if (boardType === 4) { // BOARD_V4
            powerInput.max = 28;
            powerRange.textContent = 'Valid range: -9 to 28 dBm (V4 with PA gain control)';
        } else {
            powerInput.max = 22;
            powerRange.textContent = 'Valid range: -9 to 22 dBm';
        }
    } catch (error) {
        console.error('Failed to load board info:', error);
    }
}

// LoRa Configuration
async function loadLoRaConfig() {
    try {
        const data = await apiGet('lora/config');
        document.getElementById('frequency').value = data.frequency;
        document.getElementById('bandwidth').value = data.bandwidth;
        document.getElementById('spreading').value = data.spreading;
        document.getElementById('codingRate').value = data.codingRate;
        document.getElementById('power').value = data.power;
        document.getElementById('syncWord').value = '0x' + data.syncWord.toString(16).toUpperCase();
    } catch (error) {
        showAlert('error', 'Failed to load LoRa configuration');
    }
}

async function applyLoRaConfig() {
    // Validate inputs
    const frequencyValid = validateField('frequency', (val) => {
        const num = parseFloat(val);
        return num >= 150 && num <= 960;
    }, 'Frequency must be between 150-960 MHz');

    const powerValid = validateField('power', (val) => {
        const num = parseInt(val);
        const maxPower = document.getElementById('power').max;
        return num >= -9 && num <= parseInt(maxPower);
    }, `Power must be between -9 to ${document.getElementById('power').max} dBm`);

    const syncWordValid = validateField('syncWord', (val) => {
        return /^0x[0-9A-Fa-f]{4}$/.test(val);
    }, 'Sync word must be in format 0xXXXX (4 hex digits)');

    if (!frequencyValid || !powerValid || !syncWordValid) {
        showToast('error', 'Validation Error', 'Please correct the highlighted fields');
        return;
    }

    setButtonLoading('applyLoRaBtn', true);
    showProgress('loraProgress', true);

    try {
        const syncWord = parseInt(document.getElementById('syncWord').value, 16);

        const config = {
            frequency: parseFloat(document.getElementById('frequency').value),
            bandwidth: parseFloat(document.getElementById('bandwidth').value),
            spreading: parseInt(document.getElementById('spreading').value),
            codingRate: parseInt(document.getElementById('codingRate').value),
            power: parseInt(document.getElementById('power').value),
            syncWord: syncWord
        };

        await apiPost('lora/config', config);
        showAlert('success', 'LoRa configuration applied successfully');
    } catch (error) {
        showAlert('error', 'Failed to apply LoRa configuration');
    } finally {
        setButtonLoading('applyLoRaBtn', false);
        setTimeout(() => showProgress('loraProgress', false), 1000);
    }
}

async function saveLoRaConfig() {
    setButtonLoading('saveLoRaBtn', true);
    showProgress('loraProgress', true);

    try {
        await apiPost('lora/save');
        showAlert('success', 'LoRa configuration saved to flash');
    } catch (error) {
        showAlert('error', 'Failed to save LoRa configuration');
    } finally {
        setButtonLoading('saveLoRaBtn', false);
        setTimeout(() => showProgress('loraProgress', false), 1000);
    }
}

async function resetLoRaConfig() {
    showConfirmation(
        'Reset LoRa Configuration',
        'Are you sure you want to reset LoRa configuration to defaults? This will restore factory settings.',
        async () => {
            setButtonLoading('resetLoRaBtn', true);
            showProgress('loraProgress', true);

            try {
                await apiPost('lora/reset');
                showAlert('success', 'LoRa configuration reset to defaults');
                await loadLoRaConfig();
                // Clear validation states
                clearValidation('frequency');
                clearValidation('power');
                clearValidation('syncWord');
            } catch (error) {
                showAlert('error', 'Failed to reset LoRa configuration');
            } finally {
                setButtonLoading('resetLoRaBtn', false);
                setTimeout(() => showProgress('loraProgress', false), 1000);
            }
        },
        'Reset',
        true
    );
}

// WiFi Configuration
async function loadWiFiConfig() {
    try {
        const data = await apiGet('wifi/config');
        document.getElementById('wifiMode').value = data.mode;
        document.getElementById('apSsid').value = data.ap_ssid;
        document.getElementById('staSsid').value = data.ssid;
        document.getElementById('dhcp').checked = data.dhcp;
        document.getElementById('staticIp').value = data.ip;
        document.getElementById('gateway').value = data.gateway;
        document.getElementById('subnet').value = data.subnet;
        document.getElementById('dns').value = data.dns;
        document.getElementById('tcpKissEnabled').checked = data.tcp_kiss_enabled;
        document.getElementById('tcpKissPort').value = data.tcp_kiss_port;

        updateWiFiModeUI();
        updateDHCPUI();
    } catch (error) {
        showAlert('error', 'Failed to load WiFi configuration', 'wifi');
    }
}

function updateWiFiModeUI() {
    const mode = parseInt(document.getElementById('wifiMode').value);
    const apConfig = document.getElementById('ap-config');
    const staConfig = document.getElementById('sta-config');

    apConfig.style.display = (mode === 1 || mode === 3) ? 'block' : 'none';
    staConfig.style.display = (mode === 2 || mode === 3) ? 'block' : 'none';
}

function updateDHCPUI() {
    const dhcp = document.getElementById('dhcp').checked;
    document.getElementById('static-ip-config').style.display = dhcp ? 'none' : 'block';
}

async function applyWiFiConfig() {
    setButtonLoading('applyWiFiBtn', true);
    showProgress('wifiProgress', true);

    try {
        const config = {
            mode: parseInt(document.getElementById('wifiMode').value),
            ap_ssid: document.getElementById('apSsid').value,
            ap_password: document.getElementById('apPassword').value,
            ssid: document.getElementById('staSsid').value,
            password: document.getElementById('staPassword').value,
            dhcp: document.getElementById('dhcp').checked,
            tcp_kiss_enabled: document.getElementById('tcpKissEnabled').checked,
            tcp_kiss_port: parseInt(document.getElementById('tcpKissPort').value)
        };

        const currentMode = parseInt(document.getElementById('wifiMode').value);

        await apiPost('wifi/config', config);

        // If changing WiFi mode, warn user about connection loss
        if (currentMode === 0 || currentMode === 2) {
            showAlert('success', 'WiFi configuration applied. Note: Connection may be lost if changing modes. You may need to reconnect to the new network.', 'wifi');
        } else {
            showAlert('success', 'WiFi configuration applied successfully', 'wifi');
        }
    } catch (error) {
        showAlert('error', 'Failed to apply WiFi configuration', 'wifi');
    } finally {
        setButtonLoading('applyWiFiBtn', false);
        setTimeout(() => showProgress('wifiProgress', false), 1000);
    }
}

async function saveWiFiConfig() {
    try {
        await apiPost('wifi/save');
        showAlert('success', 'WiFi configuration saved to flash', 'wifi');
    } catch (error) {
        showAlert('error', 'Failed to save WiFi configuration', 'wifi');
    }
}

async function scanWiFi() {
    try {
        // Start the scan
        const startResponse = await apiGet('wifi/scan');
        if (startResponse.status !== 'started') {
            throw new Error('Failed to start scan');
        }

        document.getElementById('networkListContent').innerHTML = '<div class="loading">Starting WiFi scan...</div>';
        document.getElementById('networkList').style.display = 'block';
        document.getElementById('networkSearch').value = ''; // Clear search

        // Poll for scan completion
        const pollScanStatus = async () => {
            try {
                const statusResponse = await apiGet('wifi/scan/status');

                if (statusResponse.status === 'completed') {
                    // Scan completed, display results
                    displayScanResults(statusResponse.networks);
                } else if (statusResponse.status === 'scanning') {
                    // Still scanning, update progress and continue polling
                    document.getElementById('networkListContent').innerHTML =
                        `<div class="loading">Scanning networks... ${statusResponse.progress}%</div>`;
                    setTimeout(pollScanStatus, 500); // Check again in 500ms
                } else if (statusResponse.status === 'idle') {
                    // No scan in progress
                    throw new Error('Scan was cancelled or failed to start');
                } else {
                    throw new Error('Scan failed');
                }
            } catch (error) {
                showAlert('error', 'Failed to check scan status', 'wifi');
            }
        };

        // Start polling
        setTimeout(pollScanStatus, 500);

    } catch (error) {
        showAlert('error', 'Failed to start WiFi scan', 'wifi');
    }
}

function displayScanResults(networks) {
    const listContent = document.getElementById('networkListContent');
    listContent.innerHTML = '';

    if (networks && networks.length > 0) {
        networks.forEach(network => {
            const item = document.createElement('div');
            item.className = 'network-item';
            item.onclick = () => {
                document.getElementById('staSsid').value = network.ssid;
                showToast('success', 'Network Selected', `Selected: ${network.ssid}`);
            };

            let signalIcon = '📶';
            if (network.rssi > -50) signalIcon = '📶📶📶';
            else if (network.rssi > -70) signalIcon = '📶📶';

            item.innerHTML = `
                <div class="network-info">
                    <div class="signal-strength">${signalIcon}</div>
                    <div class="network-details">
                        <strong>${network.ssid}</strong><br>
                        <small>${network.rssi} dBm ${network.encrypted ? '🔒' : '🔓'}</small>
                    </div>
                </div>
            `;

            listContent.appendChild(item);
        });
        showToast('success', 'Scan Complete', `Found ${networks.length} networks`);
    } else {
        listContent.innerHTML = '<div class="loading">No networks found</div>';
        showToast('warning', 'No Networks', 'No WiFi networks were found');
    }
}

// GNSS Configuration
async function loadGNSSConfig() {
    try {
        const data = await apiGet('gnss/config');
        document.getElementById('gnssEnabled').checked = data.enabled;
        document.getElementById('gnssSerialPassthrough').checked = data.serialPassthrough || false;
        document.getElementById('gnssTcpPort').value = data.tcpPort;
        document.getElementById('gnssBaudRate').value = data.baudRate;

        // Show/hide pin configuration based on board type
        if (data.hasBuiltInPort) {
            document.getElementById('gnss-pin-config').style.display = 'none';
            document.getElementById('gnss-unavailable').style.display = 'none';
        } else {
            document.getElementById('gnss-pin-config').style.display = 'block';
            document.getElementById('gnss-unavailable').style.display = 'block';

            // Load pin configuration
            document.getElementById('gnssPinRX').value = data.pinRX;
            document.getElementById('gnssPinTX').value = data.pinTX;
            document.getElementById('gnssPinCtrl').value = data.pinCtrl;
            document.getElementById('gnssPinWake').value = data.pinWake;
            document.getElementById('gnssPinPPS').value = data.pinPPS;
            document.getElementById('gnssPinRST').value = data.pinRST;
        }
    } catch (error) {
        showAlert('error', 'Failed to load GNSS configuration', 'gnss');
    }
}

async function loadGNSSStatus() {
    try {
        const data = await apiGet('gnss/status');

        if (data.running) {
            document.getElementById('gnss-fix-status').textContent = data.hasFix ? '✅ Fixed' : '⏳ Searching';
            document.getElementById('gnss-satellites').textContent = data.satellites || 0;
            document.getElementById('gnss-hdop').textContent = data.hdop ? data.hdop.toFixed(1) : '-';

            if (data.hasFix) {
                document.getElementById('gnss-lat').textContent = data.latitude ? data.latitude.toFixed(6) + '°' : '-';
                document.getElementById('gnss-lon').textContent = data.longitude ? data.longitude.toFixed(6) + '°' : '-';
                document.getElementById('gnss-alt').textContent = data.altitude ? data.altitude.toFixed(1) + ' m' : '-';
            } else {
                document.getElementById('gnss-lat').textContent = '-';
                document.getElementById('gnss-lon').textContent = '-';
                document.getElementById('gnss-alt').textContent = '-';
            }
        } else {
            document.getElementById('gnss-fix-status').textContent = '⭕ Disabled';
            document.getElementById('gnss-satellites').textContent = '-';
            document.getElementById('gnss-hdop').textContent = '-';
            document.getElementById('gnss-lat').textContent = '-';
            document.getElementById('gnss-lon').textContent = '-';
            document.getElementById('gnss-alt').textContent = '-';
        }

        if (data.nmeaServer) {
            document.getElementById('gnss-nmea-status').textContent = data.nmeaServer.running ? '✅ Running' : '⭕ Stopped';
            document.getElementById('gnss-nmea-port').textContent = data.nmeaServer.port;
            document.getElementById('gnss-nmea-clients').textContent = data.nmeaServer.clients;
        } else {
            document.getElementById('gnss-nmea-status').textContent = '-';
            document.getElementById('gnss-nmea-port').textContent = '-';
            document.getElementById('gnss-nmea-clients').textContent = '-';
        }
    } catch (error) {
        console.error('Failed to load GNSS status:', error);
    }
}

async function applyGNSSConfig() {
    try {
        const config = {
            enabled: document.getElementById('gnssEnabled').checked,
            serialPassthrough: document.getElementById('gnssSerialPassthrough').checked,
            tcpPort: parseInt(document.getElementById('gnssTcpPort').value),
            baudRate: parseInt(document.getElementById('gnssBaudRate').value)
        };

        // Add pin config if visible (V3 boards)
        if (document.getElementById('gnss-pin-config').style.display !== 'none') {
            config.pinRX = parseInt(document.getElementById('gnssPinRX').value);
            config.pinTX = parseInt(document.getElementById('gnssPinTX').value);
            config.pinCtrl = parseInt(document.getElementById('gnssPinCtrl').value);
            config.pinWake = parseInt(document.getElementById('gnssPinWake').value);
            config.pinPPS = parseInt(document.getElementById('gnssPinPPS').value);
            config.pinRST = parseInt(document.getElementById('gnssPinRST').value);
        }

        const result = await apiPost('gnss/config', config);

        if (result.rebootRequired) {
            showAlert('success', 'GNSS configuration applied. Reboot required for TCP port or baud rate changes.', 'gnss');
        } else {
            showAlert('success', 'GNSS configuration applied successfully', 'gnss');
        }

        // Reload status after a moment
        setTimeout(loadGNSSStatus, 1000);
    } catch (error) {
        showAlert('error', 'Failed to apply GNSS configuration', 'gnss');
    }
}

async function saveGNSSConfig() {
    // Apply first, which saves to NVS
    await applyGNSSConfig();
}

// System Information
async function loadSystemInfo() {
    try {
        const sysData = await apiGet('system');
        const loraData = await apiGet('lora/config');

        // Board info
        document.getElementById('sys-board-name').textContent = sysData.board.name;
        document.getElementById('sys-board-type').textContent = 'V' + sysData.board.type;

        // Chip info
        document.getElementById('sys-chip-model').textContent = sysData.chip.model;
        document.getElementById('sys-chip-revision').textContent = sysData.chip.revision;
        document.getElementById('sys-chip-cores').textContent = sysData.chip.cores;
        document.getElementById('sys-chip-freq').textContent = sysData.chip.frequency + ' MHz';

        // Memory info
        document.getElementById('sys-flash-size').textContent = (sysData.memory.flash_size / 1024 / 1024).toFixed(0) + ' MB';
        document.getElementById('sys-free-heap').textContent = (sysData.memory.free_heap / 1024).toFixed(1) + ' KB';
        document.getElementById('sys-heap-size').textContent = (sysData.memory.heap_size / 1024).toFixed(1) + ' KB';

        // LoRa config
        document.getElementById('sys-lora-freq').textContent = loraData.frequency + ' MHz';
        document.getElementById('sys-lora-bw').textContent = loraData.bandwidth + ' kHz';
        document.getElementById('sys-lora-sf').textContent = 'SF' + loraData.spreading;
        document.getElementById('sys-lora-pwr').textContent = loraData.power + ' dBm';

    } catch (error) {
        console.error('Failed to load system info:', error);
    }
}

async function rebootDevice() {
    showConfirmation(
        'Reboot Device',
        'Are you sure you want to reboot the device? This will temporarily disconnect all connections.',
        async () => {
            setButtonLoading('rebootBtn', true);
            showProgress('systemProgress', true);

            try {
                await apiPost('reboot');
                showToast('warning', 'Rebooting', 'Device is rebooting. Please wait 10 seconds and refresh the page.', 0);
            } catch (error) {
                showAlert('error', 'Failed to reboot device');
                setButtonLoading('rebootBtn', false);
                showProgress('systemProgress', false);
            }
        },
        'Reboot',
        true
    );
}

// Initialize
async function init() {
    // Show loading screen
    const loadingOverlay = document.getElementById('loadingOverlay');
    loadingOverlay.classList.remove('hidden');

    // Load initial data in parallel for faster loading
    const [statusResult, sysData, loraConfigResult] = await Promise.all([
        loadStatus(),
        apiGet('system'),
        loadLoRaConfig()
    ]);

    // Update board name from system data
    document.getElementById('boardName').textContent = sysData.board.name;

    // Start auto-refresh
    startAutoRefresh();

    // Update status every 5 seconds
    setInterval(() => {
        if (autoRefreshEnabled) {
            loadStatus();
        }
    }, 5000);

    // Update GNSS status every 3 seconds if on GNSS tab
    setInterval(() => {
        if (currentTab === 'gnss') {
            loadGNSSStatus();
        }
    }, 3000);

    // Show welcome message and hide loading screen
    setTimeout(() => {
        loadingOverlay.classList.add('hidden');
        showToast('success', 'Welcome', 'LoRaTNCX Control Center loaded successfully');
    }, 1000);
}

// API Helper Functions
async function apiGet(endpoint) {
    const response = await fetch(`/api/${endpoint}`);
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    return await response.json();
}

async function apiPost(endpoint, data = null) {
    const config = {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    };

    if (data !== null) {
        config.body = JSON.stringify(data);
    }

    const response = await fetch(`/api/${endpoint}`, config);
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    // Some endpoints return JSON, others just success messages
    const contentType = response.headers.get('content-type');
    if (contentType && contentType.includes('application/json')) {
        return await response.json();
    } else {
        return { success: true };
    }
}

// Start when page loads
window.addEventListener('load', init);