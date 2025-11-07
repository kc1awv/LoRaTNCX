// KISSProtocol.cpp
// KISS protocol implementation
#include "KISSProtocol.h"

KISSProtocol::KISSProtocol(Stream &io)
    : _io(io),
      _rxIndex(0),
      _frameInProgress(false),
      _escapeNext(false),
      _lastFrameLen(0),
      _frameAvailable(false),
      _exitRequested(false),
      _frameHandler(nullptr)
{
}

bool KISSProtocol::processByte(uint8_t byte)
{
  // ESC (0x1B) exits KISS mode
  if (byte == ESC)
  {
    _exitRequested = true;
    resetRxBuffer();
    return false;
  }

  if (byte == FEND)
  {
    if (_frameInProgress && _rxIndex > 0)
    {
      // End of frame - process it
      processFrame(_rxBuffer, _rxIndex);
      resetRxBuffer();
      return true;
    }
    // Start new frame
    resetRxBuffer();
    _frameInProgress = true;
    _escapeNext = false;
    return false;
  }

  if (_frameInProgress)
  {
    if (_escapeNext)
    {
      // Handle escaped characters
      if (byte == TFEND)
      {
        byte = FEND;
      }
      else if (byte == TFESC)
      {
        byte = FESC;
      }
      else
      {
        // Invalid escape sequence - reset frame
        resetRxBuffer();
        return false;
      }
      _escapeNext = false;
    }
    else if (byte == FESC)
    {
      // Escape next character
      _escapeNext = true;
      return false;
    }

    // Add byte to buffer
    if (_rxIndex < RX_BUFFER_SIZE)
    {
      _rxBuffer[_rxIndex++] = byte;
    }
    else
    {
      // Buffer overflow - reset frame
      resetRxBuffer();
    }
  }

  return false;
}

bool KISSProtocol::hasFrame() const
{
  return _frameAvailable;
}

size_t KISSProtocol::getFrame(uint8_t *buffer, size_t maxLen)
{
  if (!_frameAvailable)
  {
    return 0;
  }

  size_t copyLen = (_lastFrameLen < maxLen) ? _lastFrameLen : maxLen;
  memcpy(buffer, _lastFrame, copyLen);
  _frameAvailable = false;

  return copyLen;
}

void KISSProtocol::sendFrame(const uint8_t *data, size_t len)
{
  if (len == 0)
  {
    return;
  }

  // Send frame start
  _io.write((uint8_t)FEND);

  // Send CMD_DATA
  _io.write((uint8_t)CMD_DATA);

  // Send data with escaping
  for (size_t i = 0; i < len; i++)
  {
    sendEscaped(data[i]);
  }

  // Send frame end
  _io.write((uint8_t)FEND);
}

bool KISSProtocol::isExitRequested() const
{
  return _exitRequested;
}

void KISSProtocol::clearExitRequest()
{
  _exitRequested = false;
}

void KISSProtocol::setFrameHandler(FrameHandler handler)
{
  _frameHandler = handler;
}

void KISSProtocol::processFrame(const uint8_t *frame, size_t len)
{
  if (len == 0)
  {
    return;
  }

  uint8_t command = frame[0];

  // CMD_RETURN (0xFF) exits KISS mode
  if (command == CMD_RETURN)
  {
    _exitRequested = true;
    return;
  }

  // CMD_DATA (0x00) - store frame or call handler
  if (command == CMD_DATA && len > 1)
  {
    // Store frame for retrieval
    size_t dataLen = len - 1;
    if (dataLen <= RX_BUFFER_SIZE)
    {
      memcpy(_lastFrame, frame + 1, dataLen);
      _lastFrameLen = dataLen;
      _frameAvailable = true;
    }

    // Call handler if set
    if (_frameHandler)
    {
      _frameHandler(frame + 1, dataLen);
    }
    return;
  }

  // Other KISS commands (TXDELAY, P, SLOTTIME, etc.)
  // Silently ignore for now - could be extended to store parameters
}

void KISSProtocol::sendEscaped(uint8_t byte)
{
  if (byte == FEND)
  {
    _io.write((uint8_t)FESC);
    _io.write((uint8_t)TFEND);
  }
  else if (byte == FESC)
  {
    _io.write((uint8_t)FESC);
    _io.write((uint8_t)TFESC);
  }
  else
  {
    _io.write(byte);
  }
}

void KISSProtocol::resetRxBuffer()
{
  _rxIndex = 0;
  _frameInProgress = false;
  _escapeNext = false;
}
