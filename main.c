#include <stm8s.h>
#include <interruptions.h>

#define MOTOR_PORT GPIOD

#define MOTOR_L1_PIN GPIO_PIN_5
#define MOTOR_L2_PIN GPIO_PIN_6

#define MOTOR_R1_PIN GPIO_PIN_2
#define MOTOR_R2_PIN GPIO_PIN_1

#define ULTRASONIC_PORT GPIOA

#define ULTRASONIC_TRIGGER_PIN GPIO_PIN_1
#define ULTRASONIC_ECHO_PIN GPIO_PIN_2

static void CLK_Config(void);
static void TIM2_Config(void);
static void TIM4_Config(void);
static void GPIO_Config(void);

static void Delay_two_us(uint8_t step_delay);

static uint8_t Has_Object(void);

typedef enum { WALKING, STOP, TURNING } State;

uint16_t duty = 0;
State current_state = WALKING;
uint8_t step_count = 0, last_step_count = 0;
volatile uint8_t echo_received = 0, timeout = 0;

int main(void)
{
	CLK_Config();
	TIM2_Config();
	TIM4_Config();
	GPIO_Config();
	
	enableInterrupts();
	
	while (1)
	{
		switch (current_state)
		{
		case WALKING:
			if (step_count < 200) // Passa 1 segundo andando
			{
				// Checa sensor de proximidade
				if (Has_Object() != 0)
				{
					GPIO_WriteLow(MOTOR_PORT, MOTOR_L1_PIN);
					GPIO_WriteLow(MOTOR_PORT, MOTOR_R1_PIN);
					
					current_state = STOP;
					last_step_count = step_count;
				}
			}
			else // Após 1 segundo andando deve girar
			{
				GPIO_WriteLow(MOTOR_PORT, MOTOR_L1_PIN);
				GPIO_WriteHigh(MOTOR_PORT, MOTOR_L2_PIN);
				
				step_count = 0;
				current_state = TURNING;
			}
			break;
		case STOP:
			// Checa sensor e caso não retorne mais nada volta para WALKING
			if (Has_Object() == 0)
			{
				GPIO_WriteHigh(MOTOR_PORT, MOTOR_L1_PIN);
				GPIO_WriteHigh(MOTOR_PORT, MOTOR_R1_PIN);
				
				step_count = last_step_count;
				current_state = WALKING;
			}
			break;
		case TURNING:
			if (step_count > 150) // Após 750ms de giro deve voltar a andar
			{
				GPIO_WriteHigh(MOTOR_PORT, MOTOR_L1_PIN);
				GPIO_WriteLow(MOTOR_PORT, MOTOR_L2_PIN);
				
				step_count = 0;
				current_state = WALKING;
			}
			break;
		}
	}
	
	return 0;
}

static void CLK_Config(void)
{
	CLK_DeInit();
	
	CLK_HSECmd(DISABLE); // Desativa o HSE (clock externo)
	CLK_LSICmd(DISABLE); // Desativa o LSI (clock de baixa velocidade interno)
	
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV8);
	CLK_HSICmd(ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE); // Habilita o clock para o TIM2
}

static void TIM2_Config(void)
{
	TIM2_DeInit();
	
	TIM2_TimeBaseInit(TIM2_PRESCALER_1, 999); // (2 MHz / (999 + 1)) = 2 kHz -> 100% duty = 999
	TIM2_OC2Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 250, TIM2_OCPOLARITY_HIGH); // Inicia PWM no canal 2
	TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 250, TIM2_OCPOLARITY_HIGH); // Inicia PWM no canal 1
	
	TIM2_ARRPreloadConfig(ENABLE); // Habilita o Pré-carregamento do registrator ARR (auto-reload)
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
	TIM2_Cmd(ENABLE);
}

static void TIM4_Config(void)
{
	TIM4_DeInit();
	
	TIM4_TimeBaseInit(TIM4_PRESCALER_128, 9);
	TIM4_ITConfig(TIM4_IT_UPDATE, DISABLE); // Não habilita interrupção do timer 4 ainda
	
	TIM4_Cmd(DISABLE); // Não habilita o timer 4 ainda
}

static void GPIO_Config(void)
{
	// ------------------ Incializando Motores ---------------------
	
	GPIO_DeInit(MOTOR_PORT);
	
	// Motor Esquerdo
	GPIO_Init(MOTOR_PORT, MOTOR_L1_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(MOTOR_PORT, MOTOR_L2_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	
	GPIO_WriteHigh(MOTOR_PORT, MOTOR_L1_PIN);
	GPIO_WriteLow(MOTOR_PORT, MOTOR_L2_PIN);
	
	// Motor Direito
	GPIO_Init(MOTOR_PORT, MOTOR_R1_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(MOTOR_PORT, MOTOR_R2_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	
	GPIO_WriteHigh(MOTOR_PORT, MOTOR_R1_PIN);
	GPIO_WriteLow(MOTOR_PORT, MOTOR_R2_PIN);
	
	// -------------------------------------------------------------
	
		
	// ------------------- Inicializando Sensor Ultrassom ----------------------------
	
	GPIO_DeInit(ULTRASONIC_PORT);
	
	GPIO_Init(ULTRASONIC_PORT, ULTRASONIC_TRIGGER_PIN, GPIO_MODE_OUT_PP_LOW_FAST); 
	GPIO_Init(ULTRASONIC_PORT, ULTRASONIC_ECHO_PIN, GPIO_MODE_IN_FL_IT);
	
	// Interrupção do Echo na transição de subida
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOA, EXTI_SENSITIVITY_FALL_ONLY);
	ULTRASONIC_PORT->CR2 &= (uint8_t)(~0x01); // Desabilita a interrupção do echo
	
	// -------------------------------------------------------------------------------
}

// Interrupção a cada 5ms
@far @interrupt void TIM2_IRQHandler(void)
{
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE); // Limpando bit de interrupção
	
	step_count++;
}

// Interrupção a cada 640us
@far @interrupt void TIM4_IRQHandler(void)
{
	TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
	
	timeout = 1;
}

@far @interrupt void Echo_IRQHandler(void)
{
	echo_received = 1;
}

static uint8_t Has_Object(void)
{
#asm
	bres _PA_ODR, #0 // Reseta pino de trigger, Write Low
	
	// Delay 2us
	nop
	nop
	nop
	ld a, #3 // 1 ciclo, coloca o valor 3 para o delay debaixo
	
	bset _PA_ODR, #0 // Seta pino de trigger, Write High
	
	// Delay 10us
loop:
	dec a // 1 ciclo
	jrne loop // 2 ciclos
	
	bres _PA_ODR, #0 // Reseta pino de trigger, Write Low
	
	bset _PA_CR2, #0 // Habilita interrupção do echo
	bset _TIM4_IER, #0 // Habilita a interrupção do timer 4
	bset _TIM4_CR1, #0 // Habilita o timer

echo_loop:
	wfi // Espera por interrupção
	tnz _echo_received // Checa se echo_received == 0
	jrne echo_loop_exit_true // Se Z != 0 pula para saida
	tnz _timeout // Checa se timeout == 0
	jrne echo_loop_exit_false // Se Z != 0 pula para saida
	jp echo_loop // Volta para o loop
	
	echo_loop_exit_true:
	ld a, #1 // a = 1
	jp exit
	
	echo_loop_exit_false:
	clr a // a = 0
	
	exit:
	bres _PA_CR2, #0 // Desabilita interrupção do echo
	bres _TIM4_IER, #0 // Desabilita interrupção do timer 4
	bres _TIM4_CR1, #0 // Desabilita o timer
	clr _TIM4_CNTR // Limpa contador do timer
	clr _echo_received // Limpa echo
	clr _timeout // Limpa timeout
	ret // return a
#endasm
}