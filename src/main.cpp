#include <Arduino.h>

#include <pinout.hpp>
#include <server.hpp>
#include <output.hpp>
#include <input.hpp>
#include <sleep_system.hpp>

ServerClass server;
OuptutClass output;
InputClass input;
SleepSystemClass sleepSystem(input, output, server);

void setup() {
    // Start Serial for debugging
        Serial.begin(BAUD_RATE);
        delay(100);
        printf("\n\nMedication Notifier Starting...\n");

    // Start the server
        server.begin();

    // Initialize output module
        output.begin();

    // Initialize input module
        input.begin();

    // Initialize sleep system
        sleepSystem.begin();

    // Finnish setup
        printf("Setup complete.\n");
}

void loop() {
    // Your main loop tasks here
        delay(10000000);
}