#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "CommandHandler.h"

class Actuator : public CommandHandler {
protected:
    int pin;
    CommandHandler* handler;

public:

    Actuator(int pin, CommandHandler* commandHandler = nullptr);

    void handle(Command command) override;

    void setHandler(CommandHandler* commandHandler);
};

#endif