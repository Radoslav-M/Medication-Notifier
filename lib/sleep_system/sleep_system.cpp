#include "sleep_system.hpp"
#include <Arduino.h>

// Public
    void SleepSystemClass::begin() {
        // Create the sleep system task
            xTaskCreate(
                sleepSystemTask,          // Task function
                "SleepSystemTask",        // Name of the task
                4096,                     // Stack size (in words)
                this,                     // Task input parameter
                1,                        // Priority of the task
                nullptr                   // Task handle
            );
    }
    void SleepSystemClass::sleepSystemTask(void* parameter) {
        // Initialization
        SleepSystemClass* sleepSystem = static_cast<SleepSystemClass*>(parameter);
        printf("Sleep system task started on core %d\n", xPortGetCoreID());

        // Main loop for the sleep system task
            while (true) {
                // If the hatch is closed for X seconds, enter deep sleep
                    static int hatchClosedCounter = 0;
                    if(!sleepSystem->input.data.isHatchOpen) {
                        hatchClosedCounter++;
                        printf("Hatch closed for %d seconds\n", hatchClosedCounter);
                        if(hatchClosedCounter >= sleepSystem->CONF_SLEEP_DELAY_HATCH_CLOSED_S) {
                            printf("Hatch closed for %d seconds, entering deep sleep...\n", sleepSystem->CONF_SLEEP_DELAY_HATCH_CLOSED_S);
                            sleepSystem->enterDeepSleep();
                            hatchClosedCounter = 0; // Reset counter after waking up
                        }
                    } else {
                        hatchClosedCounter = 0; // Reset counter if hatch is open
                    }


                // Delay
                    vTaskDelay(pdMS_TO_TICKS(1000));
            }
    }

// Private
    void SleepSystemClass::setCurrentTime(int year, int month, int day, int hour, int minute, int second) {
        // Set the RTC time using the provided parameters
            struct tm t;
            t.tm_year = year - 1900; // tm_year is years since 1900
            t.tm_mon = month - 1;    // tm_mon is 0-11
            t.tm_mday = day;
            t.tm_hour = hour;
            t.tm_min = minute;
            t.tm_sec = second;
            time_t timeSinceEpoch = mktime(&t);
            struct timeval now = { .tv_sec = timeSinceEpoch, .tv_usec = 0 };
            settimeofday(&now, nullptr);
    }
    struct tm SleepSystemClass::getCurrentTime() {
        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);  // ✅ Thread-safe, no static memory
        return timeinfo;
    }
    void SleepSystemClass::enterDeepSleep() {
        // Configure wakeup sources: hatch open (GPIO) and scheduled times (RTC)
            // Wake on hatch open
                esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PIN_HATCH_BUTTON), 1); // Wake when GPIO goes high (hatch opened)

            // Wake on scheduled medication times
                struct tm currentTime = getCurrentTime();
                time_t now = mktime(&currentTime);
                time_t earliestWakeup = now + 24 * 3600; // Default to 24 hours later

                for(const auto& schedule : MedicationSchedule) {
                    struct tm wakeTime = currentTime;
                    wakeTime.tm_hour = schedule.hour;
                    wakeTime.tm_min = schedule.minute;
                    wakeTime.tm_sec = 0;

                    time_t scheduledTime = mktime(&wakeTime);
                    if(scheduledTime <= now) {
                        scheduledTime += 24 * 3600; // Schedule for next day if time has passed
                    }

                    if(scheduledTime < earliestWakeup) {
                        earliestWakeup = scheduledTime;
                    }
                }

                esp_sleep_enable_timer_wakeup((earliestWakeup - now) * 1000000); // Convert to microseconds

        // Print wakeup info
            printf("Entering deep sleep. Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                currentTime.tm_year + 1900, currentTime.tm_mon + 1, currentTime.tm_mday,
                currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);

            struct tm wakeupTm;
            localtime_r(&earliestWakeup, &wakeupTm);
            printf("Will wake on hatch open or at scheduled time: %04d-%02d-%02d %02d:%02d:%02d\n",
                wakeupTm.tm_year + 1900, wakeupTm.tm_mon + 1, wakeupTm.tm_mday,
                wakeupTm.tm_hour, wakeupTm.tm_min, wakeupTm.tm_sec);

        // Enter deep sleep
            esp_deep_sleep_start();
    }