#ifndef __SCHEDULER_FACTORY_H
#define __SCHEDULER_FACTORY_H

#include "kernel/scheduler/scheduler.h"
#include "kernel/scheduler/scheduler_factory.h"

scheduler* scheduler_factory_create_scheduler(const scheduler_attributes* scheduler_attributes_object);

#endif
