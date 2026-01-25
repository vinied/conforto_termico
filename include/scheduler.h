#ifndef SCHEDULER_H_
#define SCHEDULER_H_


/**
 * \file scheduler.h
 * @{
 */

/**
 * \addtogroup scheduler_module
 */


void SetTasksFlags(void);
void RunMainLoop(void);
void EnableSystemTasks(void);
void TriggerPowerOnTask(void);


/**  @}
 * End of task scheduler_module group inclusion
 */


#endif /* SCHEDULER_H_ */
