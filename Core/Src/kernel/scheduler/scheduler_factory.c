#include "kernel/scheduler/scheduler_factory.h"
#include "kernel/scheduler/scheduler_round_robin.h"

scheduler* scheduler_factory_create_scheduler(const scheduler_attributes* scheduler_attributes_object)
{
	scheduler* scheduler_object = 0;

	switch (scheduler_attributes_object->algorithm)
	{
	case round_robin_scheduling:
		scheduler_object = scheduler_round_robin_create(scheduler_attributes_object);
		break;
	case prioritized_preemptive_scheduling_with_time_slicing:
		break;
	case prioritized_preemptive_scheduling_without_time_slicing:
		break;
	case cooperative_scheduling:
		break;
	}

	return scheduler_object;
}
