/**
 * \file systick_access.c
 */


/**
 *  \defgroup systick_access_module SystickAccess
 *
 *  \brief Abstração do temporizador de sistema de um microcontrolador.
 *
 *  @{
 */

#include <stdlib.h>

#include "systick_access.h"

/** Actual configured system clock frequency, as returned by SysCtlClockFreqSet */
static uint32_t system_clock;

/**
 * \brief Ponteiro para a função de \e callback a ser invocada pelo tratamento de interrupções do temporizador do sistema, \e SysTick \e timer
 */
static SYSTICK_CALLBACK_t systick_callback = NULL;

/**
 * \brief \e Flag indicativo de que o contador do \e Systick \ timer expirou, sem que a IRQ correspondente tenha sido tratada
 */
static volatile bool wrapped_around_systick;

/**
 * \brief Inicializa o temporizador do sistema, \e SysTick \e timer
 * Programa o temporizador de sistema \e SysTick \r timer para gerar uma interrupção
 * periódica, a cada milissegundo, para referência básica de tempo do sistema
 */
void SystickAccessSetupSystick(void)
{
    /* Enable the SysTick timer to generate a periodic interrupt */
   

    /* System clock setup: configure the system clock for 16 MHz */
    system_clock = F_CPU;
}


/**
 * \brief Habilita o temporizador SysTick, e respectivas interrupções
 */
void SystickAccessEnable(void)
{
    /* Enable SysTick countdown and exception generation */
}


/**
 * \brief Desabilita o temporizador SysTick
 */
void SystickAccessDisable(void)
{

}


/**
 * \brief Obtém o número de microssegundos transcorridos desde o último evento do \e SysTick \e timer
 *
 * A referência de tempo básica do sistema é um contador incrementado pelo \e SysTick \e timer,
 *   que é programado para gerar eventos a cada milissegundo. Esta função permite medições
 *   de tempo com uma resolução bem maior, de microssegundos, pois utiliza o acesso direto
 *   ao contador de controle do \e SysTick \e timer
 *
 * @return o número de microssegundos transcorridos desde o último evento do \e SysTick \e timer
 */
uint32_t SystickAccessGetHiResTime(void)
{
    uint32_t systick_int_period_in_ticks;
    uint32_t remaining_ticks_in_this_cycle;

    /* Get number of processor clock cycles - ticks - per SysTick interrupt cycle */
    systick_int_period_in_ticks = 0;
    /* Get number of remaining processor cycles until next SysTick counter wrap around / reload / IRQ request event */
    remaining_ticks_in_this_cycle = NRFX_SYSTICK_MS_STEP - nrf_systick_val_get();

    /* Compute end return elapsed microseconds since last SysTick counter wrap around / reload / IRQ request event */
    return (systick_int_period_in_ticks - remaining_ticks_in_this_cycle) * SYSTICK_IRQ_US_PERIOD / systick_int_period_in_ticks;
}



/**
 * \brief Obtém o estado do flag de expiração do contador do \e SysTick \e timer
 *
 * O \e flag de expiração do contador do \e Systick \e timer indica que o contador expirou
 *   pelo menos uma vez, após o último tratamento da interrupção do \e Systick \e timer.
 *
 * ATENÇÂO: A leitura deste \e flag resulta na inativação do mesmo
 *
 * @return TRUE se o \e flag de expiração estava ativo, ou FALSE, em caso contrário
 */
bool SystickAccessPendingIrq(void)
{
    if (!wrapped_around_systick)
    {

    }
    return wrapped_around_systick;
}


/**
 * \brief Registra uma função de \e callback, a ser invocada pelo tratamento de interrupção do \e SysTick \e timer
 *
 * @param callback - um ponteiro para uma função de \e callback, a ser invocada pelo tratamento de interrupção do \e SysTick \e timer
 */
void SystickAccessInstallCallback(SYSTICK_CALLBACK_t callback)
{
    systick_callback = callback;
}


/**
 * \brief Trata requisições de interrupções provenientes do \e SysTick \e timer
 */
void SystickAccessSysTickCallback(void)
{
    static volatile bool dummy_cond __attribute__((unused));

    /* Clear wrapped counter bit in NVIC_ST_CTRL register, *and* it's flag in RAM */

    wrapped_around_systick = false;

    /* Call registered callback function, if any */
    if (NULL != systick_callback)
    {
        systick_callback();
    }
}

/**
 * Returns System Clock actual configured frequency.
 */
uint32_t getSystemClock(void)
{
    return system_clock;
}



/**  @}
 * End of systick_access_module group definition
 */
