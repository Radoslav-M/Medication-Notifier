#include <pinout.hpp>
#include <Arduino.h>
#include "input.hpp"
#include "pinout.hpp"

void InputClass::begin() {
    printf("Initializing input module...\n");

    // Initialize GPIO pins
        pinMode(PIN_HATCH_BUTTON, INPUT);
        pinMode(PIN_USER_BUTTON, INPUT);
        pinMode(PIN_BATTERY_VOLTAGE, INPUT);

    // Create FreeRTOS task for handling input
    xTaskCreate(
        inputTask,       // Task function
        "InputTask",     // Task name
        4096,            // Stack size (bytes)
        this,            // Parameter to pass to task
        1,               // Priority (1 = low)
        NULL             // Task handle
    );
    printf(" - Input task created!\n");
}

void InputClass::inputTask(void* parameter) {
    InputClass* input = (InputClass*)parameter;
    printf(" - Input task running on core %d\n", xPortGetCoreID());

    while (true) {
        // Read hatch switch state
        input->data.isHatchOpen = digitalRead(PIN_HATCH_BUTTON);

        // Read user button state
        input->data.isUserSwitchPressed = digitalRead(PIN_USER_BUTTON);

        // Read battery voltage
        int rawValue = analogRead(PIN_BATTERY_VOLTAGE);
        input->data.batteryVoltage = (rawValue / 4095.0f) * 3.3f * 2.0f;

        vTaskDelay(100 / portTICK_PERIOD_MS); // Read inputs every 100ms
    }
}