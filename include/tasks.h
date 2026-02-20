#ifndef TASKS_H_
#define TASKS_H_


/**
 * \file tasks.h
 */

/**
 * \addtogroup task_module
 */


/**
 * \brief System tasks
 */
typedef enum Tasks
{
    POWERON_TASK,       //!< POWERON_TASK
    FAST_TIME_TASK,     //!< FAST_TIME_TASK
    MEDIUM_TIME_TASK,   //!< MEDIUM_TIME_TASK
    SLOW_TIME_TASK,     //!< SLOW_TIME_TASK
    VERY_SLOW_TIME_TASK,//!< VERY_SLOW_TIME_TASK
    POWEROFF_TASK       //!< POWEROFF_TASK
} TASKS_t;


void RunFastTimeTask(void);
void RunMediumTimeTask(void);
void RunSlowTimeTask(void);
void RunVerySlowTimeTask(void);
void RunPowerOnTask(void);
void RunPowerOffTask(void);

bool IsSoundAlarm(void);


/**  @}
 * End of task_module group inclusion
 */


#endif /* TASKS_H_ */
