#include "Clock.hpp"

#include <ctime>

Clock::Clock()
{
    update();
}

void Clock::update()
{
    std::time_t now = std::time(nullptr);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    time_.hour = localTime.tm_hour;
    time_.minute = localTime.tm_min;
    time_.second = localTime.tm_sec;
}

const ClockTime& Clock::time() const
{
    return time_;
}