#ifndef MY_TICKER_H
#define MY_TICKER_H

class myTicker
{
public:
    myTicker();
    void begin();

    bool getInverterFrequentFlag();
    void setInverterFrequentFlagToFalse();

    bool getInverterSeldomFlag();
    void setInverterSeldomFlagToFalse();

    bool getDS18B20Flag();
    void setDS18B20FlagToFalse();

};
#endif // MY_TICKER_H