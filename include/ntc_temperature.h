#ifndef BAT_TEMPERATURE_H_
#define BAT_TEMPERATURE_H_

/* Cell temperature state enum type */
typedef enum CellTempState
{
    CELLTEMP_IDLE,
    CELLTEMP_SETUP,
    CELLTEMP_READ_SETUP,
    CELLTEMP_CHEK_SETUP,
    CELLTEMP_START_MEASUREMENT,
    CELLTEMP_MEASUREMENT,
    CELLTEMP_END
}  CELLTEMP_STATE_t;

typedef struct CellTempStatus
{
    uint64_t          transition_time;
    CELLTEMP_STATE_t  state;
    uint8_t           setup_count;
} CELLTEMP_STATUS_t;

void CliTempTest(void);

float GetTemperature(void);
// To send via radio
uint16_t GetEncodedTemperature(void);

float MeasureExtTmp(void);

CELLTEMP_STATE_t GetCellTempState(void);
void SetupCellTemperature(void);

void NTC_TEMPERATURE_run(TASKS_t running_task);

#endif /* BAT_TEMPERATURE_H_ */
