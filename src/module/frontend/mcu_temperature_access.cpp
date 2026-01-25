#include <Arduino.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "mcu_temperature_access.h"

static float volatile temp;

int8_t TemperatureAccessSetup(void){}

/** @brief Function for main application entry.
 */
float GetMcuInternalTemperature(void)
{
    // Set the reference voltage to the internal 1.1V reference (REFS1 | REFS0)
  // and select the temperature sensor channel (MUX3)
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  
  // Wait for the voltage reference to stabilize (typically requires a delay, 
  // but a "dummy" read often suffices as the first reading is discarded).
  delay(20); 

  ADCSRA |= _BV(ADEN); // Enable the ADC
  ADCSRA |= _BV(ADSC); // Start the first conversion (dummy read)
  while (ADCSRA & _BV(ADSC)); // Wait for it to complete

  ADCSRA |= _BV(ADSC); // Start the actual conversion
  while (ADCSRA & _BV(ADSC)); // Wait for it to complete

  // Read the 10-bit result (ADCL must be read first, then ADCH)
  long rawTemp = ADCW; 

  // Conversion formula from the datasheet (varies slightly by calibration, 
  // but this is a common starting point). The measured voltage has a linear 
  // relationship to temperature.
  long temperatureC = (rawTemp - 324.31) / 1.22;

  return temperatureC;
}

float GetInternalTemperature(void)
{
    return temp;
}

uint16_t GetEncodedInternalTemperature(void)
{
    return (temp+200)*100;
}


/** @} */
