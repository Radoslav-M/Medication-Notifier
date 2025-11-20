#include "server.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <output.hpp>
#include <input.hpp>

// Create WebServer instance on port 80
WebServer webServer(80);

// External output instance (declared in main.cpp)
extern OuptutClass output;
extern InputClass input;

void ServerClass::begin() {
    printf("Starting server...\n");

    printf(" - Starting Access Point...\n");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        printf(" - Failed to mount LittleFS!\n");
        return;
    }
    printf(" - LittleFS mounted successfully\n");
    
    // Start WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    // Print IP address
    IPAddress IP = WiFi.softAPIP();
    printf(" - AP IP address: %s\n", IP.toString().c_str());
    printf(" - SSID: %s\n", AP_SSID);
    printf(" - Password: %s\n", AP_PASSWORD);
    
    // Setup server routes
    webServer.on("/", [this](){ this->handleRoot(); });
    webServer.on("/state/off", [this](){ this->handleState(); });
    webServer.on("/state/on", [this](){ this->handleState(); });
    webServer.on("/state/hatch", [this](){ this->handleState(); });
    webServer.on("/state/phase1", [this](){ this->handleState(); });
    webServer.on("/state/phase2", [this](){ this->handleState(); });
    webServer.on("/state/phase3", [this](){ this->handleState(); });
    webServer.on("/state/phase4", [this](){ this->handleState(); });
    webServer.on("/input", [this](){ this->handleInput(); });
    
    // Start server
    webServer.begin();
    printf(" - Web server started!\n");
    
    // Create FreeRTOS task for handling server requests
    xTaskCreate(
        serverTask,       // Task function
        "ServerTask",     // Task name
        8192,            // Stack size (bytes)
        this,            // Parameter to pass to task
        1,               // Priority (1 = low)
        NULL             // Task handle
    );
    printf(" - Server task created!\n");
}

void ServerClass::serverTask(void* parameter) {
    ServerClass* server = (ServerClass*)parameter;
    printf(" - Server task running on core %d\n", xPortGetCoreID());
    
    while (true) {
        server->worker();
        vTaskDelay(1); // Small delay to prevent watchdog issues
    }
}

void ServerClass::worker() {
    // Handle client requests
    webServer.handleClient();
}

void ServerClass::handleRoot() {
    // Try to open the HTML file from LittleFS
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        printf("Failed to open index.html\n");
        webServer.send(500, "text/plain", "Failed to load page");
        return;
    }
    
    // Read and send the file
    String html = file.readString();
    file.close();
    webServer.send(200, "text/html", html);
}

void ServerClass::handleState() {
    String uri = webServer.uri();
    OutputState newState;
    String stateName;
    
    if (uri == "/state/off") {
        newState = OutputState::OFF;
        stateName = "OFF";
    } else if (uri == "/state/on") {
        newState = OutputState::ON;
        stateName = "ON";
    } else if (uri == "/state/hatch") {
        newState = OutputState::HATCH_OPEN;
        stateName = "HATCH_OPEN";
    } else if (uri == "/state/phase1") {
        newState = OutputState::NOTIFICATION_PHASE_1;
        stateName = "NOTIFICATION_PHASE_1";
    } else if (uri == "/state/phase2") {
        newState = OutputState::NOTIFICATION_PHASE_2;
        stateName = "NOTIFICATION_PHASE_2";
    } else if (uri == "/state/phase3") {
        newState = OutputState::NOTIFICATION_PHASE_3;
        stateName = "NOTIFICATION_PHASE_3";
    } else if (uri == "/state/phase4") {
        newState = OutputState::NOTIFICATION_PHASE_4;
        stateName = "NOTIFICATION_PHASE_4";
    } else {
        webServer.send(400, "text/plain", "Invalid state");
        return;
    }
    
    // Set the output state
    output.setState(newState);
    
    printf(">>> State changed to: %s <<<\n", stateName.c_str());
    webServer.send(200, "text/plain", "State: " + stateName);
}

void ServerClass::handleInput() {
    // Create JSON response with input data
    String json = "{";
    json += "\"isHatchOpen\":";
    json += input.data.isHatchOpen ? "true" : "false";
    json += ",\"isUserSwitchPressed\":";
    json += input.data.isUserSwitchPressed ? "true" : "false";
    json += ",\"batteryVoltage\":";
    json += String(input.data.batteryVoltage, 2);
    json += "}";
    
    webServer.send(200, "application/json", json);
}
