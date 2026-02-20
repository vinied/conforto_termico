/**
 * \file ntc_temperature.c
 * \author Eduardo Viola Nicola <enicola@cpqd.com.br>
 */

/**
 *  \defgroup cell_temperature_module NTC Temperature
 *
 *
 *
 *  @{
 */
#include <Arduino.h>
#include <stdint.h>
#include <math.h>
#include "tasks.h"
#include "ntc_temperature.h"
#include "timer.h"
//#include "lininterp.h"
//#include "system.h"
//#include "fsm_manager.h"

#define TEMP_GPIO (0U)

/**********************/
/* Module global data */
/**********************/
CELLTEMP_STATUS_t cell_temp_fsm;
static void cellTempSetState(CELLTEMP_STATE_t new_state);
static void cellTempFastTime(void);

static bool cli_request = false;

static float calculated_temperature;
static float interpolated_temperature;
static const uint64_t SENSOR_TIMEOUT = 5000;

static void temperaturePowerOn(void);
static uint8_t measureTemperature(void);

// é multiplicado por 10
// entao em 25º é 10K
// este termistor trabalha na faixa de 213K ate 470 ohms
static float temp_table[17] = { 80, 75, 70,  65,  60,  55,  50,  45, 40, 
                                35, 30, 25, 20, 15,  10,  5,   0               
                              };

static float m_table[17] =      {1.673, // 80
                                 1.929, // 75
                                 2.232, // 70
                                 2.593, // 65
                                 3.023, // 60
                                 3.539, // 55
                                 4.161, // 50
                                 4.912, // 45
                                 5.825, // 40
                                 6.940, // 35
                                 8.310, // 30
                                 10.000, // 25
                                 12.099, // 20
                                 14.720, // 15
                                 18.015, // 10
                                 22.184, // 5
                                 27.494  // 0
                                };

static const uint8_t    SAMPLE_N = 30;
static uint32_t         sum_adc = 0;
static uint8_t          adc_counter = 0;
static uint16_t         adc_sample;

/**
 * Module's tasks runner
 */
void NTC_TEMPERATURE_run(TASKS_t running_task)
{
    uint8_t ret_code;
    static uint8_t count = 0;
    switch(running_task)
    {
    case POWERON_TASK:
        temperaturePowerOn();
        break;
    case FAST_TIME_TASK:
        cellTempFastTime();
        break;
    case VERY_SLOW_TIME_TASK:
        ret_code = measureTemperature();
        if(ret_code)
        {
            /* Log internal error */
            Serial.println("Temperature error");
        }
        break;
    default:
        /* Log internal error */
        Serial.println("tried to run an unsupported task");
        break;
    }
}

static void cellTempSetState(CELLTEMP_STATE_t new_state)
{
    if (new_state == cell_temp_fsm.state)
    {
        return;
    }

    // Caso do ultimo estado
    if (cell_temp_fsm.state == CELLTEMP_END)
    {
            if(new_state != CELLTEMP_IDLE)
            {
                // Soh vai pra IDLE se estiver em END_STATE
                return;
            }
    } else if (cell_temp_fsm.state == CELLTEMP_CHEK_SETUP)
    {
        if (new_state == CELLTEMP_MEASUREMENT ||
            new_state == CELLTEMP_END ||
            new_state == CELLTEMP_READ_SETUP)
        {
            new_state = CELLTEMP_IDLE;
            Serial.println("[CELLTEMP_FSM] Invalid transition from CHECK state");
        }
    }
    // Demais estados
    else if (new_state != (cell_temp_fsm.state + 1))
    {
        // Demais transicoes devem ser sempre pra frente
        // Transicao invalida
        //  0         1          2          3               4               5          6
        // IDLE -> SETUP -> READ_SETUP -> CHECK_SETUP -> START_MEAS -> MEASUREMENT -> END
        Serial.println("[TEMP_FSM][ERRO] Transicao invalida de estado\n");
        return;
    }

    /* State change and store transition time stamp */
    cell_temp_fsm.state = new_state;
    cell_temp_fsm.transition_time = GetSystemTime();

    /* State specific entry actions */
    switch(cell_temp_fsm.state)
    {
    case CELLTEMP_IDLE:
        if(cli_request)
        {
            cli_request = false;
            Serial.print("  [NTC_TEMPERATURE]");
            Serial.print(interpolated_temperature);
            Serial.println("graus C");
        }
        break;
    case CELLTEMP_SETUP:
        break;

    case CELLTEMP_READ_SETUP:
        break;
    
    case CELLTEMP_CHEK_SETUP:
        
        break;

    case CELLTEMP_START_MEASUREMENT:
        break;

    case CELLTEMP_MEASUREMENT:
        break;
     case CELLTEMP_END:
        adc_counter = 0;
        sum_adc = 0;
        break;
     default:
         /* Log internal error */
         Serial.println("tried to process an unknown driver state");
         break;
    }
}

static void cellTempFastTime(void)
{
    uint16_t spi_timeout = 150;

    switch(cell_temp_fsm.state)
    {
        case CELLTEMP_IDLE:
        if(cli_request)
        {
            measureTemperature();
        }
        break;
        
        case CELLTEMP_SETUP:
        //if(TestTimerExpired(cell_temp_fsm.transition_time, 10))
        //{
            cellTempSetState(CELLTEMP_READ_SETUP);
        //}
        break;

        case CELLTEMP_READ_SETUP:
        //if(TestTimerExpired(cell_temp_fsm.transition_time, 10))
        //{
            cellTempSetState(CELLTEMP_CHEK_SETUP);
        //}
        break;

        case CELLTEMP_CHEK_SETUP:
        cellTempSetState(CELLTEMP_START_MEASUREMENT);
        
        break;

        case CELLTEMP_START_MEASUREMENT:
        //if(TestTimerExpired(cell_temp_fsm.transition_time, 20))
        //{
            cellTempSetState(CELLTEMP_MEASUREMENT);
        //}
        break;

        case CELLTEMP_MEASUREMENT:
            // Le o sensor algumas vezes
            if (adc_counter < SAMPLE_N)
            {
                adc_sample = analogRead(TEMP_GPIO);
                sum_adc += adc_sample;
                adc_counter++;
            } else
            {
                interpolated_temperature = MeasureExtTmp();
                cellTempSetState(CELLTEMP_END);
            }
            break;
        case CELLTEMP_END:
            cellTempSetState(CELLTEMP_IDLE);
            break;
    }
}

CELLTEMP_STATE_t GetCellTempState(void)
{
    return cell_temp_fsm.state;
}

static void temperaturePowerOn(void) {
    Serial.print("Entrada de temperatura instanciada na porta ");
    Serial.println(TEMP_GPIO);
    cell_temp_fsm.state = CELLTEMP_IDLE;
}

static uint8_t measureTemperature(void)
{
    // FsmGetTicket(FSM_TEMPERATURE);
    cellTempSetState(CELLTEMP_SETUP);

    return 0;
}

void SetupCellTemperature(void)
{
    cellTempSetState(CELLTEMP_SETUP);
    cell_temp_fsm.setup_count = 0;
}

float GetTemperature(void)
{
    return interpolated_temperature;
}

uint16_t GetEncodedTemperature(void)
{
    return (interpolated_temperature+200)*100;
}

float MeasureExtTmp(void)
{

    float volt_adc_equivalent;

    //#ifdef CALCULATED_TEMPERATURE
    const uint16_t  b_value = 3950.0; //3435;
    const uint16_t  resistor = 10000;
    const float     t0 = 25.0; //273.0 + 25.0;
    const float     rx = 10000; //resistor * exp(-b_value/t0);
    //#endif

    // Determina a resistência do termistor
    // volt_adc_equivalent = (sum_adc*3.6) / (SAMPLE_N*16384);
    volt_adc_equivalent = (sum_adc*5.0) / (SAMPLE_N*1023.0);
    float mean_adc = sum_adc / SAMPLE_N;

    // Serial.print("Tensao lida no adc para temperatura = ");
    // Serial.println(volt_adc_equivalent);

    //#ifdef CALCULATED_TEMPERATURE
    // Fórmula simplificada utilizando apenas 1 resistor
    float simplified_rt = resistor * (1023.0 / mean_adc - 1.0);

    // equacao beta
    float temp = simplified_rt / resistor;
    temp = log(temp);
    temp /= b_value;
    temp += 1.0/ (t0 + 273.15);
    temp = 1 / temp;
    temp -= 273.15;

    // float resultado_do_log = ( b_value / log(simplified_rt/rx) );
    // Serial.print("Resultado do log: ");
    // Serial.println(resultado_do_log);
     calculated_temperature =  temp;//resultado_do_log - 273;
    // Serial.print("Temperatura calculada   = %.2f graus C\n", calculated_temperature);
    //#endif


    if(volt_adc_equivalent == 0)
    {
        Serial.println("  [MEASURE_EXT_TMP][ERROR] LEU ZERO DO ADC");
    }

    // Determina a resistência do termistor

   // float rt = ( (gopamp * VDD * R1) / divisor) - R1 - 2;

    float rt = (((3*6.8)/volt_adc_equivalent)-6.8);


    // Calcula a temperatura
    float table_input = rt;

    //#ifdef CALCULATED_TEMPERATURE
    return calculated_temperature;
    //#else
    //INDEX_RATIO_t i_ratio = index_ratio(table_input, m_table, 17);
    //return interp_1d(i_ratio, temp_table, 17);
    //#endif
}

void CliTempTest(void)
{
    cli_request = true;
}

/**  @}
 * End of bat_temperature_module group definition
 */
