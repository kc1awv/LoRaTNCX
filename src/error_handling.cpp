#include "error_handling.h"

const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::UNKNOWN_ERROR:
            return "Unknown error";
        case ErrorCode::INVALID_PARAMETER:
            return "Invalid parameter";
        case ErrorCode::NOT_INITIALIZED:
            return "Not initialized";
        case ErrorCode::ALREADY_INITIALIZED:
            return "Already initialized";
        case ErrorCode::TIMEOUT:
            return "Timeout";
        case ErrorCode::OUT_OF_MEMORY:
            return "Out of memory";
        case ErrorCode::NOT_SUPPORTED:
            return "Not supported";

        case ErrorCode::HARDWARE_ERROR:
            return "Hardware error";
        case ErrorCode::BOARD_UNKNOWN:
            return "Unknown board type";
        case ErrorCode::RADIO_INIT_FAILED:
            return "Radio initialization failed";
        case ErrorCode::GPIO_ERROR:
            return "GPIO error";
        case ErrorCode::I2C_ERROR:
            return "I2C error";
        case ErrorCode::SPI_ERROR:
            return "SPI error";

        case ErrorCode::NETWORK_ERROR:
            return "Network error";
        case ErrorCode::WIFI_INIT_FAILED:
            return "WiFi initialization failed";
        case ErrorCode::WIFI_CONNECT_FAILED:
            return "WiFi connection failed";
        case ErrorCode::WIFI_DISCONNECTED:
            return "WiFi disconnected";
        case ErrorCode::TCP_CONNECT_FAILED:
            return "TCP connection failed";
        case ErrorCode::TCP_SEND_FAILED:
            return "TCP send failed";
        case ErrorCode::TCP_RECEIVE_FAILED:
            return "TCP receive failed";
        case ErrorCode::DNS_RESOLVE_FAILED:
            return "DNS resolution failed";
        case ErrorCode::HTTP_ERROR:
            return "HTTP error";
        case ErrorCode::TCP_SERVER_INIT_FAILED:
            return "TCP server initialization failed";
        case ErrorCode::NMEA_SERVER_INIT_FAILED:
            return "NMEA server initialization failed";

        case ErrorCode::FILESYSTEM_ERROR:
            return "Filesystem error";
        case ErrorCode::FILE_NOT_FOUND:
            return "File not found";
        case ErrorCode::FILE_OPEN_FAILED:
            return "File open failed";
        case ErrorCode::FILE_READ_FAILED:
            return "File read failed";
        case ErrorCode::FILE_WRITE_FAILED:
            return "File write failed";
        case ErrorCode::FILESYSTEM_FULL:
            return "Filesystem full";

        case ErrorCode::CONFIG_ERROR:
            return "Configuration error";
        case ErrorCode::CONFIG_LOAD_FAILED:
            return "Configuration load failed";
        case ErrorCode::CONFIG_SAVE_FAILED:
            return "Configuration save failed";
        case ErrorCode::CONFIG_INVALID:
            return "Configuration invalid";
        case ErrorCode::CONFIG_VERSION_MISMATCH:
            return "Configuration version mismatch";

        case ErrorCode::GNSS_ERROR:
            return "GNSS error";
        case ErrorCode::GNSS_INIT_FAILED:
            return "GNSS initialization failed";
        case ErrorCode::GNSS_NO_FIX:
            return "GNSS no fix";
        case ErrorCode::GNSS_TIMEOUT:
            return "GNSS timeout";
        case ErrorCode::GNSS_INVALID_DATA:
            return "GNSS invalid data";

        case ErrorCode::PROTOCOL_ERROR:
            return "Protocol error";
        case ErrorCode::KISS_FRAME_INVALID:
            return "Invalid KISS frame";
        case ErrorCode::JSON_PARSE_ERROR:
            return "JSON parse error";
        case ErrorCode::SERIAL_COMM_ERROR:
            return "Serial communication error";

        case ErrorCode::RADIO_ERROR:
            return "Radio error";
        case ErrorCode::RADIO_TX_FAILED:
            return "Radio transmit failed";
        case ErrorCode::RADIO_RX_FAILED:
            return "Radio receive failed";
        case ErrorCode::RADIO_BUSY:
            return "Radio busy";
        case ErrorCode::RADIO_INVALID_CONFIG:
            return "Radio invalid configuration";

        case ErrorCode::WEBSERVER_ERROR:
            return "Web server error";
        case ErrorCode::WEBSERVER_INIT_FAILED:
            return "Web server initialization failed";
        case ErrorCode::WEBSERVER_REQUEST_INVALID:
            return "Invalid web server request";
        case ErrorCode::WEBSERVER_AUTH_FAILED:
            return "Web server authentication failed";

        case ErrorCode::SYSTEM_ERROR:
            return "System error";
        case ErrorCode::WATCHDOG_TIMEOUT:
            return "Watchdog timeout";
        case ErrorCode::TASK_CREATE_FAILED:
            return "Task creation failed";
        case ErrorCode::MUTEX_ERROR:
            return "Mutex error";
        case ErrorCode::QUEUE_ERROR:
            return "Queue error";

        default:
            return "Unknown error code";
    }
}