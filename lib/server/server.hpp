#pragma once

class ServerClass {
public:
    // Methods
        void begin(); // Starts the AP, server, and FreeRTOS task

private:
    // Methods
        static void serverTask(void* parameter); // FreeRTOS task function
        void worker();                           // Handles client requests

        void handleRoot();  // Handles root URL
        void handleState(); // Handles state change requests
        void handleInput(); // Handles input data requests
};