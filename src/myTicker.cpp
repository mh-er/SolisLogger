// myTicker.cpp
//
// implement a time ticker to trigger actions, e.g. sensor read out
//
// 2023-01-30 mh
// - clean up of include structure
//
// 2022-11-16 M. Herbert
// - added two additional tickers for inverter
//
// 2022-10-21 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor, see license below.
//   https://github.com/10k-resistor/Solis4Gmini-logger
// - adapted for own use
//
// Copyright (C) M. Herbert, 2022-2023.
// Licensed under the GNU General Public License v3.0
//

/* *** Description ***
myTicker sets a Flag based on a system timer.
3 tickers are implemented.

** Usage **
myTicker.begin() is used to start the tickers using the timer on operation system level.
Time periods are given by defines in seconds:
DS18B20_READ_INTERVAL
INVERTER_READ_INTERVAL_FREQUENT
INVERTER_READ_INTERVAL_SELDOM

As soon as the time period is over, the Flag is set by a callback function within the class.
myTicker.getXXXFlag() is used to check the Flag by the user to trigger any action.
The flag needs to be reset by the user for continued use of the ticker by myTicker.setXXXFlag() to false;

** Implementation **
myTicker is using Class Ticker which is part of framework-arduinoespressif8266\libraries\Ticker.
Ticker.attach() is used to attach time period and the callback function to the ticker.
Note: each ticker requires an own Ticker object to provide a callback function for the attach() method.

Class Ticker is using system level routines.
A define wrapper in osapi.h (in framework-arduinoespressif8266\tools\sdk\include ) is used to map the os_ function names to
the ets_sys library for the access to the timer on operating system level
(definition of the timer data structure and function ets_timer_setfn() in ets_sys.h).

The implementation of the timer itself is not transparent.
According to https://www.switchdoc.com/2015/10/iot-esp8266-timer-tutorial-arduino-ide/ dated 2015
it is a software timer on operation system level.
  *** end description *** */


/*
License of Solis4Gmin-logger:
10k-resistor/Solis4Gmini-logger is licensed under the GNU General Public License v3.0

Permissions of this strong copyleft license are conditioned on making available complete source code of
licensed works and modifications, which include larger works using a licensed work, under the same license.
Copyright and license notices must be preserved. Contributors provide an express grant of patent rights.
*/
#include <Ticker.h>
#include "config.h"
#include "myTicker.h"


myTicker::myTicker()
{
}

// ####################################### Inverter frequent read ########################################
Ticker inverterFrequentTicker;
bool _inverterFrequentFlag = false;

void inverterFrequentFlagChange()
{
    _inverterFrequentFlag = true;
}

bool myTicker::getInverterFrequentFlag()
{
    return _inverterFrequentFlag;
}

void myTicker::setInverterFrequentFlagToFalse()
{
    _inverterFrequentFlag = false;
}

// ####################################### Inverter seldom read ########################################
Ticker inverterSeldomTicker;
bool _inverterSeldomFlag = false;

void inverterSeldomFlagChange()
{
    _inverterSeldomFlag = true;
}

bool myTicker::getInverterSeldomFlag()
{
    return _inverterSeldomFlag;
}

void myTicker::setInverterSeldomFlagToFalse()
{
    _inverterSeldomFlag = false;
}

// ####################################### DS18B20 #######################################################
#if(DS18B20)
    Ticker ds18b20Ticker;
    bool _ds18b20Flag = false;

    void ds18b20FlagChange()
    {
        // Serial.println("DEBUG 5 ");
        // Serial.println(_ds18b20Flag);
        _ds18b20Flag = true;
    }

    bool myTicker::getDS18B20Flag()
    {
        return _ds18b20Flag;
    }

    void myTicker::setDS18B20FlagToFalse()
    {
        _ds18b20Flag = false;
    }
#endif
// ####################################### Start Ticker ###################################################
/*
Starts ticker
*/
void myTicker::begin()
{
    inverterFrequentTicker.attach(INVERTER_READ_INTERVAL_FREQUENT, inverterFrequentFlagChange);
    inverterSeldomTicker.attach(INVERTER_READ_INTERVAL_SELDOM, inverterSeldomFlagChange);
    #if(DS18B20)
        ds18b20Ticker.attach(DS18B20_READ_INTERVAL, ds18b20FlagChange);
    #endif
}