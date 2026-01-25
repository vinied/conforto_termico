#ifndef TEMPERATURE_ACCESS_H_
#define TEMPERATURE_ACCESS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * \file temperature_access.h
 */

/**
 *  \addtogroup temperature_access_module
 *  @{
 */


int8_t TemperatureAccessSetup(void);

float GetMcuInternalTemperature(void);
float GetInternalTemperature(void);
uint16_t GetEncodedInternalTemperature(void);

//uint32_t TEMPERATURE_run(void);

/**  @}
 * End of temperature_access_module group inclusion
 */

#endif /* TEMPERATURE_ACCESS_H_ */
