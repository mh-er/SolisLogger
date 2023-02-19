// modbus.cpp for Solis Inverter Status Register Readout
//
// 2023-01-30 mh
// - clean up of include structure
//
// 2022-11-25 M. Herbert
// - return error code instead of ready flag
// - retry read after timeout
// - return error code as negative value in dc_u
//
// 2022-11-16 M. Herbert
// - additional read of DCpower, energy This/Last year
// - additional methods to support read of data in different intervals.
//
// 2022-11-12 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor, see license below.
//   https://github.com/10k-resistor/Solis4Gmini-logger
// - adapted for own use
//
// Copyright (C) M. Herbert, 2022 - 2023.
// Licensed under the GNU General Public License v3.0
//

/* *** Description ***
read data registers of Solis Inverter via modbus

** Usage **
Read of inverter data registers via modbus is done by
    requestAll(); reads all registers (as request) within one call
    request(); calls the other requests below
    requestPower();
    requestDayEnergy();
    requestMonthYearEnergy();

Afterwards, the data are available in the object attributes.
get-methods are provided for these.

Note: the implementation is using function code 0x04 registers.
Solis RS485_MODBUS Communication Protocol:
Register have 16bit, for 32bit data: high word first, low word next.
Register address needs to be decreased by 1, e.g. to access register address 3000 send address 2999.
Baud rate：9600bps, Parity checking：None, Data：8, Stop：1
More than 300ms communications frame interval is required.

** Implementation **
Implementation is using Arduino class library ModbusMaster.
  *** end description *** */

/*
License of Solis4Gmin-logger:
10k-resistor/Solis4Gmini-logger is licensed under the GNU General Public License v3.0

Permissions of this strong copyleft license are conditioned on making available complete source code of
licensed works and modifications, which include larger works using a licensed work, under the same license.
Copyright and license notices must be preserved. Contributors provide an express grant of patent rights.
*/
#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "modbus.h"


// SW-Serial object
SoftwareSerial rs485(rs485_RX, rs485_TX, false); // RX, TX, Invert signal

// instantiate ModbusMaster object
ModbusMaster node;

void postTransmission();
void preTransmission();

float _power = -1.00;
float _DCpower = -1.00;
float _totalEnergy = -1.00;
float _energyToday = -1.00;
float _energyLastDay = -1.00;
float _energyThisMonth = -1.00;
float _energyLastMonth = -1.00;
float _energyThisYear = -1.00;
float _energyLastYear = -1.00;

float _temperature = -1.00;

float _dc_u = -1.00;
float _dc_i = -1.00;

float _ac_u = -1.00;
float _ac_i = -1.00;
float _ac_f = -1.00;

bool _softRun = true;

bool reachable = false;
bool reachable_flag__lst = true;

inverter::inverter()
{
}

/*
Requests all data from the inverter
*/
uint8_t inverter::request()
{
    uint8_t result;
    result = node.ku8MBSuccess;
    reachable = true;
    result |= this->requestPower();
    result |= this->requestDayEnergy();
    result |= this->requestMonthYearEnergy();
    return result;
}
/*
Requests data from the inverter: power, DCpower, dc_u, dc_i, ac_u, ac_i, ac_f, temperature
*/
uint8_t inverter::requestAll()
{
    uint8_t result;
    int     iter;
    result = node.ku8MBSuccess;
    reachable = true;

    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    iter = 0;
    do
    {
        // note: read buffer has 64 16bit words, recommend max read length is 50 words.
        result = node.readInputRegisters(3004, 40);                  // read all registers, first is 3005
        if (result == node.ku8MBSuccess)
        {
            _power = node.getResponseBuffer(0x01);                  // 3006: active power low word in W
            _DCpower = node.getResponseBuffer(0x03);                // 3008: Total DC output power in W
                                                                    // 3009-3010: Energy this month in 1kWh
            _totalEnergy = node.getResponseBuffer(4) *65536.0 + node.getResponseBuffer(5);

                                                                    // 3011-3012: Energy this month in 1kWh
            _energyThisMonth = node.getResponseBuffer(6) *65536.0 + node.getResponseBuffer(7);
                                                                    // 3013-3014: Energy last month in 1kWh
            _energyLastMonth = node.getResponseBuffer(8) *65536.0 + node.getResponseBuffer(9);
            _energyToday = node.getResponseBuffer(10) / 10.0;       // 3015: Energy today in 0.1kWh
            _energyLastDay = node.getResponseBuffer(11) / 10.0;     // 3016: Energy last day in 0.1kWh
                                                                    // 3017-3018: Energy this year in 1kWh
            _energyThisYear  = node.getResponseBuffer(12) *65536.0 + node.getResponseBuffer(13);
                                                                    // 3019-3020: Energy last year in 1kWh
            _energyLastYear  = node.getResponseBuffer(14) *65536.0 + node.getResponseBuffer(15);
                                                                    // 3021: HMI version (internal use)
            _dc_u = node.getResponseBuffer(17) / 10.0;              // 3022: DC voltage 1 in 0.1V
            _dc_i = node.getResponseBuffer(18) / 10.0;              // 3023: DC current 1 in 0.1A
            _ac_u = node.getResponseBuffer(31) / 10.0;            // 3036: CA line /C phase voltage in 0.1V
            _ac_i = node.getResponseBuffer(34) / 10.0;            // 3039: C phase current in 0.1A
            _temperature = node.getResponseBuffer(37) / 10.0;     // 3042: temperature in 0.1 deg Celsius
            _ac_f = node.getResponseBuffer(38) / 100.0;           // 3043: Grid frequency in 0.01Hz

            reachable = true;
        }
        else
        {
            reachable = false;
            DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");

            _power = 0.0;
            _DCpower = 0.0;
            // keep these values if inverter is offline
            // _energyThisMonth = 0.0;
            // _energyLastMonth = 0.0;
            // _energyToday = 0.0;
            // _energyLastDay = 0.0;
            // _energyThisYear= 0.0;
            // _energyLastYear = 0.0;
            _dc_u = -result;
            _dc_i = 0;
            _ac_u = 0.0;
            _ac_i = 0.0;
            _ac_f = 0.0;
            _temperature = 0.0;
        }
        iter++;
        delay(MODBUS_READ_DELAY);
        delay(MODBUS_RETRY_INTERVAL);
    } while ((result != node.ku8MBSuccess) && (iter < MODBUS_RETRY_NUMBER));
  
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

    return result;
}

/*
Requests data from the inverter: power, DCpower, dc_u, dc_i, ac_u, ac_i, ac_f, temperature
*/
uint8_t inverter::requestPower()
{
    uint8_t inverterOffCounter = 0;
    uint8_t result;
    uint8_t result_or;
    int     iter;
    reachable = true;

    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

    iter = 0;
    do
    {  
        result_or = node.ku8MBSuccess;
        inverterOffCounter=0;
        result = node.readInputRegisters(3004, 4);             // read 4 registers, first is 3005
        result_or |= result;
        if (result == node.ku8MBSuccess)
        {
            _power = node.getResponseBuffer(0x01);             // 3006: active power low word in W
            _DCpower = node.getResponseBuffer(0x03);           // 3008: Total DC output power in W
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");
            inverterOffCounter++;
            _power = 0.0;
            _DCpower = 0.0;
        }

        result = node.readInputRegisters(3014, 2);
        result_or |= result;
        if (result == node.ku8MBSuccess)
        {
            _energyToday = node.getResponseBuffer(0x00) / 10.0;
            _energyLastDay = node.getResponseBuffer(0x01) / 10.0;
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET DAY ENERGY FAILED");
            inverterOffCounter++;
            _energyToday = 0.0;
            _energyLastDay = 0.0;
        }
        delay(MODBUS_READ_DELAY);

        result = node.readInputRegisters(3021, 2);              // DC voltage1, current1  in 0.1V/A
        result_or |= result;
        if (result == node.ku8MBSuccess)
        {
            _dc_u = node.getResponseBuffer(0x00) / 10.0;
            _dc_i = node.getResponseBuffer(0x01) / 10.0;
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET DC FAILED");
            inverterOffCounter++;
            _dc_u = -result;
            _dc_i = 0;
        }
        delay(MODBUS_READ_DELAY);

        result = node.readInputRegisters(3035, 10);
        result_or |= result;
        if (result == node.ku8MBSuccess)                        // ac values and inverter temperature
        {
            _ac_u = node.getResponseBuffer(0x00) / 10.0;
            _ac_i = node.getResponseBuffer(0x03) / 10.0;
            _ac_f = node.getResponseBuffer(0x07) / 100.0;
            _temperature = node.getResponseBuffer(0x06) / 10.0;   // temperature is in 3042, for read to be decrease to 3041
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET AC FAILED");
            inverterOffCounter++;
            _ac_u = 0.0;
            _ac_i = 0.0;
            _ac_f = 0.0;
            _temperature = 0.0;
        }
        delay(MODBUS_READ_DELAY);
        iter++;
        delay(MODBUS_RETRY_INTERVAL);
    } while ((result_or != node.ku8MBSuccess) && (iter < MODBUS_RETRY_NUMBER));

    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

    if(inverterOffCounter > 0){
        reachable = false;
        DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");
    }else{
        reachable = true;
    }

    return result_or;

}

/*
Requests data from the inverter: energy this/last day
*/
uint8_t inverter::requestDayEnergy()
{
    uint8_t inverterOffCounter = 0;
    uint8_t result;
    int     iter;

    reachable = true;

    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
    iter = 0;
    do
    {
        inverterOffCounter = 0;
        result = node.ku8MBSuccess;
        result = node.readInputRegisters(3014, 2);                 // energy today and last day in 0.1kWh
        if (result == node.ku8MBSuccess)
        {
            _energyToday = node.getResponseBuffer(0x00) / 10.0;
            _energyLastDay = node.getResponseBuffer(0x01) / 10.0;
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET DAY ENERGY FAILED");
            inverterOffCounter++;
            _energyToday = 0.0;
            _energyLastDay = 0.0;
        }
        delay(MODBUS_READ_DELAY);
        iter++;
        delay(MODBUS_RETRY_INTERVAL);
    } while ((result != node.ku8MBSuccess) && (iter < MODBUS_RETRY_NUMBER));

    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);


    if(inverterOffCounter > 0){
        reachable = false;
        DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");
        DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");
    }
    else
    {
        reachable = true;
    }
    return result;
}
/*
Requests data from the inverter: energy this/last month/year
*/
uint8_t inverter::requestMonthYearEnergy()
{
    uint8_t inverterOffCounter = 0;
    uint8_t result;
    uint8_t result_or;
    int     iter;
    reachable = true;

    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

    iter = 0;
    do
    {
        result_or = node.ku8MBSuccess;
        inverterOffCounter = 0;
        result = node.readInputRegisters(3010, 4);              // energy this and last month in kWh
        result_or |= result;
        if (result == node.ku8MBSuccess)
        {
            _energyThisMonth = node.getResponseBuffer(0x00) *65536.0 + node.getResponseBuffer(0x01);
            _energyLastMonth = node.getResponseBuffer(0x02) *65536.0 + node.getResponseBuffer(0x03);
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET MONTH ENERGY FAILED");
            inverterOffCounter++;
            _energyThisMonth = 0.0;
            _energyLastMonth = 0.0;

        }
        delay(MODBUS_READ_DELAY);

        result = node.readInputRegisters(3016, 4);              // energy this and last month/year in kWh
        result_or |= result;
        if (result == node.ku8MBSuccess)
        {
            _energyThisYear  = node.getResponseBuffer(0x00) *65536.0 + node.getResponseBuffer(0x01);
            _energyLastYear  = node.getResponseBuffer(0x02) *65536.0 + node.getResponseBuffer(0x03);
        }
        else
        {
            DEBUG_TRACE(VERBOSE_LEVEL_InverterAccess,"+ GET YEAR ENERGY FAILED");
            inverterOffCounter++;
            _energyThisYear= 0.0;
            _energyLastYear = 0.0;
        }
        delay(MODBUS_READ_DELAY);
        iter++;
        delay(MODBUS_RETRY_INTERVAL);
    } while ((result_or != node.ku8MBSuccess) && (iter < MODBUS_RETRY_NUMBER));

    
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

    if(inverterOffCounter > 0){
        reachable = false;
        DEBUG_TRACE(VERBOSE_LEVEL_Inverter," Inverter not reachable");
    }else
    {
        reachable = true;
    }
    return result_or;
}

/*
Returns data from AC
@param _ac_u AC voltage (float)
@param _ac_i AC current (float)
@param _ac_f AC frequency (float)
*/
void inverter::getAC(float *__ac_u, float *__ac_i, float *__ac_f)
{
    *__ac_u = _ac_u;
    *__ac_i = _ac_i;
    *__ac_f = _ac_f;
}

/*
Returns data from DC
@param _dc_u DC voltage (float)
@param _dc_i DC current (float)
*/
void inverter::getDC(float *__dc_u, float *__dc_i)
{
    *__dc_u = _dc_u;
    *__dc_i = _dc_i;
}

/*
Returns temperature
@param _temperature (float)
*/
void inverter::getTemperature(float *__temperature)
{
    *__temperature = _temperature;
}

/*
Returns power
@param _power (float)
*/
void inverter::getPower(float *__power)
{
    *__power = _power;
}

/*
Returns DC power
@param _DCpower (float)
*/
void inverter::getDCPower(float *__DCpower)
{
    *__DCpower = _DCpower;
}

/*
Returns energy today
@param _energyToday (float)
*/
void inverter::getEnergyToday(float *__energyToday)
{
    *__energyToday = _energyToday;
}

/*
Return energy last day
*/
void inverter::getEnergyLastDay(float *__energyLastDay)
{
    *__energyLastDay = _energyLastDay;
}
/*
Return energy last month
*/
void inverter::getEnergyLastMonth(float *__energyLastMonth)
{
    *__energyLastMonth = _energyLastMonth;
}
/*
Return energy this month
*/
void inverter::getEnergyThisMonth(float *__energyThisMonth)
{
    *__energyThisMonth = _energyThisMonth;
}

/*
Return energy last year
*/
void inverter::getEnergyLastYear(float *__energyLastYear)
{
    *__energyLastYear = _energyLastYear;
}
/*
Return energy this year
*/
void inverter::getEnergyThisYear(float *__energyThisYear)
{
    *__energyThisYear = _energyThisYear;
}

/*
Return total energy
*/
void inverter::getTotalEnergy(float *__totalEnergy)
{
    *__totalEnergy = _totalEnergy;
}

/*
Is inverter reachable
*/
bool inverter::isInverterReachable()
{
    return reachable;
}

/*
Is inverter reachable (Last state)
*/
bool inverter::getIsInverterReachableFlagLast()
{
    return reachable_flag__lst;
}

bool inverter::setIsInverterReachableFlagLast(bool _value)
{
    reachable_flag__lst = _value;
    return reachable_flag__lst;
}

bool inverter::isSoftRun()
{
    return _softRun;
}

void inverter::begin()
{
    rs485.begin(9600);

    pinMode(MAX485_DE, OUTPUT);
    // Init in receive mode
    digitalWrite(MAX485_DE, 0);

    node.begin(MODBUS_SLAVE_ID_INVERTER, rs485); // SlaveID,Serial
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

/////////////////////////////////////////////////////

void preTransmission()
{
    digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
    digitalWrite(MAX485_DE, 0);
}

