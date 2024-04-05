/**************************************************************************//**
 * @file
 * @brief
 * @version 0.0
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2016
 *******************************************************************************
*/
#include PLATFORM_HEADER

#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
#include "em3587.h"
#elif defined(SILABS_DEMOKIT)
#include "em3588.h"
#elif defined(SAMSUNG_DEMOKIT)
#include "em355.h"
#else
  #warning "Unknown Device"
#endif

#define DaliTIMER           	TIM2                                            //
#define DaliEVENT               EVENT_TIM2

#define DALI_TIMER_ISR			halTimer2Isr
#define DALI_TIMER_START   				DaliTIMER->CR1 |= TIM_CR1_TIM_CEN;

#define DALI_TIMER_REG          (DaliTIMER->CNT)

#if defined(ZIGBEE2DALI_V1)
  #define DALI_RxPIN				        (GPIO->P[1].IN & 0x10UL)               s                // PB4
  #define DALI_RxCAPTURE_REG        (DaliTIMER->CCR4)
  #define DALI_RxCAPTURE_RISING 	  DaliTIMER->CCER &= ~TIM_CCER_TIM_CC4P;
  #define DALI_RxCAPTURE_FALLING 	  DaliTIMER->CCER |= TIM_CCER_TIM_CC4P;
  #define DALI_RxCAPTURE_TOGGLE 	  DaliTIMER->CCER ^= TIM_CCER_TIM_CC4P;
  #define DALI_RxIRQ_FLAG           EVENT_TIM_FLAG_CC4IF
  #define IS_DALI_RxCAPTURE_FALLING (DaliTIMER->CCER & TIM_CCER_TIM_CC4P)
  #define IS_DALI_RxLEVEL_HIGH      (GPIO->P[1].IN & 0x10UL)
  #define DALI_TIMER_OPTIONREG      TIM_OR_TIM_REMAPC3 | TIM_OR_TIM_REMAPC4
  #define DALI_TIMER_CCMR1          _TIM_CCMR1_RESETVALUE
  #define DALI_TIMER_CCMR2          (_TIM_CCMR2_RESETVALUE | (0x0UL << _TIM_CCMR2_TIM_OC3M_SHIFT) | (0x3UL << _TIM_CCMR2_TIM_IC4F_SHIFT) | TIM_CCMR2_TIM_CC4S)
  #define DALI_TIMER_CCER           (_TIM_CCER_RESETVALUE | TIM_CCER_TIM_CC4E | TIM_CCER_TIM_CC3E)
#else // Normally there should be a warning if no matching device is selected...
  #define DALI_RxPIN				(GPIO->P[1].IN & 0x2UL)                               // PB1
  #define DALI_RxCAPTURE_REG        (DaliTIMER->CCR1)
  #define DALI_RxCAPTURE_RISING 	DaliTIMER->CCER &= ~TIM_CCER_TIM_CC1P;
  #define DALI_RxCAPTURE_FALLING 	DaliTIMER->CCER |= TIM_CCER_TIM_CC1P;
  #define DALI_RxCAPTURE_TOGGLE 	DaliTIMER->CCER ^= TIM_CCER_TIM_CC1P;
  #define DALI_RxIRQ_FLAG           EVENT_TIM_FLAG_CC1IF
  #define IS_DALI_RxCAPTURE_FALLING (DaliTIMER->CCER & TIM_CCER_TIM_CC1P)
  #define IS_DALI_RxLEVEL_HIGH      (GPIO->P[1].IN & 0x2UL)
  #define DALI_TIMER_OPTIONREG      TIM_OR_TIM_REMAPC3 | TIM_OR_TIM_REMAPC1
  #define DALI_TIMER_CCMR1          (_TIM_CCMR1_RESETVALUE | (0x3UL << _TIM_CCMR1_TIM_IC1F_SHIFT) | TIM_CCMR1_TIM_CC1S) 
  #define DALI_TIMER_CCMR2          (_TIM_CCMR2_RESETVALUE | (0x8UL << _TIM_CCMR2_TIM_OC3M_SHIFT)) 
  #define DALI_TIMER_CCER           (_TIM_CCER_RESETVALUE | TIM_CCER_TIM_CC1E | TIM_CCER_TIM_CC3E)
#endif


#define DALI_TxPIN				    (GPIO->P[1].IN & 0x8UL)                         // PB3
#define DALI_TxCOMPARE_REG          (DaliTIMER->CCR3)
#define DALI_Tx_FORCE_IDLE			DaliTIMER->CCMR2 &= ~(_TIM_CCMR2_TIM_OC3M_MASK); DaliTIMER->CCMR2 |= (0x4UL << _TIM_CCMR2_TIM_OC3M_SHIFT);
#define DALI_Tx_TOGGLE				DaliTIMER->CCMR2 &= ~(_TIM_CCMR2_TIM_OC3M_MASK); DaliTIMER->CCMR2 |= (0x3UL << _TIM_CCMR2_TIM_OC3M_SHIFT);
#define DALI_TxIRQ_FLAG             EVENT_TIM_FLAG_CC3IF
#define ENABLE_DALI_Tx_IRQ			DaliEVENT->CFG |= DALI_TxIRQ_FLAG;
#define DISABLE_DALI_Tx_IRQ			DaliEVENT->CFG &= ~DALI_TxIRQ_FLAG;

#define CLEAR_DALI_Tx_IRQ		    DaliEVENT->FLAG = DALI_TxIRQ_FLAG;	// writing 1 clears the flag
#define CLEAR_DALI_Rx_IRQ		    DaliEVENT->FLAG = DALI_RxIRQ_FLAG;	// writing 1 clears the flag

#define INT_CFGSET                  *((volatile uint32_t *)0xE000E100u)
#define INT_TIM2_MASK               (0x00000002u)
#define ENABLE_TIM2_IRQ				INT_CFGSET = INT_TIM2_MASK;
#define INT_PENDCLR                 *((volatile uint32_t *)0xE000E280u)
#define CLEAR_TIM2_IRQ					INT_PENDCLR = INT_TIM2_MASK;
//**********************************************************************************************************************************************************

#define Z2D_BUTTON_PIN    (GPIO->P[1].IN & 0x02UL)          // PB2
#define Z2D_LED_PIN       (GPIO->P[1].IN & 0x01UL)          // PB1

#define Z2D_LED_ON        (GPIO->P[1].IN |= 0x01UL);
#define Z2D_LED_OFF       (GPIO->P[1].IN &= (~0x01UL);
