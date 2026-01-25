/**
 * \file bat_temperature.c
 * \author Eduardo Viola Nicola <enicola@cpqd.com.br>
 */

/**
 *  \defgroup acesso ao chip DHT11 - Versão para Arduino
 *
 *  \brief Provê acesso ao sensor de temperatura DHT11
 *
 *
 *  @{
 */
#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>
#include "tasks.h"
#include "dht11_access.h"
// #include "nrf_gpio.h"
// #include "terminal.h"
// #include "nrf_delay.h"
#include "timer.h"

#define BYTES 5
#define BYTE 8
#define TEMP_GPIO (2U)

static float temperature;
static float umidade;
static const uint64_t SENSOR_TIMEOUT = 5000;

// static void temperaturePowerOn(void);
static uint8_t dht11MeasureTemperature(void);
static void sendStartSignal(void);
static uint8_t waitForSensorResponse(void);
static uint8_t waitSensorReadyToOutputSignal(void);
static uint8_t receiveDataFromDHT11(uint8_t itens[5]);
static bool isDataValid(uint8_t itens[BYTES]);

/**
 * Module's tasks runner
 */
void DHT11_run(TASKS_t running_task)
{
    static uint8_t count = 0;
    switch(running_task)
    {
    case POWERON_TASK:
        //temperaturePowerOn();
        break;
    case VERY_SLOW_TIME_TASK:
        // Wait a few seconds between measurements.
        // The DHT sensor is very slow getting the readings
        // (the sensor readings may take up to 2 seconds)
        // Serial.print("DHT11 counter:");
        // Serial.println(count);
        if(count == 2)
        {
            uint8_t ret_code = dht11MeasureTemperature();
            if(ret_code)
            {
                /* Log internal error */
                // INTERRLOG("Temperature error");
            }
            count = 0;
        } else
        {
            count++;
        }
        break;
    default:
        /* Log internal error */
        // INTERRLOG("tried to run an unsupported task");
        break;
    }
}

uint8_t DHT11_test(TASKS_t running_task)
{
    return 33;
}

// static void temperaturePowerOn(void) {}

static uint8_t dht11MeasureTemperature(void)
{
    uint8_t itens[5] = {0, // rh_int
                        0, // rh_decimal
                        0, // temperature_int
                        0, // temperature_decimal
                        0};// checksum
    sendStartSignal();
    uint8_t ret_code = waitForSensorResponse();
    if(ret_code) // timeout
    {
        temperature = 0;
        return ret_code;
    }
    ret_code = waitSensorReadyToOutputSignal();
    if(ret_code) // timeout
    {
        temperature = 0;
        return ret_code;
    }
    ret_code = receiveDataFromDHT11(itens);

    umidade = itens[0] + itens[1]/100.0f;
    if(ret_code)
    {
        temperature = 0;
        return 1;
    }

    temperature = itens[2] + itens[3]/10.0f;

    return ret_code;
}

// Step 1.1: MCU send out start signal to DHT11 and
static void sendStartSignal(void)
{
    // uint8_t start_signal;
    pinMode(TEMP_GPIO, OUTPUT);

    digitalWrite(TEMP_GPIO, LOW);  // Host send start signal
    delay(25);                   // Host pulls low - 18ms minimum
    // start_signal = 
    digitalRead(TEMP_GPIO);
    // printf("Start signal: %d\n", start_signal);
    digitalWrite(TEMP_GPIO, HIGH);    // Host pulls up and wait for sensor's response

}

// Step 1.2: DHT11 send response signal to MCU
static uint8_t waitForSensorResponse(void)
{
    uint64_t ref_time = GetSystemTime();
    //uint8_t host_holding_up;
    //uint8_t sensor_pulls_low;
    pinMode(TEMP_GPIO, INPUT);

    //host_holding_up = nrf_gpio_pin_read(TEMP_GPIO);
    while(digitalRead(TEMP_GPIO))
    {
        if(TestTimerExpired(ref_time, SENSOR_TIMEOUT))
        {
            return 1;
        }
    } // Host pulls up - 20-40us

    // sensor_pulls_low = nrf_gpio_pin_read(TEMP_GPIO); // Sensor remains low about 80us
    // printf("Sensor out response 0: %d\n", sensor_pulls_low);

    return 0;
}

static uint8_t waitSensorReadyToOutputSignal(void)
{
    uint64_t ref_time = GetSystemTime();

    //uint8_t sensor_pulls_up;
    while(!digitalRead(TEMP_GPIO))
    {
        if(TestTimerExpired(ref_time, SENSOR_TIMEOUT))
        {
            return 1;
        }
    }

    // sensor_pulls_up = 
    digitalRead(TEMP_GPIO);
    // printf("Sensor com 1: %d\n", sensor_pulls_up);
    //AD5940_Delay10us(8); // Sensor pulls up - 80us
    return 0;
}

// Step 2: DHT11 send data to MCU
/*
    When DHT11 is sending data to MCU,
    every bit's transmission begin with
    low-voltage-level that last 50us, the
    following high-voltage-level signal's length decide the bit is "1" or "0"
    if the high level stands for 26-28us, this is "0"
    if the high level stands for 70us, this is "1"
*/
static uint8_t receiveDataFromDHT11(uint8_t itens[5])
{
    uint64_t ref_time = GetSystemTime();
    while(digitalRead(TEMP_GPIO))
    {
        if(TestTimerExpired(ref_time, SENSOR_TIMEOUT))
        {
            return 1;
        }
    } // Waiting start of data

    for(uint8_t j=0 ; j<BYTES ; j++)
    {
        for(uint8_t i=0 ; i<BYTE ; i++)
        {
            while(!digitalRead(TEMP_GPIO)) {}
            delayMicroseconds(30); // se descer é "0"
            itens[j] = itens[j] << 1;
            if(digitalRead(TEMP_GPIO))
            {
                // printf("Um!\n");
                itens[j] = itens[j] | 1;
            }
            while(digitalRead(TEMP_GPIO))
            {
                if(TestTimerExpired(ref_time, SENSOR_TIMEOUT))
                {
                    return 1;
                }
            } // aguardar descer
        }
    }
    if(!isDataValid(itens))
    {
        // printf("Dado corrompido! =(\n");
        return 1;
    }
    return 0;
}

static bool isDataValid(uint8_t itens[BYTES])
{
    // checksum
    if(itens[0]+itens[1]+itens[2]+itens[3] == itens[4])
    {
        return true;
    }
    else
    {
        return false;
    }
}

float Dht11GetTemperature(void)
{
    return temperature;
}

uint16_t Dht11GetEncodedTemperature(void)
{
    return (temperature+200)*100;
}

float Dht11GetHumidity(void)
{
    return umidade;
}

/**  @}
 * End of bat_temperature_module group definition
 */