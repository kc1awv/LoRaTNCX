# LoRaTNCX Modern Web Interface

A modern, responsive web interface for the LoRaTNCX amateur radio device featuring real-time monitoring, progressive web app capabilities, and an enhanced user experience.

## ✨ Features

### 🎨 Modern UI/UX
- **Professional Design**: Clean, modern interface with card-based layouts
- **Dark/Light Theme**: Automatic theme switching with manual override
- **Responsive Layout**: Optimized for desktop, tablet, and mobile devices
- **Accessibility**: WCAG compliant with proper ARIA labels and keyboard navigation
- **Smooth Animations**: Subtle transitions and micro-interactions

### 📊 Enhanced Dashboard
- **Real-time Charts**: Live signal strength, packet rates, and error monitoring
- **Status Cards**: At-a-glance system metrics with trend indicators
- **System Monitoring**: Comprehensive device information and health status
- **Activity Feed**: Live activity log with categorized events

### 🚀 Progressive Web App (PWA)
- **Installable**: Add to home screen on mobile devices
- **Offline Support**: Service worker caching for offline functionality
- **Background Sync**: Queue actions when offline, sync when reconnected
- **Push Notifications**: Alert system for device status (future feature)

### 🛠️ Advanced Configuration
- **Quick Settings**: Common configuration options with validation
- **Configuration Presets**: Pre-configured settings for different use cases
- **Command History**: Track and replay previous commands
- **Export/Import**: Save and restore configuration profiles

### 💻 Enhanced Console
- **Terminal-like Interface**: Modern console with syntax highlighting
- **Command History**: Navigate previous commands with arrow keys
- **Quick Commands**: One-click access to common commands
- **Session Statistics**: Track commands sent and responses received

## 🏗️ Architecture

### Frontend Technologies
- **Bootstrap 5**: Modern CSS framework with custom theme
- **Chart.js**: Real-time data visualization
- **Vanilla JavaScript**: Modern ES6+ modules, no framework dependencies
- **Service Worker**: Offline functionality and caching
- **WebSocket**: Real-time bidirectional communication

### File Structure
```
data/
├── index.html              # Modern dashboard
├── status.html             # Enhanced status page  
├── config.html             # Advanced configuration
├── console.html            # Terminal-style console
├── pair.html               # Device pairing
├── 404.html                # Custom error page
├── manifest.json           # PWA manifest
├── sw.js                   # Service worker
├── icon.svg                # App icon
├── css/
│   ├── bootstrap.css       # Bootstrap framework
│   └── modern-theme.css    # Custom modern styling
└── js/
    ├── index-modern.js     # Enhanced dashboard logic
    ├── api.js              # API communication
    ├── common.js           # Shared utilities
    ├── theme.js            # Theme management
    ├── websocket.js        # Real-time communication
    └── [other js files]    # Page-specific scripts
```

### Design System

#### Color Palette
- **Primary**: `#2563eb` (Professional blue)
- **Secondary**: `#64748b` (Neutral gray)
- **Success**: `#10b981` (Green)
- **Warning**: `#f59e0b` (Amber)
- **Danger**: `#ef4444` (Red)
- **Info**: `#06b6d4` (Cyan)

#### Typography
- **Headers**: System font stack with proper hierarchy
- **Body**: Optimized for readability across devices
- **Code**: Monospace font for technical content

#### Components
- **Cards**: Modern card design with subtle shadows
- **Buttons**: Gradient effects with hover animations
- **Forms**: Enhanced form controls with validation
- **Navigation**: Sticky navigation with active indicators

## 📱 Mobile Experience

### Responsive Design
- **Mobile-first**: Optimized layouts for small screens
- **Touch-friendly**: Appropriately sized touch targets
- **Gesture Support**: Swipe navigation where appropriate
- **Performance**: Optimized for mobile networks

### PWA Features
- **Home Screen**: Install as native-like app
- **Splash Screen**: Custom splash screen with branding
- **Standalone Mode**: Full-screen app experience
- **Shortcuts**: Quick access to key features

## 🔧 Development

### Build Process
The existing `build_ui.py` script handles:
- **Minification**: HTML, CSS, and JavaScript optimization
- **Compression**: ZIP archive for SPIFFS deployment
- **Asset Pipeline**: Automated build process

### Customization
- **Theme Variables**: CSS custom properties for easy theming
- **Component System**: Modular CSS classes for consistency
- **Configuration**: Easy to modify colors, spacing, and typography

### Performance Optimizations
- **Lazy Loading**: Chart.js loaded only when needed
- **Efficient Caching**: Service worker caches static assets
- **Minimal Dependencies**: Lightweight external dependencies
- **Code Splitting**: Modular JavaScript architecture

## 🚀 Installation

The modern interface is a drop-in replacement for the existing web files:

1. **Backup Current**: Save existing `data/` directory
2. **Deploy New Files**: Copy new files to `data/` directory  
3. **Build**: Run `python tools/build_ui.py` to generate SPIFFS archive
4. **Upload**: Flash the new filesystem to the device

## 📈 Features by Page

### Dashboard (`index.html`)
- Real-time status cards with trend indicators
- Interactive charts with multiple data views
- System information panel
- Recent activity feed with categorized events
- Connection status indicator

### Status (`status.html`)
- Detailed system metrics
- Health monitoring
- Performance indicators
- Historical data views

### Configuration (`config.html`)
- Quick settings for common parameters
- Configuration presets for different scenarios
- Advanced command interface
- Command history and validation
- Export/import functionality

### Console (`console.html`)
- Terminal-style interface
- Command history with arrow key navigation
- Quick command buttons
- Session statistics
- Real-time output streaming

## 🔮 Future Enhancements

### Planned Features
- **Data Export**: CSV/JSON export of telemetry data
- **Custom Dashboards**: User-configurable layouts
- **Alert System**: Configurable notifications and thresholds
- **Remote Monitoring**: Multi-device management
- **API Documentation**: Interactive API explorer

### Technical Improvements
- **WebRTC**: Peer-to-peer communication for reduced latency
- **IndexedDB**: Client-side data storage and caching
- **Push Notifications**: Real-time alerts via service worker
- **Background Sync**: Reliable command queuing

## 📄 License

This modern web interface maintains compatibility with the existing LoRaTNCX project license and structure.