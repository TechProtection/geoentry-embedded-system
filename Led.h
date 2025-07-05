#ifndef LED_H
#define LED_H

#include "Actuator.h"

class Led : public Actuator {
private:
    bool currentState;
    bool inverted;

public:
    Led(int pin, bool inverted = false, CommandHandler* commandHandler = nullptr);
    
    void handle(Command command) override;
    
    void turnOn();
    void turnOff();
    void toggle();
    void setState(bool state);
    bool getState() const;
    
    void init();
    void blink(int times = 1, int delayMs = 500);
};

namespace LedCommands {
    const Command TURN_ON(1);
    const Command TURN_OFF(2);
    const Command TOGGLE(3);
    const Command BLINK(4);
}

#endif
