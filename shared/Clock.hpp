#pragma once

struct ClockTime
{
    int hour;
    int minute;
    int second;
};

class Clock
{
public:
    Clock();

    void update();

    const ClockTime& time() const;

private:
    ClockTime time_{};
};