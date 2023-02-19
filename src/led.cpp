// led.cpp implementing Class LED
//
// control LEDs connected by digital pins, uses os_timer (not delay())
//
// 2023-02-14 mh
// - add switch LED_USE to disable code
//
// 2023-01-30 mh
// - clean up of include structure
//
// 2022-10-21 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor, see license below.
//   https://github.com/10k-resistor/Solis4Gmini-logger
// - adapted for own use
//
// Copyright (C) M. Herbert, 2022 - 2023.
// Licensed under the GNU General Public License v3.0
//

/* *** Description ***
Controls two LEDs (yellow and blue)
- switch on or off, toggle
- enable or disable "NightBlink" of blue LED, i.e. LED is of for 20sec and on for 0.5sec.

** Usage **
LEDs are connected to PIN specified by define LED_BLUE and LED_YELLOW.

LED.begin() configures PINs as output and switches LEDs off.
LED.xxxON(), .xxxOff(), .xxxToggle() switches the LEDs
LED::enableNightBlink(), LED::disableNightBlink() switches NightBlink of blue LED

** Implementation **
Switch of LEDs is done be digitalWrite.
NightBlink is implemented using OS timer1 based on a callback function provided via os_timer_setfn() 
  *** end description *** */


/*
License of Solis4Gmin-logger:
10k-resistor/Solis4Gmini-logger is licensed under the GNU General Public License v3.0

Permissions of this strong copyleft license are conditioned on making available complete source code of
licensed works and modifications, which include larger works using a licensed work, under the same license.
Copyright and license notices must be preserved. Contributors provide an express grant of patent rights.
*/
#include <Arduino.h>
#include <Ticker.h>
#include <TimeLib.h>
#include "led.h"
#include "config.h"     // PINs for LED_BLUE and LED_YELLOW are defined here


extern "C" {
#include "user_interface.h"   // necessary for os_timer_...
}

LED::LED()
{
}

#if LED_USE         // use external LEDs
    os_timer_t Timer1;
    uint8_t Counter = 0;
    bool nightBlinkEnabled = false;
    volatile uint8_t _cnt = 0;

    void blinkTimerCallback(void *pArg)
    {
        //volatile uint8_t _cnt = *((int *)pArg);
        if (_cnt < 40)
        {
            _cnt++;
            digitalWrite(LED_BLUE, LOW);
        }
        else
        {
            _cnt = 0;
            digitalWrite(LED_BLUE, HIGH);
        }
        //*((int *)pArg) = _cnt;
    }

    void LED::begin()
    {
        pinMode(LED_YELLOW, OUTPUT);
        pinMode(LED_BLUE, OUTPUT);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_BLUE, LOW);
    }

    void LED::blueToggle()
    {
        digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
    }

    void LED::blueOn()
    {
        digitalWrite(LED_BLUE, HIGH);
    }

    void LED::blueOff()
    {
        digitalWrite(LED_BLUE, LOW);
    }

    void LED::yellowToggle()
    {
        digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
    }

    void LED::yellowOn()
    {
        digitalWrite(LED_YELLOW, HIGH);
    }

    void LED::yellowOff()
    {
        digitalWrite(LED_YELLOW, LOW);
    }

    void LED::enableNightBlink()
    {
        if (nightBlinkEnabled == false)
        {
            os_timer_setfn(&Timer1, blinkTimerCallback, &Counter);
            Serial.println("Enabled night blink");
            os_timer_arm(&Timer1, 500, true); // Timer1 Interval 0,5s
        }

        nightBlinkEnabled = true;
    }

    void LED::disableNightblink()
    {
        if (nightBlinkEnabled == true)
        {
            os_timer_disarm(&Timer1);
            digitalWrite(LED_BLUE, LOW);
            Serial.println("Disabled night blink");
        }
        nightBlinkEnabled = false;
    }
#else   // use external LEDs

    void blinkTimerCallback(void *pArg) {}
    void LED::begin() {}
    void LED::blueToggle() {}
    void LED::blueOn() {}
    void LED::blueOff() {}
    void LED::yellowToggle() {}
    void LED::yellowOn() {}
    void LED::yellowOff() {}
    void LED::enableNightBlink() {}
    void LED::disableNightblink() {}

#endif      // use external LEDs
