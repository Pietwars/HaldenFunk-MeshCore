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
  ble.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(mux.isPublicTransport());
  usb.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(!mux.isPublicTransport());
  portal.queued=1;assert(mux.checkRecvFrame(frame)==1);assert(!mux.isPublicTransport());
  assert(mux.checkRecvFrame(frame)==0);assert(!mux.isPublicTransport());
  frame[0]=14;assert(mux.writeFrame(frame,65)==65);assert(ble.written==0);assert(usb.written==65);
  frame[0]=17;assert(mux.writeFrame(frame,10)==10);assert(ble.written==10);
}
