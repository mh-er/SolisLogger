#ifndef CONFIG_H
#define CONFIG_H
//
// config.h for Solis Inverter Status Register Readout
//
// 2023-02-19 mh
// - first version for git
//
// (C) Copyright M. Herbert, 2022-2023.
// Licensed under the GNU General Public License v3.0


#define MY_TEST 0                 // use test mode, no sensor data used, only test data is sent to VZ_UUID_TEST
#define MY_TEST_SEND_UPDATE 5000  // ms

// AP mode for configuration
// Identify configuration info in EEPROM, Modifying cause a loss of the existing configuration in EEPROM
// note: EEPROM configuration remains unchanged after firmware update; update version count if you are using a new application/configuration
// otherwise the previous configuration is considered valid.
#define WIFI_AP_CONFIG_VERSION "3.2.1"   // 4 bytes are significant for check with EEPROM (IOTWEBCONF_CONFIG_VERSION_LENGTH in confWebSettings.h)

#define WIFI_AP_SSID "SolisLogger"
#define WIFI_AP_IP "192.168.4.1"            // default address, set by the framework.
#define WIFI_AP_DEFAULT_PASSWORD "<yourDefaultAPpassword>"
#define WEBCONF_AP_MODE_CONFIG_PIN D3       // to force AP mode

//your own WLAN
#define MY_SSID "<yourWLANname>"
#define MY_PASSWORD ""
#define MY_HOSTNAME "SolisLogger"

//  Time
#define NMAX_DATE_TIME 20               // max length of date and time string
#define TIMEZONE +1                     // Central europe
#define TIMEZONE_DEFAULT "1"            // string default for configuration

// build in LED is inverted for Wemos D1 mini
#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1

// Special Heart Beat values
#define HEART_BEAT_RESET 1
#define HEART_BEAT_WIFI_CONFIG 2

// Verbose Level for output to serial monitor
#define VERBOSE_LEVEL_Setup  1
#define VERBOSE_LEVEL_Loop 0
#define VERBOSE_LEVEL_WLAN  1
#define VERBOSE_LEVEL_HTTP  0
#define VERBOSE_LEVEL_Temperature 1
#define VERBOSE_LEVEL_Inverter  1
#define VERBOSE_LEVEL_InverterAccess  0
#define VERBOSE_LEVEL_InverterData  1
#define VERBOSE_LEVEL_TIME 0

#define DATE_UPDATE_INTERVAL 60000 // in ms; for Dash Board

// Volkszaehler
#define VZ_SERVER             "<yourVolkszaehlerServer_name_or_IP>"
#define VZ_MIDDLEWARE         "middleware.php"
#define VZ_DATA_JSON          "data.json"
#define VZ_UUID_NO_SEND       "null"        // use this uuid if you do not want to transmit data for a channel

#define VZ_UUID_TEMP_CH6              "abcdefgh-1234-5678-90ab-cdfghijklmno"   // channel UUID for temperature sensor
#define VZ_UUID_INV_POWER             "abcdefgh-1234-5678-90ab-cdfghijklmnp"   // 7
#define VZ_UUID_INV_DC_U              "abcdefgh-1234-5678-90ab-cdfghijklmnq"   // 8
#define VZ_UUID_INV_DC_I              "abcdefgh-1234-5678-90ab-cdfghijklmnr"   // 9
#define VZ_UUID_INV_DC_POWER          "abcdefgh-1234-5678-90ab-cdfghijklmns"   // 10
#define VZ_UUID_INV_ENERGY_LASTDAY    "abcdefgh-1234-5678-90ab-cdfghijklmnt"   // 11
#define VZ_UUID_INV_ENERGY_LASTMONTH  "abcdefgh-1234-5678-90ab-cdfghijklmnu"   // 12
#define VZ_UUID_TEST                  "abcdefgh-1234-5678-90ab-cdfghijklmnv"   // 13 for test
#define VZ_UUID_INV_ENERGY_THISDAY    "abcdefgh-1234-5678-90ab-cdfghijklmnw"   // 14
#define VZ_UUID_INV_HEART_BEAT        "abcdefgh-1234-5678-90ab-cdfghijklmnx"   // 15  InverterHeartBeat				Debug-Kanal


// DS18B20
// use DS18B20
#define DS18B20 true
#define DS18B20_PIN 4 // D2 (4)
#define DS18B20_READ_INTERVAL 300 // in s

// Solis Inverter

// RS485 - MODBUS CONFIG
// slaveID from inverter
#define MODBUS_SLAVE_ID_INVERTER 1
#define MODBUS_READ_DELAY 2550 // delay between reading data
#define MODBUS_RETRY_NUMBER 2  // retry of reads in case of MODBUS request error
#define MODBUS_RETRY_INTERVAL 4000  // ms delay between re-tries 

#define INVERTER_READ_INTERVAL_FREQUENT 60 // in s
#define INVERTER_READ_INTERVAL_SELDOM (60*20) // in s


/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
#define MAX485_DE 13 // Data Enable at D7 (13)
#define rs485_RX 12  // RX at D6 (12) 
#define rs485_TX 15  // TX at D8 (15)


// Status leds
#define LED_USE true
#define LED_BLUE   14 // D5(14)
#define LED_YELLOW 16 // D0(16)


#endif // CONFIG_H