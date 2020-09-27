// LoRa / TTN libs
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "ttnconfig.h" // Configure secrets here
void os_getDevEui (u1_t* buf) {memcpy_P(buf, DEVEUI, 8 );}
void os_getArtEui (u1_t* buf) {memcpy_P(buf, APPEUI, 8 );}
void os_getDevKey (u1_t* buf) {memcpy_P(buf, APPKEY, 16);} 

// Deep sleep lib
#include "deepsleep.h"
#define ENABLE_DEEP_SLEEP                // Enable deep sleep, otherwise use delay
#define SLEEP_TIME_SEC 250
union {
  int i = 0;
  unsigned char b[2];
} sleep_time_sec;

// Lib to read battery voltage
#include "readvolt.h"
//#define COINCELL
#ifdef COINCELL                          // If device is powered by a coin cell
  #define BATTERY_FULL            3300   // in mV, use max measured voltage
  #define BATTERY_EMPTY           2500   // in mV, use cut-off voltage
#else                                    // If device is powered by a lithium battery
  #define BATTERY_FULL            4200   // in mV, use max measured voltage
  #define BATTERY_EMPTY           3000   // in mV, use cut-off voltage
#endif
union {
  int i = 0;
  unsigned char b[2];
} bat;

// DS18B20 temperature sensor
#define DS18B20                          // Enable if DS18B20 is connected
#define DS18B20_MULTIPLIER      1        // Multiplier
#define DS18B20_OFFSET          0        // Offset in °C
#define DS18B20_POWER_PIN       2        // Powers the sensor
#define DS18B20_PIN             A3       // Data pin for sensor
#define INT_MIN -32768
#ifdef DS18B20
  #include <OneWire.h>
  OneWire ds(DS18B20_PIN);
  union {
    int i = 0;
    unsigned char b[2];
  } ds_t1;
#endif

// Pin mapping
#define V2
#ifdef V1                                // Old design
const lmic_pinmap lmic_pins = {
    .nss = 6,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {3, 4, LMIC_UNUSED_PIN},
};
#endif
#ifdef V2                                // New, more compact design
const lmic_pinmap lmic_pins = {
    .nss = A0,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = A1,
    .dio = {6, 7, LMIC_UNUSED_PIN},
};
#endif

static uint8_t mydata[5];
uint8_t counter = 0;
static osjob_t sendjob;

const unsigned TX_INTERVAL = 1;


void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK) {
              Serial.println(F("Received ack"));
            }
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
              sleep_time_sec.b[0] = LMIC.frame[LMIC.dataBeg];
              sleep_time_sec.b[1] = LMIC.frame[LMIC.dataBeg+1];
              Serial.print(F("New sleep time: "));
              Serial.println(sleep_time_sec.i);
            }
            
            #ifdef ENABLE_DEEP_SLEEP
              for(int i=0; i<(float)sleep_time_sec.i/8; i++) {
                sleep8s();
              }
            #else
              delay((int32_t)sleep_time_sec.i * 1000);
            #endif

            // Do the work
            do_send(&sendjob);

            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
  counter = counter + 1;
  mydata[0] = counter;
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
      Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
      /*************************
      *** READ BATTERY START ***
      **************************/
      int batMv = readVcc();
      bat.i = (float(batMv-BATTERY_EMPTY) / float(BATTERY_FULL-BATTERY_EMPTY)) * 10000.0f;
      if(bat.i > 10000)
        bat.i = 10000;
      Serial.print("Battery: ");
      Serial.print(batMv);
      Serial.print(" mV (");
      Serial.print(float(bat.i/100.0f));
      Serial.println(" %)");
      memcpy(mydata+1, bat.b, 2);
      /*************************
      ***  READ BATTERY END  ***
      **************************/

      /*************************
      *** READ DS18B20 START ***
      **************************/
      #ifdef DS18B20
        digitalWrite(DS18B20_POWER_PIN, HIGH);
        
        byte addr[8];
        byte data[12];
        while(ds.search(addr)) {
          if(!OneWire::crc8(addr, 7) != addr[7]) {
            ds_t1.i  = INT_MIN;
            ds.reset();
            ds.select(addr);
            ds.write(0x44, 1);
            sleep1s();
            ds.reset();
            ds.select(addr);
            ds.write(0xBE);
            for (int i = 0; i < 9; i++) {
              data[i] = ds.read();
            }
            int16_t raw = (data[1] << 8) | data[0];
            byte cfg = (data[4] & 0x60);
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
            ds_t1.i = int(((raw / 16.0) * DS18B20_MULTIPLIER + DS18B20_OFFSET) * 100);
            Serial.print("TempDS: ");
            Serial.print(float(ds_t1.i/100.0f));
            Serial.println(" C");

            memcpy(mydata+3, ds_t1.b, 2);
          }
        }
        ds.reset_search();
      #endif
      /*************************
      ***  READ DS18B20 END  ***
      **************************/

      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
      Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  // Set sleep time to union
  sleep_time_sec.i = SLEEP_TIME_SEC;

  // LMIC init
  os_init();
    
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Relax timing, see https://www.thethingsnetwork.org/forum/t/got-adafruit-feather-32u4-lora-radio-to-work-and-here-is-how/6863/46
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
