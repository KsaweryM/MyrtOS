#ifndef __SCHEDULER_ROUND_ROBIN_H
#define __SCHEDULER_ROUND_ROBIN_H

#include "kernel/scheduler/scheduler.h"

typedef struct scheduler_without_priority_t scheduler_without_priority_t;

scheduler_without_priority_t* scheduler_without_priority_create();

#endif
