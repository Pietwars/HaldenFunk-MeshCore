#pragma once

#include "BaseSerialInterface.h"

#ifndef MAX_INTERFACES
  // ble, usb, wifi, ethernet
  #define MAX_INTERFACES 4
#endif

enum class InterfaceType : uint8_t {
  NONE,
  Bluetooth,
  USB,
  WiFi,
  Ethernet,
  HardwareSerial,
  Portal // HTTP handlers enforce the independent web-admin boundary
};

class MultiSerialInterface : public BaseSerialInterface {
private:

  struct RegisteredInterface {
    InterfaceType type = InterfaceType::NONE;
    BaseSerialInterface* instance = nullptr;
  };

  bool _enabled = false;
  bool _replyOnly = false;
  InterfaceType _receivedFrom = InterfaceType::NONE;
  RegisteredInterface _interfaces[MAX_INTERFACES] = {};

public:
  bool addInterface(InterfaceType type, BaseSerialInterface* iface) {
    // make sure an interface was provided
    if(iface == nullptr){
      return false;
    }

    // put it in the first free slot
    for(int i = 0; i < MAX_INTERFACES; i++){
      if(_interfaces[i].instance == nullptr){
        _interfaces[i].instance = iface;
        _interfaces[i].type = type;
        return true;
      }
    }

    // no free slots available
    return false;
  }

  bool removeInterface(BaseSerialInterface* iface) {
    // make sure an interface was provided
    if(iface == nullptr){
      return false;
    }

    // find and remove interface
    for(int i = 0; i < MAX_INTERFACES; i++){
      if(_interfaces[i].instance == iface){
        _interfaces[i] = {};
        return true;
      }
    }

    // interface not found
    return false;
  }

  void enableBluetooth() {
    for(auto iface : _interfaces){
      if(iface.instance && iface.type == InterfaceType::Bluetooth){
        iface.instance->enable();
      }
    }
  }

  void disableBluetooth() {
    for(auto iface : _interfaces){
      if(iface.instance && iface.type == InterfaceType::Bluetooth){
        iface.instance->disable();
      }
    }
  }

  bool isBluetoothEnabled() {
    for(auto iface : _interfaces){
      if(iface.instance && iface.type == InterfaceType::Bluetooth){
        return iface.instance->isEnabled();
      }
    }
    return false; 
  }

  // enable all interfaces
  void enable() override {
    _enabled = true;
    for(auto iface : _interfaces){
      if(iface.instance){
        iface.instance->enable();
      }
    }
  }

  // disable all interfaces
  void disable() override {
    _enabled = false;
    for(auto iface : _interfaces){
      if(iface.instance){
        iface.instance->disable();
      }
    }
  }

  bool isEnabled() const override { 
    return _enabled; 
  }

  bool isConnected() const override {
    // not connected when disabled
    if(!_enabled){
      return false;
    }
    
    // check if any interface is connected
    for(auto iface : _interfaces){
      if(iface.instance && iface.instance->isConnected()) {
        return true;
      }
    }

    // nothing connected
    return false;
  }

  // loop all interfaces
  void loop() override {
    for(auto iface : _interfaces){
      if(iface.instance){
        iface.instance->loop();
      }
    }
  }

  bool isWriteBusy() const override {
    // not busy when disabled
    if(!_enabled){
      return false;
    }

    // check if any interface is busy
    for(auto iface : _interfaces){
      if(iface.instance && iface.instance->isEnabled() && iface.instance->isWriteBusy()){
        return true;
      }
    }

    // nothing busy
    return false;
  }

  size_t writeFrame(const uint8_t src[], size_t len) override {
    // don't write when disabled or nothing provided
    if(!_enabled || len == 0){
      return 0;
    }

    // write frame to all enabled interfaces
    bool allSuccessful = true;
    for(auto iface : _interfaces){
      if(iface.instance && iface.instance->isEnabled()){
        // Private-key export reply (14) goes only back to the requesting USB or Bluetooth link
        // (Bluetooth gets here only with an active PIN); never to WiFi, Ethernet or the portal.
        if(src[0] == 14 && !privateReplyTarget(iface.type)) continue;
        // Reply-only (message sync, send replies): only the requesting client gets the reply, so one
        // client's sync does not consume or duplicate messages for the other.
        if(_replyOnly && src[0] < 0x80 && _receivedFrom != InterfaceType::NONE && iface.type != _receivedFrom) continue;
        if(iface.instance->writeFrame(src, len) != len){
          allSuccessful = false;
        }
      }
    }

    // report success if all writes completed successfully
    return allSuccessful ? len : 0; 
  }

  bool isPublicTransport() const override {
    return _receivedFrom == InterfaceType::Bluetooth || _receivedFrom == InterfaceType::WiFi || _receivedFrom == InterfaceType::Ethernet;
  }

  bool privateReplyTarget(InterfaceType t) const {
    if (_receivedFrom == InterfaceType::USB || _receivedFrom == InterfaceType::Bluetooth) return t == _receivedFrom;
    return t == InterfaceType::USB;  // unsolicited: USB only
  }

  bool isBluetoothTransport() const override { return _receivedFrom == InterfaceType::Bluetooth; }
  bool isPortalTransport() const override { return _receivedFrom == InterfaceType::Portal; }
  void setReplyOnly(bool on) override { _replyOnly = on; }

  size_t checkRecvFrame(uint8_t dest[]) override {
    _receivedFrom = InterfaceType::NONE;
    // don't read when disabled
    if(!_enabled){
      return 0;
    }

    // try to read a frame from any enabled interface
    for(auto iface : _interfaces){
      if(iface.instance && iface.instance->isEnabled()){
        size_t frameSize = iface.instance->checkRecvFrame(dest);
        if(frameSize > 0){
          _receivedFrom = iface.type;
          return frameSize; 
        }
      }
    }

    // no frame received
    return 0;
  }

};

