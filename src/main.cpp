#include <Arduino.h>
#include "avr8-stub.h"
#include "app_api.h" // only needed with flash breakpoints

#include "tasks.h"
#include "scheduler.h"

#include "dht11_access.h"

void setup() {
    // put your setup code here, to run once:
    // debug_init();
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    EnableSystemTasks();
    TriggerPowerOnTask();

}


/**
 * \brief Runs main scheduler loop.
 *
 * The **scheduler main loop** is an endless loop that monitors **pending task** flags
 *     to decide if it is time to run any of the predefined system tasks.
 *     Whenever a set flag is found, the flag is cleared, the corresponding task
 *     function is called, and the loop proceeds to its next iteration;\n
 * Since higher frequency tasks flags are pooled first and only one task is
 *     ran per loop iteration, higher frequency task are, naturally, prioritized
 *     over the lower frequencies ones;\n
 * Additionally, the loop code registers the execution time of the tasks it
 *     calls, for processor load estimation.
 */
void loop() {
    // delay(1000);                     // Wait for a second - que dura 3x mais que um segundo normal

    SetTasksFlags();
    RunMainLoop();

    static uint8_t minute_counter = 0;
    if (minute_counter == 19)
    {
        DHT11_run(VERY_SLOW_TIME_TASK);
        minute_counter = 0;
    } else
    {
        minute_counter++;
        //GpioAccessSet(7);
    }

    float dht_temp =  Dht11GetTemperature();
    if (dht_temp > 30)
    {
        digitalWrite(4, LOW); // Ligar ventilador
    } else if (dht_temp < 29)
    {
        digitalWrite(4, HIGH); // Desligar ventilador
    }
}