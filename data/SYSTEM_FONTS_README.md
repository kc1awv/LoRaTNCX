# System Font Configuration

## Overview
This web interface now uses native system monospace fonts instead of custom web fonts. This provides:

- ⚡ **Instant loading** - No font files to download
- 🎯 **Zero FOUC** - Text appears immediately 
- 💾 **Reduced size** - Saves 1.5MB of storage space
- 🖥️ **Native feel** - Uses fonts optimized for each platform

## Font Stack Priority

The interface will use the best available monospace font on each platform:

### macOS/iOS
- **SF Mono** - Apple's modern system monospace (macOS 10.12+)
- **Monaco** - Classic macOS monospace font
- **Menlo** - Legacy macOS terminal font

### Windows
- **Consolas** - Microsoft's ClearType monospace font (excellent readability)

### Android
- **Roboto Mono** - Google's monospace font

### Linux
- **Ubuntu Mono** - Ubuntu's system monospace
- **DejaVu Sans Mono** - Common Linux monospace
- **Liberation Mono** - Red Hat's open-source monospace

### Universal Fallbacks
- **Courier New** - Available on all systems
- **monospace** - Browser's default monospace

## Benefits

### Performance
- **0ms** font loading time
- No network requests for fonts
- Immediate text rendering
- No FOUC (Flash of Unstyled Content)

### User Experience  
- Familiar fonts users recognize
- Optimized for each operating system
- Consistent with terminal/coding applications
- Better readability on all devices

### Technical
- Reduced SPIFFS usage (saved 1.5MB)
- Simplified CSS (no font loading logic)
- Better caching (system fonts are cached permanently)
- Cross-platform compatibility

## Font Characteristics

All fonts in the stack share these TNC-appropriate characteristics:
- **Monospaced** - Fixed-width characters for data alignment
- **High readability** - Designed for long reading sessions
- **Terminal heritage** - Feel familiar to TNC users
- **Unicode support** - Handle special characters properly

## Styling Enhancements

Added TNC-specific CSS classes:
- `.tnc-terminal` - Dark terminal-style blocks
- `.tnc-status` - Status information styling
- Enhanced form controls with dark theme
- Terminal-appropriate color schemes

## Result

Users now get:
- Immediate, crisp text rendering
- Platform-native monospace fonts
- Professional terminal appearance
- Zero loading delays

The interface maintains its technical aesthetic while loading instantly on any device.