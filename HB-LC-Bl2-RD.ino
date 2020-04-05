//- -----------------------------------------------------------------------------------------------------------------------
// HB-LC-Bl2-RD
// 2019-08-12 Stemaker Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//
// Based on HB-LC-Bl1-FM2 from pa-pa (https://github.com/jp112sdl/HM-LC-Bl1-FM-2)
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

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
#define RAIN_POLL_INTERVAL 1

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

    /* Method used to control the motor */
    virtual void switchState(uint8_t oldstate, uint8_t newstate, uint32_t stateDelay) {
      BaseChannel::switchState(oldstate, newstate, stateDelay);
      if ( newstate == AS_CM_JT_RAMPON && stateDelay > 0 ) {
        /* TODO shall we only do this if level is < 100% ? */
        motorUp();
      }
      else if ( newstate == AS_CM_JT_RAMPOFF && stateDelay > 0 ) {
        /* TODO shall we only do this if level is > 0% ? */
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

    void setupList3ForRain(Peer rainPeer) {
      BlindList3 l3 = getList3(rainPeer);
      /* We do not use the long press of the rain sensor */ 
      BlindPeerList pl3lg = l3.lg();
      pl3lg.actionType(AS_CM_ACTIONTYPE_INACTIVE);
    
      BlindPeerList pl3 = l3.sh();
      pl3.actionType(AS_CM_ACTIONTYPE_JUMP_TO_TARGET);
      pl3.ctValLo(201); /* to enable condition to always fail */
      pl3.ctValHi(180);
      pl3.jtOff(AS_CM_JT_OFFDELAY);
      pl3.ctOff(AS_CM_CT_X_GE_COND_VALUE_HI);
      pl3.jtDlyOn(AS_CM_JT_OFFDELAY);
      pl3.ctDlyOn(AS_CM_CT_X_GE_COND_VALUE_HI);
      pl3.jtRefOn(AS_CM_JT_OFFDELAY);
      pl3.ctRepOn(AS_CM_CT_X_GE_COND_VALUE_HI);
      pl3.jtRampOn(AS_CM_JT_OFFDELAY);
      pl3.ctRampOn(AS_CM_CT_X_GE_COND_VALUE_HI);
      pl3.jtOn(AS_CM_JT_OFFDELAY);
      pl3.ctOn(AS_CM_CT_X_GE_COND_VALUE_HI);
      /* In following states we want to ignore the rain event since we are anyway
       * in the right direction. We do this by using a condition that will never come true */
      pl3.jtDlyOff(AS_CM_JT_OFFDELAY);
      pl3.ctDlyOff(AS_CM_CT_COND_VALUE_LO_LE_X_LT_COND_VALUE_HI);
      pl3.jtRefOff(AS_CM_JT_REFOFF);
      pl3.ctRepOff(AS_CM_CT_COND_VALUE_LO_LE_X_LT_COND_VALUE_HI);
      pl3.jtRampOff(AS_CM_JT_RAMPOFF);
      pl3.ctRampOff(AS_CM_CT_COND_VALUE_LO_LE_X_LT_COND_VALUE_HI);
    }

    // Setup via webui doesn't work, so I am setting the one I need here
    void setRunningTime(void) {
      BlindList1 list1 = getList1();
      list1.refRunningTimeTopBottom(450);
      list1.refRunningTimeBottomTop(450);
    }
};

class RainEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, bool israining) {
      Message::init(0x0b, msgcnt, 0x53, BIDI | RPTEN, 0x43, israining & 0xff);
    }
};

class RainChannel : public Channel<Hal, RainList1, EmptyList, List4, PEERS_PER_RAIN_CHANNEL, BlindList0>, public Alarm {
  public:
    uint8_t stat;

    /* We poll every second if the rain status changed, at boot we init status to opposite of real
       status so that we send a telegram at boot */
    RainChannel() : stat(digitalRead(RAIN_INPUT_PIN)), Alarm(seconds2ticks(RAIN_POLL_INTERVAL)) {}
    virtual ~RainChannel() {}

    uint8_t flags () const {
      uint8_t flags = this->device().battery().low() ? 0x80 : 0x00;
      return flags;
    }

    uint8_t status() const {
      return stat;
    }

    virtual void trigger (AlarmClock& clock) {
      uint8_t israining = !digitalRead(RAIN_INPUT_PIN);
      if(stat != israining) {
        stat = israining;
        RainEventMsg& rainmsg = (RainEventMsg&)device().message();
        rainmsg.init(device().nextcount(), stat==0?false:true);
        device().sendMasterEvent(rainmsg);

        static uint8_t evcnt = 0;
        SensorEventMsg& rmsg = (SensorEventMsg&)device().message();
        rmsg.init(device().nextcount(), number(), evcnt++, stat==0 ? 0 : 200, false , false);
        device().sendPeerEvent(rmsg, *this);
      }
      tick = (seconds2ticks(RAIN_POLL_INTERVAL));
      clock.add(*this);
    }

    void setup(Device<Hal,BlindList0>* dev,uint8_t number,uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }
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

    BlChannel& getBlindChannel1() {
      return blc1;
    }

    BlChannel& getBlindChannel2() {
      return blc2;
    }

    RainChannel& getRainChannel() {
      return rainc;
    }
};

Hal hal;
Blind2xAndRainDevice sdev(devinfo, 0x20);
ConfigButton<Blind2xAndRainDevice> cfgBtn(sdev);
InternalButton<Blind2xAndRainDevice> btnup(sdev, 4);
InternalButton<Blind2xAndRainDevice> btndown(sdev, 5);
InternalButton<Blind2xAndRainDevice> btnup2(sdev, 6);
InternalButton<Blind2xAndRainDevice> btndown2(sdev, 7);

void initPeerings (bool first) {
  /* Create internal peerings
   *  We peer the 4 internal buttons with the actors for up and down. A
   *  peering on the receiver (blind actor) side is sufficient since the
   *  internal buttons do not care about a peering list.
   *  
   *  We peer the rain detector with the blinds to send a rain event to
   *  them on rain start and stop. We peer the blinds with the rain
   *  detector to close them in case of starting rain. This includes a
   *  setup of the respective List3 to ensure we close in any case.
   */
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    Peer pBlind1(devid, 1);
    Peer pBlind2(devid, 1);
    Peer pRain(devid, 3);
    Peer pUp1(devid, 4);
    Peer pDown1(devid, 5);
    Peer pUp2(devid, 6);
    Peer pDown2(devid, 7);

    sdev.getBlindChannel1().peer(pUp1, pDown1);
    sdev.getBlindChannel1().peer(pRain);
    sdev.getRainChannel().peer(pBlind1);
    sdev.getBlindChannel1().setupList3ForRain(pRain);

    sdev.getBlindChannel2().peer(pUp2, pDown2);
    sdev.getBlindChannel2().peer(pRain);
    sdev.getRainChannel().peer(pBlind2);
    sdev.getBlindChannel2().setupList3ForRain(pRain);

    sdev.getBlindChannel1().setRunningTime();
    sdev.getBlindChannel2().setRunningTime();
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  //storage().setByte(0,0);
  bool first = sdev.init(hal);
  sdev.getBlindChannel1().init(ON_RELAY_PIN, DIR_RELAY_PIN);
  sdev.getBlindChannel2().init(ON_RELAY2_PIN, DIR_RELAY2_PIN);

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
