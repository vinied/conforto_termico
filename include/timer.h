#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \file timer.h
 */

/**
 * \addtogroup timer_module
 */


uint64_t GetSystemTime(void);

uint64_t GetElapsedTime(uint64_t ref_time);
bool   TestTimerExpired(uint64_t ref_time, uint64_t timeout);

uint64_t GetHiResSystemTime(void);
uint64_t GetHiResElapsedTime(uint64_t reference_time);


void     SetupSystemTimer(void);
void     SystickRunner(void);


/**  @}
 * End of timer_module group inclusion
 */


#endif /* TIMER_H_ */
