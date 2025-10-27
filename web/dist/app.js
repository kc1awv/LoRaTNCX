// LoRaTNCX Web Interface JavaScript
class LoRaTNCXInterface {
    constructor() {
        this.websocket = null;
        this.reconnectInterval = null;
        this.charts = {};
        this.isConnected = false;
        this.retryCount = 0;
        this.maxRetries = 10;
        
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.setupCharts();
        this.connectWebSocket();
        this.startPeriodicUpdates();
    }

    // WebSocket Connection Management
    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;
        
        try {
            this.websocket = new WebSocket(wsUrl);
            
            this.websocket.onopen = () => {
                console.log('WebSocket connected');
                this.onWebSocketOpen();
            };
            
            this.websocket.onmessage = (event) => {
                this.handleWebSocketMessage(event);
            };
            
            this.websocket.onclose = () => {
                console.log('WebSocket disconnected');
                this.onWebSocketClose();
            };
            
            this.websocket.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.onWebSocketError();
            };
        } catch (error) {
            console.error('Failed to create WebSocket:', error);
            this.scheduleReconnect();
        }
    }

    onWebSocketOpen() {
        this.isConnected = true;
        this.retryCount = 0;
        this.updateConnectionStatus(true);
        
        // Request initial data
        this.sendWebSocketMessage({
            type: 'request',
            data: 'all_status'
        });
    }

    onWebSocketClose() {
        this.isConnected = false;
        this.updateConnectionStatus(false);
        this.scheduleReconnect();
    }

    onWebSocketError() {
        this.isConnected = false;
        this.updateConnectionStatus(false);
    }

    scheduleReconnect() {
        if (this.retryCount < this.maxRetries) {
            const delay = Math.min(1000 * Math.pow(2, this.retryCount), 30000);
            console.log(`Reconnecting in ${delay}ms (attempt ${this.retryCount + 1})`);
            
            setTimeout(() => {
                this.retryCount++;
                this.connectWebSocket();
            }, delay);
        } else {
            console.error('Max reconnection attempts reached');
        }
    }

    sendWebSocketMessage(message) {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
            this.websocket.send(JSON.stringify(message));
        } else {
            console.warn('WebSocket not connected, message not sent:', message);
        }
    }

    handleWebSocketMessage(event) {
        try {
            const data = JSON.parse(event.data);
            this.processIncomingData(data);
        } catch (error) {
            console.error('Error parsing WebSocket message:', error);
        }
    }

    processIncomingData(data) {
        switch (data.type) {
            case 'status':
                this.updateSystemStatus(data.payload);
                break;
            case 'radio':
                this.updateRadioData(data.payload);
                break;
            case 'aprs':
                this.updateAPRSData(data.payload);
                break;
            case 'gnss':
                this.updateGNSSData(data.payload);
                break;
            case 'battery':
                this.updateBatteryData(data.payload);
                break;
            case 'wifi':
                this.updateWiFiData(data.payload);
                break;
            case 'log':
                this.addLogEntry(data.payload);
                break;
            case 'error':
                this.showError(data.payload.message);
                break;
            default:
                console.log('Unknown message type:', data.type);
        }
    }

    // UI Update Methods
    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connection-status');
        if (connected) {
            statusElement.textContent = 'Connected';
            statusElement.className = 'badge bg-success';
        } else {
            statusElement.textContent = 'Disconnected';
            statusElement.className = 'badge bg-danger';
        }
    }

    updateSystemStatus(data) {
        this.updateElement('firmware-version', data.firmware_version);
        this.updateElement('uptime', this.formatUptime(data.uptime));
        this.updateElement('free-heap', this.formatBytes(data.free_heap));
        this.updateElement('flash-usage', `${this.formatBytes(data.flash_used)} / ${this.formatBytes(data.flash_total)}`);
        this.updateElement('spiffs-usage', `${this.formatBytes(data.spiffs_used)} / ${this.formatBytes(data.spiffs_total)}`);
        this.updateElement('cpu-temp', `${data.cpu_temp}°C`);
    }

    updateRadioData(data) {
        this.updateElement('radio-frequency', `${data.frequency} MHz`);
        this.updateElement('radio-power', `${data.power} dBm`);
        this.updateElement('radio-rssi', `${data.rssi} dBm`);
        this.updateElement('current-rssi', `${data.rssi} dBm`);
        this.updateElement('current-snr', `${data.snr} dB`);
        this.updateElement('packets-tx', data.packets_tx);
        this.updateElement('packets-rx', data.packets_rx);
        this.updateElement('crc-errors', data.crc_errors);
        this.updateElement('last-tx', this.formatTimeAgo(data.last_tx));
        this.updateElement('last-rx', this.formatTimeAgo(data.last_rx));

        // Update charts
        this.updateRSSIChart(data.rssi);
        this.updatePacketChart(data.packets_tx, data.packets_rx);
    }

    updateAPRSData(data) {
        if (data.messages && Array.isArray(data.messages)) {
            this.updateAPRSMessages(data.messages);
        }
    }

    updateGNSSData(data) {
        this.updateElement('gnss-fix', data.fix ? 'GPS Fix' : 'No Fix');
        this.updateElement('gnss-sats', data.satellites);
        this.updateElement('gnss-location', 
            data.latitude && data.longitude ? 
            `${data.latitude.toFixed(6)}, ${data.longitude.toFixed(6)}` : 
            'N/A'
        );
    }

    updateBatteryData(data) {
        this.updateElement('battery-voltage', `${data.voltage} V`);
        this.updateElement('battery-level', `${data.level}%`);
        this.updateElement('battery-charging', data.charging ? 'Yes' : 'No');
    }

    updateWiFiData(data) {
        this.updateElement('wifi-status', data.connected ? 'Connected' : 'Disconnected');
        this.updateElement('wifi-ip', data.ip || 'N/A');
        this.updateElement('wifi-ssid', data.ssid || 'N/A');
    }

    updateAPRSMessages(messages) {
        const tbody = document.getElementById('aprs-messages');
        tbody.innerHTML = '';
        
        messages.slice(-10).forEach(msg => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${this.formatTime(msg.timestamp)}</td>
                <td><strong>${msg.from}</strong></td>
                <td>${msg.to}</td>
                <td>${msg.message}</td>
                <td>${msg.rssi} dBm</td>
            `;
            tbody.appendChild(row);
        });
    }

    addLogEntry(logData) {
        const logContainer = document.getElementById('system-log');
        const timestamp = new Date().toLocaleTimeString();
        const logEntry = `[${timestamp}] ${logData.level}: ${logData.message}\n`;
        
        logContainer.textContent += logEntry;
        logContainer.scrollTop = logContainer.scrollHeight;
        
        // Keep only last 100 lines
        const lines = logContainer.textContent.split('\n');
        if (lines.length > 100) {
            logContainer.textContent = lines.slice(-100).join('\n');
        }
    }

    updateElement(id, value) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = value;
        }
    }

    // Chart Management
    setupCharts() {
        this.setupRSSIChart();
        this.setupPacketChart();
    }

    setupRSSIChart() {
        const ctx = document.getElementById('rssi-chart').getContext('2d');
        this.charts.rssi = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'RSSI (dBm)',
                    data: [],
                    borderColor: 'rgb(75, 192, 192)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: false,
                        min: -120,
                        max: -30
                    }
                },
                plugins: {
                    legend: {
                        display: true
                    }
                }
            }
        });
    }

    setupPacketChart() {
        const ctx = document.getElementById('packet-chart').getContext('2d');
        this.charts.packet = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: ['TX', 'RX', 'Errors'],
                datasets: [{
                    label: 'Packet Count',
                    data: [0, 0, 0],
                    backgroundColor: [
                        'rgba(54, 162, 235, 0.2)',
                        'rgba(75, 192, 192, 0.2)',
                        'rgba(255, 99, 132, 0.2)'
                    ],
                    borderColor: [
                        'rgba(54, 162, 235, 1)',
                        'rgba(75, 192, 192, 1)',
                        'rgba(255, 99, 132, 1)'
                    ],
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });
    }

    updateRSSIChart(rssi) {
        const chart = this.charts.rssi;
        const now = new Date().toLocaleTimeString();
        
        chart.data.labels.push(now);
        chart.data.datasets[0].data.push(rssi);
        
        // Keep only last 20 data points
        if (chart.data.labels.length > 20) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }
        
        chart.update('none');
    }

    updatePacketChart(tx, rx, errors = 0) {
        const chart = this.charts.packet;
        chart.data.datasets[0].data = [tx, rx, errors];
        chart.update('none');
    }

    // Event Listeners
    setupEventListeners() {
        // Tab navigation
        document.querySelectorAll('.nav-link[data-tab]').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                this.switchTab(e.target.dataset.tab);
            });
        });

        // Form submissions
        this.setupFormHandlers();

        // System actions
        this.setupSystemActions();

        // Real-time form updates
        this.setupFormUpdates();
    }

    setupFormHandlers() {
        // Radio configuration form
        document.getElementById('radio-config-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.submitRadioConfig();
        });

        // APRS configuration form
        document.getElementById('aprs-config-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.submitAPRSConfig();
        });

        // Send message form
        document.getElementById('send-message-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.sendAPRSMessage();
        });

        // WiFi configuration form
        document.getElementById('wifi-config-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.submitWiFiConfig();
        });

        // System configuration form
        document.getElementById('system-config-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.submitSystemConfig();
        });
    }

    setupSystemActions() {
        document.getElementById('restart-btn').addEventListener('click', () => {
            if (confirm('Are you sure you want to restart the device?')) {
                this.sendWebSocketMessage({
                    type: 'command',
                    action: 'restart'
                });
            }
        });

        document.getElementById('factory-reset-btn').addEventListener('click', () => {
            if (confirm('Are you sure you want to perform a factory reset? This will erase all configuration!')) {
                this.sendWebSocketMessage({
                    type: 'command',
                    action: 'factory_reset'
                });
            }
        });

        document.getElementById('backup-config-btn').addEventListener('click', () => {
            this.backupConfiguration();
        });

        document.getElementById('restore-config-btn').addEventListener('click', () => {
            document.getElementById('firmware-file').click();
        });

        document.getElementById('ota-update-btn').addEventListener('click', () => {
            this.startOTAUpdate();
        });
    }

    setupFormUpdates() {
        // Power slider real-time update
        const powerSlider = document.getElementById('power');
        const powerValue = document.getElementById('power-value');
        if (powerSlider && powerValue) {
            powerSlider.addEventListener('input', (e) => {
                powerValue.textContent = `${e.target.value} dBm`;
            });
        }

        // Message character counter
        const messageText = document.getElementById('message-text');
        const charCount = document.getElementById('char-count');
        if (messageText && charCount) {
            messageText.addEventListener('input', (e) => {
                charCount.textContent = e.target.value.length;
            });
        }
    }

    // Tab Management
    switchTab(tabName) {
        // Hide all tab contents
        document.querySelectorAll('.tab-content').forEach(tab => {
            tab.classList.remove('active');
        });

        // Show selected tab
        const selectedTab = document.getElementById(`${tabName}-tab`);
        if (selectedTab) {
            selectedTab.classList.add('active');
        }

        // Update navigation
        document.querySelectorAll('.nav-link').forEach(link => {
            link.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
    }

    // Configuration Methods
    submitRadioConfig() {
        const config = {
            frequency: parseFloat(document.getElementById('frequency').value),
            power: parseInt(document.getElementById('power').value),
            bandwidth: parseInt(document.getElementById('bandwidth').value),
            spreading_factor: parseInt(document.getElementById('spreading-factor').value),
            coding_rate: parseInt(document.getElementById('coding-rate').value)
        };

        this.sendWebSocketMessage({
            type: 'config',
            category: 'radio',
            data: config
        });

        this.showSuccess('Radio configuration updated');
    }

    submitAPRSConfig() {
        const config = {
            callsign: document.getElementById('callsign').value.toUpperCase(),
            ssid: parseInt(document.getElementById('ssid').value),
            beacon_interval: parseInt(document.getElementById('beacon-interval').value),
            beacon_text: document.getElementById('beacon-text').value,
            auto_beacon: document.getElementById('auto-beacon').checked
        };

        this.sendWebSocketMessage({
            type: 'config',
            category: 'aprs',
            data: config
        });

        this.showSuccess('APRS configuration saved');
    }

    sendAPRSMessage() {
        const message = {
            to: document.getElementById('msg-callsign').value.toUpperCase(),
            text: document.getElementById('message-text').value
        };

        this.sendWebSocketMessage({
            type: 'command',
            action: 'send_aprs_message',
            data: message
        });

        // Clear form
        document.getElementById('send-message-form').reset();
        document.getElementById('char-count').textContent = '0';

        this.showSuccess('APRS message sent');
    }

    submitWiFiConfig() {
        const config = {
            ssid: document.getElementById('wifi-ssid-input').value,
            password: document.getElementById('wifi-password').value,
            ap_mode: document.getElementById('wifi-ap-mode').checked
        };

        this.sendWebSocketMessage({
            type: 'config',
            category: 'wifi',
            data: config
        });

        this.showSuccess('WiFi configuration saved');
    }

    submitSystemConfig() {
        const config = {
            oled_enabled: document.getElementById('oled-enabled').checked,
            gnss_enabled: document.getElementById('gnss-enabled').checked,
            timezone: document.getElementById('timezone').value
        };

        this.sendWebSocketMessage({
            type: 'config',
            category: 'system',
            data: config
        });

        this.showSuccess('System configuration saved');
    }

    // Utility Methods
    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        return `${days}d ${hours}h ${minutes}m`;
    }

    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    formatTime(timestamp) {
        return new Date(timestamp).toLocaleTimeString();
    }

    formatTimeAgo(timestamp) {
        const now = Date.now();
        const diff = Math.floor((now - timestamp) / 1000);
        
        if (diff < 60) return `${diff} seconds ago`;
        if (diff < 3600) return `${Math.floor(diff / 60)} minutes ago`;
        if (diff < 86400) return `${Math.floor(diff / 3600)} hours ago`;
        return `${Math.floor(diff / 86400)} days ago`;
    }

    showSuccess(message) {
        this.showNotification(message, 'success');
    }

    showError(message) {
        this.showNotification(message, 'danger');
    }

    showNotification(message, type) {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `alert alert-${type} alert-dismissible fade show position-fixed`;
        notification.style.cssText = 'top: 20px; right: 20px; z-index: 9999; min-width: 300px;';
        notification.innerHTML = `
            ${message}
            <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
        `;

        document.body.appendChild(notification);

        // Auto-dismiss after 5 seconds
        setTimeout(() => {
            if (notification.parentNode) {
                notification.remove();
            }
        }, 5000);
    }

    backupConfiguration() {
        this.sendWebSocketMessage({
            type: 'command',
            action: 'backup_config'
        });
    }

    startOTAUpdate() {
        const fileInput = document.getElementById('firmware-file');
        if (fileInput.files.length === 0) {
            this.showError('Please select a firmware file');
            return;
        }

        // For now, just show a message - actual OTA implementation would be more complex
        this.showError('OTA update functionality not yet implemented');
    }

    startPeriodicUpdates() {
        // Request status updates every 5 seconds
        setInterval(() => {
            if (this.isConnected) {
                this.sendWebSocketMessage({
                    type: 'request',
                    data: 'status'
                });
            }
        }, 5000);
    }
}

// Initialize the application when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.loratncx = new LoRaTNCXInterface();
});