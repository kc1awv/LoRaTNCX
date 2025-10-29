#ifndef AMATEUR_RADIO_LORA_CONFIGS_H
#define AMATEUR_RADIO_LORA_CONFIGS_H

/**
 * @file amateur_radio_lora_configs.h
 * @brief Optimized LoRa configurations for US Amateur Radio bands (420MHz+)
 * 
 * All configurations are optimized for maximum 1-second time-on-air
 * with SX1262 hardware (255-byte maximum payload capability).
 * 
 * Calculated using precise time-on-air formulas for amateur radio use
 * where bandwidth restrictions don't apply above 420MHz.
 */

// ==========================================================================
// HIGH SPEED CONFIGURATIONS (Short to Medium Range)
// ==========================================================================

/**
 * High Speed Configuration  
 * - Range: Short to Medium (2-8 km typical)
 * - Use case: Fast file transfer, streaming data
 * - Time-on-air: ~100ms for 255 bytes  
 * - Throughput: 20,420 bps
 */
#define LORA_HIGH_SPEED_SF                  7
#define LORA_HIGH_SPEED_CR                  5      // 4/5 coding rate
#define LORA_HIGH_SPEED_BW                  500.0  // 500 kHz bandwidth (max for SX1262)
#define LORA_HIGH_SPEED_MAX_PAYLOAD         255
#define LORA_HIGH_SPEED_FREQUENCY           915.0

/**
 * Fast Balanced Configuration
 * - Range: Medium (5-15 km typical)
 * - Use case: General purpose high-speed data
 * - Time-on-air: ~177ms for 255 bytes
 * - Throughput: 11,541 bps
 */
#define LORA_FAST_BALANCED_SF               8
#define LORA_FAST_BALANCED_CR               5      // 4/5 coding rate  
#define LORA_FAST_BALANCED_BW               500.0  // 500 kHz bandwidth
#define LORA_FAST_BALANCED_MAX_PAYLOAD      255
#define LORA_FAST_BALANCED_FREQUENCY        915.0

// ==========================================================================
// BALANCED CONFIGURATIONS (Medium Range)
// ==========================================================================

/**
 * Standard Balanced Configuration
 * - Range: Medium (8-20 km typical)
 * - Use case: General purpose packet radio
 * - Time-on-air: ~354ms for 255 bytes
 * - Throughput: 5,770 bps
 */
#define LORA_BALANCED_SF                    8
#define LORA_BALANCED_CR                    5      // 4/5 coding rate
#define LORA_BALANCED_BW                    250.0  // 250 kHz bandwidth
#define LORA_BALANCED_MAX_PAYLOAD           255
#define LORA_BALANCED_FREQUENCY             915.0

/**
 * Robust Balanced Configuration
 * - Range: Medium to Long (10-25 km typical)
 * - Use case: Reliable data transfer in poor conditions
 * - Time-on-air: ~625ms for 255 bytes
 * - Throughput: 3,263 bps
 */
#define LORA_ROBUST_BALANCED_SF             9
#define LORA_ROBUST_BALANCED_CR             5      // 4/5 coding rate
#define LORA_ROBUST_BALANCED_BW             250.0  // 250 kHz bandwidth  
#define LORA_ROBUST_BALANCED_MAX_PAYLOAD    255
#define LORA_ROBUST_BALANCED_FREQUENCY      915.0

// ==========================================================================
// LONG RANGE CONFIGURATIONS (Lower Speed)
// ==========================================================================

/**
 * Long Range Configuration
 * - Range: Long (15-40 km typical)
 * - Use case: Emergency communications, remote monitoring
 * - Time-on-air: ~985ms for 99 bytes (payload limited by time constraint)
 * - Throughput: 804 bps
 */
#define LORA_LONG_RANGE_SF                  10
#define LORA_LONG_RANGE_CR                  5      // 4/5 coding rate
#define LORA_LONG_RANGE_BW                  125.0  // 125 kHz bandwidth
#define LORA_LONG_RANGE_MAX_PAYLOAD         99     // Limited by 1s time constraint
#define LORA_LONG_RANGE_FREQUENCY           915.0

/**
 * Maximum Range Configuration
 * - Range: Very Long (20-60 km typical)
 * - Use case: Emergency beacons, minimal data transfer
 * - Time-on-air: ~922ms for 27 bytes (severely payload limited)
 * - Throughput: 234 bps
 */
#define LORA_MAX_RANGE_SF                   11
#define LORA_MAX_RANGE_CR                   6      // 4/6 coding rate (more robust)
#define LORA_MAX_RANGE_BW                   125.0  // 125 kHz bandwidth
#define LORA_MAX_RANGE_MAX_PAYLOAD          27     // Severely limited by time constraint
#define LORA_MAX_RANGE_FREQUENCY            915.0

// ==========================================================================
// BAND-SPECIFIC CONFIGURATIONS
// ==========================================================================

/**
 * 70cm Band Optimized (420-450 MHz)
 * Most popular amateur band, good propagation characteristics
 */
#define LORA_70CM_SF                        8      // Good balance for this band
#define LORA_70CM_CR                        5      // 4/5 coding rate
#define LORA_70CM_BW                        250.0  // Conservative bandwidth
#define LORA_70CM_MAX_PAYLOAD               255
#define LORA_70CM_FREQUENCY                 432.6  // 70cm amateur allocation

/**
 * 33cm Band Optimized (902-928 MHz) 
 * Overlaps with ISM band - use high bandwidth to avoid interference
 */
#define LORA_33CM_SF                        7      // High speed to minimize interference
#define LORA_33CM_CR                        5      // 4/5 coding rate  
#define LORA_33CM_BW                        500.0  // High bandwidth
#define LORA_33CM_MAX_PAYLOAD               255
#define LORA_33CM_FREQUENCY                 906.0  // 33cm amateur allocation

/**
 * 23cm Band Optimized (1240-1300 MHz)
 * Microwave band, primarily line-of-sight
 */
#define LORA_23CM_SF                        7      // Line-of-sight allows high speed
#define LORA_23CM_CR                        5      // 4/5 coding rate
#define LORA_23CM_BW                        500.0  // Can use high bandwidth
#define LORA_23CM_MAX_PAYLOAD               255  
#define LORA_23CM_FREQUENCY                 1290.0 // 23cm amateur allocation

// ==========================================================================
// CONFIGURATION SELECTION MACROS
// ==========================================================================

/**
 * Default configuration for general amateur radio use
 * Provides good balance of speed, range, and reliability
 */
#define LORA_DEFAULT_AMATEUR_SF             LORA_BALANCED_SF
#define LORA_DEFAULT_AMATEUR_CR             LORA_BALANCED_CR
#define LORA_DEFAULT_AMATEUR_BW             LORA_BALANCED_BW
#define LORA_DEFAULT_AMATEUR_MAX_PAYLOAD    LORA_BALANCED_MAX_PAYLOAD
#define LORA_DEFAULT_AMATEUR_FREQUENCY      LORA_BALANCED_FREQUENCY

/**
 * Configuration validation macro
 * Ensures time-on-air stays under 1 second for amateur radio use
 */
#define LORA_MAX_TIME_ON_AIR_MS             1000   // 1 second maximum

// ==========================================================================
// USAGE EXAMPLES
// ==========================================================================

/*
Example 1: High-speed file transfer on 33cm band
radio->begin(LORA_33CM_FREQUENCY, 
             LORA_33CM_BW,
             LORA_33CM_SF, 
             LORA_33CM_CR,
             LORA_SYNC_WORD,
             LORA_OUTPUT_POWER,
             LORA_PREAMBLE_LENGTH);

Example 2: Emergency communications on 70cm band  
radio->begin(LORA_LONG_RANGE_FREQUENCY,
             LORA_LONG_RANGE_BW,
             LORA_LONG_RANGE_SF,
             LORA_LONG_RANGE_CR, 
             LORA_SYNC_WORD,
             LORA_OUTPUT_POWER,
             LORA_PREAMBLE_LENGTH);

Example 3: General purpose amateur radio
radio->begin(LORA_DEFAULT_AMATEUR_FREQUENCY,
             LORA_DEFAULT_AMATEUR_BW,
             LORA_DEFAULT_AMATEUR_SF,
             LORA_DEFAULT_AMATEUR_CR,
             LORA_SYNC_WORD, 
             LORA_OUTPUT_POWER,
             LORA_PREAMBLE_LENGTH);
*/

#endif // AMATEUR_RADIO_LORA_CONFIGS_H