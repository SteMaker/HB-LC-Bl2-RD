//- -----------------------------------------------------------------------------------------------------------------------
// HB-LC-Bl2-24VRain
// 2019-08-12 Stemaker Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//
// Based on HB-LC-Bl1-FM2 from pa-pa (https://github.com/jp112sdl/HM-LC-Bl1-FM-2)
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

/* Entwicklungsschritte
 *   1) Umstellung der Implementierung von MutliChannelDevice auf allgemeines ChannelDevice, das
 *      die Kanäle registriert -> done
 *   2) Eigene device ID im sketch und entsprechend neues xml file auf der Zentrale. Keine
 *      Funktionserweiterung
 *   3) Regenkanal mit aufnehmen (noch ohne Funktion). Dritten Kanal im sketch einbauen und das xml
 *      für die Zentrale anpassen.
 *   4) Regensensor handling einbauen
 */


#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Blind.h>

// we use a Pro Mini
// Initial board revision has no LED, using pin 9since it is unused
#define LED_PIN 9
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

#define ON_RELAY_PIN 15
#define DIR_RELAY_PIN 14

#define ON_RELAY2_PIN 16
#define DIR_RELAY2_PIN 17

#define UP_BUTTON_PIN 4
#define DOWN_BUTTON_PIN 3

#define UP_BUTTON2_PIN 6
#define DOWN_BUTTON2_PIN 5

#define RAIN_INPUT_PIN 7

// number of available peers per channel
#define PEERS_PER_BLIND_CHANNEL 6
#define PEERS_PER_RAIN_CHANNEL 12

// all library classes are placed in the namespace 'as'
using namespace as;


// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xfa, 0x00, 0x01},            // Device ID
  "STEMBL2R01",                  // Device Serial
  {0xfa, 0x00},                  // Device Model
  0x01,                          // Firmware Version
  as::DeviceType::BlindActuator, // Device Type
  {0x01, 0x00}                   // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<RadioSPI, 2> > Hal;

DEFREGISTER(BlindReg0, MASTERID_REGS, DREG_INTKEY, DREG_CONFBUTTONTIME, DREG_LOCALRESETDISABLE)

class BlindList0 : public RegList0<BlindReg0> {
  public:
    BlindList0 (uint16_t addr) : RegList0<BlindReg0>(addr) {}
    void defaults () {
      clear();
      // intKeyVisible(false);
      confButtonTime(0xff);
      // localResetDisable(false);
    }
};

DEFREGISTER(RainReg1,CREG_AES_ACTIVE, CREG_TRANSMITTRYMAX)
class RainList1 : public RegList1<RainReg1> {
public:
  RainList1 (uint16_t addr) : RegList1<RainReg1>(addr) {}
  void defaults () {
    clear();
    // aesActive(false);
    transmitTryMax(6);
  }
};



class BlChannel : public ActorChannel<Hal, BlindList1, BlindList3, PEERS_PER_BLIND_CHANNEL, BlindList0, BlindStateMachine> {
  private:
    uint8_t on_relay_pin;
    uint8_t dir_relay_pin;
  public:
    typedef ActorChannel<Hal, BlindList1, BlindList3, PEERS_PER_BLIND_CHANNEL, BlindList0, BlindStateMachine> BaseChannel;

    BlChannel () : on_relay_pin(0), dir_relay_pin(0) {}
    virtual ~BlChannel () {}

    virtual void switchState(uint8_t oldstate, uint8_t newstate, uint32_t stateDelay) {
      BaseChannel::switchState(oldstate, newstate, stateDelay);
      if ( newstate == AS_CM_JT_RAMPON && stateDelay > 0 ) {
        motorUp();
      }
      else if ( newstate == AS_CM_JT_RAMPOFF && stateDelay > 0 ) {
        motorDown();
      }
      else {
        motorStop();
      }
    }

    void motorUp () {
      digitalWrite(dir_relay_pin, HIGH);
      digitalWrite(on_relay_pin, HIGH);
    }

    void motorDown () {
      digitalWrite(dir_relay_pin, LOW);
      digitalWrite(on_relay_pin, HIGH);
    }

    void motorStop () {
      digitalWrite(dir_relay_pin, LOW);
      digitalWrite(on_relay_pin, LOW);
    }

    void init (uint8_t op, uint8_t dp) {
      on_relay_pin = op;
      dir_relay_pin = dp;
      pinMode(on_relay_pin, OUTPUT);
      pinMode(dir_relay_pin, OUTPUT);
      motorStop();
      BaseChannel::init();
    }
};

class RainEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, bool israining) {
      Message::init(0x09, msgcnt, 0x53, BIDI | RPTEN, 0x41, israining & 0xff);
    }
};

class RainChannel : public Channel<Hal, RainList1, EmptyList, EmptyList, PEERS_PER_RAIN_CHANNEL, BlindList0>, public Alarm {
  public:
    uint8_t stat;

    RainChannel() : stat(0), Alarm(seconds2ticks(10)) {}
    virtual ~RainChannel() {}

    uint8_t flags () const {
      uint8_t flags = this->device().battery().low() ? 0x80 : 0x00;
      return flags;
    }

    uint8_t status() const {
      return stat;
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      stat = ~stat;
      RainEventMsg& rainmsg = (RainEventMsg&)device().message();
      rainmsg.init(device().nextcount(), stat==0?false:true);
      device().sendMasterEvent(rainmsg);
      tick = (seconds2ticks(10));
      clock.add(*this);
    }

    void setup(Device<Hal,BlindList0>* dev,uint8_t number,uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }
  /* TODO */
};

class Blind2xAndRainDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, BlindList0>, 3, BlindList0> {
  public:
    VirtChannel<Hal, BlChannel, BlindList0>  blc1, blc2;
    VirtChannel<Hal, RainChannel, BlindList0> rainc;

    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, BlindList0>, 3, BlindList0> DeviceType;
    Blind2xAndRainDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(blc1, 1);
      DeviceType::registerChannel(blc2, 2);
      DeviceType::registerChannel(rainc, 3);
    }
    virtual ~Blind2xAndRainDevice () {}

    BlChannel& getChannel1() {
      return blc1;
    }

    BlChannel& getChannel2() {
      return blc2;
    }

    RainChannel& getRainChannel() {
      return rainc;
    }
};

Hal hal;
Blind2xAndRainDevice sdev(devinfo, 0x20);
ConfigButton<Blind2xAndRainDevice> cfgBtn(sdev);
InternalButton<Blind2xAndRainDevice> btnup(sdev, 1);
InternalButton<Blind2xAndRainDevice> btndown(sdev, 2);
InternalButton<Blind2xAndRainDevice> btnup2(sdev, 3);
InternalButton<Blind2xAndRainDevice> btndown2(sdev, 4);

void initPeerings (bool first) {
  // create internal peerings - CCU2 needs this
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    Peer p1(devid, 1);
    Peer p2(devid, 2);
    Peer p3(devid, 3);
    Peer p4(devid, 4);
    sdev.channel(1).peer(p1, p2);
    sdev.channel(2).peer(p3, p4);
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  //storage().setByte(0,0);
  bool first = sdev.init(hal);
  sdev.getChannel1().init(ON_RELAY_PIN, DIR_RELAY_PIN);
  sdev.getChannel2().init(ON_RELAY2_PIN, DIR_RELAY2_PIN);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  buttonISR(btnup, UP_BUTTON_PIN);
  buttonISR(btndown, DOWN_BUTTON_PIN);
  buttonISR(btnup2, UP_BUTTON2_PIN);
  buttonISR(btndown2, DOWN_BUTTON2_PIN);

  initPeerings(first);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
