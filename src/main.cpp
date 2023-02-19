// main.cpp to evaluate Solis Inverter and Temperature Sensor DS18B20
// 
// 2023-02-19 mh
// - first version for git
//
// 2022-11-12 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor, see license below.
//   https://github.com/10k-resistor/Solis4Gmini-logger
// - adapted for own use
//
// Copyright (C) M. Herbert, 2022-2023.
// Licensed under the GNU General Public License v3.0
//

/* ***

See README.md for a description.

Based on Solis4Gmin-logger, see https://github.com/10k-resistor/Solis4Gmini-logger with major modifications.

*** end description *** */

/*
License of Solis4Gmin-logger:
10k-resistor/Solis4Gmini-logger is licensed under the GNU General Public License v3.0

Permissions of this strong copyleft license are conditioned on making available complete source code of
licensed works and modifications, which include larger works using a licensed work, under the same license.
Copyright and license notices must be preserved. Contributors provide an express grant of patent rights.
*/

// c and cpp
#include <stdio.h>
#include <string.h>
// Arduino + ESP framework
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <ESPDash.h>
// own libs
#include <confWeb.h>
#include <confWebParameter.h>
//#include <timeControl.h>
#include <TimeLib.h>
// local dir
#include "main.h"
#include "config.h"
#include "ds18b20.h"
#include "led.h"
#include "modbus.h"
#include "myTicker.h"
#include "vzHttp.h"

// local function declaration
//String toStringIp(IPAddress ip);

// inverter stuff
inverter Inverter;
char s_inverterStatus[8];
enum inverterRead {readAll, readPower, readDayEnergy, readMonthYearEnergy};
void readInverter();
void publishInverterFrequentValues();
void publishInverterSeldomValues();

// callback handler and html page functions
void wifiConnected();
void configSaved();
void notFound(AsyncWebServerRequest *request);

void homePage(String& content);
void handleRoot(AsyncWebServerRequest *request);
void startHtml(AsyncWebServerRequest *request);
void onConfiguration(AsyncWebServerRequest *request);

String buildResponse(byte type);

void onReset(AsyncWebServerRequest *request);
boolean needReset = false;

String currentHtmlPage =" ";     // sub-headline for different modes
String currentSSID = "unknown";
String currentIP   = "unknown";
char c_wifiIP[40];
String vzServerIP = "unknown";

// setup clock interface, ticker and timer stuff
myTicker ticker;

uint16_t  Year = 1960;
uint8_t   Month = 0;
uint8_t   Day = 0;
uint8_t   Hour = 0;
uint8_t   Minute = 0;
int       Timezone = TIMEZONE;  
char      s_TimezoneOffset[4] = TIMEZONE_DEFAULT;

time_t    getLocalTime();                 // callback for TimeLib
time_t    getEpochTime();                 // UNIX epoch time in sec
void      getDateTime(char* s_DateTime);  // update date and time, return char string

uint64_t epochtime = 0;   // in seconds - not ms to avoid overrun
String s_epochtime = "0";
String s_timeStamp = "0";   // in seconds
char s_DateTime[NMAX_DATE_TIME] = "1960-01-01 00:00";

uint32_t lastDateUpdate=0;
uint8_t lastDay = 100;        // used to store last day
uint8_t lastMonth = 100;      // used to store last month

// LED stuff
LED led;

// temperature sensor stuff
  void readDS18B20();
#if(DS18B20)
  myDS18B20 ds18b20;
  float ds18b20Temperature = -1.0;
  boolean readDS18B20firstTime = true;
#endif
  
// volkszaehler stuff
VzHttpConfig vzHttpConfig;
VzHttp  vz_http;
int httpStatus = 0;
String vzUUID;

// server and WiFi stuff
// class for WiFi and webserver configuration page, connects to WiFi in AP or STA mode
// note: constructor does some presets
DNSServer dnsServer;
AsyncWebServer server(80);

IotWebConf confWeb(WIFI_AP_SSID, &dnsServer, &server, WIFI_AP_DEFAULT_PASSWORD, WIFI_AP_CONFIG_VERSION);
boolean b_WiFi_connected = false;
boolean b_WiFi_firstTime = true;

// custom configurationparameter: TextParameter(label,id,valueBuffer,lengthValueBuffer,defaultValue,placeholder,customHtml)
// configuration input is in valueBuffer
TextParameter confVZserverParam = TextParameter("VZ Server", "vzServer", vzHttpConfig.vzServer, sizeof(vzHttpConfig.vzServer),
                                                   VZ_SERVER, nullptr, "vzServer");
TextParameter confVZmiddlewareParam = TextParameter("VZ Middleware", "vzMiddleware", vzHttpConfig.vzMiddleware, sizeof(vzHttpConfig.vzMiddleware),
                                                   VZ_MIDDLEWARE, nullptr, "vzMiddleware");
TextParameter confVZuuidTemp6Param = TextParameter("UUID Temp6", "UUID-Temp6", &vzHttpConfig.uuidValue[vzTEMP_CH6][0], sizeOfUUID,
                                                   VZ_UUID_TEMP_CH6, nullptr, "UUID-Temp6");
TextParameter confVZuuidInvPowParam = TextParameter("UUID InvPower", "UUID-InvPower", &vzHttpConfig.uuidValue[vzINV_POWER][0], sizeOfUUID,
                                                   VZ_UUID_INV_POWER, nullptr, "UUID-InvPower");
TextParameter confVZuuidInvEnLastDayParam = TextParameter("UUID InvEnergyLastDay", "UUID-InvEnergyLastDay", &vzHttpConfig.uuidValue[vzINV_ENERGY_LASTDAY][0], sizeOfUUID,
                                                   VZ_UUID_INV_ENERGY_LASTDAY, nullptr, "UUID-InvEnergyLastDay");
TextParameter confVZuuidInvEnLastMonthParam = TextParameter("UUID InvEnergyLastMonth", "UUID-InvEnergyLastMonth", &vzHttpConfig.uuidValue[vzINV_ENERGY_LASTMONTH][0], sizeOfUUID,
                                                   VZ_UUID_INV_ENERGY_LASTMONTH, nullptr, "UUID-InvEnergyLastMonth");
TextParameter confVZuuidInvDCIParam = TextParameter("UUID InvDCI", "UUID-InvDCI", &vzHttpConfig.uuidValue[vzINV_DC_I][0], sizeOfUUID,
                                                   VZ_UUID_INV_DC_I, nullptr, "UUID-InvDCI");
TextParameter confVZuuidInvDCUParam = TextParameter("UUID InvDCU", "UUID-InvDCU", &vzHttpConfig.uuidValue[vzINV_DC_U][0], sizeOfUUID,
                                                   VZ_UUID_INV_DC_U, nullptr, "UUID-InvDCU");
TextParameter confVZuuidInvDCPowParam = TextParameter("UUID InvDCPow", "UUID-InvDCPow", &vzHttpConfig.uuidValue[vzINV_DC_POWER][0], sizeOfUUID,
                                                   VZ_UUID_INV_DC_POWER, nullptr, "UUID-InvDCU");
TextParameter confVZuuidTestParam = TextParameter("UUID Test", "UUID-Test", &vzHttpConfig.uuidValue[vzTEST][0], sizeOfUUID,
                                                   VZ_UUID_TEST, nullptr, "UUID-Test");
TextParameter confVZuuidInvEnThisDayParam = TextParameter("UUID InvEnergyThisDay", "UUID-InvEnergyThisDay", &vzHttpConfig.uuidValue[vzINV_ENERGY_THISDAY][0], sizeOfUUID,
                                                   VZ_UUID_INV_ENERGY_THISDAY, nullptr, "UUID-InvEnergyThisDay");
TextParameter confVZuuidInvHeartBeatParam = TextParameter("UUID InvHeartBeat", "UUID-InvHeartBeat", &vzHttpConfig.uuidValue[vzINV_HEART_BEAT][0], sizeOfUUID,
                                                   VZ_UUID_INV_HEART_BEAT, nullptr, "UUID-InvHeartBeat");

NumberParameter confTimezoneParam = NumberParameter("TimezoneOffset[h]", "TimezoneOffset", s_TimezoneOffset, sizeof(s_TimezoneOffset),
                                                   TIMEZONE_DEFAULT, nullptr, "TimezoneOffset");
ParameterGroup paramGroup = ParameterGroup("VZ Settings", "VZ-Settings");

Parameter* thingName;                   // name set on configuration page, might override WIFI_AP_SSID
char wifiAPssid[IOTWEBCONF_WORD_LEN] = WIFI_AP_SSID;

// ESP-Dash
ESPDash dashboard(&server);

Card card_Title(&dashboard, GENERIC_CARD, "Title");
Card card_Time(&dashboard, GENERIC_CARD, "Date & Time");
Card card_power(&dashboard, GENERIC_CARD, "Power (W)");
Card card_DC_P(&dashboard, GENERIC_CARD, "DC power (W)"); 

Card card_energyToday(&dashboard, GENERIC_CARD, "Energy Today (kWh)");
Card card_energyThisMonth(&dashboard, GENERIC_CARD, "Energy This Month (kWh)");
Card card_energyThisYear(&dashboard, GENERIC_CARD, "Energy This Year (kWh)");

Card card_energyLastDay(&dashboard, GENERIC_CARD, "Energy Last Day (kWh)");
Card card_energyLastMonth(&dashboard, GENERIC_CARD, "Energy Last Month (kWh)");
Card card_energyLastYear(&dashboard, GENERIC_CARD, "Energy Last Year (kWh)");

Card card_DC_U(&dashboard, GENERIC_CARD, "DC U (V)");
Card card_DC_I(&dashboard, GENERIC_CARD, "DC I (A)");
Card card_EpochTime(&dashboard, GENERIC_CARD, "Epoch Time (s)");

Card card_temperatureDS18B20(&dashboard, TEMPERATURE_CARD, "Room Temperature (°C)");
Card card_temperatureInverter(&dashboard, TEMPERATURE_CARD, "Inverter Temperature (°C)");

Card card_status(&dashboard, STATUS_CARD, "Loop Status", "empty");
Card card_inverterStatus(&dashboard, STATUS_CARD, "Inverter Status", "empty");


float dc_u = -1.0;
float dc_i = -1.0;

float ac_u = -1.0;
float ac_i = -1.0;
float ac_f = -10.0;

float power = -1.0;
float DCpower = -1.0;
float energyToday = -1.0;
float energyLastDay = -1.0;
float energyThisMonth = -1.0;
float energyLastMonth = -1.0;
float energyThisYear = -1.0;
float energyLastYear = -1.0;
float temperature = -1.0;  // inverter temperature

String s_loopCount;
char myStringBuf[80]; 
float vzTestValue=0.0;

void setup()   // -----------------------------------------------------------------------
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);     // Pin2 = D4  = D1 mini LED_BUILTIN
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

  delay(2000);  // delay in ms, allow a monitor to connect.
  Serial.println(" ");
  Serial.println("Hello");

  led.begin();
  
  confWeb.setConfigPin(WEBCONF_AP_MODE_CONFIG_PIN);  // used to enter config mode if PIN is pulled to ground during init()

  //--- Start AsyncWebServer + Handler --------------------------------------------------------------
  server.onNotFound(notFound);

  server.on("/", handleRoot);   			// note: will address Dash board, handleRoot is ignored
  server.on("/index.html", startHtml);    	// url for home/start page
  server.on("/start", startHtml);         	// url for home/start page
  server.on("/config", onConfiguration);  	// url for configuration page
  server.on("/reset", onReset);           	// url to reset ESP
  

  // allows to request data in json format by addressing these urls
  server.on("/api/power.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", buildResponse(0)); });
  server.on("/api/all.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", buildResponse(1)); });


  // own config parameter group
  // note: system configuration parameter group is built-in
  paramGroup.addItem(&confVZserverParam);
  paramGroup.addItem(&confVZmiddlewareParam);
  paramGroup.addItem(&confVZuuidTemp6Param);
  paramGroup.addItem(&confVZuuidInvPowParam);
  paramGroup.addItem(&confVZuuidInvEnLastDayParam);
  paramGroup.addItem(&confVZuuidInvEnLastMonthParam);
  paramGroup.addItem(&confVZuuidInvDCIParam);
  paramGroup.addItem(&confVZuuidInvDCUParam);
  paramGroup.addItem(&confVZuuidInvDCPowParam);
  paramGroup.addItem(&confVZuuidInvEnThisDayParam);
  paramGroup.addItem(&confVZuuidInvHeartBeatParam);
  paramGroup.addItem(&confVZuuidTestParam);
  paramGroup.addItem(&confTimezoneParam);
  confWeb.addParameterGroup(&paramGroup);


  // handler for web configuration
  confWeb.setConfigSavedCallback(&configSaved);
  confWeb.setWifiConnectionCallback(&wifiConnected);

  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

  // Start server
  //--- we start in AP mode to allow configuration and switch to STA mode after timeout.

  boolean validConfig = confWeb.init();     // read configuration from EEPROM

  if (!validConfig)
  {
    DEBUG_TRACE(true,"Missing or invalid config. Possibly unable to initialize all functionality.");
  }
  else
  {
    DEBUG_TRACE(true,"Setup: valid config.");
    // here we can do some setup stuff that requires valid configuration data
      // fetch name from configuration
    thingName =  confWeb.getThingNameParameter();
    strncpy(wifiAPssid, thingName->valueBuffer,IOTWEBCONF_WORD_LEN);
    Timezone = atoi(s_TimezoneOffset);
   
    vz_http.init(vzHttpConfig);   // transfer data from config structure to main class
  }

  card_Title.update(wifiAPssid);
  card_status.update("Starting");
  dashboard.sendUpdates();


  // --- set time, callback for TimeLib to get the time, is called in given interval to sync
  setSyncInterval(300);             // note: not necessary as preset value of TimeLib is 300
  setSyncProvider(getLocalTime);    // callback for TimeLib, setSyncProvider() calls immediately getLocalTime() via now() in TimeLib

//  Clock.begin();
  delay(100);

  //  ticker.begin() - uses operating system software ticker
  ticker.begin();

// here we probably do not have a connection to NTP server yet. time is relative to boot until time from NTP server is received, typically after 40s

  getDateTime(s_DateTime);
  DEBUG_TRACE(true,"%s",s_DateTime);

    s_epochtime = String(getEpochTime());
    card_status.update("entering loop");
    card_Time.update(s_DateTime);
    card_EpochTime.update(s_epochtime);
    dashboard.sendUpdates();
    lastDateUpdate = millis();

  if(MY_TEST)
  {
    DEBUG_TRACE(true,"Inverter not used.");
  }
  else
  {
    //  initialize modbus
    Inverter.begin();
    DEBUG_TRACE(true,"Inverter setup done.");
  }

  // start in AP mode
  currentSSID = String(wifiAPssid);
  currentIP = WIFI_AP_IP;

// --- read inverter and send data to dash board and monitor ----------------------------

  led.yellowOn();
    card_inverterStatus.update("Reading inverter ... ");
    dashboard.sendUpdates();

  if(!MY_TEST)
  {
    readInverter();   // we just read the inverter for the dash board, no http post
      card_inverterStatus.update("Reading inverter ... done");
      dashboard.sendUpdates();
  }
  led.yellowOff();

  DEBUG_TRACE(VERBOSE_LEVEL_Setup,"%s: Setup done.----------------------------------------------", s_DateTime);
    card_status.update("Setup done");
    dashboard.sendUpdates();
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

}

u32_t count=0;
u32_t count10000;
void loop()                // -----------------------------------------------------------
{
	if (needReset)  // Doing a chip reset caused by config changes
	{
		DEBUG_TRACE(true,"Rebooting after 1 second.");
    needReset = false;
    led.yellowOn();
    led.blueOn();
       // post to volkszaehler
    s_epochtime = String(getEpochTime());
    vz_http.postHttp(String(confVZuuidInvHeartBeatParam.valueBuffer), s_epochtime, HEART_BEAT_RESET);
		delay(1000);
		ESP.restart();
	}

  confWeb.doLoop();     // keep the configuration UI and the web server running.

  // need to wait until WiFi connection is established.
  if(b_WiFi_connected)
  {
    if(b_WiFi_firstTime)
    {
      if(millis() > 40000)  // we wait 40sec, then we should have a valid time
      {
        // first we need to get a valid time
        getDateTime(s_DateTime);
        epochtime = getEpochTime();
        s_epochtime = String(epochtime);

          card_Time.update(s_DateTime);
          card_EpochTime.update(s_epochtime);
          dashboard.sendUpdates();

        uint32_t waitForTime = 0;   // just a loop for waiting, avoiding delay()
        while( (waitForTime < 10000) && (epochtime < 1672531200ULL) )  // 1672531200ULL = 2023-01-01
        {
            epochtime = getEpochTime();
            waitForTime++;
        }

        vz_http.postHttp(String(confVZuuidInvHeartBeatParam.valueBuffer), s_epochtime, HEART_BEAT_WIFI_CONFIG);
        if(epochtime > 1672531200ULL)     // now we have a valid time
        {
          // here, we should have connection to ntp server and valid time
          b_WiFi_firstTime = false;
          setSyncProvider(getLocalTime);    // setting again will force TimeLib to sync with system time
          getDateTime(s_DateTime);

          IPAddress result;
          if (WiFi.hostByName(vzHttpConfig.vzServer, result))
          {
            vzServerIP = result.toString();
            DEBUG_TRACE(true,"vzServerIP = %s",vzServerIP.c_str());
          }

          DEBUG_TRACE(true,"%s: system time set",s_DateTime);
        }
      }
    }
    else
    {
      if(MY_TEST)   // send data to test channel
      {
        vz_http.testHttp();
      }
      else
      {
        publishInverterFrequentValues();
        publishInverterSeldomValues();      // values for day, month, year
        readDS18B20();
      }
    }
  }
	yield();
  
// --- wait for next trigger ------------------------------------------------------------

  led.blueOff();
  led.yellowOff();

    // update dash board
  if((millis()- lastDateUpdate) > DATE_UPDATE_INTERVAL)  // diff of unsigned is positive, if millis() wraps around diff becomes big
  { 
    lastDateUpdate = millis();

    // update dashboard status
    count10000 = count/10000;
    s_loopCount = "waiting in loop, #" + String(count10000);
      card_status.update(s_loopCount);
    getDateTime(s_DateTime);
    s_epochtime = String(getEpochTime());
      card_Time.update(s_DateTime);
      card_EpochTime.update(s_epochtime);
      dashboard.sendUpdates();

    if(MY_TEST)
    {
      vzTestValue = vz_http.getValue(vzTEST);
      sprintf(myStringBuf,"%.5f",vzTestValue);
      card_energyLastYear.update(myStringBuf);    // show value on dash board
      dashboard.sendUpdates();
    }
    else
    {
      // heart beat post to volkszaehler
      if ((count10000 != HEART_BEAT_RESET) && (count10000 != HEART_BEAT_WIFI_CONFIG))
      {
        vz_http.postHttp(String(confVZuuidInvHeartBeatParam.valueBuffer), s_epochtime, count10000); // count10000 should fit into a float
      }
    }


  }
  if((millis()- lastDateUpdate)%5000 == 0)
  {
    String s_timestamp = String(getEpochTime());        // timestamp with resolution of 1 sec
    DEBUG_TRACE(MY_TEST,"loop timestamp = %s",s_timestamp.c_str() );
  }
  if(count%10000 == 0)
  {
    DEBUG_TRACE(VERBOSE_LEVEL_Loop,"loop_count=%d", count/10000);
  }
  count++;

//  delay(500);  // wait ms - avoid in loop() !!!

//  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);  // Pin2 = D4
}

// ##########################################################################################
/* ***
readInverter()
- read the data from inverter using modbus class Inverter.
- transfer data to global variables for further processing
- publish data to dash board

2023-0-31 mh
- removed enum inverterRead; all data is transfered to global variables and all dash board values are upated.

*** */
void readInverter()
{
  int inverterStatus=0;

  DEBUG_TRACE(VERBOSE_LEVEL_Inverter,"----readInverter");

    inverterStatus = Inverter.requestAll();        // read all registers with one request;

  // instantaneous values
    Inverter.getPower(&power);
    Inverter.getDCPower(&DCpower);
    Inverter.getAC(&ac_u, &ac_i, &ac_f);
    Inverter.getDC(&dc_u, &dc_i);
    Inverter.getTemperature(&temperature);

    DEBUG_TRACE(VERBOSE_LEVEL_InverterData,
             "Power: %.2fW, DC Power Inverter: %.2fW, DC U: %.2fV, DC_I: %.2fA, AC U: %.2fV, AC I %.2fA, AC F: %.4fHz",
              power, DCpower, dc_u, dc_i, ac_u, ac_i,ac_f);
    DEBUG_TRACE(VERBOSE_LEVEL_Inverter,"Inverter temperature %.2fC",temperature);

    card_temperatureInverter.update(temperature);
    card_power.update(power);
    card_DC_P.update(dc_u*dc_i);
    card_DC_U.update(dc_u);
    card_DC_I.update(dc_i);

    //card_AC_U.update(ac_u);
    //card_AC_I.update(ac_i);
    //card_AC_F.update(ac_f);

  // daily values
    Inverter.getEnergyToday(&energyToday);
    Inverter.getEnergyLastDay(&energyLastDay);
    DEBUG_TRACE(VERBOSE_LEVEL_InverterData,"Energy today: %.2fkWh, Energy Last Day: %.2fkWh",energyToday,energyLastDay);
    
    if (Inverter.isInverterReachable() == true)   // update only if new values available
    {
      card_energyToday.update(energyToday);
      card_energyLastDay.update(energyLastDay);
    }

  // monthly and yearly values
    Inverter.getEnergyThisMonth(&energyThisMonth);
    Inverter.getEnergyLastMonth(&energyLastMonth);
    Inverter.getEnergyThisYear(&energyThisYear);
    Inverter.getEnergyLastYear(&energyLastYear);
    DEBUG_TRACE(VERBOSE_LEVEL_InverterData,
                "Energy ThisMonth: %.2fkWh, Energy LastMonth: %.2fkWh, Energy ThisYear: %.2fkWh, Energy LastYear: %.2fkWh",
                energyThisMonth,energyLastMonth,energyThisYear,energyLastYear);

    if (Inverter.isInverterReachable() == true)   // update only if new values available
    {
      card_energyThisMonth.update(energyThisMonth);
      card_energyLastMonth.update(energyLastMonth);
      card_energyThisYear.update(energyThisYear);
      card_energyLastYear.update(energyLastYear);
    }

  sprintf(s_inverterStatus,"0x%X",inverterStatus);
  if (Inverter.isInverterReachable() == true)
  {
    Inverter.setIsInverterReachableFlagLast(true);
      card_inverterStatus.update(s_inverterStatus, "success");
  }
  else
  {
    led.blueOn();
      card_inverterStatus.update(s_inverterStatus, "danger");
  }
  dashboard.sendUpdates();

  DEBUG_TRACE(VERBOSE_LEVEL_InverterData, "Inverter Status: %s", s_inverterStatus);


}
// ##########################################################################################
/* ***
publishInverterFrequentValues():  
  read inverter frequently based on a ticker event and send data to http server
                yellow led is switched on during execution.
                yellow and blue led is switched on if inverter was not reachable.
  The function calls readInverter() to get all the inverter data from the hardware into global variables.
  The values for frequent updates are published to dash board and to http server then.

2023-02-01 M. Herbert
- first version, code carved out from loop()

*** */
void publishInverterFrequentValues()
{
  // --- read inverter and send data to dash board and monitor ----------------------------
  if (ticker.getInverterFrequentFlag() == true)           // ------- frequent read
  {
    ticker.setInverterFrequentFlagToFalse();
    led.yellowOn();

    getDateTime(s_DateTime);
    epochtime = getEpochTime();
    s_epochtime = String(epochtime);

    s_timeStamp = String(epochtime - (epochtime % INVERTER_READ_INTERVAL_FREQUENT)); // allign to full intervals
      card_inverterStatus.update("Reading inverter","idle");
      card_Time.update(s_DateTime);
      card_EpochTime.update(s_timeStamp);
      dashboard.sendUpdates();

    DEBUG_TRACE(VERBOSE_LEVEL_Inverter,"%s",s_DateTime);

    readInverter();            // this call accesses the inverter and reads all data registers

    if(Inverter.isInverterReachable() == true)
    {
      // post to volkszaehler
      vz_http.postHttp(String(confVZuuidInvPowParam.valueBuffer), s_timeStamp, power);
      vz_http.postHttp(String(confVZuuidInvDCUParam.valueBuffer), s_timeStamp, dc_u);
      vz_http.postHttp(String(confVZuuidInvDCIParam.valueBuffer), s_timeStamp, dc_i);
      vz_http.postHttp(String(confVZuuidInvDCPowParam.valueBuffer), s_timeStamp, dc_i*dc_u);
    }
    if((Inverter.isInverterReachable() == true) && (200 == httpStatus)) 
    {
      led.yellowOff();
    }
    else
    {
      led.blueOn();
    }
  }
}

// ##########################################################################################
/* ***
publishInverterSeldomValues():  
  read inverter data that changes seldomly (e.g. data of day, month, year)
                based on a ticker event send data to http server
                yellow led is switched on during execution.
                yellow and blue led is switched on if inverter was not reachable.
                assumption: global variables are up-to-date, i.e. set previously by publishInverterFrequentValues()

2023-01-31 M. Herbert
- first version, code carved out from loop()

*** */
void publishInverterSeldomValues()
{
  if (ticker.getInverterSeldomFlag() == true)                // ------- seldom read
  {
    ticker.setInverterSeldomFlagToFalse();
    
    DEBUG_TRACE(VERBOSE_LEVEL_Inverter,"%s  ******  seldom part  ******",s_DateTime);

    led.yellowOn();

    getDateTime(s_DateTime);
    epochtime = getEpochTime();
    s_epochtime = String(epochtime);
    
    s_timeStamp = String(epochtime - (epochtime % INVERTER_READ_INTERVAL_SELDOM)); // allign to full intervals
      card_inverterStatus.update("Reading inverter seldom","idle");
      card_Time.update(s_DateTime);
      card_EpochTime.update(s_timeStamp);
      dashboard.sendUpdates();

    if (Inverter.isInverterReachable() == true)
    {
      httpStatus=vz_http.postHttp(String(confVZuuidInvEnThisDayParam.valueBuffer), s_timeStamp, energyToday);
    }
    if((Inverter.isInverterReachable() == true) && (200 == httpStatus)) 
    {
      led.yellowOff();  // transfer ok
    }
    else
    {
      led.blueOn();
    }  
  
    // post lastDay to volkszaehler daily
    if(lastDay != Day)            // ------- daily is enough
    {
      led.yellowOn();

      if (Inverter.isInverterReachable() == true)
      {
        httpStatus=vz_http.postHttp(String(confVZuuidInvEnLastDayParam.valueBuffer), s_timeStamp, energyLastDay);
        if (200 == httpStatus)    // transfer ok
        {
          lastDay = Day;
          led.yellowOff();
        }
        else
        {
          led.blueOn();
        }
      }
      else
      {
        led.blueOn();
      }

    }
    // post to volkszaehler monthly
    if(lastMonth != Month)             // ------- monthly, is enough
    {
      led.yellowOn();

      if (Inverter.isInverterReachable() == true)
      {
        httpStatus = vz_http.postHttp(String(confVZuuidInvEnLastMonthParam.valueBuffer), s_timeStamp, energyLastMonth);
        if (200 == httpStatus)   // transfer ok
        {
          lastMonth = Month;
          led.yellowOff();
        }
        else
        {
          led.blueOn();
        }
      }
      else
      {
        led.blueOn();
      }
    }
  }
}

// ##########################################################################################
/* ***
readDS18B20():
- read temperature sensor DS18B20 based on ticker event and send data to Dash Board and http server
- blue led is switched on during execution.

2023-01-31 M. Herbert
- first version, code carved out from loop()

*** */
void readDS18B20()
{
#if(DS18B20)

  if(readDS18B20firstTime)
  {
    led.blueOn();
    readDS18B20firstTime = false;
    epochtime=getEpochTime();
    s_timeStamp = String(epochtime - (epochtime % DS18B20_READ_INTERVAL)); // allign to full intervals
      card_status.update("1st get started");
      card_Time.update(s_DateTime);
      card_EpochTime.update(s_timeStamp);
      dashboard.sendUpdates();

    // Read DS18B20 - first time including additional Info on serial monitor
    ds18b20Temperature = ds18b20.getTempSensorType();  
      card_temperatureDS18B20.update(ds18b20Temperature);
      card_status.update("1st get done");
      dashboard.sendUpdates();

    // post to volkszaehler
    vz_http.postHttp(String(confVZuuidTemp6Param.valueBuffer), s_timeStamp, ds18b20Temperature);

    led.blueOff();
  }

// Read DS18B20 every x seconds, depended on ticker which is driggered by OS timer
  else if (ticker.getDS18B20Flag() == true)
  {
    led.blueOn();

    ticker.setDS18B20FlagToFalse();
      card_status.update("updating ....","success");
      dashboard.sendUpdates();
    
    // --- read DS18B20 ---------------------------------------------------------------------
     ds18b20Temperature = ds18b20.getTemperature();

    epochtime=getEpochTime();

    s_timeStamp = String(epochtime - (epochtime % DS18B20_READ_INTERVAL)); // allign to full intervals
    DEBUG_TRACE(VERBOSE_LEVEL_Temperature,"%s temperature: %.2f",s_DateTime, ds18b20Temperature);
  
    // post to volkszaehler
    vz_http.postHttp(String(confVZuuidTemp6Param.valueBuffer), s_timeStamp, ds18b20Temperature);
      card_temperatureDS18B20.update(ds18b20Temperature);
      card_EpochTime.update(s_timeStamp);
      dashboard.sendUpdates();
    
    led.blueOff();
  }
#endif // #if(DS18B20)
}
// ##########################################################################################
void configSaved()
//
// configSaved() callback handler for confWeb, when configuration was saved.
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
	DEBUG_TRACE(SERIAL_DEBUG,"Configuration was updated.");
	//needReset = true;   // mh: currently, no reset - needs to be done manually to get new config values
}
// ##########################################################################################

void wifiConnected()
{
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  currentSSID = WiFi.SSID();
  currentIP = WiFi.localIP().toString();

	DEBUG_TRACE(SERIAL_DEBUG,"WiFi connection established, SSID=%s, IP=%s",currentSSID.c_str(),currentIP.c_str());
  b_WiFi_connected = true;
}
// ##########################################################################################
void notFound(AsyncWebServerRequest *request)
{
  String _content = "<div style='padding-top:25px;'><b>NotFoundHandler: Page not found</b></div>";
  _content += "<div style='padding-top:25px;'><a href='/start'>Return to Start Page '/start'</a></div>";
  request->send(404, "text/html", _content);
}
// ##########################################################################################
// request handler for /reset
void onReset(AsyncWebServerRequest *request)
//
// onReset() request handler for /reset
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  needReset = true;
  request->send(200, "text/html; charset=UTF-8", "Rebooting after 1 sec.");
}
// ##########################################################################################
// request handler for /start
void startHtml(AsyncWebServerRequest *request)
//
// indexHtml() provides an index page as anchor for a webpage
//
// 2022-12-30	mh
// - first version
//
// (C) M. Herbert, 2022.
// Licensed under the GNU General Public License v3.0
{
  String	content;

  if(b_WiFi_connected)
  {
    currentHtmlPage = "Local Net Start Page";
  }
  else
  {
    currentHtmlPage = "Access Point Start Page";    
  }

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);

}
// ##########################################################################################
void onConfiguration(AsyncWebServerRequest *request)
//
// onConfiguration() handler for configuration page /config
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{

    DEBUG_TRACE(true,"onConfiguration:calling handleConfig()");

    // this shows the configuration page
    confWeb.handleConfig(reinterpret_cast<WebRequestWrapper*>(request));
    //confWeb.handleConfig();
  
}
// ##########################################################################################
void handleRoot(AsyncWebServerRequest *request)
//
// handleRoot() handler for root page /
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
{
  String	content;

  if(b_WiFi_connected)
  {
    currentHtmlPage = "Local Net Start Page";
  }
  else
  {
    currentHtmlPage = "Access Point Start Page";    
  }

  homePage(content);
  request->send(200, "text/html; charset=UTF-8", content);
}
// ##########################################################################################
//
// homePage() compose a html home page
//
// 2023-02-01 mh
// - version for SolisLogger
//
// 2023-01-26 mh
// - modified for SMLReader
//
// 2023-01-12	mh
// - first version
//
// (C) M. Herbert, 2023.
// Licensed under the GNU General Public License v3.0
//
// global variables used:
//  currentSSID, currentIP, currentHtmlPage, wifiAPssid, vzServerIP
void homePage(String& _content)
{
  #define MY_HTML_HEAD 		"<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{t}</title>"
  #define MY_HTML_HEAD_END 	"</head><body>"
  #define MY_HTML_END 		"</div></body></html>"
  #define MY_HTML_HEADLINE		"<h1 style='padding-top:25px;'>Welcome to {h} Site</h1></a></div>"
  #define MY_HTML_START		"<div style='padding-top:25px;'><a href='/start'>Return to Start Page</a></div>"
  #define MY_HTML_CONFIG	"<div style='padding-top:25px;'><a href='/config'>Configuration Page</a></div>"
  #define MY_HTML_DASH		"<div style='padding-top:25px;'><a href='/'>Dash Board</a></div>"
  #define MY_RESET_HTML		"<div style='padding-top:25px;'><a href='/reset'>Reset ESP</a></div>\n"
  #define MY_HTML_CONFIG_VER "<div style='padding-top:25px;font-size: .6em;'>Version {v} {d}</div>"


  _content = MY_HTML_HEAD;
  _content += MY_HTML_HEAD_END;
  _content.replace("{t}", wifiAPssid);
  _content += MY_HTML_HEADLINE;
  _content.replace("{h}", wifiAPssid);
  _content += "You are connected to  <b>" + currentSSID + "</b> with IP " + currentIP;
  _content += "<p> VZ-Server <b>" + String(vzHttpConfig.vzServer) + "</b> with IP " + vzServerIP +"</p>";
  _content += "<h2 style='padding-top:25px;'>" + currentHtmlPage + "</h2></a></div>";
  _content += MY_HTML_START;
  _content += MY_HTML_CONFIG;
  _content += MY_HTML_DASH;
  _content += MY_RESET_HTML;
  _content += MY_HTML_CONFIG_VER;
  _content.replace("{v}", WIFI_AP_CONFIG_VERSION);
  _content.replace("{d}", MY_VERSION_TYPE);
  _content += MY_HTML_END;

}
// ##########################################################################################
//
// buildResponse() compose data for json transfer
//
// used to handle requests to /api/all.json and /api/power.json
//
// unchanged from Solis4Gmini-logger 2023-02-01 mh
//
String buildResponse(byte type)
{
  String str;

  switch (type)
  {

  case 0: // Only return power
    str = "{";
    str += "\"power\": ";
    str += String(power);
    str += ",\"energyToday\": ";
    str += String(energyToday);
    str += ",\"isOnline\": ";
    str += String(Inverter.isInverterReachable());
    str += "}";
    break;

  case 1: // Return all data
    str = "{";
    str += "\"power\": ";
    str += String(power);
    str += ",\"energyToday\": ";
    str += String(energyToday);
    str += ",\"isOnline\": ";
    str += String(Inverter.isInverterReachable());

    str += ",\"dc_u\": ";
    str += String(dc_u);
    str += ",\"dc_i\": ";
    str += String(dc_i);

    str += ",\"ac_u\": ";
    str += String(ac_u);
    str += ",\"ac_i\": ";
    str += String(ac_i);
    str += ",\"ac_f\": ";
    str += String(ac_f);

#if(DS18B20)
    str += ",\"ds18b20Temperature\": ";
    str += String(ds18b20Temperature);
#endif
    str += "}";
    break;


  default:
    str = "Function not supported";
    break;
  }

  return str;
}
// ##########################################################################################
// time helper functions

time_t getLocalTime()
//
// assumption is, that the system time tv is GMT, i.e. no timezones and no dayligth saving time offsets are set.
// time offset is applied manually based on configuration parameter
//
{
    timeval tv;
    DEBUG_TRACE_(VERBOSE_LEVEL_TIME,"Sync to Unix Timestamp: ");
    gettimeofday(&tv, NULL);                // use system time; note that usec also contains ms --> divide by 1000 to get ms
    DEBUG_TRACE(VERBOSE_LEVEL_TIME,"%lld",tv.tv_sec);
    return(tv.tv_sec+Timezone*3600);
}
time_t getEpochTime()
//
// return UNIX epoch time
{
    timeval tv;
    DEBUG_TRACE_(VERBOSE_LEVEL_Loop,"Unix Timestamp: ");
    gettimeofday(&tv, NULL);                // use local time; note that usec also contains ms --> divide by 1000 to get ms
    DEBUG_TRACE(VERBOSE_LEVEL_Loop,"%lld",tv.tv_sec);
    return(tv.tv_sec);
}
void getDateTime(char* s_DateTime)
//
// return a char string with the current date and time in the format
//    yyyy-mm-dd hh:mm
// set global variables Year,Month,Day,Hour,Minute
// uses TimeLib
//
// 2023-02-14 mh
// - first version 
{
  Year = year();
  Month = month();
  Day = day();
  Hour = hour();
  Minute = minute();
  sprintf(s_DateTime,"%4d-%02d-%02d %02d:%02d",Year,Month,Day,Hour,Minute);
}