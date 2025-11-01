/**
 * LoRaTNCX Web Interface Main Application
 * Handles API communication, status updates, and user interactions
 */

class LoRaTNCXApp {
    constructor() {
        this.apiEndpoints = {
            systemStatus: '/api/system/status',
            loraStatus: '/api/lora/status',
            wifiNetworks: '/api/wifi/networks',
            addNetwork: '/api/wifi/add',
            removeNetwork: '/api/wifi/remove'
        };
        
        this.refreshInterval = 5000; // 5 seconds
        this.refreshTimer = null;
        this.isOnline = navigator.onLine;
        
        this.init();
    }

    init() {
        console.log('🚀 LoRaTNCX App initializing...');
        
        // Set up event listeners
        this.setupEventListeners();
        
        // Start status monitoring
        this.startStatusUpdates();
        
        // Load initial data
        this.loadAllData();
        
        // Set up network status monitoring
        window.addEventListener('online', () => {
            this.isOnline = true;
            this.showNotification('Connection restored', 'success');
            this.startStatusUpdates();
        });
        
        window.addEventListener('offline', () => {
            this.isOnline = false;
            this.showNotification('Connection lost', 'error');
            this.stopStatusUpdates();
        });

        console.log('✅ LoRaTNCX App initialized');
    }

    setupEventListeners() {
        // Add WiFi network form
        const addWiFiForm = document.getElementById('add-wifi-form');
        if (addWiFiForm) {
            addWiFiForm.addEventListener('submit', (e) => {
                e.preventDefault();
                this.handleAddNetwork();
            });
        }

        // Smooth scrolling for navigation links
        document.querySelectorAll('a[href^="#"]').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const target = document.querySelector(link.getAttribute('href'));
                if (target) {
                    target.scrollIntoView({ behavior: 'smooth' });
                }
            });
        });
    }

    // API Communication
    async makeApiRequest(endpoint, options = {}) {
        try {
            const response = await fetch(endpoint, {
                ...options,
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                }
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            return await response.json();
        } catch (error) {
            console.error('API request failed:', error);
            this.showNotification(`API Error: ${error.message}`, 'error');
            throw error;
        }
    }

    // Status Updates
    async loadDeviceStatus() {
        try {
            const [systemData, loraData] = await Promise.all([
                this.makeApiRequest(this.apiEndpoints.systemStatus),
                this.makeApiRequest(this.apiEndpoints.loraStatus)
            ]);
            this.updateStatusDisplay(systemData, loraData);
        } catch (error) {
            console.error('Failed to load device status:', error);
            this.updateStatusDisplay(null, null, true);
        }
    }

    updateStatusDisplay(systemData, loraData, isError = false) {
        if (isError || !systemData) {
            this.updateConnectionStatus('offline', 'Connection Error');
            return;
        }

        // Parse system status data (it's a text response)
        const status = {
            uptime: systemData.uptime / 1000, // Convert ms to seconds
            freeHeap: systemData.free_heap,
            cpuFreq: systemData.cpu_freq,
            chipModel: systemData.chip_model
        };

        // Update connection status - we'll assume connected if we got data
        this.updateConnectionStatus('online', 'Connected');

        // Update WiFi details (placeholder - will be updated with proper WiFi status)
        this.updateElement('wifi-mode', 'Connected');
        this.updateElement('wifi-ssid', 'LoRaTNCX');
        this.updateElement('wifi-ip', '192.168.1.100');
        this.updateElement('wifi-rssi', '-');

        // Update system info
        this.updateElement('system-uptime', this.formatUptime(status.uptime));
        this.updateElement('system-memory', this.formatMemory(status.freeHeap));
        this.updateElement('system-cpu', status.cpuFreq ? `${status.cpuFreq} MHz` : '-');
        this.updateElement('system-chip', status.chipModel || '-');

        // Update LoRa status
        if (loraData && loraData.status === 'ok') {
            this.updateElement('lora-status', 'Active');
            // Parse LoRa data from text response
            const loraText = loraData.data || '';
            this.updateElement('lora-frequency', this.extractValue(loraText, 'Frequency') || '-');
            this.updateElement('lora-bandwidth', this.extractValue(loraText, 'Bandwidth') || '-');
            this.updateElement('lora-coding-rate', this.extractValue(loraText, 'Coding Rate') || '-');
            this.updateElement('lora-spreading-factor', this.extractValue(loraText, 'Spreading Factor') || '-');
        } else {
            this.updateElement('lora-status', 'Inactive');
        }

        // Update last updated timestamp
        this.updateElement('last-updated', new Date().toLocaleTimeString());
    }

    updateConnectionStatus(state, text) {
        const statusBadge = document.getElementById('connection-status');
        if (statusBadge) {
            statusBadge.className = `status-badge ${state}`;
            statusBadge.textContent = text;
        }
    }

    updateCurrentWiFiInfo(status) {
        const container = document.getElementById('current-wifi-info');
        if (!container) return;

        let html = '';
        if (status.stationConnected) {
            html = `
                <div class="network-card connected">
                    <div class="network-info">
                        <h4>📶 ${status.stationSSID}</h4>
                        <p>IP: ${status.stationIP} | Signal: ${this.formatRSSI(status.rssi)}</p>
                    </div>
                    <div class="signal-strength">
                        ${this.renderSignalBars(status.rssi)}
                    </div>
                </div>
            `;
        } else if (status.apActive) {
            html = `
                <div class="network-card ap-mode">
                    <div class="network-info">
                        <h4>📡 ${status.apSSID} (Access Point)</h4>
                        <p>IP: ${status.apIP} | Password: ${status.apPassword}</p>
                    </div>
                    <div class="network-actions">
                        <small>Connect devices to this network</small>
                    </div>
                </div>
            `;
        } else {
            html = `
                <div class="network-card">
                    <div class="network-info">
                        <h4>🔍 Scanning for networks...</h4>
                        <p>Device is attempting to connect</p>
                    </div>
                </div>
            `;
        }

        container.innerHTML = html;
    }

    // WiFi Network Management
    async loadNetworks() {
        try {
            const data = await this.makeApiRequest(this.apiEndpoints.wifiNetworks);
            // Parse the networks data from text response
            this.updateNetworksDisplay(this.parseNetworksText(data.data || ''));
        } catch (error) {
            console.error('Failed to load networks:', error);
        }
    }

    updateNetworksDisplay(networks) {
        const container = document.getElementById('saved-networks');
        if (!container) return;

        if (networks.length === 0) {
            container.innerHTML = '<p><em>No saved networks</em></p>';
            return;
        }

        const html = networks.map(network => `
            <div class="network-card ${network.connected ? 'connected' : ''}">
                <div class="network-info">
                    <h4>${network.connected ? '🟢' : '⚪'} ${network.ssid}</h4>
                    <p>${network.connected ? 'Currently connected' : 'Saved network'}</p>
                </div>
                <div class="network-actions">
                    <button class="secondary outline" onclick="app.removeNetwork('${network.ssid}')" 
                            ${network.connected ? 'disabled' : ''}>
                        Remove
                    </button>
                </div>
            </div>
        `).join('');

        container.innerHTML = html;
    }

    async handleAddNetwork() {
        const ssidInput = document.getElementById('wifi-ssid-input');
        const passwordInput = document.getElementById('wifi-password-input');
        const form = document.getElementById('add-wifi-form');

        if (!ssidInput || !passwordInput) return;

        const ssid = ssidInput.value.trim();
        const password = passwordInput.value;

        if (!ssid || !password) {
            this.showNotification('Please enter both SSID and password', 'error');
            return;
        }

        try {
            this.showLoading(true);
            
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);

            const result = await this.makeApiRequest(this.apiEndpoints.addNetwork, {
                method: 'POST',
                body: formData
            });

            if (result.status === 'ok') {
                this.showNotification(result.message || 'Network added successfully', 'success');
                form.reset();
                await this.loadNetworks();
                // Refresh status to show connection attempt
                setTimeout(() => this.loadDeviceStatus(), 1000);
            } else {
                this.showNotification(result.message || 'Failed to add network', 'error');
            }
        } catch (error) {
            this.showNotification('Failed to add network', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async removeNetwork(ssid) {
        if (!confirm(`Remove network "${ssid}"?`)) return;

        try {
            this.showLoading(true);
            
            const formData = new FormData();
            formData.append('ssid', ssid);

            const result = await this.makeApiRequest(this.apiEndpoints.removeNetwork, {
                method: 'POST',
                body: formData
            });

            if (result.status === 'ok') {
                this.showNotification(result.message || 'Network removed successfully', 'success');
                await this.loadNetworks();
                // Refresh status
                setTimeout(() => this.loadDeviceStatus(), 1000);
            } else {
                this.showNotification(result.message || 'Failed to remove network', 'error');
            }
        } catch (error) {
            this.showNotification('Failed to remove network', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    // Status Updates Management
    startStatusUpdates() {
        if (this.refreshTimer) {
            clearInterval(this.refreshTimer);
        }

        this.refreshTimer = setInterval(() => {
            if (this.isOnline) {
                this.loadDeviceStatus();
            }
        }, this.refreshInterval);
    }

    stopStatusUpdates() {
        if (this.refreshTimer) {
            clearInterval(this.refreshTimer);
            this.refreshTimer = null;
        }
    }

    async loadAllData() {
        this.showLoading(true);
        try {
            await Promise.all([
                this.loadDeviceStatus(),
                this.loadNetworks()
            ]);
        } catch (error) {
            console.error('Failed to load initial data:', error);
        } finally {
            this.showLoading(false);
        }
    }

    // Utility Methods
    updateElement(id, content) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = content;
        }
    }

    formatUptime(seconds) {
        if (!seconds) return '-';
        
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        
        if (days > 0) {
            return `${days}d ${hours}h ${minutes}m`;
        } else if (hours > 0) {
            return `${hours}h ${minutes}m`;
        } else {
            return `${minutes}m`;
        }
    }

    formatMemory(bytes) {
        if (!bytes) return '-';
        return `${Math.round(bytes / 1024)} KB`;
    }

    formatRSSI(rssi) {
        if (!rssi || rssi === 0) return '-';
        return `${rssi} dBm`;
    }

    renderSignalBars(rssi) {
        if (!rssi || rssi === 0) return '';
        
        // Convert RSSI to signal strength (0-4 bars)
        let bars = 0;
        if (rssi >= -50) bars = 4;
        else if (rssi >= -60) bars = 3;
        else if (rssi >= -70) bars = 2;
        else if (rssi >= -80) bars = 1;
        
        const barsHtml = [];
        for (let i = 1; i <= 4; i++) {
            const active = i <= bars ? 'active' : '';
            barsHtml.push(`<div class="signal-bar ${active}"></div>`);
        }
        
        return `<div class="signal-bars">${barsHtml.join('')}</div>`;
    }

    showLoading(show) {
        const overlay = document.getElementById('loading-overlay');
        if (overlay) {
            overlay.style.display = show ? 'flex' : 'none';
        }
    }

    showNotification(message, type = 'info', duration = 5000) {
        const notification = document.createElement('div');
        notification.className = `notification ${type} fade-in`;
        notification.innerHTML = `
            <div>${message}</div>
            <button onclick="this.parentElement.remove()" style="background: none; border: none; color: inherit; float: right; cursor: pointer;">×</button>
        `;
        
        document.body.appendChild(notification);
        
        setTimeout(() => {
            if (notification.parentElement) {
                notification.remove();
            }
        }, duration);
    }

    // Helper method to extract values from text responses
    extractValue(text, key) {
        const regex = new RegExp(`${key}:\\s*([^\\n\\r]+)`, 'i');
        const match = text.match(regex);
        return match ? match[1].trim() : null;
    }

    // Parse network list from text response
    parseNetworksText(text) {
        if (!text || text.includes('No STA networks configured')) {
            return [];
        }

        const networks = [];
        const lines = text.split('\n');
        
        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('  ')) {
                const networkName = trimmed.replace(/^\s*/, '').replace(/ \(active\)$/, '');
                const isActive = trimmed.includes('(active)');
                
                networks.push({
                    ssid: networkName,
                    connected: isActive
                });
            }
        }

        return networks;
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new LoRaTNCXApp();
});

// Handle page visibility changes to pause/resume updates
document.addEventListener('visibilitychange', () => {
    if (window.app) {
        if (document.hidden) {
            window.app.stopStatusUpdates();
        } else {
            window.app.startStatusUpdates();
            window.app.loadAllData();
        }
    }
});

// Export for debugging
if (typeof module !== 'undefined' && module.exports) {
    module.exports = LoRaTNCXApp;
}