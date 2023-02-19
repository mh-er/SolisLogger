#ifndef LED_H
#define LED_H

class LED
{
public:
    LED();
    void begin();

    void blueToggle();
    void blueOn();
    void blueOff();

    void yellowToggle();
    void yellowOn();
    void yellowOff();

    void enableNightBlink();
    void disableNightblink();
    
};

#endif // LED_H