#pragma once

class EventTarget
{
    private:
        bool _disconnected;

    public:
        EventTarget() : _disconnected(false) {}
        virtual ~EventTarget() {}

        bool isDisconnected() const { return _disconnected; }
        void markDisconnected()      { _disconnected = true; }
};
