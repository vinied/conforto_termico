/**
 * \file scheduler.c
 */

/**
 * \defgroup scheduler_module Scheduler
 *
 * \brief Controls system tasks scheduling
 *
 *  The scheduler is implemented by two main functions. One of them - SetTasksFlags() -
 *      runs periodically (at each 1ms) called by SysTick timer interrupt, and uses
 *      several milliseconds counters to trigger a number of predefined periodic
 *      tasks - a task triggering is performed simply by setting a "pending"
 *      task flag, associated to that task.\n
 *  The second function - RunMainLoop() - just runs an endless loop that pools the
 *      "pending" flags set by the first function. When a set "pending" flag is found,
 *      it is cleared, and it's corresponding task function is executed.
 *
 *  * Since the loop pools the higher frequency task flags first, and only one
 *    task function is ran at each loop iteration, the higher frequency task
 *    have priority over the lower frequency ones.
 *  * Tasks are implemented as dedicate functions - named after
 *    "void RunModuleName(void)" pattern.
 *  * Task functions call, one at a time, all modules' "runner" functions,
 *    named after <b>"void MODULENAME_run(TASKS_t task)"</b>, passing the
 *    calling task as argument. Each module's "runner" function will
 *    perform, at module level, actions as required for caller task.
 *  * There are also additional provisions for "missed task" detection and to
 *    measure each task execution time.
 *
 * @{
 */

#include <Arduino.h>

// #include "system.h"

#include "tasks.h"
#include "timer.h"

#include "scheduler.h"
// #include "gpio_access.h"
// #include "panel.h"

/* ******************* ATTENTION: *******************
 * FAST_TIME_TASK_PERIOD   *must* be less than MEDIUM_TIME_TASK_PERIOD
 * MEDIUM_TIME_TASK_PERIOD *must* be less than SLOW_TIME_TASK_PERIOD
 * SLOW_TIME_TASK_PERIOD   *must* be less than VERY_SLOW_TIME_TASK_PERIOD
 */


/** Fast time task period in ms */
#define FAST_TIME_TASK_PERIOD         5
/** Medium time task period in ms */
#define MEDIUM_TIME_TASK_PERIOD      10
/** Slow time task period in ms */
#define SLOW_TIME_TASK_PERIOD       100
/** Very slow time task period in ms */
#define VERY_SLOW_TIME_TASK_PERIOD 1000

/** Pending fast time task flag */
static bool fastTimeTaskPending;
/** Pending medium time task flag */
static bool mediumTimeTaskPending;
/** Pending slow time task flag */
static bool slowTimeTaskPending;
/** Pending very slow time task flag */
static bool verySlowTimeTaskPending;
/** Pending power off event task flag */
static bool powerOffEventTaskPending;

/** Fast time task control counter */
static unsigned int fast_time_task_sched_cntr;
/** Medium time task control counter */
static unsigned int medium_time_task_sched_cntr;
/** Slow time task control counter */
static unsigned int slow_time_task_sched_cntr;
/** Very slow time task control counter */
static unsigned int very_slow_time_task_sched_cntr;

/** Fast time missed task counter */
static unsigned int missed_fast_time_task_cntr;
/** Medium time missed task counter */
static unsigned int missed_medium_time_task_cntr;
/** Slow time missed task counter */
static unsigned int missed_slow_time_task_cntr;
/** Very slow time missed task counter */
static unsigned int missed_very_slow_time_task_cntr;

/** Fast_time task start time, for CPU load monitoring */
static uint64_t fast_time_task_start_time;
/** Medium_time task start time, for CPU load monitoring */
static uint64_t medium_time_task_start_time;
/** Slow_time task start time, for CPU load monitoring */
static uint64_t slow_time_task_start_time;
/** Very slow_time task start time, for CPU load monitoring */
static uint64_t very_slow_time_task_start_time;
/** Power on event task start time, for CPU load monitoring */
static uint64_t power_on_event_task_start_time;
/** Power off event task start time, for CPU load monitoring */
static uint64_t power_off_event_task_start_time;

/** Fast time task run time, in microseconds */
static volatile uint64_t fast_time_task_run_time;
/** Medium time task run time, in microseconds */
static volatile uint64_t medium_time_task_run_time;
/** Slow time task run time, in microseconds */
static volatile uint64_t slow_time_task_run_time;
/** Very slow time task run time, in microseconds */
static volatile uint64_t very_slow_time_task_run_time;
/** Power on event task run time, in microseconds */
static volatile uint64_t power_on_event_task_run_time;
/** Power off event task run time, in microseconds */
static volatile uint64_t power_off_event_task_run_time;

/** Global tasks enable flag */
static bool tasks_enabled;

/*******************************/
/* Local function declarations */
/*******************************/
static void setFastTimeTaskFlag(void);
static void setMediumTimeTaskFlag(void);
static void setSlowTimeTaskFlag(void);
static void setVerySlowTimeTaskFlag(void);

/****************************/
/* Functions implementation */
/****************************/

/**
 * \brief Triggers time based tasks
 *
 * SetTasksFlags() is called, periodically (at each 1ms), by the SysTick time
 *     IRQ Handler function. It controls all system tasks execution timing.
 * * Increments one milliseconds counter for each time based, periodic task;\n
 * * Sets "pending" task flags when corresponding milliseconds counters reach
 *     their corresponding task period threshold value;\n
 * * Increments "missed" task counters whenever a task counter reaches its
 *     activation threshold and the corresponding "pending" flag is still set
 *     (meaning that said task was not ran, since its last triggering time);\n
 * Since this function is called at each 1ms, it must be kept as lean as possible.
 *    This implementation uses about 20ticks, i.e., ~ 0.25% of CPU time,
 *    including running the caller function "SysTick_Handler", which is
 *    the 1ms timer Interrupt Handler itself.
 */
void SetTasksFlags(void)
{
	if (!tasks_enabled)
    {
        return;
    }

    /* Update fast time task control counter */
    fast_time_task_sched_cntr++;
    /* Fast time task control: is it already time to run? */
    if (fast_time_task_sched_cntr >= FAST_TIME_TASK_PERIOD)
    {
        setFastTimeTaskFlag();
    }
}

static void setFastTimeTaskFlag(void)
{
    if (fastTimeTaskPending)
    {
        /* Really bad: fast time task was not ran since last time it was flagged as pending - increment it's "missed task" counter */
        missed_fast_time_task_cntr++;
    }
    else
    {
        /* Everything seems to be fine, flag fast time task to be ran, asap */
        fastTimeTaskPending = true;
    }

    /* Update medium time task control counter */
    medium_time_task_sched_cntr = medium_time_task_sched_cntr + fast_time_task_sched_cntr;
    /* Medium time task control: is it already time to run? */
    if (medium_time_task_sched_cntr >= MEDIUM_TIME_TASK_PERIOD)
    {
        setMediumTimeTaskFlag();
    }
    fast_time_task_sched_cntr = 0;
}

static void setMediumTimeTaskFlag(void)
{
    if (mediumTimeTaskPending)
    {
        /* Really bad: medium time task was not ran since last time it was flagged as pending - increment it's "missed task" counter */
        missed_medium_time_task_cntr++;
    }
    else
    {
        /* Everything seems to be fine, flag medium time task to be ran, asap */
        mediumTimeTaskPending = true;
    }

    /* Update slow time task control counter */
    slow_time_task_sched_cntr = slow_time_task_sched_cntr + medium_time_task_sched_cntr;
    /* Slow time task control: is it already time to run? */
    if (slow_time_task_sched_cntr >= SLOW_TIME_TASK_PERIOD)
    {
        setSlowTimeTaskFlag();
    }
    medium_time_task_sched_cntr = 0;
}

static void setSlowTimeTaskFlag(void)
{
    if (slowTimeTaskPending)
    {
        /* Really bad: slow time task was not ran since last time it was flagged as pending - increment it's "missed task" counter */
        missed_slow_time_task_cntr++;
    }
    else
    {
        /* Everything seems to be fine, flag slow time task to be ran, asap */
        slowTimeTaskPending = true;
    }

    /* Update very slow time task control counter */
    very_slow_time_task_sched_cntr = very_slow_time_task_sched_cntr + slow_time_task_sched_cntr;
    /* Very slow time task control: is it already time to run? */
    if (very_slow_time_task_sched_cntr >= VERY_SLOW_TIME_TASK_PERIOD)
    {
        setVerySlowTimeTaskFlag();
    }
    slow_time_task_sched_cntr = 0;
}

static void setVerySlowTimeTaskFlag(void)
{
    if (verySlowTimeTaskPending)
    {
        /* Really bad: very slow time task was not ran since last time it was flagged as pending - increment it's "missed task" counter */
        missed_very_slow_time_task_cntr++;
    }
    else
    {
        /* Everything seems to be fine, flag very slow time task to be ran, asap */
        verySlowTimeTaskPending = true;
    }
    very_slow_time_task_sched_cntr = 0;
}

/**
 * \brief Runs main scheduler loop.
 *
 * The **scheduler main loop** is an endless loop that monitors **pending task** flags
 *     to decide if it is time to run any of the predefined system tasks.
 *     Whenever a set flag is found, the flag is cleared, the corresponding task
 *     function is called, and the loop proceeds to its next iteration;\n
 * Since higher frequency tasks flags are pooled first and only one task is
 *     ran per loop iteration, higher frequency task are, naturally, prioritized
 *     over the lower frequencies ones;\n
 * Additionally, the loop code registers the execution time of the tasks it
 *     calls, for processor load estimation.
 */
void RunMainLoop(void)
{
	
    if (fastTimeTaskPending)
    {
        // fast_time_task_start_time = GetHiResSystemTime();
        fastTimeTaskPending = false;
        RunFastTimeTask();
        // fast_time_task_run_time = GetHiResElapsedTime(fast_time_task_start_time);
    }
    else if (mediumTimeTaskPending)
    {
        // medium_time_task_start_time = GetHiResSystemTime();
        mediumTimeTaskPending = false;
        RunMediumTimeTask();
        // medium_time_task_run_time = GetHiResElapsedTime(medium_time_task_start_time);
    }
    else if (slowTimeTaskPending)
    {
        // slow_time_task_start_time = GetHiResSystemTime();
        slowTimeTaskPending = false;
        RunSlowTimeTask();
        // slow_time_task_run_time = GetHiResElapsedTime(slow_time_task_start_time);
    }
    else if (verySlowTimeTaskPending)
    {
        // very_slow_time_task_start_time = GetHiResSystemTime();
        verySlowTimeTaskPending = false;
        RunVerySlowTimeTask();
        // very_slow_time_task_run_time = GetHiResElapsedTime(very_slow_time_task_start_time);
    }
    /* else if (powerOffEventTaskPending)
    {
        power_off_event_task_start_time = GetHiResSystemTime();
        powerOffEventTaskPending = false;
        RunPowerOffTask();
        power_off_event_task_run_time = GetHiResElapsedTime(power_off_event_task_start_time);
    } */
}

/**
 * \brief Enables all time based tasks
 *
 * Clears all task activation and missed task counters, as well as
 *     all pending task flags, and enables all time based, periodic
 *     tasks triggering.
 */
void EnableSystemTasks(void)
{
	if (false == tasks_enabled)
	{
		/* Clear pending flags */
		fastTimeTaskPending = false;
		mediumTimeTaskPending = false;
		slowTimeTaskPending = false;
		verySlowTimeTaskPending = false;
		powerOffEventTaskPending = false;

		/* Clear time tasks activation counters */
		fast_time_task_sched_cntr = 0;
		medium_time_task_sched_cntr = 0;
		slow_time_task_sched_cntr = 0;
		very_slow_time_task_sched_cntr = 0;

		/* Clear missed time tasks counters */
		missed_fast_time_task_cntr = 0;
		missed_medium_time_task_cntr = 0;
		missed_slow_time_task_cntr = 0;
		missed_very_slow_time_task_cntr = 0;

		/* Enable tasks */
		tasks_enabled = true;
	}
}

/**
 * \brief Triggers the Power On task
 *
 * Activates power on task - it should run just once per power on - power off cycle
 */
void TriggerPowerOnTask(void)
{
    // power_on_event_task_start_time = GetHiResSystemTime();
    RunPowerOnTask();
    // power_on_event_task_run_time = GetHiResElapsedTime(power_on_event_task_start_time);
}

/**  @}
 * End of task scheduler_module group definition
 */
