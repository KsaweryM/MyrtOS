#ifndef __SCHEDULER_PRIORITY_TIME_SLICING_H
#define __SCHEDULER_PRIORITY_TIME_SLICING_H

#include "kernel/scheduler/scheduler.h"

typedef struct scheduler_with_priority_t scheduler_with_priority_t;

scheduler_with_priority_t* scheduler_with_priority_create();

#endif
