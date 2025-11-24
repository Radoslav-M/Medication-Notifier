#pragma once
#include <vector>
#include <string>

struct WiFiNetwork {
    const char* ssid;
    const char* password;
};

class ServerClass {
public:
    // Methods
        void begin(); // Starts the AP, server, and FreeRTOS task
        bool syncTimeWithNTP(); // Attempts to connect to WiFi and sync time
        bool isTimeSynced() { return timeSynced; }

private:
    // Methods
        static void serverTask(void* parameter); // FreeRTOS task function
        void worker();                           // Handles client requests

        void handleRoot();  // Handles root URL
        void handleState(); // Handles state change requests
        void handleInput(); // Handles input data requests
        
        bool tryConnectToKnownNetworks(); // Try to connect to known WiFi networks

    // Attributes
        bool timeSynced = false;
        
    // Known WiFi networks (priority order)
        const std::vector<WiFiNetwork> knownNetworks = {
            {"Upstairs", "Radko1Radko23"},
            {"YourWorkWiFi", "password2"},
            {"YourPhoneHotspot", "password3"}
        };
};