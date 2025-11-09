// Temporary debug build flag
// Remove this file and rebuild for production (silent KISS mode)

#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_MODE 1

#ifdef DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif // DEBUG_H
