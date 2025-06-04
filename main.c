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
static void GPIO_Config(void);

static void Delay_two_us(uint8_t step_delay);

static uint8_t Get_Distance(void);

typedef enum { WALKING, STOP, TURNING } State;

uint16_t duty = 0;
State current_state = WALKING;
uint8_t step_count = 0, last_step_count = 0;

int main(void)
{
	CLK_Config();
	TIM2_Config();
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
				// current_state = STOP;
			}
			else // Após 1 segundo andando deve girar
			{
				step_count = 0;
				current_state = TURNING;
			}
			break;
		case STOP:
			// Checa sensor e caso não retorne mais nada volta para WALKING
			// step_count = last_step_count;
			break;
		case TURNING:
			if (step_count > 150) // Após 750ms de giro deve voltar a andar
			{
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

static void GPIO_Config(void)
{
	// ------------------ Incializando Motores ---------------------
	
	GPIO_DeInit(MOTOR_PORT);
	
	// Motor Esquerdo
	GPIO_Init(MOTOR_PORT, MOTOR_L1_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(MOTOR_PORT, MOTOR_L2_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	
	GPIO_WriteLow(MOTOR_PORT, MOTOR_L1_PIN);
	GPIO_WriteLow(MOTOR_PORT, MOTOR_L2_PIN);
	
	// Motor Direito
	GPIO_Init(MOTOR_PORT, MOTOR_R1_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(MOTOR_PORT, MOTOR_R2_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	
	GPIO_WriteLow(MOTOR_PORT, MOTOR_R1_PIN);
	GPIO_WriteLow(MOTOR_PORT, MOTOR_R2_PIN);
	
	// -------------------------------------------------------------
	
		
	// ------------------- Inicializando Sensor Ultrassom ----------------------------
	
	GPIO_DeInit(ULTRASONIC_PORT);
	
	GPIO_Init(ULTRASONIC_PORT, ULTRASONIC_TRIGGER_PIN, GPIO_MODE_OUT_PP_LOW_FAST); 
	GPIO_Init(ULTRASONIC_PORT, ULTRASONIC_ECHO_PIN, GPIO_MODE_IN_FL_IT);
	
	// Interrupção do Echo na transição de subida
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOA, EXTI_SENSITIVITY_RISE_ONLY);
	
	// -------------------------------------------------------------------------------
}

// Interrupção a cada 5ms
@far @interrupt void TIM2_IRQHandler(void)
{
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE); // Limpando bit de interrupção
	
	step_count++;
}

@far @interrupt void Echo_IRQHandler(void)
{
	
}

static uint8_t Get_Distance(void)
{
	GPIO_WriteLow(ULTRASONIC_PORT, ULTRASONIC_TRIGGER_PIN);
	
	// Usar assembly para delays de 2us
  
	GPIO_WriteHigh(ULTRASONIC_PORT, ULTRASONIC_TRIGGER_PIN);
  
	// Usar assembly para delays de 10us
  
	GPIO_WriteLow(ULTRASONIC_PORT, ULTRASONIC_TRIGGER_PIN);

  // Medir o tempo do pulso de echo
	
  // Calcula a distância em centímetros
  // distance = (duration / 2.0) * 0.0343;
}