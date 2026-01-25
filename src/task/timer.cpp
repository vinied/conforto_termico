/**
 * \file timer.c
 */

/**
 * \defgroup timer_module Timer
 *
 * \brief Referência de tempo do sistema
 *
 * O módulo Timer implementa funções de manutenção da referência
 *   de tempo básica do sistema, bem como funções de medição de tempo
 *   baseadas nessa base de tempo.
 *
 * A referência de tempo do sistema é um contador de milissegundos
 *   transcorridos desde o evento de \e Power \e On, que é incrementado
 *   a partir de uma interrupção periódica gerada pelo \e SysTick \e timer
 *
 * O tempo de sistema pode ser obtido como o valor bruto do contador
 *   de milissegundos, através da função GetSystemTime().
 *
 * As funções mais elaboradas, GetElapsedTime() e TestTimerExpired()
 *   permitem a medição conveniente de intervalos de tempo, com resolução
 *   de milissegundos.
 *
 * Adicionalmente, as funções GetHiResSystemTime() e GetHiResElapsedTime()
 *   permitem medição de intervalos de tempo com resolução mais alta,
 *   de microssegundos.
 *
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

// #include "system.h"

#include "scheduler.h"
// #include "sysstart.h"
// #include "systick_access.h"

// #include "timer.h"

// #include "rtc_access.h"

/** Tempo transcorrido, em milissegundos, desde o último evento de power-on */
static uint64_t system_timer;


/**
 * \brief Inicialização da base de tempo do sistema
 *
 * Esta função inicializa o contador, o temporizador \e SysTick
 *   e registra uma função de \e callback para ser invocada
 *   pela função de tratamento de interrupções do mesmo.
 */
void SetupSystemTimer(void)
{
    /* Initialize SysTick timer driver */
    // RTCAccessSetup(0);
    /* Register call back function into SysTick driver */
    // RTCAccessInstallCallback(0, &SystickRunner);
}


/**
 * \brief Manutenção da base de tempo do sistema
 *
 * Esta função é registrada como \e callback para ser chamado pela
 *   função de tratamento das interrupções geradas pelo temporizador
 *   do sistema, \e SysTick, e executa duas funções:
 *
 *   * atualizar a base de tempo do sistema (contador de milissegundos);
 *   * controlar o agendamento das tarefas periódicas/temporais do sistema.
 *
 * Uma vez que esta função é executada com alta frequência, é importante
 *   que a mesma seja mantida curta e eficiente.
 */
void SystickRunner(void)
{
    // int32_t system_up;
    /* Update system timer */
    system_timer += 1;
    if( system_timer % 1000 == 0 )
    {
      // system_up = system_timer / 1000;
      // printf("%d segundos\n", system_up);
      // DevledToggleLed();
      // IncSystemLife();
    }
    /* Set pending task flags */
    SetTasksFlags();
}


/**
 * \brief Obtém o número de milissegundos transcorridos desde o evento de \e Power \e On
 *
 * @return O número de milissegundos transcorridos desde o evento de \e Power \e On
 */
uint64_t GetSystemTime(void)
{
    return system_timer;
}


/**
 * \brief Obtém o número de milissegundos transcorridos desde o armazenamento de ref_time
 *
 * @param [in] ref_time - uma referência de tempo armazenada em um momento anterior, via GetSystemTime()
 * @return O número de milissegundos transcorridos desde o armazenamento de ref_time, passado como argumento
 */
uint64_t GetElapsedTime(uint64_t ref_time)
{
    uint64_t result = (0U);
    if (system_timer > ref_time)
    {
        result = system_timer - ref_time;
    }
    return result;
}


/**
 * \brief Verifica se um intervalo de tempo especificado já expirou
 *
 * @param [in] ref_time - uma referência de tempo previamente armazenada, usando a função GetSystemTime()
 * @param [in] timeout - o intervalo de tempo, e, milissegundos, a ser verificado
 * @return TRUE, se o tempo transcorrido desde 'ref_time' supera 'timeout', ou FALSE, em caso contrário
 */
bool TestTimerExpired(uint64_t ref_time, uint64_t timeout)
{
    bool result = false;
    if (system_timer > ref_time)
    {
        if ((system_timer - ref_time) >= timeout)
        {
            result = true;
        }
    }
    return result;
}


/**
 * \brief Obtém uma referência de tempo de alta resolução (microssegundos transcorridos desde o evento de \e PowerOn )
 *
 * Uma referência de tempo de alta resolução armazena, juntamente ao valor
 *   do \e System \e timer no momento de armazenamento, o número de microssegundos
 *   transcorridos desde o último evento do \e System \e timer. Com isso,
 *   é possível medir intervalos de tempo com resolução de microssegundos,
 *   utilizando-se a função GetHiResElapsedTime().
 *
 * @return uma referência de tempo de alta resolução
 */
uint64_t GetHiResSystemTime(void)
{
    uint32_t microseconds;
    uint64_t milliseconds;

    // EnterCritical();
    // microseconds = SystickAccessGetHiResTime();
    milliseconds = GetSystemTime();
    // if (SystickAccessPendingIrq())
    // {
    //    milliseconds++;
    //}
    // LeaveCritical();

    return milliseconds * (1000U) + (uint64_t)microseconds;
}


/**
 * \brief Obtém o intervalo de tempo, em microssegundos, transcorrido desde o armazenamento de 'reference_time'
 *
 * Dada uma referência de tempo de alta resolução armazenada anteriormente, através da
 *   função GetHiResSystemTime(), a função 'GetHiResElapsedTime()' calcula o tempo
 *   transcorrido desde aquele momento até o momento presente, em microssegundos.
 *
 * @param [in] reference_time - uma referência de tempo de alta resolução, armazenada anteriormente
 * @return o número de microssegundos transcorridos desde o momento do armazenamento de 'reference_time', até o momento presente
 */
uint64_t GetHiResElapsedTime(uint64_t reference_time)
{
    return GetHiResSystemTime() - reference_time;
}


/**  @}
 * End of timer_module group definition
 */
