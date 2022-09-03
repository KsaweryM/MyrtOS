#ifndef __SCHEDULER_PRIORITY_TIME_SLICING_H
#define __SCHEDULER_PRIORITY_TIME_SLICING_H

#include "kernel/scheduler/scheduler.h"

typedef struct scheduler_priority_time_slicing_t scheduler_priority_time_slicing_t;

scheduler_priority_time_slicing_t* scheduler_priority_time_slicing_create();

#endif
