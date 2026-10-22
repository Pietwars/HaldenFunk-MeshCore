// SPDX-License-Identifier: MIT
#pragma once
// Web-free local companion client (Issue #35, step 2). It owns one serialized Companion-protocol session with
// MyMesh (InterfaceType::Portal in MultiSerialInterface) and keeps:
//   - the startup sequence and caches of channels and contacts,
//   - local sending with 5 s send pause, strictly increasing timestamps and send results,
//   - the message history (/dr-history.bin, HistoryRecord/SlotFile) and the optional SD archive (ArchiveSd),
//   - the durable direct-message outbox (DeliveryOutbox) and the room-server session (guest password),
//   - app sends mirrored into the history (recordAppSend); the app echo of local sends is MyMesh::queueWebEchoForApp.
// The web portal (DisasterPortal.h) derives from this class and only adds HTTP and WLAN. The T-Deck Plus
// standalone build (StandaloneFrontend.cpp) uses it directly for the display, keyboard and trackball.
#include "../simple_room_server/RoomAccess.h"  // DISASTER_ROOM_GUEST_PASSWORD ("hello")
// BLE (MeshCore-App) darf parallel laufen: MultiSerialInterface verteilt alle Antworten an den lokalen Client und BLE.
#if defined(ENABLE_USB_INTERFACE) || defined(WIFI_SSID)
#error "The local companion client must own the Companion interface in this profile"
#endif
#if LORA_TX_POWER != 10
#error "Unexpected local companion default TX power"
#endif
#include <helpers/BaseSerialInterface.h>
#include <SPIFFS.h>
#include "HistoryRecord.h"
#include "SlotFile.h"
#include "DeliveryOutbox.h"
#include "ArchiveSd.h"
#include "LocalSend.h"

class LocalCompanion : public BaseSerialInterface {
protected:
  DeliveryOutbox delivery;
  ArchiveSd archive;
  int activeJob=-1;
  uint32_t routeJob=0;uint8_t routeAttempt=255;
  bool enabled = false, storageOK = true;
  uint8_t startup = 0, active = 0;
  uint32_t nextPoll = 0, seq = 0, lastSend = 0, lastStamp = 0;
  bool sentOnce = false, userCommand = false;
  uint8_t pending[MAX_FRAME_SIZE] = {}, pendingLen = 0;
  uint8_t outgoing[MAX_FRAME_SIZE] = {}, outgoingLen = 0;
  String info, contacts[64], channels[8];
  String roomTarget;
  uint8_t roomRights=0, loginLen=0, loginFrame[49]={};
  bool roomPending=false;
  uint32_t roomSince=0;
  // Result of the last send started from the device itself (localSend); the web keeps using `outcome`.
  bool localOutgoing=false;
  char localOutcome[96]="Bereit";
  // Reject ambiguous short identifiers instead of choosing an arbitrary contact.
  const String *roomContact(const String &target) const {
    if(target.length()!=13||target[0]!='d')return nullptr;
    const String *found=nullptr;
    for(const auto &c:contacts)if(c.length()>=264&&c.substring(2,14)==target.substring(1)&&c.substring(66,68)=="03"){
      if(found)return nullptr;found=&c;
    }
    return found;
  }
  String outcome = "Bereit";
  uint32_t activeSince = 0;
  using Record = disaster::HistoryRecord;
  static_assert(MAX_FRAME_SIZE == disaster::FRAME_BYTES, "Companion frame size changed");
  static String hex(const uint8_t *p, size_t n) {
    static const char digits[]="0123456789abcdef";
    String out; out.reserve(n*2);
    for(size_t i=0;i<n;++i){out+=digits[p[i]>>4];out+=digits[p[i]&15];}
    return out;
  }
  void append(const uint8_t *p, size_t n) {
    if (!n || n>MAX_FRAME_SIZE) return;
    Record r{}; r.seq=++seq; r.len=n; memcpy(r.data,p,n); r.crc=disaster::checksum(r);
    File f=disaster::openSlotFile(SPIFFS,"/dr-history.bin");
    bool ok=f && disaster::seekSlot(f,disaster::offset(r.seq)) && f.write((uint8_t*)&r,sizeof(r))==sizeof(r);
    if(f)f.close();
    if(!ok)storageOK=false;
    if(!ok)Serial.printf("[VERLAUF] Schreiben fehlgeschlagen (seq %u)\n",(unsigned)r.seq);
    archive.append(r);
  }
  static bool hasNull(const String &s) {
    for(size_t i=0;i<s.length();++i)if(s[i]==0)return true;
    return false;
  }
  static int nibble(char c) { return disaster::hexNibble(c); }
  // Shared rules of the web and the device sender.
  bool busy() const { return startup<12 || pendingLen || active || loginLen; }
  bool sendPaused() const { return disaster::sendPaused(sentOnce,lastSend,millis()); }
  void queueCommand(const uint8_t *p,size_t n) { memcpy(pending,p,n); pendingLen=n; }
  // Remember an accepted text send: history record after the core's reply, pause, strictly increasing stamp.
  void commitOutgoing(const uint8_t *p,size_t n,uint32_t stamp) {
    memcpy(outgoing,p,n);outgoingLen=n;lastStamp=stamp;sentOnce=true;lastSend=millis();
  }
  // Room-server login with the public guest password. First the clock (frame returned in `clock`), then the
  // login frame on the next serialized command.
  void prepareRoomLogin(const String &contact,const String &target,uint32_t stamp,uint8_t clock[5]) {
    memset(loginFrame,0,sizeof(loginFrame));loginFrame[0]=26;
    for(int i=0;i<32;++i)loginFrame[1+i]=(nibble(contact[2+i*2])<<4)|nibble(contact[3+i*2]);
    // Automatisch mit dem oeffentlichen Gast-Passwort "hello" anmelden (Lesen und Schreiben).
    static const char guest[]=DISASTER_ROOM_GUEST_PASSWORD;
    memcpy(loginFrame+33,guest,sizeof(guest)-1);loginLen=33+sizeof(guest)-1;
    delivery.syncClock(stamp);roomTarget=target;roomRights=0;roomPending=true;roomSince=millis();
    clock[0]=6;memcpy(clock+1,&stamp,4);
  }
  // History, outbox and SD archive; call once after SPIFFS is mounted.
  void beginHistory() {
    delivery.begin();lastStamp=delivery.latestStamp();archive.begin();
    File f=disaster::openSlotFileRead(SPIFFS,"/dr-history.bin"); Record r;
    uint32_t records=0;
    while(f && f.read((uint8_t*)&r,sizeof(r))==sizeof(r))
      if(disaster::valid(r)){records++;if(r.seq>seq) seq=r.seq;}
    if(f)f.close();
    Serial.printf("[VERLAUF] %u Eintraege, letzte Nummer %u\n",(unsigned)records,(unsigned)seq);
  }
  void serviceRoom() {
    if(roomPending&&millis()-roomSince>45000){roomPending=false;roomRights=0;outcome="Raumanmeldung ohne Antwort; Funk und Uhrzeit pruefen";}
  }
  bool fail(char *status,size_t size,const char *reason) const { if(size)snprintf(status,size,"%s",reason); return false; }
public:
  // ---- read access for the device display (standalone) ----
  bool ready() const { return enabled && startup>=12; }
  bool historyOK() const { return storageOK; }
  bool archiveReady() const { return archive.ready(); }
  uint32_t historySeq() const { return seq; }
  // Physical history slot (0..HISTORY_SLOTS-1); false for empty or damaged slots.
  bool readHistory(size_t slot, Record &r) const {
    File f=openHistory(); bool ok=readHistory(f,slot,r);
    if(f)f.close();
    return ok;
  }
  // Reading many slots: open once, read, close (display rebuild).
  File openHistory() const { return disaster::openSlotFileRead(SPIFFS,"/dr-history.bin"); }
  static bool readHistory(File &f, size_t slot, Record &r) {
    if(slot>=disaster::HISTORY_SLOTS)return false;
    return f&&f.seek(slot*sizeof(Record))&&f.read((uint8_t*)&r,sizeof(r))==sizeof(r)&&disaster::valid(r);
  }
  // Start the history (SPIFFS mounted) for a client without its own begin() (standalone display).
  void beginLocal() { beginHistory(); }
  void serviceLocal() { serviceRoom(); }
  const String &channelHex(unsigned i) const { static const String none; return i<8?channels[i]:none; }
  const String &contactHex(unsigned i) const { static const String none; return i<64?contacts[i]:none; }
  const String &roomTargetId() const { return roomTarget; }
  uint8_t roomRightsNow() const { return roomRights; }
  bool roomPendingNow() const { return roomPending; }
  const char *localStatus() const { return localOutcome; }
  const String &lastOutcome() const { return outcome; }
  uint32_t pauseLeftMs() const { return disaster::pauseLeftMs(sentOnce,lastSend,millis()); }

  // ---- local sending from the device (same serialized path as /api/send) ----
  bool localSend(const char *id,const char *text,uint32_t stamp,char *status,size_t size) {
    if(!enabled||busy())return fail(status,size,"Knoten beschaeftigt; erneut versuchen");
    if(sendPaused())return fail(status,size,"Bitte 5 Sekunden Sendepause");
    if(!disaster::validStamp(stamp))return fail(status,size,"Uhr fehlt: GPS-Fix abwarten / App verbinden");
    size_t tl=text?strlen(text):0;
    if(tl<1||tl>disaster::LOCAL_TEXT_LIMIT)return fail(status,size,"1 bis 100 UTF-8-Bytes erforderlich");
    String target(id?id:"");
    if(disaster::channelTarget(target.c_str())){
      if(!channels[target[1]-'0'].length())return fail(status,size,"Kanal nicht eingerichtet");
    } else if(disaster::contactTarget(target.c_str())){
      String key=target.substring(1);key.toLowerCase();
      unsigned matches=0;String type;
      for(const auto &c:contacts)if(c.length()>=264&&c.substring(2,14)==key){++matches;type=c.substring(66,68);}
      if(matches!=1)return fail(status,size,"Kontakt fehlt oder ist mehrdeutig");
      if(type=="03"){
        if(roomTarget!=String("d")+key||roomRights<2)return fail(status,size,"Zuerst am Brett mit Schreibrecht anmelden");
      } else if(type!="01")return fail(status,size,"Kein Chat-Kontakt");
      target=String("d")+key;
    } else return fail(status,size,"Ungueltiges Ziel");
    stamp=disaster::nextStamp(stamp,lastStamp);
    uint8_t p[MAX_FRAME_SIZE];
    size_t n=disaster::textFrame(target.c_str(),text,stamp,p,sizeof(p));
    if(!n)return fail(status,size,"Ziel, Uhrzeit oder 100-Byte-Limit pruefen");
    queueCommand(p,n);commitOutgoing(p,n,stamp);
    outcome="Warteschlange";localOutgoing=true;
    snprintf(localOutcome,sizeof(localOutcome),"Vorgemerkt; noch nicht gesendet");
    if(size)snprintf(status,size,"%s",localOutcome);
    return true;
  }
  // Room-server login from the device (same frames as /api/room-login).
  bool localRoomLogin(const char *id,uint32_t stamp,char *status,size_t size) {
    if(!enabled||busy())return fail(status,size,"Knoten beschaeftigt; erneut versuchen");
    String target(id?id:"");target.toLowerCase();
    const String *contact=roomContact(target);
    if(!contact)return fail(status,size,"Brett (Room-Server) nicht eindeutig bekannt");
    if(!disaster::validStamp(stamp))return fail(status,size,"Uhr fehlt: GPS-Fix abwarten / App verbinden");
    uint8_t clock[5];prepareRoomLogin(*contact,target,stamp,clock);queueCommand(clock,5);
    outcome="Anmeldung vorbereitet";
    if(size)snprintf(status,size,"Anmeldung laeuft (Gast-Passwort)");
    return true;
  }
  // Re-read contacts (CMD_GET_CONTACTS) or set the node name (CMD_SET_ADVERT_NAME), same as the web.
  bool localRefresh() { if(!enabled||busy())return false; const uint8_t p[]={4}; queueCommand(p,1); return true; }
  bool localSetName(const char *name) {
    size_t n=name?strlen(name):0;
    if(!enabled||busy()||n<1||n>24)return false;
    uint8_t p[26]={8};memcpy(p+1,name,n);queueCommand(p,n+1);return true;
  }

  // 0.9.3: Von der MeshCore-App (Bluetooth) gesendete Text-/Kanalnachricht in den Verlauf schreiben
  // (gleiches Format wie eigene Nachrichten: f3 Kanal, f4 Direkt mit ACK-Kennung, f2 ohne Kennung).
  void recordAppSend(const uint8_t *c,size_t n,const uint8_t *tag) {
    if(n<7||n>MAX_FRAME_SIZE)return;
    uint8_t r[MAX_FRAME_SIZE];memcpy(r,c,n);size_t rl=n;
    if(c[0]==3)r[0]=0xf3;
    else if(c[0]==2&&n>=13){
      if(tag&&n+4<=MAX_FRAME_SIZE){memmove(r+17,r+13,n-13);memcpy(r+13,tag,4);r[0]=0xf4;rl+=4;}
      else r[0]=0xf2;
    } else return;
    append(r,rl);
  }
  void enable() override {enabled=true;}
  void disable() override {enabled=false;}
  bool isEnabled() const override {return enabled;}
  bool isConnected() const override {return enabled;}
  bool isWriteBusy() const override {return false;}
  size_t checkRecvFrame(uint8_t *dest) override {
    if(!enabled)return 0;
    if(active) {
      if(millis()-activeSince>15000) {
        if(localOutgoing){snprintf(localOutcome,sizeof(localOutcome),"Antwort unklar; nicht erneut senden");localOutgoing=false;}
        activeJob=-1;active=0;outgoingLen=0;outcome="Antwort unklar; nicht automatisch erneut senden";}
      else return 0;
    }
    size_t n=0;
    if(startup==0){dest[0]=22;dest[1]=3;n=2;startup++;}
    else if(startup==1){memset(dest,0,8);dest[0]=1;n=8;startup++;}
    else if(startup>=2 && startup<10){dest[0]=31;dest[1]=startup-2;n=2;startup++;}
    else if(startup==10){dest[0]=4;n=1;startup++;}
    else if(startup==11){dest[0]=20;n=1;startup++;}
    else if(loginLen&&!pendingLen){userCommand=true;n=loginLen;memcpy(dest,loginFrame,n);loginLen=0;memset(loginFrame,0,sizeof(loginFrame));}
    else if(pendingLen){userCommand=true;n=pendingLen;memcpy(dest,pending,n);pendingLen=0;}
    else if((!sentOnce||millis()-lastSend>=5000)&&delivery.ready()){
      int job=delivery.next();
      if(job>=0){const auto &j=delivery.get(job);bool known=false,ambiguous=false;
        for(const auto &c:contacts)if(c.length()>=264&&c.substring(2,14)==hex(j.recipient,6)){
          if(known||c.substring(2,66)!=hex(j.recipient,32)||c.substring(66,68)!="01")ambiguous=true;known=true;
        }
        if(!known||ambiguous)delivery.defer(job);
        else if(j.attempts&&(routeJob!=j.id||routeAttempt!=j.attempts)){
          dest[0]=13;memcpy(dest+1,j.recipient,32);n=33;routeJob=j.id;routeAttempt=j.attempts;
        }
        else if(delivery.start(job)){
          dest[0]=2;dest[1]=0;dest[2]=j.attempts-1;memcpy(dest+3,&j.stamp,4);memcpy(dest+7,j.recipient,6);
          size_t len=strlen(j.text);memcpy(dest+13,j.text,len);n=13+len;memcpy(outgoing,dest,n);outgoingLen=n;
          activeJob=job;userCommand=true;sentOnce=true;lastSend=millis();
        }
      }
      if(!n&&(int32_t)(millis()-nextPoll)>=0){dest[0]=10;n=1;nextPoll=millis()+1000;}
    }
    else if((int32_t)(millis()-nextPoll)>=0){dest[0]=10;n=1;nextPoll=millis()+1000;}
    if(n){
      if(dest[0]==4)for(auto &contact:contacts)contact="";
      active=dest[0];activeSince=millis();
    }
    return n;
  }
  size_t writeFrame(const uint8_t *p,size_t n) override {
    if(!n||n>MAX_FRAME_SIZE)return 0;
    uint8_t code=p[0];
    if((code==0x85||code==0x86)&&n>=8&&roomTarget.substring(1)==hex(p+2,6)){
      roomPending=false;roomRights=code==0x85?((n>=13&&(p[12]&3))?(p[12]&3):1):0;  // Gast ohne Schreibrecht (0) = angemeldet, nur Lesen
      outcome=roomRights>=2?"Raum angemeldet: Lesen und Schreiben":roomRights==1?"Raum angemeldet: nur Lesen":"Raumanmeldung fehlgeschlagen";
    }
    if(code==0x83){nextPoll=0;return n;}
    if(code==5)info=hex(p,n);
    if(code==18 && n>=34 && p[1]<8)channels[p[1]]=hex(p,34); // omit channel secret
    if(code==0x8f && n==33){
      String prefix=hex(p+1,32);
      for(auto &c:contacts)if(c.substring(2,66)==prefix)c="";
    }
    if((code==3 || code==0x8a) && n>=36){
      String prefix=hex(p+1,32);int slot=-1;
      for(int i=0;i<64;i++){if(contacts[i].substring(2,66)==prefix){slot=i;break;}if(slot<0&&!contacts[i].length())slot=i;}
      if(slot>=0)contacts[slot]=hex(p,n);
    }
    if(code==0x82&&n>=9)delivery.acknowledge(p+1);
    if(code==16 || code==17 || code==7 || code==8 || code==0x82)append(p,n);
    if(code<0x80 && active) {
      if(activeJob>=0&&code==6&&n>=10)delivery.tag(activeJob,p+2);
      if((active==2||active==3) && (code==0||code==6||code==1)) {
        if(localOutgoing){snprintf(localOutcome,sizeof(localOutcome),"%s",code==1?"Funkkern meldet Sendefehler":"Gesendet; Zustellung offen");localOutgoing=false;}
        if(code!=1 && outgoingLen){uint8_t record[MAX_FRAME_SIZE];memcpy(record,outgoing,outgoingLen);record[0]=active==2?0xf2:0xf3;
          size_t recordLen=outgoingLen;
          // v0.2: preserve the core's ACK tag beside the accepted direct message.
          // f4: legacy 13-byte header, 4-byte tag, UTF-8 text. Old records remain readable.
          if(active==2 && code==6 && n>=10 && outgoingLen>=13 && outgoingLen+4<=MAX_FRAME_SIZE) {
            memmove(record+17,record+13,outgoingLen-13);
            memcpy(record+13,p+2,4);record[0]=0xf4;recordLen+=4;
          }
          append(record,recordLen);outcome="Vom Funkkern angenommen; Zustellung offen";}
        else outcome="Funkkern meldet Sendefehler";
        outgoingLen=0;
      } else if(active==26){
        if(code==1){roomPending=false;roomRights=0;outcome="Raumanmeldung vom Funkkern abgelehnt";}
        else outcome="Anmeldung gesendet; Serverantwort ausstehend";
      } else if(code==1 && userCommand)outcome="Befehl vom Funkkern abgelehnt";
      else if(code==0 && userCommand)outcome="Gespeichert / angenommen";
      bool complete = active==4 ? (code==4||code==1) : true;
      if(complete) {
        if(code==0 && (active==8 || active==11 || active==32)) startup=1;
        active=0; activeJob=-1;userCommand=false;
      }
    }
    return n;
  }
};
