/**
 * Lightweight Chart Implementation for LoRaTNCX
 * A simple, self-contained charting solution that doesn't require external CDNs
 */

class SimpleChart {
    constructor(canvas, options = {}) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.options = {
            responsive: true,
            maintainAspectRatio: false,
            padding: 20,
            gridColor: '#e2e8f0',
            textColor: '#64748b',
            ...options
        };
        this.data = {
            labels: [],
            datasets: []
        };
        
        this.resizeChart();
        if (this.options.responsive) {
            window.addEventListener('resize', () => this.resizeChart());
        }
    }
    
    resizeChart() {
        const container = this.canvas.parentElement;
        const rect = container.getBoundingClientRect();
        this.canvas.width = rect.width;
        this.canvas.height = rect.height;
        this.draw();
    }
    
    updateData(data) {
        this.data = data;
        this.draw();
    }
    
    update(mode = 'default') {
        this.draw();
    }
    
    draw() {
        const { ctx, canvas, options } = this;
        const { width, height } = canvas;
        const padding = options.padding;
        
        // Clear canvas
        ctx.clearRect(0, 0, width, height);
        
        // Get dataset
        const dataset = this.data.datasets[0];
        if (!dataset || !dataset.data || dataset.data.length === 0) {
            this.drawNoData();
            return;
        }
        
        // Calculate drawing area
        const chartWidth = width - (padding * 2);
        const chartHeight = height - (padding * 2);
        const chartX = padding;
        const chartY = padding;
        
        // Find data range
        const values = dataset.data.filter(v => v !== null && v !== undefined);
        if (values.length === 0) {
            this.drawNoData();
            return;
        }
        
        const minValue = Math.min(...values);
        const maxValue = Math.max(...values);
        const valueRange = maxValue - minValue;
        
        // Draw grid
        this.drawGrid(chartX, chartY, chartWidth, chartHeight);
        
        // Draw line
        this.drawLine(dataset, chartX, chartY, chartWidth, chartHeight, minValue, valueRange);
        
        // Draw current value
        this.drawCurrentValue(dataset);
    }
    
    drawGrid(x, y, width, height) {
        const { ctx, options } = this;
        
        ctx.strokeStyle = options.gridColor;
        ctx.lineWidth = 1;
        ctx.setLineDash([2, 2]);
        
        // Horizontal grid lines
        for (let i = 0; i <= 4; i++) {
            const lineY = y + (height / 4) * i;
            ctx.beginPath();
            ctx.moveTo(x, lineY);
            ctx.lineTo(x + width, lineY);
            ctx.stroke();
        }
        
        ctx.setLineDash([]);
    }
    
    drawLine(dataset, chartX, chartY, chartWidth, chartHeight, minValue, valueRange) {
        const { ctx } = this;
        const data = dataset.data;
        const pointCount = data.length;
        
        if (pointCount < 2) return;
        
        // Set line style
        ctx.strokeStyle = dataset.borderColor || '#2563eb';
        ctx.lineWidth = 2;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';
        
        // Fill area if specified
        if (dataset.fill && dataset.backgroundColor) {
            ctx.fillStyle = dataset.backgroundColor;
            ctx.beginPath();
            
            for (let i = 0; i < pointCount; i++) {
                const value = data[i];
                if (value !== null && value !== undefined) {
                    const x = chartX + (chartWidth / (pointCount - 1)) * i;
                    const y = chartY + chartHeight - ((value - minValue) / valueRange) * chartHeight;
                    
                    if (i === 0) {
                        ctx.moveTo(x, y);
                    } else {
                        ctx.lineTo(x, y);
                    }
                }
            }
            
            // Complete the fill area
            const lastX = chartX + chartWidth;
            const firstX = chartX;
            ctx.lineTo(lastX, chartY + chartHeight);
            ctx.lineTo(firstX, chartY + chartHeight);
            ctx.closePath();
            ctx.fill();
        }
        
        // Draw line
        ctx.strokeStyle = dataset.borderColor || '#2563eb';
        ctx.beginPath();
        
        let firstPoint = true;
        for (let i = 0; i < pointCount; i++) {
            const value = data[i];
            if (value !== null && value !== undefined) {
                const x = chartX + (chartWidth / (pointCount - 1)) * i;
                const y = chartY + chartHeight - ((value - minValue) / valueRange) * chartHeight;
                
                if (firstPoint) {
                    ctx.moveTo(x, y);
                    firstPoint = false;
                } else {
                    ctx.lineTo(x, y);
                }
            }
        }
        
        ctx.stroke();
        
        // Draw points on hover (simplified - just draw last point)
        if (data.length > 0) {
            const lastValue = data[data.length - 1];
            if (lastValue !== null && lastValue !== undefined) {
                const x = chartX + chartWidth;
                const y = chartY + chartHeight - ((lastValue - minValue) / valueRange) * chartHeight;
                
                ctx.fillStyle = dataset.borderColor || '#2563eb';
                ctx.beginPath();
                ctx.arc(x, y, 3, 0, Math.PI * 2);
                ctx.fill();
            }
        }
    }
    
    drawCurrentValue(dataset) {
        const { ctx, canvas, options } = this;
        const data = dataset.data;
        
        if (data.length === 0) return;
        
        const currentValue = data[data.length - 1];
        if (currentValue === null || currentValue === undefined) return;
        
        // Draw current value in top-right corner
        const text = `${currentValue}${dataset.unit || ''}`;
        ctx.font = '14px system-ui, -apple-system, sans-serif';
        ctx.fillStyle = options.textColor;
        ctx.textAlign = 'right';
        ctx.textBaseline = 'top';
        ctx.fillText(text, canvas.width - 10, 10);
    }
    
    drawNoData() {
        const { ctx, canvas, options } = this;
        
        ctx.font = '12px system-ui, -apple-system, sans-serif';
        ctx.fillStyle = options.textColor;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('No data available', canvas.width / 2, canvas.height / 2);
    }
}

// Export for use in modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = SimpleChart;
} else {
    window.SimpleChart = SimpleChart;
}