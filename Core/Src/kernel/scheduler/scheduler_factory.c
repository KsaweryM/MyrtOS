#include "kernel/scheduler/scheduler_factory.h"
#include "kernel/scheduler/scheduler_round_robin.h"
#include "kernel/scheduler/scheduler_priority_time_slicing.h"

scheduler* scheduler_factory_create_scheduler(const scheduler_attributes* scheduler_attributes_object)
{
	scheduler* scheduler_object = 0;

	switch (scheduler_attributes_object->algorithm)
	{
	case ROUND_ROBIN_SCHEDULING:
		scheduler_object = scheduler_round_robin_create(scheduler_attributes_object);
		break;
	case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING:
		scheduler_object = scheduler_priority_time_slicing_create(scheduler_attributes_object);
		break;
	case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING:
		break;
	case COOPERATIVE_SCHEDULING:
		break;
	}

	return scheduler_object;
}
