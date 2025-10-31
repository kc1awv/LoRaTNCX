import { initPage, REALTIME_EVENTS, getLatestTelemetry, getActivityHistory } from './common.js';

// Chart instance for real-time data
let statusChart = null;
let chartData = {
    signal: [],
    packets: [],
    errors: [],
    timestamps: []
};
const MAX_CHART_POINTS = 20; // Reduced from 50 to 20 to improve performance
let lastChartUpdate = 0;
const CHART_UPDATE_THROTTLE = 5000; // Only update chart every 5 seconds

function escapeHtml(value) {
    if (value == null) {
        return '';
    }
    return String(value)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

function formatTimestamp(timestamp) {
    if (!timestamp) {
        return '';
    }
    try {
        const date = new Date(timestamp);
        if (Number.isNaN(date.getTime())) {
            return '';
        }
        return date.toLocaleTimeString();
    } catch (error) {
        return '';
    }
}

function truncate(value, length = 120) {
    if (value == null) {
        return '';
    }
    const text = String(value);
    if (text.length <= length) {
        return text;
    }
    return `${text.slice(0, length - 1)}…`;
}

function initChart() {
    const ctx = document.getElementById('statusChart');
    if (!ctx || typeof Chart === 'undefined') {
        console.warn('Chart.js not available or canvas not found');
        return;
    }

    console.log('Initializing Chart.js...');
    
    try {
        statusChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: chartData.timestamps,
                datasets: [{
                    label: 'Signal Strength (dBm)',
                    data: chartData.signal,
                    borderColor: '#2563eb',
                    backgroundColor: 'rgba(37, 99, 235, 0.1)',
                    tension: 0.4,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: {
                    duration: 0 // Disable animations to reduce CPU load
                },
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    x: {
                        display: false
                    },
                    y: {
                        beginAtZero: false,
                        grid: {
                            color: 'rgba(148, 163, 184, 0.1)'
                        }
                    }
                },
                elements: {
                    point: {
                        radius: 0,
                        hoverRadius: 4
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'index'
                }
            }
        });
        
        console.log('Chart.js initialized successfully');
    } catch (error) {
        console.error('Failed to initialize Chart.js:', error);
    }
}

function initFallbackChart() {
    const container = document.querySelector('.chart-container');
    if (!container) return;
    
    container.innerHTML = `
        <div class="fallback-chart">
            <div class="chart-header d-flex justify-content-between align-items-center mb-3">
                <span class="small text-muted">Real-time Signal Strength</span>
                <span class="badge bg-primary" id="current-signal">-- dBm</span>
            </div>
            <div class="signal-bars d-flex align-items-end gap-1" style="height: 120px;">
                ${Array.from({length: 20}, (_, i) => `
                    <div class="signal-bar" style="flex: 1; background: #e2e8f0; border-radius: 2px; min-height: 4px;" data-bar="${i}"></div>
                `).join('')}
            </div>
            <div class="chart-footer mt-2 text-center">
                <small class="text-muted">Simple signal visualization (Chart.js unavailable)</small>
            </div>
        </div>
    `;
    
    // Mark as fallback chart
    statusChart = 'fallback';
}

function updateChart(telemetry) {
    if (!statusChart || !telemetry) return;

    const display = telemetry?.tnc?.display || telemetry?.display || {};
    const now = Date.now();
    
    // Add data points using API structure
    const signalValue = display.last_rssi ?? telemetry.radio?.rssi ?? -100;
    chartData.signal.push(signalValue);
    chartData.packets.push((display.tx_count || 0) + (display.rx_count || 0));
    chartData.errors.push(telemetry.errors || 0);
    chartData.timestamps.push(formatTimestamp(new Date()));

    // Keep only recent data
    if (chartData.timestamps.length > MAX_CHART_POINTS) {
        chartData.timestamps.shift();
        chartData.signal.shift();
        chartData.packets.shift();
        chartData.errors.shift();
    }

    // Throttle chart updates to prevent overwhelming the system
    if (now - lastChartUpdate > CHART_UPDATE_THROTTLE) {
        lastChartUpdate = now;
        
        if (statusChart && typeof statusChart.update === 'function') {
            // Use requestAnimationFrame to ensure smooth rendering
            requestAnimationFrame(() => {
                const selectedChart = document.querySelector('input[name="chartType"]:checked')?.id || 'chart-signal';
                updateChartDataset(selectedChart);
            });
        }
    }
}

function updateFallbackChart(signalStrength) {
    // Update current signal display
    const currentSignalElement = document.getElementById('current-signal');
    if (currentSignalElement) {
        currentSignalElement.textContent = signalStrength !== -100 ? `${signalStrength} dBm` : '-- dBm';
    }
    
    // Update signal bars
    const bars = document.querySelectorAll('.signal-bar');
    const signalData = chartData.signal.slice(-20); // Last 20 readings
    
    bars.forEach((bar, index) => {
        if (index < signalData.length) {
            const value = signalData[index];
            const normalizedHeight = Math.max(5, Math.min(100, ((value + 120) / 50) * 100)); // Normalize -120 to -70 dBm to 0-100%
            const color = value > -70 ? '#10b981' : value > -85 ? '#f59e0b' : value > -100 ? '#ef4444' : '#6b7280';
            
            bar.style.height = `${normalizedHeight}%`;
            bar.style.backgroundColor = color;
        } else {
            bar.style.height = '4px';
            bar.style.backgroundColor = '#e2e8f0';
        }
    });
}

function updateChartDataset(chartType) {
    if (!statusChart || typeof statusChart.update !== 'function') return;

    let data, label, color, backgroundColor;
    
    switch (chartType) {
        case 'chart-packets':
            data = chartData.packets;
            label = 'Total Packets';
            color = '#10b981';
            backgroundColor = 'rgba(16, 185, 129, 0.1)';
            break;
        case 'chart-errors':
            data = chartData.errors;
            label = 'Error Count';
            color = '#ef4444';
            backgroundColor = 'rgba(239, 68, 68, 0.1)';
            break;
        default:
            data = chartData.signal;
            label = 'Signal Strength (dBm)';
            color = '#2563eb';
            backgroundColor = 'rgba(37, 99, 235, 0.1)';
    }

    try {
        // Update Chart.js data efficiently
        statusChart.data.datasets[0].data = data;
        statusChart.data.datasets[0].label = label;
        statusChart.data.datasets[0].borderColor = color;
        statusChart.data.datasets[0].backgroundColor = backgroundColor;
        statusChart.data.labels = chartData.timestamps;
        
        // Update without animation to prevent blocking
        statusChart.update('none');
    } catch (error) {
        console.error('Error updating chart:', error);
    }
}

function renderStatusCards(telemetry) {
    console.log('Rendering status cards with telemetry:', telemetry);
    
    // Extract display data from the API response structure
    const display = telemetry?.tnc?.display || telemetry?.display || {};
    
    // If no telemetry, show loading or placeholder data
    if (!telemetry) {
        console.log('No telemetry data available, showing placeholders');
        
        // Show loading or placeholder values
        const signalElement = document.querySelector('[data-signal-strength]');
        if (signalElement) signalElement.textContent = '--';
        
        const signalTrendElement = document.querySelector('[data-signal-trend]');
        if (signalTrendElement) signalTrendElement.textContent = 'Loading...';
        
        const packetsSentElement = document.querySelector('[data-packets-sent]');
        if (packetsSentElement) packetsSentElement.textContent = '--';
        
        const packetsReceivedElement = document.querySelector('[data-packets-received]');
        if (packetsReceivedElement) packetsReceivedElement.textContent = '--';
        
        const uptimeElement = document.querySelector('[data-uptime]');
        if (uptimeElement) uptimeElement.textContent = '--';
        
        const uptimeDetailElement = document.querySelector('[data-uptime-detail]');
        if (uptimeDetailElement) uptimeDetailElement.textContent = 'Loading...';
        
        return;
    }

    // Signal Strength (from last_rssi in the API)
    const signalElement = document.querySelector('[data-signal-strength]');
    if (signalElement) {
        const rssi = display.last_rssi ?? telemetry.radio?.rssi ?? telemetry.rssi;
        if (rssi !== undefined && rssi !== null) {
            signalElement.textContent = `${rssi}`;
        } else {
            signalElement.textContent = '--';
        }
        
        const trendElement = document.querySelector('[data-signal-trend]');
        if (trendElement) {
            if (rssi !== undefined && rssi !== null) {
                const quality = rssi > -70 ? 'Excellent' : rssi > -85 ? 'Good' : rssi > -100 ? 'Fair' : 'Poor';
                trendElement.textContent = quality;
            } else {
                trendElement.textContent = 'No Signal';
            }
        }
    }

    // Packets Sent (tx_count from API)
    const packetsSentElement = document.querySelector('[data-packets-sent]');
    if (packetsSentElement) {
        const txCount = display.tx_count ?? telemetry.packets_sent ?? telemetry.tx_packets ?? 0;
        packetsSentElement.textContent = txCount;
    }

    // Packets Received (rx_count from API)
    const packetsReceivedElement = document.querySelector('[data-packets-received]');
    if (packetsReceivedElement) {
        const rxCount = display.rx_count ?? telemetry.packets_received ?? telemetry.rx_packets ?? 0;
        packetsReceivedElement.textContent = rxCount;
    }

    // Packet rates (placeholder for now)
    const packetsRateElement = document.querySelector('[data-packets-rate]');
    if (packetsRateElement) {
        packetsRateElement.textContent = '--/min';
    }
    
    const receivedRateElement = document.querySelector('[data-received-rate]');
    if (receivedRateElement) {
        receivedRateElement.textContent = '--/min';
    }

    // Uptime (from uptime_ms in API, convert to seconds)
    const uptimeElement = document.querySelector('[data-uptime]');
    if (uptimeElement) {
        const uptimeMs = display.uptime_ms ?? telemetry.uptime;
        let uptimeSeconds = 0;
        
        if (uptimeMs) {
            uptimeSeconds = Math.floor(uptimeMs / 1000);
        } else if (telemetry.uptime) {
            uptimeSeconds = telemetry.uptime;
        }
        
        uptimeElement.textContent = uptimeSeconds > 0 ? formatUptime(uptimeSeconds) : '--';
        
        const uptimeDetailElement = document.querySelector('[data-uptime-detail]');
        if (uptimeDetailElement) {
            if (uptimeSeconds > 0) {
                uptimeDetailElement.textContent = `${uptimeSeconds}s`;
            } else {
                uptimeDetailElement.textContent = 'Unknown';
            }
        }
    }
}

function renderDeviceInfo(telemetry) {
    const container = document.querySelector('[data-device-info]');
    if (!container) {
        console.log('Device info container not found');
        return;
    }

    console.log('Rendering device info with telemetry:', telemetry);

    if (!telemetry) {
        container.innerHTML = `
            <div class="text-center py-3 text-muted">
                <div class="spinner-border spinner-border-sm mb-2"></div>
                <div>Loading device information...</div>
            </div>
        `;
        return;
    }

    // Extract display data from the API response structure  
    const display = telemetry?.tnc?.display || telemetry?.display || {};
    const tnc = telemetry?.tnc || {};
    
    const info = [
        { 
            label: 'Operating Mode', 
            value: display.mode || telemetry.mode || 'Unknown', 
            icon: 'bi-gear' 
        },
        { 
            label: 'Frequency', 
            value: display.frequency_mhz ? `${display.frequency_mhz} MHz` : 'Unknown', 
            icon: 'bi-broadcast' 
        },
        { 
            label: 'Bandwidth', 
            value: display.bandwidth_khz ? `${display.bandwidth_khz} kHz` : 'Unknown', 
            icon: 'bi-speedometer' 
        },
        { 
            label: 'Spreading Factor', 
            value: display.spreading_factor ? `SF${display.spreading_factor}` : 'Unknown', 
            icon: 'bi-layers' 
        },
        { 
            label: 'Coding Rate', 
            value: display.coding_rate || 'Unknown', 
            icon: 'bi-code-slash' 
        },
        { 
            label: 'TX Power', 
            value: display.tx_power_dbm ? `${display.tx_power_dbm} dBm` : 'Unknown', 
            icon: 'bi-lightning' 
        }
    ];

    const html = info.map(item => `
        <div class="d-flex justify-content-between align-items-center py-2 border-bottom">
            <div class="d-flex align-items-center">
                <i class="bi ${item.icon} me-2 text-primary"></i>
                <span class="small">${item.label}</span>
            </div>
            <span class="fw-medium">${escapeHtml(item.value)}</span>
        </div>
    `).join('');

    container.innerHTML = html;
}

function renderConfigSummary(telemetry) {
    const container = document.querySelector('[data-config-summary]');
    if (!container) {
        console.log('Config summary container not found');
        return;
    }

    console.log('Rendering config summary with telemetry:', telemetry);

    if (!telemetry) {
        container.innerHTML = `
            <div class="text-center py-3 text-muted">
                <div class="spinner-border spinner-border-sm mb-2"></div>
                <div>Loading configuration...</div>
            </div>
        `;
        return;
    }

    const configs = [
        { 
            label: 'Call Sign', 
            value: telemetry.callsign || telemetry.call_sign || 'Not Set', 
            status: (telemetry.callsign || telemetry.call_sign) ? 'success' : 'warning' 
        },
        { 
            label: 'APRS Beacon', 
            value: telemetry.beacon_enabled ? 'Enabled' : 'Disabled', 
            status: telemetry.beacon_enabled ? 'success' : 'secondary' 
        },
        { 
            label: 'GPS Status', 
            value: telemetry.gps?.valid ? 'Valid' : 'No Fix', 
            status: telemetry.gps?.valid ? 'success' : 'warning' 
        }
    ];

    const html = configs.map(item => `
        <div class="d-flex justify-content-between align-items-center py-2">
            <span class="small">${escapeHtml(item.label)}</span>
            <span class="badge bg-${item.status}">${escapeHtml(item.value)}</span>
        </div>
    `).join('');

    container.innerHTML = html;
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) return `${days}d ${hours}h`;
    if (hours > 0) return `${hours}h ${minutes}m`;
    return `${minutes}m`;
}

function renderStatus(telemetry) {
    renderStatusCards(telemetry);
    renderDeviceInfo(telemetry);
    renderConfigSummary(telemetry);
    updateChart(telemetry);
    
    // Update last update time
    const lastUpdateElement = document.querySelector('[data-last-update]');
    if (lastUpdateElement) {
        lastUpdateElement.textContent = new Date().toLocaleTimeString();
    }
}

function renderActivity(entries) {
    const container = document.querySelector('[data-activity-feed]');
    if (!container) {
        return;
    }

    if (!entries || entries.length === 0) {
        container.innerHTML = `
            <div class="list-group-item text-center py-4 text-muted">
                <i class="bi bi-hourglass-split fs-1 mb-2 d-block"></i>
                No recent activity...
            </div>
        `;
        return;
    }

    const items = entries.slice(0, 10) // Show only last 10 items
        .map((entry) => {
            const time = formatTimestamp(entry.timestamp);
            const { type, payload } = entry;
            let iconClass = 'bi-info-circle';
            let badgeClass = 'bg-secondary';
            let summary = '';

            switch (type) {
                case 'command_result':
                    iconClass = payload?.success ? 'bi-check-circle' : 'bi-exclamation-triangle';
                    badgeClass = payload?.success ? 'bg-success' : 'bg-warning';
                    summary = `Command ${escapeHtml(payload?.command) || 'N/A'}: ${escapeHtml(payload?.message) || ''}`;
                    break;
                case 'config_result':
                    iconClass = payload?.success ? 'bi-check-circle' : 'bi-exclamation-triangle';
                    badgeClass = payload?.success ? 'bg-success' : 'bg-warning';
                    summary = `Config ${escapeHtml(payload?.command) || 'N/A'}: ${escapeHtml(payload?.message) || ''}`;
                    break;
                case 'packet':
                    iconClass = 'bi-arrow-left-right';
                    badgeClass = 'bg-primary';
                    summary = `Packet ${payload?.length ?? '?'} bytes`;
                    if (payload?.rssi != null) {
                        summary += ` (RSSI ${payload.rssi} dBm)`;
                    }
                    break;
                case 'error':
                    iconClass = 'bi-x-circle';
                    badgeClass = 'bg-danger';
                    summary = `Error: ${escapeHtml(payload?.message) || 'Unknown error'}`;
                    break;
                default:
                    summary = `${escapeHtml(type.replace(/_/g, ' '))} event`;
                    break;
            }

            return `
                <div class="list-group-item list-group-item-action d-flex align-items-center">
                    <div class="me-3">
                        <span class="badge ${badgeClass} p-2">
                            <i class="bi ${iconClass}"></i>
                        </span>
                    </div>
                    <div class="flex-grow-1">
                        <div class="fw-medium">${summary}</div>
                        ${time ? `<small class="text-muted">${escapeHtml(time)}</small>` : ''}
                    </div>
                </div>
            `;
        })
        .join('');

    container.innerHTML = items;
}

function updateConnectionStatus(connected) {
    const statusElement = document.querySelector('[data-connection-status]');
    if (!statusElement) return;

    const statusText = statusElement.querySelector('span');
    if (connected) {
        statusElement.className = 'status-indicator online';
        statusText.textContent = 'Connected';
    } else {
        statusElement.className = 'status-indicator offline';
        statusText.textContent = 'Disconnected';
    }
}

function initEventListeners() {
    // Chart type selector
    document.querySelectorAll('input[name="chartType"]').forEach(radio => {
        radio.addEventListener('change', (e) => {
            updateChartDataset(e.target.id);
        });
    });

    // Refresh button
    const refreshBtn = document.querySelector('[data-refresh-btn]');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', () => {
            window.location.reload();
        });
    }

    // Clear activity button
    const clearActivityBtn = document.querySelector('[data-clear-activity]');
    if (clearActivityBtn) {
        clearActivityBtn.addEventListener('click', () => {
            const container = document.querySelector('[data-activity-feed]');
            if (container) {
                container.innerHTML = `
                    <div class="list-group-item text-center py-4 text-muted">
                        <i class="bi bi-hourglass-split fs-1 mb-2 d-block"></i>
                        Activity cleared
                    </div>
                `;
            }
        });
    }

    // Export button (placeholder for future implementation)
    const exportBtn = document.querySelector('[data-export-btn]');
    if (exportBtn) {
        exportBtn.addEventListener('click', () => {
            // TODO: Implement export functionality
            alert('Export functionality coming soon!');
        });
    }
}

function initialiseEventSubscriptions() {
    document.addEventListener(REALTIME_EVENTS.TELEMETRY, (event) => {
        renderStatus(event.detail?.telemetry || null);
    });

    document.addEventListener(REALTIME_EVENTS.ACTIVITY, () => {
        renderActivity(getActivityHistory());
    });

    document.addEventListener(REALTIME_EVENTS.CONNECTION, (event) => {
        updateConnectionStatus(event.detail?.connected || false);
    });
}

// Initialize everything when the page loads
async function fetchStatusData() {
    try {
        console.log('Fetching fresh status data from API...');
        // Import the API function dynamically to avoid circular dependencies
        const { getStatus } = await import('./api.js');
        const statusData = await getStatus();
        console.log('Received status data:', statusData);
        return statusData;
    } catch (error) {
        console.error('Failed to fetch status data:', error);
        return null;
    }
}

function initialize() {
    console.log('Initializing modern dashboard...');
    
    // Initialize the common page functionality
    initPage({ activeNav: 'index' });
    
    // Set up event listeners
    initEventListeners();
    initialiseEventSubscriptions();
    
    // Initialize chart asynchronously to avoid blocking
    console.log('Initializing chart...');
    setTimeout(() => {
        initChart();
    }, 100);
    
    // Show initial loading state
    renderStatus(null); // This will show loading spinners
    renderActivity(null);
    
    // Load initial data from common.js
    console.log('Loading initial telemetry data...');
    const initialTelemetry = getLatestTelemetry();
    console.log('Initial telemetry from common.js:', initialTelemetry);
    
    // Render with whatever we have from common.js
    if (initialTelemetry) {
        renderStatus(initialTelemetry);
    }
    
    const initialActivity = getActivityHistory();
    console.log('Initial activity from common.js:', initialActivity);
    if (initialActivity) {
        renderActivity(initialActivity);
    }
    
    // Always try to fetch fresh data from the API
    console.log('Fetching fresh data from API...');
    fetchStatusData().then(freshData => {
        if (freshData) {
            console.log('Rendering with fresh API data:', freshData);
            renderStatus(freshData);
            updateConnectionStatus(true); // Successfully connected
        } else {
            console.log('No fresh API data available');
            updateConnectionStatus(false);
        }
    }).catch(error => {
        console.error('Error fetching status data:', error);
        // If API fails, show a message but keep any existing data
        updateConnectionStatus(false);
    });
    
    // Set up periodic refresh every 30 seconds (reduced frequency)
    setInterval(async () => {
        try {
            const freshData = await fetchStatusData();
            if (freshData) {
                // Use requestAnimationFrame to ensure rendering doesn't block other tasks
                requestAnimationFrame(() => {
                    renderStatus(freshData);
                    updateConnectionStatus(true);
                });
            } else {
                updateConnectionStatus(false);
            }
        } catch (error) {
            console.error('Periodic refresh failed:', error);
            updateConnectionStatus(false);
        }
    }, 30000); // Increased from 15s to 30s
    
    // Update connection status to show we're trying to connect initially
    updateConnectionStatus(false); // Start as disconnected
    
    // Show loading completed
    console.log('Modern dashboard initialization complete');
}

// Run initialization
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initialize);
} else {
    // DOM is already ready
    initialize();
}