#include "Led.h"
#include <Arduino.h>

Led::Led(int pin, bool inverted, CommandHandler* commandHandler)
    : Actuator(pin, commandHandler), currentState(false), inverted(inverted) {
    init();
}

void Led::init() {
    pinMode(pin, OUTPUT);
    setState(false);
}

void Led::handle(Command command) {
    if (command == LedCommands::TURN_ON) {
        turnOn();
    } else if (command == LedCommands::TURN_OFF) {
        turnOff();
    } else if (command == LedCommands::TOGGLE) {
        toggle();
    } else if (command == LedCommands::BLINK) {
        blink();
    } else {
        Actuator::handle(command);
    }
}

void Led::turnOn() {
    setState(true);
}

void Led::turnOff() {
    setState(false);
}

void Led::toggle() {
    setState(!currentState);
}

void Led::setState(bool state) {
    currentState = state;
    bool physicalState = inverted ? !state : state;
    digitalWrite(pin, physicalState ? HIGH : LOW);
}

bool Led::getState() const {
    return currentState;
}

void Led::blink(int times, int delayMs) {
    bool originalState = currentState;
    
    for (int i = 0; i < times; i++) {
        turnOn();
        delay(delayMs);
        turnOff();
        delay(delayMs);
    }
    
    setState(originalState);
}
