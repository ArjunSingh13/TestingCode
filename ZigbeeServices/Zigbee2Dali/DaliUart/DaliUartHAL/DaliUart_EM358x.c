//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   DaliUart_EM3588.c
///   \since  2016-12-12
///   \brief  Hardware-dependent part of the DALI-Rx and DALI-Tx functions (InputCapture and OutputCompare)
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "GlobalDefs.h"
#include "DaliHAL.h"
#include "DaliUart_EM358x.h"													                          // includes complete hal-headers

#ifdef DALI_MULTI_MASTER_TEST
  #include "Zigbee2Dali.h"
  #include "Zigbee2DaliPrivate.h"
  extern TypeZigbee2Dali zigbee2Dali;
#endif

#define DALI_TIMER_PERIOD_16BIT     (uint32_t)((0x10000UL*3/4)-1)               // inhibit timer overrun by using this period
#define DALI_SCALE_1us33_TO_1us     (uint32_t)((0x10000UL*4/3)-1)               // 87381UL, scale timer values (count and input capture) to nominal times
#define DALI_SCALE_1us_TO_1us33     DALI_TIMER_PERIOD_16BIT                     // scale nominal times (output compare) to timer values
#define DALI_COUNTER_MAX            (0xFFFF)                                    // inhibit OutputCompare-toggling

///<- data structure for DaliRxTx control
#define MAX_IDX_DaliCapture     63                                              // MAX_IDX_DaliCapture = (2^x -1), with x avoiding buffer overrun!
typedef struct {
	uint8_t captureIdx;
	uint16_t times[MAX_IDX_DaliCapture+1];
	bool levels[MAX_IDX_DaliCapture+1];
} rx_t;

#define MAX_IDX_TX     63                                                       // MAX_IDX_TX = (2^x -1), with x avoiding buffer overrun!
typedef struct {
  uint8_t sendIdx;                                                              //
  uint8_t receiveIdx;                                                           //
  uint16_t pattern[MAX_IDX_TX+1];                                               // buffer for both idleState and activeState
} tx_t;

#ifdef DALI_MULTI_MASTER
#define MAX_IDX_MM  15
typedef struct {
  uint16_t lastTxEdge;                                                          // buffer for both idleState and activeState
  #ifdef DALI_MULTI_MASTER_TEST
  uint16_t detects1;
  uint16_t detects2;
  uint8_t idx;
  uint16_t lastRx;
  uint16_t rx[MAX_IDX_MM];
  uint16_t tx[MAX_IDX_MM];
  uint16_t timer[MAX_IDX_MM];
  #endif
} TypeMultimaster;
#endif

#ifdef UART_TEST
#define CHECK_MaxIDX	31
typedef struct {
	uint16_t rxTimer[MAX_IDX_DaliCapture+1];
  uint16_t txTimer[MAX_IDX_TX+1];
  uint16_t compare[MAX_IDX_TX+1];
  uint16_t checkTimes[4][CHECK_MaxIDX+1];
  uint8_t compareIndex;
} TypeTest;
#endif

typedef struct {
  rx_t rx;
  tx_t tx;
#ifdef DALI_MULTI_MASTER
  TypeMultimaster multimaster;
#endif
#ifdef UART_TEST
  TypeTest test;
#endif
} daliHAL_t;
daliHAL_t daliHAL;

/******************************************************************************/
void DaliHAL_Init(void)
{
#if 0
  DaliTIMER->CCER = _TIM_CCER_RESETVALUE;                                       // disable all channels: some writes are inhibited, when the channel is enables
  DaliTIMER->CR1 = _TIM_CR1_RESETVALUE;                                         // ARR is not buffered, edge-aligned mode, counting up, counter disabled!
  DaliTIMER->CR2 = _TIM_CR2_RESETVALUE;                                         // select TI1 as input, no master mode
  DaliTIMER->SMCR = _TIM_SMCR_RESETVALUE;                                       // no master/slave mode, no external clock, no external trigger
  // initialize registers
  DALI_TIMER_REG = _TIM_CNT_RESETVALUE;                                         // reset counter register DaliTIMER->CNT
  DaliTIMER->PSC = 0x4UL;                                                       // set prescaler to 2^4: 12 MHz/16 = 0,75 MHz (1.33 µs)
  DaliTIMER->ARR = DALI_TIMER_PERIOD_16BIT;                                     // set auto-reload register: scale to 2^16 µs
  DALI_RxCAPTURE_REG = _TIM_CNT_RESETVALUE;                                     // reset input capture registerDaliTIMER->CCR1
  DALI_TxCOMPARE_REG = DALI_COUNTER_MAX;                                        // set OutputCompare value to inhibit compare
  // config input capture and output compare
  DaliTIMER->CCMR1 = DALI_TIMER_CCMR1;
  DaliTIMER->CCMR2 = DALI_TIMER_CCMR2;
  DaliTIMER->CCER = DALI_TIMER_CCER;
  DaliTIMER->OR = DALI_TIMER_OPTIONREG;                                         // map Timer2 pins to PB1 (input capture) resp. PB3 (output compare)
  // event manager
  DaliEVENT->CFG = _EVENT_TIM_CFG_RESETVALUE | DALI_RxIRQ_FLAG;                 // DaliRx-IRQ always enabled, DaliTx only while sending
  DaliEVENT->FLAG = (DALI_TxIRQ_FLAG | DALI_RxIRQ_FLAG);                        // clear all IRQ-flags
  DaliEVENT->MISS = (DALI_TxIRQ_FLAG | DALI_RxIRQ_FLAG);                        // clear all missed-IRQ-flags

  CLEAR_TIM2_IRQ;
  ENABLE_TIM2_IRQ;
  //	DALI_TIMER_START;
#endif
}


///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// \brief                (Re)starts the DaliUart hardware in InputCapture mode.
/// \param                nothing
/// \retval               nothing
/// \calling extern       none
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
uint8_t DaliHAL_Start(void) {
#if 0
  if (IS_DALI_RxLEVEL_HIGH) {
    DALI_RxCAPTURE_FALLING;                                                     // polarity = non-inverted: capture falling edge
    daliHAL.rx.captureIdx = 0;
  }
  else {
    DALI_RxCAPTURE_RISING;                                                      // polarity = non-inverted: capture rising edge
    daliHAL.rx.captureIdx = 1;
  }
	DALI_TIMER_START;
	DaliEVENT->CFG |= DALI_RxIRQ_FLAG;                                            // DaliRx-IRQ always enabled, DaliTx only while sending
#ifdef DALI_MULTI_MASTER
  daliHAL.multimaster.lastTxEdge = DALI_COUNTER_MAX;
#endif

  return daliHAL.rx.captureIdx;
#endif
  return 0;
}

///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// \brief                Stops DaliUart functions and sets the µC-parts used by DaliUart into power saving state.
/// \param                nothing
/// \retval               nothing
/// \calling extern       none
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
void DaliHAL_Stop( void ) {
//  DISABLE_DALI_RxCAPTURE_IRQ;
//  DALI_GPIO_Tx_SET_IDLE;                                                      // avoid power consumption (if DaliTx is active, answer is finalized "internally", because interrupts are not disabled)
}

///--------------------------------------------------------------------------------------------------------------------------------
/// \brief                Handles common IRQs of the DALI_TIMER. All times are calculated to 1 µs resolution.
/// \param                none
/// \retval               none
/// \calling extern       none
//---------------------------------------------------------------------------------------------------------------------------------
//
void DALI_TIMER_ISR(void) {
  #if 0
  // Common Interrupt request
  uint32_t interruptFlags = DaliEVENT->FLAG;                                    // Clear all IRQ not done (interruptFlags &= DaliEVENT->CFG;)
  // Handle Tx-IRQ (Output Compare)
  if (interruptFlags & DALI_TxIRQ_FLAG) {                                       // for multimaster-mode,
    uint8_t sendIdx = daliHAL.tx.sendIdx;
    if (sendIdx == daliHAL.tx.receiveIdx) {                                     // last output edge = rising edge
      DISABLE_DALI_Tx_IRQ;
      DALI_TxCOMPARE_REG = DALI_COUNTER_MAX;                                    // inhibit further edges;
      DALI_Tx_FORCE_IDLE;
#ifdef DALI_MULTI_MASTER
      daliHAL.multimaster.lastTxEdge = DALI_COUNTER_MAX;                        // stop collision detection
#endif
//      daliUart.status = DALI_TX_END;
    }
    else {
      uint16_t nextTimeSpan;
      sendIdx = (sendIdx+1) & MAX_IDX_TX;
      daliHAL.tx.sendIdx = sendIdx;
      nextTimeSpan = daliHAL.tx.pattern[sendIdx];
      nextTimeSpan += DALI_TxCOMPARE_REG;
      if (nextTimeSpan >= DALI_TIMER_PERIOD_16BIT) {                            // Check, if wrapAround is necessary:
        nextTimeSpan -= DALI_TIMER_PERIOD_16BIT;                                // .. then change value for OutputCompare (counter wraps around automatically by setting ARR-register to DALI_TIMER_PERIOD_16BIT)
      }
#ifdef DALI_MULTI_MASTER
      daliHAL.multimaster.lastTxEdge = DALI_TxCOMPARE_REG;                      // save value before update
#endif
      DALI_TxCOMPARE_REG = nextTimeSpan;

#ifdef UART_TEST
      daliHAL.test.txTimer[sendIdx] = DALI_TIMER_REG;
      daliHAL.test.compare[sendIdx] = DALI_TxCOMPARE_REG;
#endif
    }
#ifdef UART_TEST
//  if (compareIndex < CHECK_MaxIDX) {
    daliHAL.test.checkTimes[0][daliHAL.test.compareIndex] = DALI_TIMER_REG;
    daliHAL.test.checkTimes[1][daliHAL.test.compareIndex] = DALI_TxCOMPARE_REG;
    daliHAL.test.checkTimes[2][daliHAL.test.compareIndex] = daliHAL.tx.receiveIdx;
    daliHAL.test.checkTimes[3][daliHAL.test.compareIndex] = sendIdx;
    daliHAL.test.compareIndex++;
    daliHAL.test.compareIndex &= CHECK_MaxIDX;
//  }
#endif
    CLEAR_DALI_Tx_IRQ;
  } // .. handle Tx-IRQ
  // Handle Rx-IRQ (Input Capture)
  if (interruptFlags & DALI_RxIRQ_FLAG) {                                       // Dali-Rx-IRQ (flag cleared, when reading DALI_RxCAPTURE_REG)
    uint16_t rxCapture = DALI_RxCAPTURE_REG;
    static bool disturbance = false;
    uint8_t captureIdx = (daliHAL.rx.captureIdx+1) & MAX_IDX_DaliCapture;
    if ( ((captureIdx & 0x01) && (IS_DALI_RxLEVEL_HIGH == false)) || (((captureIdx & 0x01)==0) && (IS_DALI_RxLEVEL_HIGH)) ) {
      uint32_t calcTime = (DALI_SCALE_1us33_TO_1us * (uint32_t)rxCapture) >>16; // scale 0.75 MHz timer to 1 MHz timer
      daliHAL.rx.times[captureIdx] = (uint16_t)calcTime;                        // Capture also, if overrun: for timeout check.
      daliHAL.rx.captureIdx = captureIdx;
      DALI_RxCAPTURE_TOGGLE;
    }
    else {                                                                      // Rx-IRQ not cleared ..
      if (disturbance == false) {disturbance = true;}                           // .. to force re-entering ISR
      else {
        uint16_t captureTime = DALI_RxCAPTURE_REG;
        disturbance = false;
      }
    }
#ifdef DALI_MULTI_MASTER
    if (daliHAL.multimaster.lastTxEdge != DALI_COUNTER_MAX) {                   // if an internal tx-pattern is running
  #ifdef DALI_MULTI_MASTER_TEST
      daliHAL.multimaster.lastRx = rxCapture;
  #endif
      if ( (captureIdx & 0x01) && ((daliHAL.tx.sendIdx & 0x01)==0) ) {          // if Rx-edge is falling and next Tx-edge is (still!) falling: Rx-edge before Tx-edge, means collision
        uint16_t stopTxTime = DALI_TIMER_REG +10;
        if (stopTxTime >= DALI_TIMER_PERIOD_16BIT) {                            // Check, if wrapAround is necessary:
          stopTxTime -= DALI_TIMER_PERIOD_16BIT;                                // .. then change value for OutputCompare (counter wraps around automatically by setting ARR-register to DALI_TIMER_PERIOD_16BIT)
        }
        // DALI_TxCOMPARE_REG = stopTxTime;                                     // at next IRQ ..
        daliHAL.tx.sendIdx = daliHAL.tx.receiveIdx;                             // .. inhibit further edges
        DALI_Tx_FORCE_IDLE;
  #ifdef DALI_MULTI_MASTER_TEST
        if (daliHAL.multimaster.detects1 < 0xFFFF) {daliHAL.multimaster.detects1++;}
        zigbee2Dali.dali.collision = true;
  #endif
      } // no else: rising edge may not occur, if overwritten by low level from external
    } // no else: no collision possible, if no tx-pattern running
#endif

#ifdef UART_TEST
    //calcTime = (DALI_SCALE_1us33_TO_1us * (uint32_t)DALI_TIMER_REG) >>16;     // scale 0.75 MHz timer to 1 MHz timer
    daliHAL.test.rxTimer[captureIdx] = rxCapture; //calcTime;
#endif
    CLEAR_DALI_Rx_IRQ;
  } // if (interruptFlags & EVENT_TIM_FLAG_CC1IF): Dali-Rx-IRQ
#endif
}

///-----------------------------------------------------------------------------
///-----------------------------------------------------------------------------
/// \brief

//------------------------------------------------------------------------------
//
/*
TypeDaliVOLTAGE DaliHAL_GetVoltage(void) {
  if (IS_DALI_RxLEVEL_HIGH)   {return DALI_HIGH;}
  else                        {return DALI_LOW;}
}
*/
bool DaliHAL_IsVoltageHigh(void) {
#if 0
  if (IS_DALI_RxLEVEL_HIGH)   {return true;}
  else                        {return false;}
#endif
  return false;
}


///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// \brief                Provides the actual time in µs to the hardware independend part.
/// \param                nothing
/// \retval               actual 16 bit time in µs (after scaling the Dali-Timer value from 1.33 µs to 1 µs)
/// \calling extern       none
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
uint16_t DaliHAL_GetTimer_us(void) {
#if 0
  uint32_t calcTimer;
  uint16_t timer = DALI_TIMER_REG;
#ifdef DALI_MULTI_MASTER
  __disable_interrupt();                                                        //  must not be interrupted to get consistent values
  if (daliHAL.multimaster.lastTxEdge != DALI_COUNTER_MAX) {                     // if an internal tx-pattern is running
    uint16_t diffTime = DALI_TIMER_REG - daliHAL.multimaster.lastTxEdge;
    if ( diffTime > 0x8000 ) { diffTime += DALI_TIMER_PERIOD_16BIT;}            // compensate for wrap-around of timer
    if (diffTime > 60 ) {                                                       // time delay of rx-edge to tx-edge < 30 /0.75 µs ..
      if ( ((daliHAL.tx.sendIdx & 0x01) == 0) && (daliHAL.rx.captureIdx & 0x01) ) {  // if next edge is still falling, last tx-edge was rising : Missing Rx-edge "rising" after Tx-rising-edge, means collision
        daliHAL.tx.sendIdx = daliHAL.tx.receiveIdx;                             // .. inhibit further edges
        DALI_Tx_FORCE_IDLE;
  #ifdef DALI_MULTI_MASTER_TEST
        if (daliHAL.multimaster.detects2 < 0xFFFF) {daliHAL.multimaster.detects2++;}
        daliHAL.multimaster.rx[daliHAL.multimaster.idx] = daliHAL.multimaster.lastRx;
        daliHAL.multimaster.tx[daliHAL.multimaster.idx] = daliHAL.multimaster.lastTxEdge;
        daliHAL.multimaster.timer[daliHAL.multimaster.idx] = timer;
        daliHAL.multimaster.idx++;
        daliHAL.multimaster.idx &= MAX_IDX_MM;
        zigbee2Dali.dali.collision = true;
#endif
      }
      else {  // debug
        daliHAL.multimaster.lastTxEdge = DALI_COUNTER_MAX;                      // no collision, disable check to avoid race with next tx-edge:
        daliHAL.multimaster.lastTxEdge++;
      }
    } // no else: rising edge may not occur, if overwritten by low level from external
  } // no else: no collision possible, if no tx-pattern running
  __enable_interrupt();
#endif
  calcTimer = (DALI_SCALE_1us33_TO_1us * (uint32_t)timer) >>16;                 // scale 0.75 MHz timer to 1 MHz timer
  return (uint16_t)calcTimer;
#endif
  return 0;
}

///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// \brief                Provides the captured times to the hardware independent part.
/// \param                readIndex: index to last edge read
///                       *captTime: pointer where to store the captured time of readIndex in µs
///                       *actTime: pointer where to store the actual time in µs
///                       *readIndex: pointer to the index of last edge read (writable for limiting to the legal range)
/// \retval               index of the last edge captured
/// \calling extern       none
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
TypeDaliHalCapture DaliHAL_GetCapture( uint8_t *readIndex ) {
#if 0
  TypeDaliHalCapture capture;
  uint32_t actTimeScaled;

  *readIndex &= MAX_IDX_DaliCapture;                                            // limit read index to valid range
  capture.readTime = daliHAL.rx.times[*readIndex];
  actTimeScaled = DALI_TIMER_REG * DALI_SCALE_1us33_TO_1us;
  capture.actTime = (uint16_t)(actTimeScaled >>16);
  capture.actIdx = daliHAL.rx.captureIdx;
  return capture;
#endif
  TypeDaliHalCapture capture = {0};
  return capture;
}

//------------------------------------------------------------------------------
/// \brief .
///
//------------------------------------------------------------------------------
//

bool DaliHAL_AddTxPattern(uint16_t idleStateTime_us, uint16_t periodTime_us) {
  uint32_t scaledTime;
  uint16_t scaledIdle, scaledActive;
  uint16_t activeStateTime_us;

  activeStateTime_us = periodTime_us - idleStateTime_us;
#if 0
  if ( activeStateTime_us >= 100) {                                             // Interrupt needs time to start and execute, 100 µs should guaranty, that compare value is updated before this value is overrun
    scaledTime = (DALI_SCALE_1us_TO_1us33 * idleStateTime_us) +0x00008000;
    scaledIdle = scaledTime >>16;
    scaledTime = (DALI_SCALE_1us_TO_1us33 * (uint32_t)(activeStateTime_us)) +0x00008000;
    scaledActive = scaledTime >>16;
    if (DALI_TxCOMPARE_REG == DALI_COUNTER_MAX) {                               // Start of a new frame ..
      uint16_t nextTimeSpan;
//      __disable_interrupt();                                                    // update of compare register must not be interrupted to avoid overrun of compare value
      nextTimeSpan = DALI_TIMER_REG + scaledIdle;
      if (nextTimeSpan >= DALI_TIMER_PERIOD_16BIT) {                         		// Check, if wrapAround is necessary, ..
        nextTimeSpan -= DALI_TIMER_PERIOD_16BIT;                              	// .. then change value for OutputCompare (counter wraps around automatically by setting ARR-register to DALI_TIMER_PERIOD_16BIT)
      }
      DALI_TxCOMPARE_REG = nextTimeSpan;
      daliHAL.tx.pattern[1] = scaledActive;
      daliHAL.tx.receiveIdx = 1;                                                // times of activeLevel stored to odd index
      daliHAL.tx.sendIdx = 0;                                                   // actual level = idle
      CLEAR_DALI_Tx_IRQ;
      ENABLE_DALI_Tx_IRQ;
      DALI_Tx_TOGGLE;
//      __enable_interrupt();
#ifdef UART_TEST
      daliHAL.test.compareIndex++;
      daliHAL.test.compareIndex &= CHECK_MaxIDX;
      daliHAL.test.checkTimes[0][0] = DALI_TIMER_REG;
      daliHAL.test.checkTimes[1][0] = DALI_TxCOMPARE_REG;
      daliHAL.test.checkTimes[2][0] = daliHAL.tx.receiveIdx;
      daliHAL.test.checkTimes[3][0] = daliHAL.tx.sendIdx;
#endif
      return true;
    }
    else {                                                                      // .. or continue running frame
      uint8_t indexIdle = (daliHAL.tx.receiveIdx+1) & MAX_IDX_TX;
      if (indexIdle != daliHAL.tx.sendIdx) {
        uint8_t indexActive = (indexIdle+1) & MAX_IDX_TX;
        if (indexActive != daliHAL.tx.sendIdx) {
          if ( idleStateTime_us > 100) {                                        // ignore in other cases
            daliHAL.tx.pattern[indexIdle] = scaledIdle;
            daliHAL.tx.pattern[indexActive] = scaledActive;
            daliHAL.tx.receiveIdx = indexActive;
            return true;
          } // no else: ignore, if pattern cannot be created
        } // no else: ignore, if buffer full when writing idle time
      } // no else: ignore, if buffer full when writing active time
    }
  } // no else: ignore, if pattern cannot be created

#endif
  return false;                                                                 // in all cases, where the buffer is not updated
}


#ifdef HARDWARE_TEST
bool ButtonHal_IsPressed(uint8_t buttonNumber) {
  if (Z2D_BUTTON_PIN) { return true; }
  else                { return false; }
}

void LedHal_Switch (bool switchOn) {
  if (switchOn) { Z2D_LED_ON; }
  else          { Z2D_LED_OFF; }
}
#endif
