#include <assert.h>
#include <string.h>
#include <initializer_list>
#include "../examples/companion_radio/OpenAccess.h"
#include "../src/helpers/MultiSerialInterface.h"

struct Port : BaseSerialInterface {
  bool on=true;size_t queued=0,written=0;
  void enable() override {on=true;}
  void disable() override {on=false;}
  bool isEnabled() const override {return on;}
  bool isConnected() const override {return on;}
  bool isWriteBusy() const override {return false;}
  size_t writeFrame(const uint8_t*,size_t n) override {written+=n;return n;}
  size_t checkRecvFrame(uint8_t* p) override {size_t n=queued;queued=0;if(n)p[0]=11;return n;}
};
int main() {
  const unsigned blocked[]={8,9,11,12,14,15,19,21,23,24,25,32,33,34,35,37,38,41,50,51,54,55,57,58,61,62,63,65,66,255};
  for(auto cmd:blocked) assert(!disaster::publicCompanionCommand(cmd));
  for(auto cmd:{1,2,3,4,5,6,7,10,13,16,17,18,20,22,26,27,28,29,30,31,39,40,42,43,52,56,59,60,64}) assert(disaster::publicCompanionCommand(cmd));
  // TEST pin123456 (ble-voll): PIN-paired Bluetooth gets every command, like stock MeshCore
  for(unsigned cmd=0;cmd<256;cmd++) assert(disaster::allowCompanionCommand(cmd,true,true,123456));
  // open WiFi/Ethernet (public, not Bluetooth): unchanged restricted list, PIN irrelevant
  for(auto cmd:blocked){
    assert(!disaster::allowCompanionCommand(cmd,true,false,123456));
    assert(!disaster::allowCompanionCommand(cmd,true,false,0));
  }
  // Bluetooth without PIN: unchanged restricted list
  for(auto cmd:blocked) assert(!disaster::allowCompanionCommand(cmd,true,true,0));
  for(auto cmd:{1,2,3,4,5,6,7,10,13,16,17,18,20,22,26,27,28,29,30,31,39,40,42,43,52,56,59,60,64}){
    assert(disaster::allowCompanionCommand(cmd,true,false,0));
    assert(disaster::allowCompanionCommand(cmd,true,true,0));
  }
  // USB / Portal (not public): everything as before
  for(unsigned cmd=0;cmd<256;cmd++) assert(disaster::allowCompanionCommand(cmd,false,false,0));
  uint8_t frame[176]={};
  assert(!disaster::validCompanionLength(nullptr,1,176));
  assert(!disaster::validCompanionLength(frame,0,176));
  assert(!disaster::validCompanionLength(frame,177,176));
  const unsigned pairs[][2]={{11,11},{12,2},{15,33},{16,33},{19,7},{21,9},{30,33},{38,2},{51,6},{58,2},{61,3}};
  for(auto &p:pairs){frame[0]=p[0];for(unsigned n=0;n<p[1];n++)assert(!disaster::validCompanionLength(frame,n,176));assert(disaster::validCompanionLength(frame,p[1],176));}
  Port ble,usb,portal;MultiSerialInterface mux;
  assert(mux.addInterface(InterfaceType::Bluetooth,&ble));
  assert(mux.addInterface(InterfaceType::USB,&usb));
  assert(mux.addInterface(InterfaceType::Portal,&portal));
  mux.enable();
  ble.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(mux.isPublicTransport());assert(mux.isBluetoothTransport());
  usb.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(!mux.isPublicTransport());assert(!mux.isBluetoothTransport());
  portal.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(!mux.isPublicTransport());
  assert(mux.checkRecvFrame(frame)==0);assert(!mux.isPublicTransport());
  frame[0]=14;assert(mux.writeFrame(frame,65)==65);assert(ble.written==0);assert(usb.written==65);
  frame[0]=17;assert(mux.writeFrame(frame,10)==10);assert(ble.written==10);
  // private-key export reply (14) only back to the requesting PIN-Bluetooth or USB link
  ble.written=usb.written=portal.written=0;
  ble.queued=1;assert(mux.checkRecvFrame(frame)==1);
  frame[0]=14;assert(mux.writeFrame(frame,65)==65);assert(ble.written==65&&usb.written==0&&portal.written==0);
  ble.written=0;usb.queued=1;assert(mux.checkRecvFrame(frame)==1);
  frame[0]=14;assert(mux.writeFrame(frame,65)==65);assert(usb.written==65&&ble.written==0&&portal.written==0);
  usb.written=0;portal.queued=1;assert(mux.checkRecvFrame(frame)==1);
  frame[0]=14;mux.writeFrame(frame,65);assert(usb.written==65&&ble.written==0&&portal.written==0);
  // key export/import allowed over PIN Bluetooth, blocked over WiFi/Ethernet and Bluetooth without PIN
  for(auto cmd:{23,24}){
    assert(disaster::allowCompanionCommand(cmd,true,true,123456));
    assert(disaster::allowCompanionCommand(cmd,true,true,654321));
    assert(!disaster::allowCompanionCommand(cmd,true,false,123456));
    assert(!disaster::allowCompanionCommand(cmd,true,true,0));
  }
  // Bluetooth PIN: any 6-digit value, otherwise default
  assert(disaster::validBlePin(100000)&&disaster::validBlePin(999999)&&disaster::validBlePin(123456));
  assert(!disaster::validBlePin(0)&&!disaster::validBlePin(99999)&&!disaster::validBlePin(1000000));
  assert(disaster::effectiveBlePin(654321)==654321);
  assert(disaster::effectiveBlePin(0)==DISASTER_BLE_PIN&&disaster::effectiveBlePin(12345)==DISASTER_BLE_PIN);
}
