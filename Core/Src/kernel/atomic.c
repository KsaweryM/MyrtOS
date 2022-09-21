#include <kernel/atomic.h>
#include <assert.h>

volatile int32_t critical_path_depth = 0;

void YIELD()
{
		assert(!critical_path_depth);

		__disable_irq();
		SysTick->VAL = 0;

		INTCTRL = PENDSTET;
		__enable_irq();

		while (INTCTRL & PENDSTET);
}

void critical_path_depth_test(void)
{
	assert(critical_path_depth == 0);
}
