// vzHttp.cpp
//
// transfer data to and from a web server
//
// 2023-02-14 mh
// - split up input for server url
// - not transmission, if uuid = VZ_UUID_NO_SEND
// 
// 2023-01-30 mh
// - expansion of class
// - adaptation of interfaces
// - clean up of include structure
//
// 2022-11-09	M. Herbert
// - based on https://randomnerdtutorials.com/esp8266-nodemcu-http-get-post-arduino/ (see Copyright notice below)
// - modified for own use
//
//
// (C) Copyright M. Herbert, 2022-2023.
// Licensed under the GNU General Public License v3.0
//


/* *** Description ***
http transfer of a data tupel (timestamp,value) of a sensor channel to the Volkszaehler data base via middleware.php
timestamp is Unix epoch time in seconds.
sensor channel is defined by its data base UUID.


** Usage **
vzHttp.postHttp(vzUUID, s_timeStamp, value);

The server name where middleware.php is hosted is given by the define VZ_SERVER.

** Implementation **
For transfer to volkszaehler, the http transfer should look like this:
// http://volks-raspi/middleware.php/data.json?uuid=ae53c580-1234-5678-90ab-cdefghijklmn&operation=add&ts=1666801000000&value=22

Implementation is done using classes WiFiClient and HTTPClient.

  *** end description *** */

/*
  License notice Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  
  Code compatible with ESP8266 Boards Version 3.0.0 or above 
  (see in Tools > Boards > Boards Manager > ESP8266)
*/
#include <string.h>
#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include "vzHttp.h"
#include "config.h"

VzHttp::VzHttp()
{
}

WiFiClient _client;
HTTPClient http;


void VzHttp::init(VzHttpConfig &config)
{
  // transfer data from config structure to main class
  //
  _serverName = String(config.vzServer);
  _middlewareName = String(config.vzMiddleware);
  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"vzServer: %s",_serverName.c_str());
  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"vzMiddleware: %s",_middlewareName.c_str());

  uint16_t i;
  for (i=0;i<N_UUID_VALUE;i++)
  {
    _uuid[i] = &config.uuidValue[i][0];
    DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"uuid[%d] = %s",i,_uuid[i]);
  }

};

void VzHttp::setServerName(String serverName)
{
  _serverName = serverName;
};
void VzHttp::setMiddlewareName(String middlewareName)
{
  _middlewareName = middlewareName;
};

int VzHttp::postHttp(String vzUUID, String timeStamp, float value)
{

    //For transfer to volkszaehler, the http transfer should look like this:
    // http://volks-raspi/middleware.php/data.json?uuid=ae53c580-1234-5678-90ab-cdefghijklmn&operation=add&ts=1666801000000&value=22


  if(!strcmp(vzUUID.c_str(),VZ_UUID_NO_SEND))
  {
    return -99;
  }
  String vzUrl = "http://";
  vzUrl += _serverName + "/" + _middlewareName;
  vzUrl += "/" + String(VZ_DATA_JSON);

  if(!http.begin(_client, vzUrl))
  {
    DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"No connection to %s",_serverName.c_str());
    return 404;
  };

  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  // HTTP request with a content type: x-www-form-urlencoded
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  //construct the message body
  //example for data to be sent: uuid=ae53c580-1234-5678-90ab-cdefghijklmn&operation=add&ts=1666801000000&value=22

   String httpRequestData = "uuid=" + vzUUID;
   String httpOperation = "&operation=add";
   
   String s_timestamp = "&ts=";
   s_timestamp += timeStamp;
   s_timestamp += "000";   // convert seconds to milli seconds
  
   String s_value = "&value="; 
   s_value += String(value);
   
   httpRequestData = httpRequestData + s_timestamp + s_value;
   DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"Post message: %s",httpRequestData.c_str());

  int httpResponseCode = http.POST(httpRequestData);

  DEBUG_TRACE(VERBOSE_LEVEL_HTTP,"HTTP Response code: %d",httpResponseCode);
      
  // Free resources
  http.end();
  return httpResponseCode;
};

String VzHttp::getTimeStamp()
{
  return _TimeStamp;
}
float VzHttp::getValue(UuidValueName _select)
{
  return _value[_select];
}
void VzHttp::testHttp()
//
// 2023-01-26 mh
// - test channel to database, use system time as test data
{
  static uint32_t lastSendTime=0;
  uint32_t currentTime;
  currentTime = millis();
  if((currentTime-lastSendTime) > MY_TEST_SEND_UPDATE)
  {
    lastSendTime = currentTime;
    String _vzUUID = String(_uuid[vzTEST]);
      struct timeval tv;                      // defined in time.h
      gettimeofday(&tv, NULL);                // use local time; note that usec also contains ms --> divide by 1000 to get ms
      String s_timestamp = String(tv.tv_sec);        // timestamp with resolution of 1 sec

    this->postHttp(_vzUUID, s_timestamp, float(currentTime/1000.));
    this->_TimeStamp = s_timestamp +"000";  
    this->_value[vzTEST] = float(currentTime/1000.);
  }
}
