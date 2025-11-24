#pragma once
#include "pinout.hpp"
#include "input.hpp"
#include "output.hpp"
#include "server.hpp"
// This manages sleep and the RTC.

// - Keep track of current time and medication schedule
// - Wake any time the hatch is opened
// - Wake on scheduled intervals when medication is due
// - Sleep when hatch is closed for more then a set time


// Cnofigure the sleep system

    struct WakeTimestamp {
        uint8_t hour;
        uint8_t minute;
    };

class SleepSystemClass {
public:
    // Constructor
        SleepSystemClass(InputClass& p_input, OuptutClass& p_output, ServerClass& p_server)
            : input(p_input), output(p_output), server(p_server) {}

    // Methods
        void begin(); // Initializes the sleep system

private:
    // Methods
        static void sleepSystemTask(void* parameter); // FreeRTOS task function

        void setCurrentTime(int year, int month, int day, int hour, int minute, int second); // Sets the current time from the server
        struct tm getCurrentTime(); // Gets the current time from the RTC

        void enterDeepSleep(); // Enters deep sleep mode until next wakeup event. Automaticly configures to wake on hatch open or scheduled time.

    // Atributes
        WakeTimestamp MedicationSchedule[8] {
            {7, 0},  // 07:00
            {9, 0},  // 09:00
            {11, 0}, // 11:00
            {13, 0}, // 13:00
            {15, 0}, // 15:00
            {17, 0}, // 17:00
            {19, 0}, // 19:00
            {21, 0}, // 21:00
        };

        int CONF_SLEEP_DELAY_HATCH_CLOSED_S = 10; // Time in seconds before entering sleep after hatch is closed

    // References to other modules
        InputClass& input;
        OuptutClass& output;
        ServerClass& server;
};