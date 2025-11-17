#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>
#include <type_traits>

/**
 * @brief Standardized error codes for the LoRaTNCX system
 *
 * This enum class provides consistent error handling across all modules.
 * Error codes are categorized by subsystem for easier debugging.
 */
enum class ErrorCode {
    // Success
    SUCCESS = 0,

    // General errors
    UNKNOWN_ERROR = 1,
    INVALID_PARAMETER = 2,
    NOT_INITIALIZED = 3,
    ALREADY_INITIALIZED = 4,
    TIMEOUT = 5,
    OUT_OF_MEMORY = 6,
    NOT_SUPPORTED = 7,

    // Hardware errors
    HARDWARE_ERROR = 100,
    BOARD_UNKNOWN = 101,
    RADIO_INIT_FAILED = 102,
    GPIO_ERROR = 103,
    I2C_ERROR = 104,
    SPI_ERROR = 105,

    // Network/WiFi errors
    NETWORK_ERROR = 200,
    WIFI_INIT_FAILED = 201,
    WIFI_CONNECT_FAILED = 202,
    WIFI_DISCONNECTED = 203,
    TCP_CONNECT_FAILED = 204,
    TCP_SEND_FAILED = 205,
    TCP_RECEIVE_FAILED = 206,
    DNS_RESOLVE_FAILED = 207,
    HTTP_ERROR = 208,
    TCP_SERVER_INIT_FAILED = 209,
    NMEA_SERVER_INIT_FAILED = 210,

    // File system errors
    FILESYSTEM_ERROR = 300,
    FILE_NOT_FOUND = 301,
    FILE_OPEN_FAILED = 302,
    FILE_READ_FAILED = 303,
    FILE_WRITE_FAILED = 304,
    FILESYSTEM_FULL = 305,

    // Configuration errors
    CONFIG_ERROR = 400,
    CONFIG_LOAD_FAILED = 401,
    CONFIG_SAVE_FAILED = 402,
    CONFIG_INVALID = 403,
    CONFIG_VERSION_MISMATCH = 404,

    // GNSS errors
    GNSS_ERROR = 500,
    GNSS_INIT_FAILED = 501,
    GNSS_NO_FIX = 502,
    GNSS_TIMEOUT = 503,
    GNSS_INVALID_DATA = 504,

    // Protocol errors
    PROTOCOL_ERROR = 600,
    KISS_FRAME_INVALID = 601,
    JSON_PARSE_ERROR = 602,
    SERIAL_COMM_ERROR = 603,

    // Radio/LoRa errors
    RADIO_ERROR = 700,
    RADIO_TX_FAILED = 701,
    RADIO_RX_FAILED = 702,
    RADIO_BUSY = 703,
    RADIO_INVALID_CONFIG = 704,

    // Web server errors
    WEBSERVER_ERROR = 800,
    WEBSERVER_INIT_FAILED = 801,
    WEBSERVER_REQUEST_INVALID = 802,
    WEBSERVER_AUTH_FAILED = 803,

    // System errors
    SYSTEM_ERROR = 900,
    WATCHDOG_TIMEOUT = 901,
    TASK_CREATE_FAILED = 902,
    MUTEX_ERROR = 903,
    QUEUE_ERROR = 904
};

/**
 * @brief Get a human-readable string for an error code
 *
 * @param code The error code to convert
 * @return const char* Human-readable error message
 */
const char* errorCodeToString(ErrorCode code);

/**
 * @brief Result<T> template class for consistent error handling
 *
 * This class provides a standardized way to return either a successful result
 * or an error code. It follows the Result pattern commonly used in Rust and other languages.
 *
 * Usage examples:
 *   Result<int> successResult(42);
 *   Result<int> errorResult(ErrorCode::INVALID_PARAMETER);
 *
 *   if (successResult.isOk()) {
 *       int value = successResult.unwrap();
 *   } else {
 *       ErrorCode error = successResult.error();
 *   }
 */
template<typename T>
class Result {
private:
    bool _isOk;
    union {
        T _value;
        ErrorCode _error;
    };

public:
    /**
     * @brief Construct a successful result
     *
     * @param value The successful value
     */
    Result(const T& value) : _isOk(true), _value(value) {}

    /**
     * @brief Construct a successful result (move constructor)
     *
     * @param value The successful value to move
     */
    Result(T&& value) : _isOk(true), _value(std::move(value)) {}

    /**
     * @brief Construct an error result
     *
     * @param error The error code
     */
    Result(ErrorCode error) : _isOk(false), _error(error) {}

    /**
     * @brief Copy constructor
     */
    Result(const Result& other) : _isOk(other._isOk) {
        if (_isOk) {
            new (&_value) T(other._value);
        } else {
            _error = other._error;
        }
    }

    /**
     * @brief Move constructor
     */
    Result(Result&& other) noexcept : _isOk(other._isOk) {
        if (_isOk) {
            new (&_value) T(std::move(other._value));
        } else {
            _error = other._error;
        }
    }

    /**
     * @brief Destructor
     */
    ~Result() {
        if (_isOk) {
            _value.~T();
        }
    }

    /**
     * @brief Check if the result is successful
     *
     * @return true if successful, false if error
     */
    bool isOk() const { return _isOk; }

    /**
     * @brief Check if the result is an error
     *
     * @return true if error, false if successful
     */
    bool isErr() const { return !_isOk; }

    /**
     * @brief Get the successful value (only call if isOk() returns true)
     *
     * @return const T& The successful value
     */
    const T& unwrap() const {
        if (!_isOk) {
            // In a real implementation, this might panic or throw
            // For embedded systems, we'll just return a default value
            static T defaultValue{};
            return defaultValue;
        }
        return _value;
    }

    /**
     * @brief Get the successful value as modifiable reference (only call if isOk() returns true)
     *
     * @return T& The successful value
     */
    T& unwrap() {
        if (!_isOk) {
            static T defaultValue{};
            return defaultValue;
        }
        return _value;
    }

    /**
     * @brief Get the error code (only call if isErr() returns true)
     *
     * @return ErrorCode The error code
     */
    ErrorCode error() const {
        if (_isOk) {
            return ErrorCode::UNKNOWN_ERROR;
        }
        return _error;
    }

    /**
     * @brief Get the successful value with a default fallback
     *
     * @param defaultValue The default value to return on error
     * @return const T& The successful value or default
     */
    const T& unwrapOr(const T& defaultValue) const {
        return _isOk ? _value : defaultValue;
    }

    /**
     * @brief Apply a function to the successful value
     *
     * @tparam U The return type of the function
     * @param func The function to apply
     * @return Result<U> The result of applying the function
     */
    template<typename U>
    Result<U> map(std::function<U(const T&)> func) const {
        if (_isOk) {
            return Result<U>(func(_value));
        } else {
            return Result<U>(_error);
        }
    }

    /**
     * @brief Chain operations that may fail
     *
     * @tparam U The return type of the function
     * @param func The function to apply (returns Result<U>)
     * @return Result<U> The chained result
     */
    template<typename U>
    Result<U> andThen(std::function<Result<U>(const T&)> func) const {
        if (_isOk) {
            return func(_value);
        } else {
            return Result<U>(_error);
        }
    }
};

/**
 * @brief Specialization of Result for void type
 *
 * This allows Result<void> for operations that don't return a value but may fail.
 */
template<>
class Result<void> {
private:
    bool _isOk;
    ErrorCode _error;

public:
    /**
     * @brief Construct a successful void result
     */
    Result() : _isOk(true), _error(ErrorCode::SUCCESS) {}

    /**
     * @brief Construct an error result
     *
     * @param error The error code
     */
    Result(ErrorCode error) : _isOk(false), _error(error) {}

    /**
     * @brief Check if the result is successful
     *
     * @return true if successful, false if error
     */
    bool isOk() const { return _isOk; }

    /**
     * @brief Check if the result is an error
     *
     * @return true if error, false if successful
     */
    bool isErr() const { return !_isOk; }

    /**
     * @brief Get the error code (only call if isErr() returns true)
     *
     * @return ErrorCode The error code
     */
    ErrorCode error() const {
        return _error;
    }

    /**
     * @brief Apply a function to the successful void result
     *
     * @tparam U The return type of the function
     * @param func The function to apply
     * @return Result<U> The result of applying the function
     */
    template<typename U>
    Result<U> map(std::function<U()> func) const {
        if (_isOk) {
            return Result<U>(func());
        } else {
            return Result<U>(_error);
        }
    }

    /**
     * @brief Chain operations that may fail
     *
     * @tparam U The return type of the function
     * @param func The function to apply (returns Result<U>)
     * @return Result<U> The chained result
     */
    template<typename U>
    Result<U> andThen(std::function<Result<U>()> func) const {
        if (_isOk) {
            return func();
        } else {
            return Result<U>(_error);
        }
    }
};

#endif // ERROR_HANDLING_H