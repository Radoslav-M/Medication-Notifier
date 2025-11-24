#include "server.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <output.hpp>
#include <input.hpp>
#include "time.h"

// Create WebServer instance on port 80
WebServer webServer(80);

// External output instance (declared in main.cpp)
extern OuptutClass output;
extern InputClass input;

const char* ntpServer = "pool.ntp.org";
const char* timezone = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

void ServerClass::begin() {
    printf("Starting server...\n");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        printf(" - Failed to mount LittleFS!\n");
        return;
    }
    printf(" - LittleFS mounted successfully\n");
    
    // Try to sync time with NTP first
    printf(" - Attempting NTP time sync...\n");
    if (syncTimeWithNTP()) {
        printf(" - Time synced successfully!\n");
    } else {
        printf(" - NTP sync failed, continuing without network time\n");
    }
    
    printf(" - Starting Access Point...\n");
    
    // Start WiFi in AP+STA mode (allows both AP and Station simultaneously)
    WiFi.mode(WIFI_AP_STA);
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

bool ServerClass::tryConnectToKnownNetworks() {
    printf(" - Scanning for known networks...\n");
    
    // Scan for available networks
    int networksFound = WiFi.scanNetworks();
    printf(" - Found %d networks\n", networksFound);
    
    if (networksFound == 0) {
        return false;
    }
    
    // Try each known network in priority order
    for (const auto& known : knownNetworks) {
        // Check if this network is available
        for (int i = 0; i < networksFound; i++) {
            if (WiFi.SSID(i) == known.ssid) {
                printf(" - Found known network: %s\n", known.ssid);
                printf(" - Attempting to connect...\n");
                
                WiFi.begin(known.ssid, known.password);
                
                // Wait up to 10 seconds for connection
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                    delay(500);
                    printf(".");
                    attempts++;
                }
                printf("\n");
                
                if (WiFi.status() == WL_CONNECTED) {
                    printf(" - Connected to: %s\n", known.ssid);
                    printf(" - IP address: %s\n", WiFi.localIP().toString().c_str());
                    return true;
                }
                
                printf(" - Failed to connect to: %s\n", known.ssid);
            }
        }
    }
    
    return false;
}

bool ServerClass::syncTimeWithNTP() {
    if (!tryConnectToKnownNetworks()) {
        printf(" - No known networks available\n");
        return false;
    }

    // Configure time from NTP FIRST (without timezone)
    printf(" - Configuring time from NTP server...\n");
    configTime(0, 0, ntpServer);

    // Wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    struct tm timeinfo;

    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        printf(" - Waiting for time sync... (%d/%d)\n", retry + 1, retry_count);
        delay(1000);
        retry++;
    }

    if (retry >= retry_count) {
        printf(" - Failed to obtain time from NTP\n");
        WiFi.disconnect(true);
        return false;
    }

    // NOW set timezone AFTER time is synced
    printf(" - Setting timezone to: %s\n", timezone);
    setenv("TZ", timezone, 1);
    tzset();
    
    // Get time again with proper timezone
    getLocalTime(&timeinfo);

    // Print the synced time with timezone info
    char timeString[64];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);
    printf(" - Time synchronized: %s\n", timeString);
    printf(" - DST active: %s\n", timeinfo.tm_isdst ? "Yes (CEST)" : "No (CET)");

    // Debug output
    time_t now;
    time(&now);
    printf(" - Timezone env var: %s\n", getenv("TZ"));

    timeSynced = true;
    WiFi.disconnect(true);
    printf(" - Disconnected from WiFi (AP remains active)\n");

    return true;
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