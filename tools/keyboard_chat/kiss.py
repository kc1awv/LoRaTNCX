"""Minimal KISS encoder/decoder utilities.

This implements basic KISS framing and byte-stuffing for DATA frames.
Only what's needed for a simple chat client is implemented.
"""

FEND = 0xC0
FESC = 0xDB
TFEND = 0xDC
TFESC = 0xDD

# KISS commands (matching firmware include/config.h)
CMD_DATA = 0x00
CMD_TXDELAY = 0x01
CMD_P = 0x02
CMD_SLOTTIME = 0x03
CMD_TXTAIL = 0x04
CMD_FULLDUPLEX = 0x05
CMD_SETHARDWARE = 0x06
CMD_GETHARDWARE = 0x07
CMD_RETURN = 0xFF

# SETHARDWARE subcommands
HW_SET_FREQUENCY = 0x01
HW_SET_BANDWIDTH = 0x02
HW_SET_SPREADING = 0x03
HW_SET_CODINGRATE = 0x04
HW_SET_POWER = 0x05
HW_GET_CONFIG = 0x06
HW_SAVE_CONFIG = 0x07
HW_SET_SYNCWORD = 0x08
HW_SET_GNSS_ENABLE = 0x09
HW_RESET_CONFIG = 0xFF

# GETHARDWARE subcommands (queries)
HW_QUERY_CONFIG = 0x01
HW_QUERY_BATTERY = 0x02
HW_QUERY_BOARD = 0x03
HW_QUERY_GNSS = 0x04
HW_QUERY_ALL = 0xFF


def _escape(data: bytes) -> bytes:
    out = bytearray()
    for b in data:
        if b == FEND:
            out.extend([FESC, TFEND])
        elif b == FESC:
            out.extend([FESC, TFESC])
        else:
            out.append(b)
    return bytes(out)


def _unescape(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        b = data[i]
        if b == FESC and i + 1 < n:
            i += 1
            nxt = data[i]
            if nxt == TFEND:
                out.append(FEND)
            elif nxt == TFESC:
                out.append(FESC)
            else:
                out.append(nxt)
        else:
            out.append(b)
        i += 1
    return bytes(out)


def encode_data(payload: bytes, port: int = 0) -> bytes:
    """Encode a DATA-frame payload into a full KISS frame."""
    # For data frames the command byte is CMD_DATA | (port<<4) in some KISS variants;
    # firmware uses CMD_DATA directly (port 0). We'll prefix CMD_DATA as first byte.
    frame = bytes([CMD_DATA]) + payload
    return bytes([FEND]) + _escape(frame) + bytes([FEND])


def encode_command(cmd: int, subcmd: int | None = None, data: bytes | None = None) -> bytes:
    """Encode a KISS command frame (e.g., CMD_SETHARDWARE/CMD_GETHARDWARE).

    Frame payload starts with command byte, optional subcmd, then data.
    """
    if subcmd is not None:
        frame = bytes([cmd, subcmd])
        if data:
            frame += data
    else:
        frame = bytes([cmd])
        if data:
            frame += data
    return bytes([FEND]) + _escape(frame) + bytes([FEND])


def decode_kiss(stream: bytes) -> tuple:
    """Decode a raw byte stream into a list of unescaped frame payloads.

    Returns a tuple (frames, leftover_bytes).

    - frames: list of payload bytes (first byte is command byte for command frames)
    - leftover_bytes: bytes remaining at the end of the stream that couldn't form a complete frame

    This function is suitable for use with a rolling buffer: feed the accumulated
    bytes in, process the returned frames, and keep the leftover bytes for the next read.
    """
    results = []
    i = 0
    n = len(stream)
    last_consumed = 0
    while i < n:
        # find next FEND (start marker)
        try:
            start = stream.index(FEND, i)
        except ValueError:
            # no start marker in remainder
            break
        # look for the next FEND after start
        next_search = start + 1
        if next_search >= n:
            # incomplete, keep from start
            break
        try:
            end = stream.index(FEND, next_search)
        except ValueError:
            # no closing FEND yet, keep from start
            break
        # extract raw between start+1 and end
        raw = stream[start+1:end]
        payload = _unescape(raw)
        results.append(payload)
        last_consumed = end + 1
        i = last_consumed

    # leftover is anything after last_consumed, or the whole stream if nothing consumed
    leftover = stream[last_consumed:]
    return results, leftover
