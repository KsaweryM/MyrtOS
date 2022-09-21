#ifndef __MUTEX_WITH_PRIORITY_INHERITANCE_H
#define __MUTEX_WITH_PRIORITY_INHERITANCE_H

#include <kernel/mutex/mutex.h>
#include <kernel/scheduler/scheduler.h>

typedef struct mutex_with_priority_inheritance_t mutex_with_priority_inheritance_t;

mutex_with_priority_inheritance_t* mutex_with_priority_inheritance_create(scheduler_t* scheduler);

#endif
