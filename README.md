# ttn_otaa_arduino_base
A base sketch for [The Things Network](https://www.thethingsnetwork.org/) experiments with Arduino.

It includes OTAA support and the readings from the following sensors:
- Battery
- DS18B20

There is a counter which is limited to 1 byte (0-255) which wraps around. By sending a downlink packet the sleep time can be set when in operation.

# Images with Arduino Pro Mini

<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/arduino_1.jpg" height=200px> <img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/arduino_2.jpg" height=200px> <img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/case.jpg" height=200px>

# Configuration

## TTN details

Copy `ttnconfig_sample.h` to `ttnconfig.h` and fill in the secrets from the TTN console. Make sure that LSB / MSB setting is correct when copying from the TTN console.

### DEVEUI
```
static const u1_t PROGMEM DEVEUI[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // lsb
```
<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/deveui.png" height=100px>

### APPEUI
```
static const u1_t PROGMEM APPEUI[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // lsb
```
<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/appeui.png" height=100px>

### APPKEY
```
static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // msb
```
<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/appkey.png" height=100px>

## TTN Decoder
```
function Decoder(bytes, port) {
  var decoded = {
    counter: bytes[0],
    battery: (bytes[2] << 8 | bytes[1])/100,
    temp_ds: (bytes[4] << 8 | bytes[3])/100
  };
  return decoded;
}
```
<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/decoder.png" height=100px>

Counter is one byte. Battery are two bytes in percent multiplied by 100 before. Therefore, the precision is 2. Same applies for the temperature reading.

### Example:
(no temperature sensor connected, battery 35%)
```
{
  "battery": 35,
  "counter": 1,
  "temp_ds": 0
}
```

Battery is given in percent, relative to the configured connected battery. Two options available: coin cell (2.5–3.3V) or lithium battery (3.0–4.2V). Temperature is in °C.

## TTN Encoder
```
function Encoder(object, port) {
  var bytes = [];

  bytes[0] = object.sleepTimeSec & 0xff
  bytes[1] = object.sleepTimeSec >> 8

  return bytes;
}
```
<img src="https://raw.githubusercontent.com/sebastianha/ttn_otaa_arduino_base/master/doc/images/encoder.png" height=100px>

### Example
(set sleep time to 1000 seconds):
```
{
  "sleepTimeSec": 1000
}
```

# References
- Based on https://github.com/matthijskooijman/arduino-lmic/tree/master/examples/ttn-otaa
- Deep sleep code from http://www.gammon.com.au/power
- Battery voltage code from http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/

# License
[GNU GENERAL PUBLIC LICENSE, Version 3](./LICENSE)

