# Web Interface API

This document summarizes the REST-style endpoints exposed by the asynchronous web
interface. These routes are intended for tooling integrations that want to
monitor runtime state, update configuration presets, or submit console-style
commands remotely.

## Common characteristics

- All endpoints live under the device host (the captive portal IP or STA
  address) and return JSON payloads.
- Requests and responses use UTF-8 encoding.
- Clients should send `Content-Type: application/json` when providing a body.
- Error responses include an `error` field or a `message` field describing the
  problem so that UI layers can surface user-friendly validation details.

## `GET /api/status`

Returns a snapshot of the current TNC status along with Wi-Fi information.

```json
{
  "wifi": {
    "sta_connected": true,
    "ap_ssid": "LoRaTNCX-XXXXXX"
  },
  "tnc": {
    "available": true,
    "status_text": "... multi-line status ...",
    "display": {
      "mode": "COMMAND",
      "tx_count": 12,
      "rx_count": 34,
      "last_packet_millis": 1234567,
      "has_recent_packet": true,
      "last_rssi": -98.5,
      "last_snr": 7.2,
      "frequency_mhz": 915.0,
      "bandwidth_khz": 125.0,
      "spreading_factor": 7,
      "coding_rate": 5,
      "tx_power_dbm": 10,
      "uptime_ms": 12345678,
      "battery": {
        "voltage": 4.1,
        "percent": 87
      },
      "power_off": {
        "active": false,
        "progress": 0.0,
        "complete": false
      },
      "gnss": {
        "enabled": true,
        "has_fix": true,
        "is_3d_fix": true,
        "latitude": 42.123456,
        "longitude": -71.987654,
        "altitude_m": 90.0,
        "speed_knots": 0.0,
        "course_degrees": 0.0,
        "hdop": 0.9,
        "satellites": 10,
        "time_valid": true,
        "time_synced": true,
        "year": 2025,
        "month": 10,
        "day": 28,
        "hour": 12,
        "minute": 34,
        "second": 56,
        "pps_available": true,
        "pps_last_millis": 1234500,
        "pps_count": 42
      }
    }
  }
}
```

Latitude/longitude/altitude fields are set to `null` when GNSS data is not
available. The `status_text` value mirrors the serial console output that would
normally be printed by the firmware.

## `GET /api/config`

Provides human-readable information about the active LoRa configuration preset.
The response includes `config.available` (boolean) and a `status_text` field that
mirrors the serial command output of `GETCONFIG`.

```json
{
  "config": {
    "available": true,
    "status_text": "Current LoRa Configuration Status:\r\n..."
  }
}
```

## `POST /api/config`

Accepts a JSON body with a `command` string. The command is forwarded to the
configuration manager (`SETCONFIG`, `GETCONFIG`, `LISTCONFIG`, etc.).

### Request

```http
POST /api/config
Content-Type: application/json

{"command": "SETCONFIG balanced"}
```

### Successful response

```json
{
  "success": true,
  "message": "Configuration command accepted.",
  "status_text": "Current LoRa Configuration Status:\r\n..."
}
```

### Failure response

If the command fails validation or is malformed, the endpoint returns HTTP 400
with `success: false` and a message indicating that the command was rejected.
Other HTTP status codes (for example `503` when the TNC manager is offline)
reflect transport-level or firmware errors.

## `POST /api/command`

Executes a general console command through the TNC command subsystem. The body
schema matches `/api/config` (`{"command": "..."}`), but the response encodes
structured result metadata.

```json
{
  "success": false,
  "result": "ERROR_INVALID_PARAMETER",
  "message": "Invalid parameter."
}
```

The HTTP status code mirrors the semantic result so that clients can distinguish
between missing commands (`404`), validation problems (`400`), and device errors
(`500`/`503`). On success, `result` will be either `SUCCESS` or
`SUCCESS_SILENT` and the response is delivered with HTTP 200.
