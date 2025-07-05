#ifndef SENSOR_H
#define SENSOR_H

#include "EventHandler.h"

class Sensor : public EventHandler {
protected:
    int pin;
    EventHandler* handler;

public:

    Sensor(int pin, EventHandler* eventHandler = nullptr);

    void on(Event event) override;

    void setHandler(EventHandler* eventHandler);
};

#endif