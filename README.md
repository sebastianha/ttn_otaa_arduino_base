# ttn_otaa_arduino_base
A base sketch for TTN experiments with Arduino.

It includes OTAA support and the readings from the following sensors:
- Battery
- DS18B20

There is a counter which is limited to 1 byte (0-255) which wraps around. By sending a downlink packet the sleep time can be set when in operation.

# Configuration
Copy `ttnconfig_sample.h` to `ttnconfig.h` and fill in the secrets from the TTN console

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

Example:
```
{
  "battery": 35,
  "counter": 1,
  "temp_ds": 0
}
```

## TTN Encoder
```
function Encoder(object, port) {
  var bytes = [];

  bytes[0] = object.sleepTimeSec & 0xff
  bytes[1] = object.sleepTimeSec >> 8

  return bytes;
}
```

Example (set sleep time to 1000 seconds):
```
{
  "sleepTimeSec": 1000
}
```
