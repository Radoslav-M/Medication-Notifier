#pragma once
// This module handles interupts every time either of the switches changes state. It also periodicaly reads the battery voltage.
// The data is provided to other modules through a public struct

struct InputData {
    bool isHatchOpen;         // True if hatch is open, false if closed
    bool isUserSwitchPressed; // True if the user button is pressed
    float batteryVoltage;     // Current battery voltage in volts
};

class InputClass {
public:
    // Methods
        void begin(); // Initializes the output module

    // Attributes
        InputData data; // Current input data

private:
    // Methods
        static void inputTask(void* parameter); // FreeRTOS task function
};