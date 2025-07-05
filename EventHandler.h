#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

struct Event {
    int id;

    explicit Event(int eventId) : id(eventId) {}
    bool operator==(const Event& other) const { return id == other.id; }
};

class EventHandler {
public:
    virtual void on(Event event) = 0;
    virtual ~EventHandler() = default;
};

#endif