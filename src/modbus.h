#ifndef MODBUS_H
#define MODBUS_H
// modbus.h for Solis Inverter Status Register Readout
//
// 2022-10-14 M. Herbert
// - based on Solis4Gmini-logger by 10k-resistor.
// - adapted for own use
//
//
#ifndef DEBUG_TRACE
    #define DEBUG_TRACE(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout); Serial.println();}
#endif
class inverter
{
public:
    inverter();

    void begin();
    uint8_t request();
    uint8_t requestAll();
    uint8_t requestPower();
    uint8_t requestDayEnergy();
    uint8_t requestMonthYearEnergy();
    
    void getAC(float *__ac_u, float *__ac_i, float *__ac_f);
    void getDC(float *__dc_u, float *__dc_i);
    void getTemperature(float *__temperature);
    void getPower(float *__power);
    void getDCPower(float *__DCpower);
    void getEnergyToday(float *__energyToday);

    void getEnergyLastDay(float *__energyLastDay);
    void getEnergyLastMonth(float *__energyLastMonth);
    void getEnergyThisMonth(float *__energyThisMonth);

    void getEnergyLastYear(float *__energyLastMonth);
    void getEnergyThisYear(float *__energyThisMonth);
    void getTotalEnergy(float *__totalEnergy);

    bool isInverterReachable();
    bool isSoftRun();
    bool setIsInverterReachableFlagLast(bool _value);
    bool getIsInverterReachableFlagLast();

};
#endif // MODBUS_H