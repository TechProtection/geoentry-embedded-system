#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

struct Command {
    int id;

    explicit Command(int commandId) : id(commandId) {}
    bool operator==(const Command& other) const { return id == other.id; }
};

class CommandHandler {
public:
    virtual void handle(Command command) = 0;
    virtual ~CommandHandler() = default;
};

#endif