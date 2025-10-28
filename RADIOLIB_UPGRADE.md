# RadioLib Version Update

## Update: October 28, 2025

### Upgraded from RadioLib 6.6.0 to 7.4.0

**Why the update?**
- RadioLib 7.4.0 was released on October 26, 2025 (just 2 days ago)
- Using an old version (6.6.0 from May 2024) meant missing out on 5+ months of improvements
- Better to stay current with latest bug fixes and features

**What's New in 7.x Series:**
- Enhanced HAL (Hardware Abstraction Layer) system
- Improved SX126x support with new command and configuration modules
- Enhanced LR11x0 support with crypto, GNSS, and WiFi functionality
- Better error handling and utilities
- Performance improvements and bug fixes

**Impact on LoRaTNCX:**
- ✅ **No Breaking Changes**: All existing code compiled without modification
- ✅ **Minimal Memory Impact**: Only 56 bytes more RAM, 7KB more flash
- ✅ **Future-Proof**: Access to latest RadioLib features as we expand functionality
- ✅ **Better Support**: Current library with active development and support

**Memory Usage Comparison:**
```
RadioLib 6.6.0: RAM 3.7% (19260 bytes), Flash 4.4% (289665 bytes)
RadioLib 7.4.0: RAM 3.7% (19316 bytes), Flash 4.5% (297069 bytes)
Difference:     +56 bytes RAM,         +7404 bytes Flash
```

**Potential New Features Available:**
- Enhanced SX126x LR-FHSS support
- Improved command interface for SX126x
- Better configuration management
- Enhanced crypto support (for future security features)
- GNSS integration support (useful for our APRS goals)

**Recommendation:**
✅ **Keep 7.4.0** - The benefits far outweigh the minimal memory cost, and we're now on the bleeding edge of RadioLib development.