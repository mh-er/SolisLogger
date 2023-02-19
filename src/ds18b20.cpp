// ds18b20.cpp read temperature sensor DS18B20 which is connected via Onewire bus
//
// 2023-01-30 mh
// - add DEBUG_TRACE() macro
// - add conditional compiler flag DS18B20
//
// 2022-10-21 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor, see license below.
//   https://github.com/10k-resistor/Solis4Gmini-logger
// - adapted for own use
//
// (C) M. Herbert, 2022 - 2023.
// Licensed under the GNU General Public License v3.0
//

/* *** Description ***
myDS18B20 reads the temperature of a DS28B20 sensor and returns it in Celcius.

If no sensor is found: 85.1 C is returned.
IF DS18B20 == false, 127.0 C is returned.

** Usage **
myDS18B20::getTemperature() read temperature and return is as float celcius.
float myDS18B20::getTempSensorType()  read sensor type (serial.print) and temperature (return as float)
Connection PIN is defined by DS18B20_PIN.

DS18B20 has to be defined for conditional compilation as true or false.

Note: At reset, temperature register of the sensor is initialized to 85 C.

** Implementation **
Implementation is using class OneWire.
  *** end description *** */


/*
License of Solis4Gmin-logger:
10k-resistor/Solis4Gmini-logger is licensed under the GNU General Public License v3.0

Permissions of this strong copyleft license are conditioned on making available complete source code of
licensed works and modifications, which include larger works using a licensed work, under the same license.
Copyright and license notices must be preserved. Contributors provide an express grant of patent rights.
*/
#include <Arduino.h>
#include <OneWire.h>
#include "config.h"
#include "ds18b20.h"


#if(DS18B20)
  OneWire  ds(DS18B20_PIN);  // on pin 10 (a 4.7K resistor is necessary)
#endif

myDS18B20::myDS18B20()
{
}

/*
* Source: https://github.com/adlerweb/RM-RF-Module/blob/master/Software/RMRFSensor/RMRFSensor.ino (Modified)
*/

float myDS18B20::getTemperature()
{

#if(DS18B20)
    byte addr[8];
    byte data[12];
    byte present = 0;
    byte i = 0;
    float celsius;

    ds.reset();
    ds.reset_search();
    ds.search(addr);

    if (addr[0] != 0x28)    //  0x28 means Chip = DS18B20
        return 85.1;

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);
    delay(775); // conversion takes <=750ms

    present = ds.reset();
    if(!present) return -1.0f;
    ds.select(addr);
    ds.write(0xBE); // Read Scratchpad

    for (i = 0; i < 9; i++)     // result is 9 bytes
    {
        data[i] = ds.read();
    }

    int16_t raw = (data[1] << 8) | data[0];         // temperature is in byte 0 (lsb) and byte 1 (msb)
    celsius = (float)raw / 16.0;

    return (float)(celsius);

#else
    return 127.0;
#endif
}
float myDS18B20::getTempSensorType()
{
  // get address and type of the sensor
  // and print it on serial monitor
  // return temperature in Celsius

/*
description of data scratchpad acc. to DS18B20 Data Sheet 
Byte 0 and byte 1 of the scratchpad contain the LSB and the MSB of the temperature register, respectively.
These bytes are read-only.
Bytes 2 and 3 provide access to TH and TL registers.
Byte 4 is the configuration register data, which is explained in detail in the Configuration Register section.
Bytes 5, 6, and 7 are reserved for internal use by the device and cannot be overwritten.
Byte 8 of the scratchpad is read-only and contains the CRC code for bytes 0 through 7 of the scratchpad.

Data is written to bytes 2, 3, and 4 of the scratchpad using the Write Scratchpad [4Eh] command; the data
must be transmitted to the DS18B20 starting with the least significant bit of byte 2. To verify data
integrity, the scratchpad can be read (using the Read Scratchpad [BEh] command) after the data is
written. When reading the scratchpad, data is transferred over the 1-Wire bus starting with the least
significant bit of byte 0. To transfer the TH, TL and configuration data from the scratchpad to EEPROM,
the master must issue the Copy Scratchpad [48h] command.

Configuration register Byte4: 0 R1 R0 1 1 1 1 1 with R1 (bit 6) and R0 (bit5) mean
R1 R0  Resolution
0  0   9bit
0  1   10bit
1  0   11bit
1  1   12bit
*/

#if(DS18B20)

  byte i;
  byte present = 0;
  byte type_s;
  byte data[9];
  byte addr[8];
  float celsius, fahrenheit;
  

    ds.reset();
    ds.reset_search();


  if ( !ds.search(addr)) {      // check if valid search
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();    // reset the bus
    delay(250);
    return -1.0f;
  }
  
  Serial.print("ROM =");   // print the address of the sensor, byte 0 is the type, bytes 1-6 are unique addr, byte 7 is a CRC
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return  -1.0f;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return -1.0f;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  #ifdef MASK_DS18B20_TEMP_RES
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default 0x60 is 12 bit resolution, 750 ms conversion time
  }
  #else
      byte cfg = (data[4] & 0x60);
      Serial.print("Resolution= ");
      Serial.print(cfg, HEX);
      Serial.println();
  #endif
  celsius = (float)raw / 16.0;   // for 12 bit resolution LSB corresponds to 0.0625 = 1/16
  fahrenheit = celsius * 1.8 + 32.0;
DEBUG_TRACE(true,"  Temperature = %.2f Celsius, %.2f Fahrenheit.",celsius, fahrenheit);
  return celsius;
#else
    Serial.println("  no Chip DS18B20");
    return 127.0;
#endif

}