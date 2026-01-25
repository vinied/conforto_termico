#ifndef SYSTICK_ACCESS_H_
#define SYSTICK_ACCESS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * \file systick_access.h
 */

/**
 * \addtogroup systick_access_module
 * @{
 */

  /**
 * @brief Number of milliseconds to wait in single loop
 *
 * Constant used by @ref nrd_drv_systick_delay_ms function
 * to split waiting into loops and rest.
 *
 * It describes the number of milliseconds to wait in single loop.
 *
 * See @ref nrfx_systick_delay_ms source code for details.
 */
#define NRFX_SYSTICK_MS_STEP (64U)


 /**
 * A frequência do \e tick de sistema (\e timer de referência do \e scheduler), em Hz
 */
#define SYSTEM_TICK_FREQUENCY       (64000000U)

 /**
 * \brief Frequência de geração de eventos do temporizador do sistema, \e SysTick \e timer, em Hz
 */
#define SYSTICK_IRQ_FREQUENCY   SYSTEM_TICK_FREQUENCY

/**
 * \brief Período do temporizador do sistema, \e SysTick \e timer, em microssegundos
 */
#define SYSTICK_IRQ_US_PERIOD   (1000000 / SYSTICK_IRQ_FREQUENCY)

/**
 * \brief tipo/assinatura de uma função de \e callback a ser chamada pelo tratamento de interrupções do temporizador do sistema, \e SysTick \e timer
 */
typedef void (*SYSTICK_CALLBACK_t)(void);



void 		SystickAccessSetupSystick(void);
void 		SystickAccessSysTickCallback(void);
void 		SystickAccessEnable(void);
void 		SystickAccessDisable(void);
uint32_t	SystickAccessGetHiResTime(void);
bool        SystickAccessPendingIrq(void);
void 		SystickAccessInstallCallback(SYSTICK_CALLBACK_t callback);
uint32_t    getSystemClock(void);



/**  @}
 * End of systick_access_module group inclusion
 */


#endif /* SYSTICK_ACCESS_H_ */
