#ifndef DS18B20_H
#define DS18B20_H

#ifndef DEBUG_TRACE
    #define DEBUG_TRACE(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout); Serial.println();}
#endif

class myDS18B20
{
public:
    myDS18B20();

    float getTemperature();
    float getTempSensorType();

};
#endif // DS18B20_H