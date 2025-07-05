#include "Actuator.h"

Actuator::Actuator(int pin, CommandHandler* commandHandler)
    : pin(pin), handler(commandHandler) {}

void Actuator::handle(Command command) {
    if (handler != nullptr) {
        handler->handle(command);
    }
}

void Actuator::setHandler(CommandHandler* commandHandler) {
    handler = commandHandler;
}
