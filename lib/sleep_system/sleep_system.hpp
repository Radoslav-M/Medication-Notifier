#pragma once
#include "input.hpp"
#include "output.hpp"
#include "server.hpp"
// This tracks what state the device should be in with a state machine, and manages sleep too.

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

    // References to other modules
        InputClass& input;
        OuptutClass& output;
        ServerClass& server;
};