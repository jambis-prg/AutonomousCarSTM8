#ifndef __INTERRUPTIONS_H
#define __INTERRUPTIONS_H

@far @interrupt void TIM1_IRQHandler(void);
@far @interrupt void TIM2_IRQHandler(void);

@far @interrupt void Echo_IRQHandler(void);

#endif