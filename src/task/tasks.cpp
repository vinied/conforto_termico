/**
 * \file tasks.c
 */

/**
 * \defgroup task_module Task
 *
 * \brief Controls all activities that must be performed by each system tasks
 *
 * All activity in the system is controlled by a number of system tasks.\n
 * There are event triggered tasks - such as the PowerOnTask, and there are
 * also tasks that are triggered in a timely, periodic basis.
 *
 * @{
 */

#include <Arduino.h>

// #include "system.h"
#include "tasks.h"
// #include "panel.h"
// #include "terminal.h"
// #include "cell_temperature.h"
#include "mcu_temperature_access.h"
// #include "watchdog.h"
#include "dht11_access.h"

static bool alarme_sonoro = false;

/**
 * Main power on task.
 */
void RunPowerOnTask(void)
{
    // TERMINAL_run(POWERON_TASK);
    
    // DisplaySystemResetReason();
    // PANEL_run(POWERON_TASK);

    // TemperatureAccessSetup();
    // SYSTEM_run(POWERON_TASK);
    #ifdef WATCHDOG
    WATCHDOG_run(POWERON_TASK);
    #endif

    // GpioAccessSetupGpio(14); // Relay
    // GpioAccessSetupGpio(5);  // Sound alarm
    // GpioAccessSetupGpio(2);  // Sound alarm
}

/**
 * Main fast time task.
 */
void RunFastTimeTask(void)
{
    #ifndef SUPER_UM
    // TERMINAL_run(FAST_TIME_TASK);
    // SYSTEM_run(FAST_TIME_TASK);
    #else
    MODBUS_run(FAST_TIME_TASK);
    #endif

    static uint8_t luz = 0;
    // GetNRF52832InternalTemperature();
    digitalWrite(8, ~luz);
    
}

/**
 * Main medium time task.
 */
void RunMediumTimeTask(void)
{
    #ifdef HARDWARE_CPQD
    #ifndef SUPER_UM
    MCP3562_run(MEDIUM_TIME_TASK);
    #endif
    #endif

    uint8_t state_sound = 0;
    state_sound = digitalRead(3); // Detector de som forte

    if (state_sound)
    {
        Serial.println("abaixar o volume, nao maltrate o seu pet");
        alarme_sonoro = true;
    }
}

/**
 * Main slow time task.
 */
void RunSlowTimeTask(void)
{


}

/**
 * Main very slow time task.
 */
void RunVerySlowTimeTask(void)
{
    static uint8_t luz = 0;
    // GetNRF52832InternalTemperature();
    digitalWrite(8, ~luz);


    // SYSTEM_run(VERY_SLOW_TIME_TASK);
    #ifdef WATCHDOG
        WATCHDOG_run(VERY_SLOW_TIME_TASK);
    #endif

    static uint8_t minute_counter = 0;
    if (minute_counter == 19)
    {
        float ntc_temp, mcu_temp, humidity, dht11_temp;
        //ntc_temp = GetTemperature();
        dht11_temp = Dht11GetTemperature();
        mcu_temp = GetMcuInternalTemperature();
        humidity = Dht11GetHumidity();
        Serial.print(  String(dht11_temp) + " " 
                    +  String(mcu_temp) + " "
                    +  String(alarme_sonoro) + " ");
        Serial.println(humidity);
        // Serial.println(alarme_sonoro, humidity);
        if (ntc_temp > 30)
        {
            digitalWrite(4, LOW); // Ligar ventilador
        } else
        {
            digitalWrite(4, HIGH); // Desligar ventilador
        }

        if (alarme_sonoro)
        {
            alarme_sonoro = 0;
        }

        minute_counter = 0;
        
        //GpioAccessClear(7);
        
    } else
    {
        minute_counter++;
        
        //GpioAccessSet(7);
        
    }

    DHT11_run(VERY_SLOW_TIME_TASK);

}

/**
 * Main power off task.
 */
void RunPowerOffTask(void)
{
}


/**  @}
 * End of task_module group definition
 */
