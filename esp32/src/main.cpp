#include "Clock.hpp"

extern "C" void app_main()
{
    Clock clock;

    while (true) {

        clock.update();

        const ClockTime& time = clock.time();

        // ESP32 display code will go here.

    }
}