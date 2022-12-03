#include <kernel/measure_time.h>
#include <kernel/kernel.h>

TIM_HandleTypeDef htim4;

static void error(void)
{
  __disable_irq();
  while (1)
  {
  }
}

void measure_start(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 1000;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
  	error();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
  	error();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
  	error();
  }

  HAL_TIM_Base_Start(&htim4);
}

uint32_t measure_get(void)
{
	uint32_t value = __HAL_TIM_GET_COUNTER(&htim4);
	return value;
}

uint32_t measure_end(void)
{
	uint32_t value = __HAL_TIM_GET_COUNTER(&htim4);
	HAL_TIM_Base_Stop(&htim4);
	return value;
}
