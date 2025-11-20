#pragma once
// This module is responisble for managing the outpit devices, manely the:
//  - LED BUILTIN
//  - WS2812B
//  - Beeper (Buzzer)
//  - Vibration Motor

// It provides a api to set the state of the outputs. The worker will automaticly control the GPIOs based on the selected state.
// Table of states:
//  - OFF: Active when device is sleeping. All outputs are off.
//  - ON: Active when the device is awake, but no other states are active. LED BUILTIN is blinking ON.
//  - HATCH_OPEN: Active when device is awake but no notifications are active. LED BUILTIN is blinking
//  - HATCH_OPEN: Active when the hatch is open. WS2812B color is determined by battery level.
//  - NOTIFICATION_PHASE_1: Vibrating ocasionaly.
//  - NOTIFICATION_PHASE_2: Vibrating more often.
//  - NOTIFICATION_PHASE_3: Starts beeping (slowly) too.
//  - NOTIFICATION_PHASE_4: Vibrating and beeping rapidly.

enum class OutputState {
    OFF,
    ON,
    HATCH_OPEN,
    NOTIFICATION_PHASE_1,
    NOTIFICATION_PHASE_2,
    NOTIFICATION_PHASE_3,
    NOTIFICATION_PHASE_4
};

class OuptutClass {
public:
    // Methods
        void begin();               // Initializes the output module
        void setState(OutputState); // Sets the current output state

private:
    // Methods
        static void outputTask(void* parameter); // FreeRTOS task function

    // Members
        OutputState currentState;   // Current output state
};