#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

/* Incluir o header da série e da sua linha específica */
#include "stm8s103f3p.h"

/* Incluir os headers dos periféricos que você vai usar */
#include "stm8s_clk.h"
#include "stm8s_tim4.h"
#include "stm8s_tim2.h"
#include "stm8s_gpio.h"
#include "stm8s_exti.h"
// Adicione outros conforme necessário: uart, i2c, adc, etc.

/* Ativa o assert_param (opcional, útil para debug) */
// #define USE_FULL_ASSERT    (descomente para ativar as verificações em tempo de execução)

/* Macro de assert (ativa se USE_FULL_ASSERT estiver definida) */
#ifdef USE_FULL_ASSERT
#include <stdio.h>
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif /* __STM8S_CONF_H */
