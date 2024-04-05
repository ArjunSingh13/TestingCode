/******************** (C) COPYRIGHT 2012 - 2014 OSRAM AG *********************************************************************************
* File Name         : DaliUart.c
* Description       : Hardware-independent part of the DaliUart for DALI-Rx and DALI-Tx functions.
*                     Non-volatile memory: none
*                     Calling extern: none
* Author(s)         : Jutta Avril (JA), DS D LMS-COM DE-2
* Created           : 14.05.2012 (JA)
* Modified by       : DATE       (AUTHOR)    DESCRIPTION OF CHANGE
*                   :
**********************************************************************************************************************************/

#include "GlobalDefs.h"

#include "DaliUart.h"                                                         // public interface to DALI module (including definition of Uart-properties)
#include "DaliUartPrivate.h"                                                  // private definitions of the DALI module
#include "DaliHAL.h"                                                      // interface to hardware-dependent DALI module file

#if !(defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #include "DaliCommands.h"                                                       // list of Dali commands available
  uint8_t mockAnswer = 0;
  bool newMockAnswer = false;
  uint8_t mockLevel = 0xFE;
#endif

TypeDaliUart daliUart;                                                      ///< \brief
TypeDaliUart daliUartDefault = {.status = DALI_UART_IDLE, .tx.status = DALI_TX_IDLE}; // all others are init to 0

/* not used yet:
#define DALI_TE_LENGTH_ERROR          0x01
#define DALI_START_ERROR              0x04
#define DALI_PATTERN_MIN_ERROR        0x08
#define DALI_PATTERN_MAX_ERROR        0x10
#define DALI_PATTERN_MANCH_ERROR      0x20
#define DALI_LENGTH_ERROR             0x80 */


//===========================================================================================================================
void DaliUartRxFrameStart (void);
void DaliUartRxFrameEnd (uint16_t frameEndTime);

void DaliUartCalcTxPattern(void);

///--------------------------------------------------------------------------------------------------------------------------------
/// \brief Initialises variables of the daliUart decoder, calling DaliUartHAL_Init() to initialize the hardware.
///
/// calling extern       none
///
/// \retval               nothing
//---------------------------------------------------------------------------------------------------------------------------------
void DaliUart_Init( void )
{
  DaliHAL_Init();
  daliUart = daliUartDefault;
}

void DaliUart_Start( uint8_t failureTime_FTT ) {
  daliUart.rx.readIdx = DaliHAL_Start();
  if ( (daliUart.rx.readIdx & 0x01) == 0) {
    daliUart.status = DALI_UART_IDLE;
    daliUart.failureCounter_FTT = 0;
  }
  else {
    if (daliUart.failureCounter_FTT == 0) {
      daliUart.status = DALI_BUS_FAILURE;
      daliUart.failureCounter_FTT = failureTime_FTT;
    } // no else: leave failureCounter unchanged, if already running while short power interruption
  }
}


void DaliUart_Stop( void ) {
  DaliHAL_Stop();
}


uint8_t DaliUart_GetTimeTicks( void ) {
  uint16_t tempTime_us, diffTime;
  uint16_t tick_us = (uint16_t)(GLOBAL_FastTIME_TICK_ms*1000);
  static uint16_t lastTickTime_us = 0;                                            // time of last time tick
  uint8_t ticks = 0;                                                        //
  tempTime_us = DaliHAL_GetTimer_us();
  diffTime = tempTime_us - lastTickTime_us;

  while (diffTime >= tick_us ) {
    lastTickTime_us += tick_us ;                                                // set time of next tick
    diffTime -= tick_us;
    if (ticks < 255) {ticks++;}
  }
  return ticks;
}

//---------------------------------------------------------------------------------------------------------------------------
/// \brief Function must be called each action timer interval of 2.5 ms
///
/// Detects a missing of the DALI voltages and counts the lengths. Any receive single (in the interrupt) resets the counter.
///
/// \par If the maximal length reached, a DALI receive message created:
/// - command->type = conManchesterTypeFailure;
/// - daliUart.receiveOk = true;
///
/// \return      nothing
//---------------------------------------------------------------------------------------------------------------------------
//
TypeDaliUartBusSTATUS DaliUart_FastTimer( void )
{
  TypeDaliUartBusSTATUS busFailure = DALI_BUS_OK;                               // set default

  if (daliUart.status == DALI_BUS_FAILURE) {
    busFailure = DALI_BUS_DOWN;                                                 // set as new default
    if (daliUart.failureCounter_FTT > 0 ) {
      daliUart.failureCounter_FTT--;
      if (daliUart.failureCounter_FTT == 0 ) {
        busFailure = DALI_BUS_FAIL;
      }
    } // no else
  }
  else {
    if ( daliUart.tx.sendDelay_us > 0 ) {
      if (daliUart.tx.sendDelay_us >= (2*GLOBAL_FastTIME_TICK_ms*1000) ) {
        daliUart.tx.sendDelay_us -= (GLOBAL_FastTIME_TICK_ms*1000);             // to guaranty daliUart.tx.sendDelay_us >= DALI_Te_us µs at next call
      }
      else {
        DaliUart_SendFrame( daliUart.tx.sendData, daliUart.tx.sendBit, daliUart.tx.sendDelay_us );  // DaliBallast device and DaliOsram response
        daliUart.tx.sendDelay_us = 0;
      }
    }
  }
  return busFailure;
}

TypeDaliUartReceive DaliUart_Receive( void )
{
  TypeDaliUartReceive newTelegram;
  newTelegram = daliUart.rx.receivedFrame;
  if (  newTelegram.length == 0 ) {
#if !(defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
    if (newMockAnswer) {
      newTelegram.length = 8;
      newTelegram.data = mockAnswer;
      newMockAnswer = false;
    }
#else
  do {
                                                          // no frame evaluation in calling function, therefore time to continue edge-evaluation
  }
  while (DaliUart_RxUpdate());
#endif
  }
  else {
    daliUart.rx.receivedFrame.length = 0;                                       // clear after handling
  }
  return newTelegram;
}

///--------------------------------------------------------------------------------------------------------------------------------
/// \brief Evaluates Rx-edges, indicates status of receipt. Legal telegramm stored to daliUart.
///
///
/// \param daliCommand    Pointer to the data structure to fill with the received data. Left aligned
///
/// \return      Returns the type of received frame, if new data avariable. Returns DALI_ReceiveTYPE_NOTHING else.
//---------------------------------------------------------------------------------------------------------------------------------
//
bool DaliUart_RxUpdate( void )
{
  bool ret = false;
  uint8_t captIdx = daliUart.rx.readIdx;
  TypeDaliHalCapture capture = DaliHAL_GetCapture( &captIdx );

//------------------------- DaliUart frame evaluation --------------------------------------------------------------------------------------
  if (captIdx == capture.actIdx) {                                              // if no new edge ..
    bool fallingEdge = ((captIdx & 0x01) == 1);                                 // falling edge is captured on odd index, rising edge on even index
    uint16_t diffTime = capture.actTime - capture.readTime;                     // calculate time since last edge for timeout measurement
    if ( fallingEdge ) {                                                        // in any state ..
      if ( daliUart.status != DALI_BUS_FAILURE) {                               // .. exept DALI_BUS_FAILURE, check ..
        if ( diffTime > DALI_FAILURE_START_us ) {                               // .. "bus power down" according to edition 2, 101, 3.8 + 4.11.1 (longer than stopbits of OSRAM-frame!)
          daliUart.status = DALI_BUS_FAILURE;                                   // .. flag error status
          daliUart.failureCounter_FTT = DALI_FAILURE_TIME_FTT;                  // .. and start timer
        }
      }
    }
    else {      // only rising edges are handled here
      switch( daliUart.status ) {                                               //  .. check if end-of-frame
      case DALI_TRANSMIT:                                                       // answer started .. (#ifndef DALI_READ_BACK, daliUart.rx.halfBits stay 0)
      case DALI_RECEIVE:                                                        ///-> valid DaliUart telegram has been received
        if ( diffTime > DALI_STOPBIT_MIN_us ) {                                 //  .. after time of Stopbits ..
          DaliUartRxFrameEnd(capture.readTime);                                 // .. store properties of receveived frame ..
          daliUart.status = DALI_IDLE_RX;                                       // .. re-enable receipt
        } // no else: wait for timeout
        break;
      case DALI_RX_OSRAM:                                                       ///-> valid Osram-frame has been received (length was already checked)
        if ( diffTime > (uint16_t)(2*DALI_Te_us) ) {                              //  .. after time of 2Te to be on the save side
          daliUart.rx.receivedFrame.data = daliUart.rx.telegram;
          daliUart.rx.receivedFrame.length = 0x80 + DALI_RxBIT_OSRAM;           // 0x80 is used to indicate, that new OSRAM command is available
          daliUart.status = DALI_UART_IDLE;
        } // no else: wait for timeout
        break;
      case DALI_RX_FRAME_ERROR:
        if ( diffTime > DALI_STOPBIT_MIN_us ) {                                 //  .. after time of Stopbits ..
          daliUart.rx.receivedFrame.length = 0xFF;                                // 0xFF is used to indicate an invalid Frame (DALI YES)
          daliUart.rx.receivedFrame.data = 0xFF;
          daliUart.status = DALI_IDLE_RX;
        }
        break;
      case DALI_UART_IDLE:                                                      // nothing to do. Wait for next edge
        break;
      case DALI_IDLE_RX:                                                        // Wait for minimum interframe time elapsed
        diffTime = capture.actTime - daliUart.rx.frameEnd;                      // use daliUart.rx.frameEnd as reference !?
        if ( diffTime > DALI_COMMAND_DELAY_us ) {
          daliUart.status = DALI_UART_IDLE;
        }
        break;
      case DALI_IDLE_TX:                                                        // If Tx-frame cannot be read back ..
        if ( daliUart.tx.sendDelay_us == 0) {
          if ( diffTime > (4* GLOBAL_FastTIME_TICK_ms*1000) ) {                 // .. reset after timeout
            daliUart.status = DALI_UART_IDLE;
          }
        }
        break;
      default:                                                                  // DALI_BUS_FAILURE, DALI_RX_FRAME_ERROR, DALI_IDLE_TX
        if ( diffTime > DALI_STOPBIT_MIN_us ) {                           // .. ignore disturbancies, then re-enable receipt
          daliUart.status = DALI_UART_IDLE;
          daliUart.failureCounter_FTT = 0;
        }
        break;
      } // end of switch( daliUart.status ). No default, default only if edge.
    } // else to if ( fallingEdge )
  } // if (captIdx == actIdx)
  //------------------------- DaliUart edge evaluation ----------------------------------------------------------------------------------
  else {                                                                        // else, i.e. if new edge
    ret = true;
    uint8_t actIdx = capture.actIdx;
    bool fallingEdge;
    uint16_t diffTime;
    do {  // while actIdx != captIdx
      diffTime = - capture.readTime;
      captIdx++;
      capture = DaliHAL_GetCapture( &captIdx );
      diffTime += capture.readTime;
      fallingEdge = captIdx & 0x01;                                             // falling edge is captured on odd index, rising edge on even index

      switch( daliUart.status ) {
      case DALI_IDLE_RX:                                                  // check start of frame
      case DALI_UART_IDLE:                                                     // check start of frame
        DaliUartRxFrameStart();                                               // reset receive variables
        daliUart.status = DALI_RECEIVE;
        break;
      case DALI_IDLE_TX:
        DaliUartRxFrameStart();                                               // reset receive variables upon 1st tx-edge ..
        daliUart.status = DALI_TRANSMIT;                                // .. and idicate, that transmit has started
        break;
      case DALI_TRANSMIT:
        if ( daliUart.tx.sendDelay_us == 0 ) {
          DaliUartCalcTxPattern();
        }
#ifndef  DALI_READ_BACK
        break;                                                                  // fall-through in case of DALI_READ_BACK
#endif
      case DALI_RECEIVE:                                                  ///-> DaliUart telegram evaluation
        if ( fallingEdge && (diffTime >= DALI_STOPBIT_MIN_us) ) {         // End of Dali-frame + frame-restart? ..
          uint8_t frameEndIdx = captIdx -1;
          capture = DaliHAL_GetCapture( &frameEndIdx );
          DaliUartRxFrameEnd(capture.readTime);                               // .. store properties of receveived frame ..
          DaliUartRxFrameStart();                                             // .. reset receive variables (stay in DALI_RECEIVE or DALI_RX_MEASURE)
#ifdef  DALI_READ_BACK
          daliUart.status = DALI_RECEIVE;
#endif
        }
        else if ( (!fallingEdge) && (diffTime >= DALI_OSRAM_STOPBIT_MIN_us)) {       // End of Osram-frame or failure start? ..
          if (diffTime < DALI_OSRAM_STOPBIT_MAX_us) {                                // .. check if end of Osram-frame
            if ( ( (daliUart.rx.halfBits == (2*DALI_RxBIT_OSRAM+1)) && (diffTime > (5.5*DALI_Te_us)) )
              || ( (daliUart.rx.halfBits == (2*DALI_RxBIT_OSRAM+2)) && (diffTime > DALI_OSRAM_STOPBIT_MIN_us)&&(diffTime < (5.5*DALI_Te_us)) )  ) {
              daliUart.status = DALI_RX_OSRAM;
              daliUart.rx.frameEnd = capture.readTime;
            }
            else {
              daliUart.status = DALI_RX_FRAME_ERROR;
            }
          }
          else if ( diffTime > DALI_FAILURE_START_us ) {                        // .. "bus power down" according to edition 2, 101, 3.8 + 4.11.1 (longer than stopbits of OSRAM-frame!)
            daliUart.status = DALI_BUS_FAILURE;                                 //    .. flag error status XXX: Bus fehler bereits behoben!!
            daliUart.failureCounter_FTT = DALI_FAILURE_TIME_FTT;                //    .. and start timer
          }
          else {                                                                // .. wrong length of pause
              daliUart.status = DALI_RX_FRAME_ERROR;
          }
        } // if/else if: check of end-of-frame conditions
        else {                                                                  // Continue frame evaluation
          if (diffTime < DALI_Te_MIN_us) {                                // timeDiff < 0.5 half-bit: not accepted
            daliUart.status = DALI_RX_FRAME_ERROR;
          }
          else if (diffTime < DALI_BIT_MIN_us) {                          // 0.5 bit < timeDiff < 1.5 bit: data or daliUart bit
            daliUart.rx.halfBits++;
          }
          else if (diffTime < DALI_BIT_MAX_us) {                             // 1.5 bit < timeDiff < 2.5 bit: data and daliUart bit
            daliUart.rx.halfBits +=2;
            if ((daliUart.rx.halfBits & 0x01) == 0) {                         // error: odd number = daliUart bit + no edge at previous half-bit = data edge, ..
              daliUart.status = DALI_RX_FRAME_ERROR;
            }
          }
          else {
            daliUart.status = DALI_RX_FRAME_ERROR;
          } // else to if+else if ( diffTime ..): evaluate pattern

          if ( daliUart.rx.halfBits & 0x01 ) {                                // evaluate data (data edge at each odd halfbit count)
            daliUart.rx.telegram <<=1;
            if ( (captIdx & 0x01) == 0) {                                       // if rising edge: data=1
              daliUart.rx.telegram +=1;
            }
          }
        } // else to frame end conditions: continue frame evaluation
        break;
      case DALI_RX_OSRAM:                                                       // always falling edge: either error or start of next frame
        if ( diffTime < DALI_STOPBIT_MIN_us ) {                                 // frame not terminated correctly: error
          daliUart.status = DALI_RX_FRAME_ERROR;
        }
        else {                                                                  // frame terminated correctly:
          DaliUartRxFrameEnd(capture.readTime);                               // store properties of receveived frame
          daliUart.rx.receivedFrame.length |= 0x80;                           // 0x80 used to indicate, that special frame (OSRAM command) is available
          DaliUartRxFrameStart();                                             // reset receive variables
          daliUart.status = DALI_RECEIVE;
        }
        break;
      case DALI_BUS_FAILURE:
        if ( fallingEdge == false) {                                            // if new edge is rising ..
          daliUart.status = DALI_RX_FRAME_ERROR;
          daliUart.failureCounter_FTT = 0;                                    // .. leave error-state
        }
        break;
      default:                                                                  // DALI_RX_FRAME_ERROR
        if ( fallingEdge ) {                                                    // if new edge is falling ..
          if ( diffTime >= DALI_STOPBIT_MIN_us ) {                        // .. check, if length of idle state is long enough ..
            DaliUartRxFrameStart();                                           // .. to start new frame
            daliUart.status = DALI_RECEIVE;
          }
        }
        break;
      } // end of switch( daliUart.status ), including default.
    } while (captIdx != actIdx);
    //------------------------- DaliUart telegram evaluation -------------------
    daliUart.rx.readIdx = captIdx;
  }  // else to if (captIdx == actIdx)
  return ret;
}



//---------------------------------------------------------------------------------------------------------------------------
/// \brief    Starts any DALI frame with delay
/// \param    sendData: byte to be replied
/// \return   none.
//---------------------------------------------------------------------------------------------------------------------------
//
void DaliUart_SendFrame( uint32_t sendData, uint32_t sendLength, uint32_t sendDelay_us )  // DaliSlave device and DaliOsram response
{
  daliUart.tx.sendData = sendData;
  daliUart.tx.sendBit = sendLength;
  daliUart.status = DALI_IDLE_TX;
  daliUart.tx.status = DALI_TX_DATA;                                            // TX_DATA = bit start, TX_DALI = middle of bit
  if ( sendDelay_us < (2*GLOBAL_FastTIME_TICK_ms*1000) ) {
    if ( sendDelay_us < DALI_Te_us ) { sendDelay_us = DALI_Te_us; }
    DaliHAL_AddTxPattern(sendDelay_us, sendDelay_us+DALI_Te_us);                // 1st pattern: no check of return value necessary
    daliUart.tx.sendDelay_us = 0;
    DaliUartCalcTxPattern();
  }
  else {                                                                        // if sendDelay > timer resolution: handle sendDelay_us in DaliUart_FastTimer()
    daliUart.tx.sendDelay_us = sendDelay_us;
  }
}

//---------------------------------------------------------------------------------------------------------------------------
/// \brief    Starts the answer byte to a DALI query with fixed delay to the last edge of the query
/// \param    sendData: byte to be replied
/// \return   none.
//---------------------------------------------------------------------------------------------------------------------------
//
void DaliUart_SendAnswer( uint8_t sendData )                                  // DaliBallast device and DaliOsram response
{
  uint16_t elapsedTime = DaliHAL_GetTimer_us() - daliUart.rx.frameEnd;        //
  uint32_t sendDelay = DALI_ANSWER_DELAY_us - (uint32_t)(elapsedTime);

  if (daliUart.status != DALI_IDLE_TX) {                                // send also in case receive running
    DaliUart_SendFrame( sendData, 0x80, sendDelay );
  }
  else {
    if (daliUart.tx.sendData != sendData) {
      if (daliUart.tx.sendBit == 0) {                                         // destroy frame (send >= 9 bit), used at next call of DaliHAL_TxPattern()
        daliUart.tx.sendBit = 1;
        daliUart.tx.status = DALI_TX_DATA;                              // status = DALI_TX_IDLE, if  sendBit = 0
      }
      else {
        daliUart.tx.sendBit <<= 1;
      }
    } // no else needed
  }
}


bool DaliUart_SendCommand( uint16_t sendData ) {                                // DaliControl device
  bool commandSent = false;
  if ( daliUart.status == DALI_UART_IDLE ){                                  // wait until running frame (including inter-frame time) is finished (and DALI voltage applied)..
    DaliUart_SendFrame( sendData, 0x8000, 0 );                                // .. without additional delay
    commandSent = true;
  }
  return commandSent;
}

bool DaliUart_Send3Byte( uint32_t sendData ) {                                  // Address DaliInput device
  bool commandSent = false;
  if ( daliUart.status == DALI_UART_IDLE ){                                     // wait until running frame (including inter-frame time) is finished (and DALI voltage applied)..
    DaliUart_SendFrame( sendData, 0x800000, 0 );                                // .. without additional delay
    commandSent = true;
  }
  return commandSent;
}



#ifdef DALI_TEST
void DaliUart_SendBit( void ) {                                               // TestFrame
  if (daliUart.status == DALI_IDLE) {
    daliUart.status = DALI_TRANSMIT;
    DaliUartHAL_AddTxPattern(DALI_Te_us,2*DALI_Te_us);
    daliUart.tx.sendBit = 0x0000;
  }
}
#endif

//---------------------------------------------------------------------------------------------------------------------------
/// \brief Private functions called from DaliUart_Receive()

void DaliUartRxFrameStart (void) {
  daliUart.rx.halfBits = 0;
  daliUart.rx.telegram = 0;
}

void DaliUartRxFrameEnd (uint16_t frameEndTime) {
  uint32_t length, mask;
  daliUart.rx.frameEnd = frameEndTime;                                        // needed for answer-frame, if any
  if ( ((daliUart.rx.halfBits & 0x01) !=0) && (((uint8_t)(daliUart.rx.telegram) & 0x01) != 0)) {
    daliUart.rx.halfBits++;                                                   // last half-bit was already high: not counted yet
  }
  length = (daliUart.rx.halfBits>>1) -1;
  mask = 1 << length;
  daliUart.rx.receivedFrame.data = daliUart.rx.telegram ^ mask;             // recevied frame: right aligned (without startbit)
  daliUart.rx.receivedFrame.length = length;
}


//---------------------------------------------------------------------------------------------------------------------------
/// \brief    Creates PWM-patterns according to the answer frame (sendData) until HAL-buffer is full or send-frame finished
/// \param    none
/// \return   none.
//---------------------------------------------------------------------------------------------------------------------------
//
void DaliUartCalcTxPattern(void) {
  TypeDaliUartTxSTATUS status = daliUart.tx.status;
  uint32_t sendBit = daliUart.tx.sendBit;
  uint32_t sendData = daliUart.tx.sendData;
  uint16_t idleState_us =0, period_us=0;
  bool bufferFull = false;
  while ( (bufferFull == false) && (status != DALI_TX_IDLE) ) {
    switch( status ) {
    default:                                                                    //
      return;
    case DALI_TX_DATA:
      if ( (sendData & sendBit) == 0) {
        sendBit >>=1;
        if ( (sendData & sendBit) == 0) {                                       // both, if sendDataBit = 0 and sendBit = 0
          if (sendBit == 0) { status = DALI_TX_IDLE; }                          // transmit finished
          else              { status = DALI_TX_MANCHESTER; }                    // next state is daliUart
          sendBit >>=1;
          period_us = (uint16_t)(3*DALI_Te_us);
        }
        else { // 2nd bit = 1
          sendBit >>=1;
          if (sendBit == 0) { status = DALI_TX_IDLE; }                          // transmit finished
          period_us = (uint16_t)(4*DALI_Te_us);
        }
        idleState_us = 2*DALI_Te_us;                                      // if bit=0 after bit=1: pulse of 2Te
      }
      else { // 1st bit = 1
        period_us = 2*DALI_Te_us;
        idleState_us = DALI_Te_us;                                        // if bit=1 after bit=1: pulse of 1Te
        sendBit >>=1;
        if (sendBit == 0) { status = DALI_TX_IDLE; }                      // transmit finished
      }
      break;
    case DALI_TX_MANCHESTER:
      if ( (sendData & sendBit) == 0) {
        if (sendBit == 0) { status = DALI_TX_IDLE; }                      // transmit finished
        else { sendBit >>=1; }
        period_us = (uint16_t)(2*DALI_Te_us);                             // both, if sendDataBit = 0 and sendBit = 0
      }
      else {
        sendBit >>=1;
        if (sendBit == 0) { status = DALI_TX_IDLE; }         	                  // transmit finished
        else              { status = DALI_TX_DATA; }								            // change state
        period_us = (uint16_t)(3*DALI_Te_us);
      }
      idleState_us = DALI_Te_us;
      break;
    } // switch
    if ( DaliHAL_AddTxPattern(idleState_us, period_us) ) {
      daliUart.tx.sendBit = sendBit;
      daliUart.tx.status = status;
    }
    else {
      bufferFull = true;
    }
  } // end while
}




