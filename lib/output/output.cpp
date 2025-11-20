#include "output.hpp"
#include <Arduino.h>
#include <FastLED.h>
#include <pinout.hpp>

// WS2812B LED strip configuration
#define NUM_PIXELS 1
CRGB leds[NUM_PIXELS];

// Timing constants (in milliseconds)
#define BUILT_IN_BLINK_INTERVAL 200
#define BUZZ_PHASE1_INTERVAL 4000
#define BUZZ_PHASE2_INTERVAL 2000
#define BUZZ_PHASE3_INTERVAL 1000
#define BUZZ_PHASE4_INTERVAL 500
#define BEEP_PHASE3_INTERVAL 1000
#define BEEP_PHASE4_INTERVAL 500
#define BUZZ_DURATION 200
#define BEEP_DURATION 100

// FreeRTOS task configuration
#define WORKER_TASK_STACK_SIZE 2048
#define WORKER_TASK_PRIORITY 1
#define WORKER_TASK_DELAY_MS 10

// Static pointer for FreeRTOS task
static OuptutClass* instancePtr = nullptr;


void OuptutClass::begin() {
    // Store instance pointer
    instancePtr = this;
    
    // Initialize GPIO pins
    pinMode(PIN_LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_VIBE, OUTPUT);
    
    // Initialize FastLED for WS2812B
    FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, NUM_PIXELS);
    FastLED.setBrightness(50); // Set brightness (0-255)
    FastLED.clear();
    FastLED.show();
    
    // Set initial state
    currentState = OutputState::OFF;
    
    // Turn everything off
    digitalWrite(PIN_LED_BUILTIN, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_VIBE, LOW);
    
    // Create FreeRTOS task for worker
    xTaskCreate(
        outputTask,           // Task function
        "OutputWorker",             // Task name
        WORKER_TASK_STACK_SIZE,     // Stack size
        this,                       // Parameter passed to task
        WORKER_TASK_PRIORITY,       // Priority
        NULL                        // Task handle (not needed)
    );
    
    Serial.println("Output module initialized with FreeRTOS task");
}

void OuptutClass::setState(OutputState newState) {
    currentState = newState;
}

void OuptutClass::outputTask(void* parameter) {
    OuptutClass* instance = static_cast<OuptutClass*>(parameter);
    
    // Timing variables
    unsigned long lastBlinkTime = 0;
    unsigned long lastBuzzTime = 0;
    unsigned long lastBeepTime = 0;
    bool blinkState = false;
    bool buzzActive = false;
    bool beepActive = false;
    unsigned long buzzStartTime = 0;
    unsigned long beepStartTime = 0;
    
    // Infinite loop - runs continuously in the background
    while (true) {
        unsigned long currentTime = millis();
        OutputState state = instance->currentState;

        // Blink built in led if not OFF
            if (state != OutputState::OFF) {
                if (currentTime - lastBlinkTime >= BUILT_IN_BLINK_INTERVAL) {
                    blinkState = !blinkState;
                    digitalWrite(PIN_LED_BUILTIN, blinkState ? HIGH : LOW);
                    lastBlinkTime = currentTime;
                }
            }else{
                digitalWrite(PIN_LED_BUILTIN, LOW);
            }

        // Show battery state if HATCH_OPEN
            if (state != OutputState::OFF && state != OutputState::ON) {
                // For simplicity, show green for now
                leds[0] = CRGB::Green;
                FastLED.show();
            }else{
                // Turn off LEDs
                leds[0] = CRGB::Black;
                FastLED.show();
            }

        // Handle output states
            switch (state) {
                case OutputState::OFF:
                    // All outputs off
                    digitalWrite(PIN_BUZZER, LOW);
                    digitalWrite(PIN_VIBE, LOW);
                    break;

                case OutputState::ON:
                    // All outputs off
                    digitalWrite(PIN_BUZZER, LOW);
                    digitalWrite(PIN_VIBE, LOW);
                    break;

                case OutputState::HATCH_OPEN:
                    // Buzzer and vibe off
                    digitalWrite(PIN_BUZZER, LOW);
                    digitalWrite(PIN_VIBE, LOW);
                    break;

                case OutputState::NOTIFICATION_PHASE_1:
                    if (!buzzActive && currentTime - lastBuzzTime >= BUZZ_PHASE1_INTERVAL) {
                        buzzActive = true;
                        buzzStartTime = currentTime;
                        lastBuzzTime = currentTime;
                    }

                    if (buzzActive) {
                        if (currentTime - buzzStartTime < BUZZ_DURATION) {
                            digitalWrite(PIN_VIBE, HIGH);
                        } else {
                            digitalWrite(PIN_VIBE, LOW);
                            buzzActive = false;
                        }
                    }

                    digitalWrite(PIN_BUZZER, LOW);
                    break;

                case OutputState::NOTIFICATION_PHASE_2:
                    if (!buzzActive && currentTime - lastBuzzTime >= BUZZ_PHASE2_INTERVAL) {
                        buzzActive = true;
                        buzzStartTime = currentTime;
                        lastBuzzTime = currentTime;
                    }

                    if (buzzActive) {
                        if (currentTime - buzzStartTime < BUZZ_DURATION) {
                            digitalWrite(PIN_VIBE, HIGH);
                        } else {
                            digitalWrite(PIN_VIBE, LOW);
                            buzzActive = false;
                        }
                    }

                    digitalWrite(PIN_BUZZER, LOW);
                    break;

                case OutputState::NOTIFICATION_PHASE_3:
                    // Buzzing
                        if (!buzzActive && currentTime - lastBuzzTime >= BUZZ_PHASE3_INTERVAL) {
                            buzzActive = true;
                            buzzStartTime = currentTime;
                            lastBuzzTime = currentTime;
                        }

                        if (buzzActive) {
                            if (currentTime - buzzStartTime < BUZZ_DURATION) {
                                digitalWrite(PIN_VIBE, HIGH);
                            } else {
                                digitalWrite(PIN_VIBE, LOW);
                                buzzActive = false;
                            }
                        }

                    // Beeping
                        if (!beepActive && currentTime - lastBeepTime >= BEEP_PHASE3_INTERVAL) {
                            beepActive = true;
                            beepStartTime = currentTime;
                            lastBeepTime = currentTime;
                        }

                        if (beepActive) {
                            if (currentTime - beepStartTime < BEEP_DURATION) {
                                digitalWrite(PIN_BUZZER, HIGH);
                            } else {
                                digitalWrite(PIN_BUZZER, LOW);
                                beepActive = false;
                            }
                        }
                    break;

                case OutputState::NOTIFICATION_PHASE_4:
                    // Buzzing
                    if (!buzzActive && currentTime - lastBuzzTime >= BUZZ_PHASE4_INTERVAL) {
                        buzzActive = true;
                        buzzStartTime = currentTime;
                        lastBuzzTime = currentTime;
                    }

                    if (buzzActive) {
                        if (currentTime - buzzStartTime < BUZZ_DURATION) {
                            digitalWrite(PIN_VIBE, HIGH);
                        } else {
                            digitalWrite(PIN_VIBE, LOW);
                            buzzActive = false;
                        }
                    }

                    // Beeping
                    if (!beepActive && currentTime - lastBeepTime >= BEEP_PHASE4_INTERVAL) {
                        beepActive = true;
                        beepStartTime = currentTime;
                        lastBeepTime = currentTime;
                    }

                    if (beepActive) {
                        if (currentTime - beepStartTime < BEEP_DURATION) {
                            digitalWrite(PIN_BUZZER, HIGH);
                        } else {
                            digitalWrite(PIN_BUZZER, LOW);
                            beepActive = false;
                        }
                    }
            }

            // Small delay to prevent task from hogging CPU
            vTaskDelay(pdMS_TO_TICKS(WORKER_TASK_DELAY_MS));
        }
}
