#pragma once

#include <Arduino.h>

#define MAX_FRAME_SIZE  176   // +4 for transport codes (region scoping)

class BaseSerialInterface {
protected:
  BaseSerialInterface() { }

public:
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual bool isEnabled() const = 0;

  virtual bool isConnected() const = 0;
  virtual void loop() {};
  // True only for the transport which supplied the current frame.
  virtual bool isPublicTransport() const { return false; }
  // True only when the current frame came over Bluetooth.
  virtual bool isBluetoothTransport() const { return false; }
  // True only when the current frame came from the built-in web portal (DisAHRsterRadio).
  virtual bool isPortalTransport() const { return false; }
  // While set, replies (codes < 0x80) go only to the transport which supplied the current frame;
  // pushes (>= 0x80) still go to every client. Used for message sync and send replies.
  virtual void setReplyOnly(bool) { }

  virtual bool isWriteBusy() const = 0;
  virtual size_t writeFrame(const uint8_t src[], size_t len) = 0;
  virtual size_t checkRecvFrame(uint8_t dest[]) = 0;
};

