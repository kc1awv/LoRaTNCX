// LoRaTNCX Web Interface JavaScript

class LoRaTNCXInterface {
    constructor() {
        this.apiBase = '';
        this.autoRefresh = true;
        this.refreshInterval = null;
        this.board = null; // Store board information
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.loadInitialData();
        this.startAutoRefresh();
        this.loadDarkModePreference();
    }

    setupEventListeners() {
        // Auto-refresh toggle
        document.getElementById('autoRefresh').addEventListener('change', (e) => {
            this.autoRefresh = e.target.checked;
            if (this.autoRefresh) {
                this.startAutoRefresh();
            } else {
                this.stopAutoRefresh();
            }
        });

        // LoRa form
        document.getElementById('loraForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.applyLoRaConfig();
        });

        document.getElementById('resetBtn').addEventListener('click', () => {
            this.resetLoRaConfig();
        });

        document.getElementById('saveBtn').addEventListener('click', () => {
            this.saveLoRaConfig();
        });

        // WiFi scan button
        document.getElementById('scanBtn').addEventListener('click', () => {
            this.startWiFiScan();
        });

        // WiFi mode change - show/hide scan button
        document.getElementById('wifiMode').addEventListener('change', (e) => {
            this.toggleWiFiMode(e.target.value);
            this.toggleScanButton(e.target.value);
        });

        document.getElementById('wifiMode').addEventListener('change', (e) => {
            this.toggleWiFiMode(e.target.value);
        });

        // GNSS form
        document.getElementById('gnssForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.applyGNSSConfig();
        });

        document.getElementById('resetGnssBtn').addEventListener('click', () => {
            this.resetGNSSConfig();
        });

        document.getElementById('saveGnssBtn').addEventListener('click', () => {
            this.saveGNSSConfig();
        });

        // System
        document.getElementById('restartBtn').addEventListener('click', () => {
            this.restartDevice();
        });

        // Navigation
        document.querySelectorAll('.nav-link').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const targetId = e.target.getAttribute('href').substring(1);
                this.scrollToSection(targetId);
            });
        });

        // Dark mode toggle
        document.getElementById('darkMode').addEventListener('change', (e) => {
            this.toggleDarkMode(e.target.checked);
        });
    }

    async loadInitialData() {
        // Load system info first to get board information and populate forms
        await this.loadSystemInfo();
        
        // Then load other data in parallel
        await Promise.all([
            this.loadWiFiConfig(),
            this.loadGNSSConfig(),
            this.loadGNSSStatus()
        ]);
    }

    async loadStatus() {
        try {
            const response = await fetch('/api/status');
            const data = await response.json();

            // WiFi status - determine mode and connection status
            let wifiStatus = 'Disconnected';
            if (data.wifi) {
                if (data.wifi.ap_active && data.wifi.connected) {
                    wifiStatus = 'AP+STA';
                } else if (data.wifi.ap_active) {
                    wifiStatus = 'Access Point';
                } else if (data.wifi.connected) {
                    wifiStatus = 'Station';
                }
            }
            document.getElementById('wifiStatus').textContent = wifiStatus;

            // IP Address - show appropriate IP based on mode
            let ipAddress = 'Not connected';
            if (data.wifi) {
                if (data.wifi.ap_active && data.wifi.sta_ip && data.wifi.sta_ip !== '0.0.0.0') {
                    ipAddress = `${data.wifi.sta_ip} (STA), ${data.wifi.ap_ip} (AP)`;
                } else if (data.wifi.ap_active) {
                    ipAddress = `${data.wifi.ap_ip} (AP)`;
                } else if (data.wifi.connected && data.wifi.sta_ip) {
                    ipAddress = data.wifi.sta_ip;
                }
            }
            document.getElementById('ipAddress').textContent = ipAddress;

            // Uptime
            document.getElementById('uptime').textContent = this.formatUptime(data.system?.uptime || 0);

            // Board info is handled by loadSystemInfo()
            // Don't reset it here during auto-refresh

            // Battery status
            const batteryVoltage = data.battery?.voltage || 0;
            let batteryStatus = 'Unknown';
            if (batteryVoltage > 0) {
                if (batteryVoltage >= 3.9) {
                    batteryStatus = `${batteryVoltage.toFixed(2)}V (Good)`;
                } else if (batteryVoltage >= 3.7) {
                    batteryStatus = `${batteryVoltage.toFixed(2)}V (Medium)`;
                } else {
                    batteryStatus = `${batteryVoltage.toFixed(2)}V (Low)`;
                }
            }
            document.getElementById('batteryStatus').textContent = batteryStatus;

        } catch (error) {
            console.error('Failed to load status:', error);
            // Set fallback values
            document.getElementById('wifiStatus').textContent = 'Error';
            document.getElementById('ipAddress').textContent = 'Error';
            document.getElementById('uptime').textContent = 'Error';
            document.getElementById('boardInfo').textContent = 'Error';
        }
    }

    updatePowerLimits() {
        const powerInput = document.getElementById('power');
        const powerHelp = document.getElementById('powerHelp');
        
        if (!powerInput || !powerHelp) return; // Elements not loaded yet
        
        if (this.board && this.board.type === 4) {
            powerInput.max = 28;
            powerHelp.textContent = 'Valid range: -9 to 28 dBm (V4 device)';
        } else if (this.board && this.board.type === 3) {
            powerInput.max = 22;
            powerHelp.textContent = 'Valid range: -9 to 22 dBm (V3 device)';
        } else {
            // Board info not loaded yet, set conservative default
            powerInput.max = 22;
            powerHelp.textContent = 'Valid range: -9 to 22 dBm (loading board info...)';
        }
    }

    async loadSystemInfo() {
        try {
            const response = await fetch('/api/system');
            const data = await response.json();

            // Store board information
            this.board = data.board;

            // Update TX power limits if LoRa config is already loaded
            this.updatePowerLimits();

            // Update board info card
            const boardName = data.board?.name || data.board?.type || 'Unknown';
            document.getElementById('boardInfo').textContent = boardName;

            document.getElementById('firmwareInfo').textContent = `Board: ${boardName}, Type: ${data.board?.type || 'Unknown'}`;
            document.getElementById('hardwareInfo').textContent = `Chip: ${data.chip?.model || 'Unknown'} (${data.chip?.cores || 1} cores @ ${data.chip?.frequency || 0}MHz)`;
            document.getElementById('memoryInfo').textContent = `Free: ${data.memory?.free_heap || 0} bytes, Total: ${data.memory?.heap_size || 0} bytes`;
            document.getElementById('storageInfo').textContent = `Flash: ${data.memory?.flash_size ? (data.memory.flash_size / 1024 / 1024).toFixed(1) + 'MB' : 'Unknown'}`;
        } catch (error) {
            console.error('Failed to load system info:', error);
            // Set fallback values
            document.getElementById('boardInfo').textContent = 'Unknown';
            document.getElementById('firmwareInfo').textContent = 'Error loading firmware info';
            document.getElementById('hardwareInfo').textContent = 'Error loading hardware info';
            document.getElementById('memoryInfo').textContent = 'Error loading memory info';
            document.getElementById('storageInfo').textContent = 'Error loading storage info';
        }
    }

    async loadWiFiConfig() {
        try {
            const response = await fetch('/api/wifi/config');
            const config = await response.json();

            // Convert numeric mode to string
            const modeMap = {
                0: 'off',
                1: 'ap', 
                2: 'sta',
                3: 'apsta'
            };
            const modeString = modeMap[config.mode] || 'ap';

            // Set WiFi mode
            document.getElementById('wifiMode').value = modeString;

            // Set SSID and password for STA mode
            document.getElementById('ssid').value = config.ssid || '';
            document.getElementById('password').value = ''; // Don't populate password for security

            // Set AP SSID and password
            document.getElementById('apSsid').value = config.ap_ssid || 'LoRaTNCX-XXXX';
            document.getElementById('apPassword').value = ''; // Don't populate password for security

            // Update form visibility based on mode
            this.toggleWiFiMode(modeString);

        } catch (error) {
            console.error('Failed to load WiFi config:', error);
        }
    }

    async applyLoRaConfig() {
        const config = {
            frequency: parseFloat(document.getElementById('frequency').value),
            bandwidth: parseFloat(document.getElementById('bandwidth').value),
            spreadingFactor: parseInt(document.getElementById('spreadingFactor').value),
            codingRate: parseInt(document.getElementById('codingRate').value),
            power: parseInt(document.getElementById('power').value),
            syncWord: this.parseSyncWord(document.getElementById('syncWord').value)
        };

        try {
            const response = await fetch('/api/lora/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            });

            const result = await response.json();
            if (result.success) {
                this.showAlert('LoRa configuration applied successfully', 'success');
            } else {
                this.showAlert('Failed to apply LoRa configuration', 'danger');
            }
        } catch (error) {
            console.error('Failed to apply LoRa config:', error);
            this.showAlert('Error applying LoRa configuration', 'danger');
        }
    }

    async resetLoRaConfig() {
        try {
            const response = await fetch('/api/lora/reset', { method: 'POST' });
            const result = await response.json();
            if (result.success) {
                this.showAlert('LoRa configuration reset to defaults', 'success');
                this.loadLoRaConfig();
            } else {
                this.showAlert('Failed to reset LoRa configuration', 'danger');
            }
        } catch (error) {
            console.error('Failed to reset LoRa config:', error);
            this.showAlert('Error resetting LoRa configuration', 'danger');
        }
    }

    async saveLoRaConfig() {
        try {
            const response = await fetch('/api/lora/save', { method: 'POST' });
            const result = await response.json();
            if (result.success) {
                this.showAlert('LoRa configuration saved to NVS', 'success');
            } else {
                this.showAlert('Failed to save LoRa configuration', 'danger');
            }
        } catch (error) {
            console.error('Failed to save LoRa config:', error);
            this.showAlert('Error saving LoRa configuration', 'danger');
        }
    }

    async applyWiFiConfig() {
        // Convert string mode to numeric
        const modeMap = {
            'off': 0,
            'ap': 1,
            'sta': 2, 
            'apsta': 3
        };
        
        const config = {
            mode: modeMap[document.getElementById('wifiMode').value] || 1,
            ssid: document.getElementById('ssid').value,
            password: document.getElementById('password').value,
            apSsid: document.getElementById('apSsid').value,
            apPassword: document.getElementById('apPassword').value
        };

        try {
            const response = await fetch('/api/wifi/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            });

            const result = await response.json();
            if (result.success) {
                this.showAlert('WiFi configuration applied. Device may restart.', 'success');
            } else {
                this.showAlert('Failed to apply WiFi configuration', 'danger');
            }
        } catch (error) {
            console.error('Failed to apply WiFi config:', error);
            this.showAlert('Error applying WiFi configuration', 'danger');
        }
    }

    async loadGNSSConfig() {
        try {
            const response = await fetch('/api/gnss/config');
            const config = await response.json();

            document.getElementById('gnssEnabled').checked = config.enabled || false;
            document.getElementById('serialPassthrough').checked = config.serialPassthrough || false;
            document.getElementById('gnssBaudRate').value = config.baudRate || 9600;
            document.getElementById('gnssTcpPort').value = config.tcpPort || 10110;

            // Pin configuration (only show if no built-in port)
            if (!config.hasBuiltInPort) {
                document.getElementById('pinConfig').style.display = 'block';
                document.getElementById('pinRX').value = config.pinRX || '';
                document.getElementById('pinTX').value = config.pinTX || '';
                document.getElementById('pinPPS').value = config.pinPPS || '';
                document.getElementById('pinRST').value = config.pinRST || '';
            }

        } catch (error) {
            console.error('Failed to load GNSS config:', error);
        }
    }

    async loadGNSSStatus() {
        try {
            const response = await fetch('/api/gnss/status');
            const status = await response.json();

            let gnssText = 'Disabled';
            if (status.running) {
                if (status.hasFix) {
                    gnssText = `Fix: ${status.satellites || 0} sats`;
                } else {
                    gnssText = 'No Fix';
                }
            }

            document.getElementById('gnssStatus').textContent = gnssText;

        } catch (error) {
            console.error('Failed to load GNSS status:', error);
            document.getElementById('gnssStatus').textContent = 'Error';
        }
    }

    async loadSystemInfo() {
        try {
            const response = await fetch('/api/system');
            const data = await response.json();

            // Store board information for power limit updates
            this.board = data.board;

            // Set board info in status card
            const boardInfo = `${data.board?.name || 'Unknown'}`;
            document.getElementById('boardInfo').textContent = boardInfo;

            // Hardware info
            const hardwareInfo = `
                <strong>Board:</strong> ${data.board?.name || 'Unknown'} (type ${data.board?.type || 'Unknown'})<br>
                <strong>Chip:</strong> ${data.chip?.model || 'Unknown'} @ ${data.chip?.cores || 'Unknown'} cores, ${data.chip?.frequency || 'Unknown'}MHz
            `;
            document.getElementById('hardwareInfo').innerHTML = hardwareInfo;

            // Memory info
            const memoryInfo = `
                <strong>Heap:</strong> ${this.formatBytes(data.memory?.free_heap || 0)} free / ${this.formatBytes(data.memory?.heap_size || 0)} total<br>
                <strong>Flash:</strong> ${this.formatBytes(data.memory?.flash_size || 0)}
            `;
            document.getElementById('memoryInfo').innerHTML = memoryInfo;

            // Storage info
            const storageInfo = `
                <strong>SPIFFS:</strong> ${this.formatBytes(data.storage?.spiffs_used || 0)} used / ${this.formatBytes(data.storage?.spiffs_total || 0)} total
            `;
            document.getElementById('storageInfo').innerHTML = storageInfo;

            // Current configuration summary
            let currentConfig = 'Loading...';
            try {
                const loraResponse = await fetch('/api/lora/config');
                const loraConfig = await loraResponse.json();
                
                // Populate LoRa form fields
                document.getElementById('frequency').value = loraConfig.frequency || '';
                document.getElementById('bandwidth').value = loraConfig.bandwidth || '';
                document.getElementById('spreadingFactor').value = loraConfig.spreading || '';
                document.getElementById('codingRate').value = loraConfig.codingRate || '';
                document.getElementById('power').value = loraConfig.power || '';
                // Display sync word as hex
                const syncWord = loraConfig.syncWord || 0;
                document.getElementById('syncWord').value = '0x' + syncWord.toString(16).toUpperCase().padStart(4, '0');
                
                // Update TX power limits based on board type
                this.updatePowerLimits();
                
                currentConfig = `
                    <strong>LoRa:</strong> ${loraConfig.frequency || 'Unknown'}MHz, SF${loraConfig.spreading || 'Unknown'}, ${loraConfig.bandwidth || 'Unknown'}kHz, CR${loraConfig.codingRate || 'Unknown'}<br>
                    <strong>Power:</strong> ${loraConfig.power || 'Unknown'}dBm
                `;
            } catch (error) {
                currentConfig = '<strong>LoRa:</strong> Unable to load configuration';
            }
            document.getElementById('currentConfig').innerHTML = currentConfig;

            // Network status
            let networkStatus = 'Loading...';
            try {
                const wifiResponse = await fetch('/api/wifi/config');
                const wifiConfig = await wifiResponse.json();
                const modeMap = { 0: 'Off', 1: 'AP', 2: 'STA', 3: 'AP+STA' };
                networkStatus = `<strong>WiFi Mode:</strong> ${modeMap[wifiConfig.mode] || 'Unknown'}`;
                if (wifiConfig.mode === 2 || wifiConfig.mode === 3) {
                    networkStatus += `<br><strong>STA SSID:</strong> ${wifiConfig.ssid || 'Not configured'}`;
                }
                if (wifiConfig.mode === 1 || wifiConfig.mode === 3) {
                    networkStatus += `<br><strong>AP SSID:</strong> ${wifiConfig.ap_ssid || 'LoRaTNCX-XXXX'}`;
                }
            } catch (error) {
                networkStatus = '<strong>WiFi:</strong> Unable to load configuration';
            }
            document.getElementById('networkStatus').innerHTML = networkStatus;

            // GNSS status
            let gnssStatus = 'Loading...';
            try {
                const gnssResponse = await fetch('/api/gnss/status');
                const gnssData = await gnssResponse.json();
                gnssStatus = `
                    <strong>Status:</strong> ${gnssData.running ? 'Enabled' : 'Disabled'}<br>
                    <strong>Satellites:</strong> ${gnssData.satellites || 0} visible<br>
                    <strong>Fix:</strong> ${gnssData.hasFix ? 'Yes' : 'No'}
                `;
                if (gnssData.position) {
                    gnssStatus += `<br><strong>Position:</strong> ${gnssData.position.lat?.toFixed(6) || 'N/A'}, ${gnssData.position.lon?.toFixed(6) || 'N/A'}`;
                }
            } catch (error) {
                gnssStatus = '<strong>GNSS:</strong> Unable to load status';
            }
            document.getElementById('gnssDetailStatus').innerHTML = gnssStatus;

        } catch (error) {
            console.error('Failed to load system info:', error);
            // Set fallback values
            document.getElementById('boardInfo').textContent = 'Error';
            document.getElementById('hardwareInfo').textContent = 'Unable to load hardware information';
            document.getElementById('memoryInfo').textContent = 'Unable to load memory information';
            document.getElementById('storageInfo').textContent = 'Unable to load storage information';
            document.getElementById('currentConfig').textContent = 'Unable to load configuration';
            document.getElementById('networkStatus').textContent = 'Unable to load network status';
            document.getElementById('gnssDetailStatus').textContent = 'Unable to load GNSS status';
        }
    }

    async applyGNSSConfig() {
        const config = {
            enabled: document.getElementById('gnssEnabled').checked,
            serialPassthrough: document.getElementById('serialPassthrough').checked,
            baudRate: parseInt(document.getElementById('gnssBaudRate').value),
            tcpPort: parseInt(document.getElementById('gnssTcpPort').value)
        };

        // Only include pin config if pinConfig is visible
        if (document.getElementById('pinConfig').style.display !== 'none') {
            config.pinRX = parseInt(document.getElementById('pinRX').value) || 0;
            config.pinTX = parseInt(document.getElementById('pinTX').value) || 0;
            config.pinPPS = parseInt(document.getElementById('pinPPS').value) || 0;
            config.pinRST = parseInt(document.getElementById('pinRST').value) || 0;
        }

        try {
            const response = await fetch('/api/gnss/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            });

            const result = await response.json();
            if (result.success) {
                this.showAlert('GNSS configuration applied. Device may need restart.', 'success');
                // Reload status after config change
                setTimeout(() => this.loadGNSSStatus(), 1000);
            } else {
                this.showAlert('Failed to apply GNSS configuration', 'danger');
            }
        } catch (error) {
            console.error('Failed to apply GNSS config:', error);
            this.showAlert('Error applying GNSS configuration', 'danger');
        }
    }

    async resetGNSSConfig() {
        if (!confirm('Reset GNSS configuration to defaults?')) {
            return;
        }

        try {
            // For reset, we can just reload the config which will get defaults
            await this.loadGNSSConfig();
            this.showAlert('GNSS configuration reset to defaults', 'info');
        } catch (error) {
            console.error('Failed to reset GNSS config:', error);
            this.showAlert('Error resetting GNSS configuration', 'danger');
        }
    }

    async saveGNSSConfig() {
        // Apply first, then it should save to NVS
        await this.applyGNSSConfig();
        // The API should handle saving to NVS
    }

    async restartDevice() {
        if (!confirm('Are you sure you want to restart the device?')) {
            return;
        }

        try {
            const response = await fetch('/api/reboot', { method: 'POST' });
            const result = await response.json();
            if (result.success) {
                this.showAlert('Device restarting...', 'info');
            } else {
                this.showAlert('Failed to restart device', 'danger');
            }
        } catch (error) {
            console.error('Failed to restart device:', error);
            this.showAlert('Error restarting device', 'danger');
        }
    }

    toggleWiFiMode(mode) {
        const staConfig = document.getElementById('staConfig');
        const apConfig = document.getElementById('apConfig');

        if (mode === 'sta' || mode === 'apsta') {
            staConfig.style.display = 'block';
        } else {
            staConfig.style.display = 'none';
        }

        if (mode === 'ap' || mode === 'apsta') {
            apConfig.style.display = 'block';
        } else {
            apConfig.style.display = 'none';
        }

        // Also toggle scan button visibility
        this.toggleScanButton(mode);
    }

    toggleScanButton(mode) {
        const scanBtn = document.getElementById('scanBtn');
        if (mode === 'sta' || mode === 'apsta') {
            scanBtn.style.display = 'inline-block';
        } else {
            scanBtn.style.display = 'none';
        }
    }

    async startWiFiScan() {
        try {
            // Start the scan
            const startResponse = await fetch('/api/wifi/scan');
            const startResult = await startResponse.json();

            if (!startResult.status || startResult.status !== 'started') {
                this.showAlert(startResult.error || 'Failed to start WiFi scan', 'danger');
                return;
            }

            // Show modal
            const modal = new bootstrap.Modal(document.getElementById('wifiScanModal'));
            modal.show();

            // Start polling for results
            this.pollScanStatus();

        } catch (error) {
            console.error('Failed to start WiFi scan:', error);
            this.showAlert('Error starting WiFi scan', 'danger');
        }
    }

    async pollScanStatus() {
        try {
            const response = await fetch('/api/wifi/scan/status');
            const result = await response.json();

            if (result.status === 'scanning') {
                // Still scanning, continue polling
                document.getElementById('scanStatus').innerHTML = `
                    <div class="spinner-border text-primary" role="status">
                        <span class="visually-hidden">Scanning...</span>
                    </div>
                    <p class="mt-2">Scanning for WiFi networks... ${result.progress || 0}%</p>
                `;
                setTimeout(() => this.pollScanStatus(), 1000);
            } else if (result.status === 'completed') {
                // Scan complete, show networks
                this.displayNetworkList(result.networks);
            } else {
                // Idle or error
                document.getElementById('scanStatus').innerHTML = `
                    <p class="text-muted">No scan in progress</p>
                `;
            }
        } catch (error) {
            console.error('Failed to poll scan status:', error);
            document.getElementById('scanStatus').innerHTML = `
                <p class="text-danger">Error checking scan status</p>
            `;
        }
    }

    displayNetworkList(networks) {
        const scanStatus = document.getElementById('scanStatus');
        const networkList = document.getElementById('networkList');
        const networksContainer = document.getElementById('networksContainer');

        // Hide scanning status, show network list
        scanStatus.classList.add('d-none');
        networkList.classList.remove('d-none');

        // Clear previous results
        networksContainer.innerHTML = '';

        if (!networks || networks.length === 0) {
            networksContainer.innerHTML = '<p class="text-muted">No networks found</p>';
            return;
        }

        // Sort networks by signal strength (RSSI)
        networks.sort((a, b) => b.rssi - a.rssi);

        // Create network list items
        networks.forEach(network => {
            const item = document.createElement('button');
            item.type = 'button';
            item.className = 'list-group-item list-group-item-action d-flex justify-content-between align-items-center';
            
            const ssid = network.ssid || '(hidden network)';
            const rssi = network.rssi || 0;
            const encrypted = network.encrypted ? '🔒' : '🔓';
            
            // Calculate signal strength bars
            const signalBars = this.getSignalBars(rssi);
            
            item.innerHTML = `
                <div>
                    <strong>${ssid}</strong>
                    <small class="text-muted ms-2">${encrypted}</small>
                </div>
                <div class="text-end">
                    <small class="text-muted">${rssi} dBm</small>
                    <div class="ms-2">${signalBars}</div>
                </div>
            `;

            item.addEventListener('click', () => {
                this.selectNetwork(ssid);
            });

            networksContainer.appendChild(item);
        });
    }

    getSignalBars(rssi) {
        // Convert RSSI to signal strength (0-4 bars)
        let bars = 0;
        if (rssi >= -50) bars = 4;
        else if (rssi >= -60) bars = 3;
        else if (rssi >= -70) bars = 2;
        else if (rssi >= -80) bars = 1;

        const fullBars = '█'.repeat(bars);
        const emptyBars = '░'.repeat(4 - bars);
        return `<span class="text-success">${fullBars}</span><span class="text-muted">${emptyBars}</span>`;
    }

    selectNetwork(ssid) {
        // Close modal
        const modal = bootstrap.Modal.getInstance(document.getElementById('wifiScanModal'));
        modal.hide();

        // Populate SSID field
        document.getElementById('ssid').value = ssid;

        // Focus on password field
        document.getElementById('password').focus();

        this.showAlert(`Selected network: ${ssid}`, 'success');
    }

    startAutoRefresh() {
        this.stopAutoRefresh();
        this.refreshInterval = setInterval(() => {
            if (this.autoRefresh) {
                this.loadStatus();
            }
        }, 5000);

        // Refresh system info every 30 seconds (less frequently)
        this.systemRefreshInterval = setInterval(() => {
            if (this.autoRefresh) {
                this.loadSystemInfo();
                this.loadGNSSStatus(); // Refresh GNSS status with system info
            }
        }, 30000);
    }

    stopAutoRefresh() {
        if (this.refreshInterval) {
            clearInterval(this.refreshInterval);
            this.refreshInterval = null;
        }
        if (this.systemRefreshInterval) {
            clearInterval(this.systemRefreshInterval);
            this.systemRefreshInterval = null;
        }
    }

    scrollToSection(sectionId) {
        const element = document.getElementById(sectionId);
        if (element) {
            element.scrollIntoView({ behavior: 'smooth' });
        }
    }

    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;

        let result = '';
        if (days > 0) result += `${days}d `;
        if (hours > 0) result += `${hours}h `;
        if (minutes > 0) result += `${minutes}m `;
        result += `${secs}s`;

        return result.trim();
    }

    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
    }

    showAlert(message, type) {
        // Create alert element
        const alertDiv = document.createElement('div');
        alertDiv.className = `alert alert-${type} alert-dismissible fade show position-fixed`;
        alertDiv.style.cssText = 'top: 20px; right: 20px; z-index: 9999; min-width: 300px;';
        alertDiv.innerHTML = `
            ${message}
            <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
        `;

        document.body.appendChild(alertDiv);

        // Auto-remove after 5 seconds
        setTimeout(() => {
            if (alertDiv.parentNode) {
                alertDiv.remove();
            }
        }, 5000);
    }

    parseSyncWord(value) {
        if (!value) return 0;
        
        // Remove 0x prefix if present
        const cleanValue = value.replace(/^0x/i, '');
        
        // Parse as hex
        const parsed = parseInt(cleanValue, 16);
        return isNaN(parsed) ? 0 : parsed;
    }

    loadDarkModePreference() {
        const darkModeEnabled = localStorage.getItem('darkMode') === 'true';
        document.getElementById('darkMode').checked = darkModeEnabled;
        this.setDarkMode(darkModeEnabled);
    }

    toggleDarkMode(enabled) {
        this.setDarkMode(enabled);
        localStorage.setItem('darkMode', enabled);
    }

    setDarkMode(enabled) {
        const html = document.documentElement;
        const body = document.body;
        
        if (enabled) {
            html.setAttribute('data-bs-theme', 'dark');
            body.classList.remove('bg-light');
            body.classList.add('bg-dark');
        } else {
            html.removeAttribute('data-bs-theme');
            body.classList.remove('bg-dark');
            body.classList.add('bg-light');
        }
    }
}

// Initialize the interface when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    new LoRaTNCXInterface();
});