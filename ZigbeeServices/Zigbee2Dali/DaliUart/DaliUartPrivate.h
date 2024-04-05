//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliUartPrivate.h
///   \since 2016-10-27
///   \brief Private definitions provided for module DaliUart
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// M a n c h e s t e r - C o n s t a n t s      --------------------------------------------------------------------------------------------------------------


///------------------------------------------------------------------------------------------------------------------------------------------------------------
/// M a n c h e s t e r - D e f i n i t i o n s      ----------------------------------------------------------------------------------------------------------

///<- definitions of the daliTx.sendMode state machine
typedef enum {
  DALI_UART_IDLE             = 0,
  DALI_IDLE_RX,                                                           // set while interframe-time (transmit of command is blocked)
  DALI_RECEIVE,
  DALI_RX_OSRAM,
                                                                                        //DALI_RX_MEASURE_START = 0x04,
                                                                                        //DALI_RX_MEASURE       = 0x05,
  DALI_RX_FRAME_ERROR,
  DALI_IDLE_TX,                                                           // waiting for 1st Tx-edge
  DALI_TRANSMIT,
                                                                                        //DALI_TX_MEASURE_START = 0x40,
                                                                                        //DALI_TX_MEASURE       = 0x50,
                                                                                        //DALI_TX_COLLISION      = 0xF1,    // not supported yet!
  DALI_BUS_FAILURE      = 0xFF
} TypeDaliUartSTATUS;

typedef struct {                                                                //
  uint8_t readIdx;                                                            // index of last received edge
  uint8_t halfBits;                                                                // number of halfbit received
  uint32_t telegram;                                                              // received 24 bit
  uint16_t frameEnd;
  TypeDaliUartReceive receivedFrame;                                          // TypeDaliUartRx defined in DaliUart.h
} TypeDaliUartRx;

typedef enum {                                                                  // status definitions for daliTx.status (state machine)
  DALI_TX_IDLE		       = 0,
  DALI_TX_DATA,
  DALI_TX_MANCHESTER,
} TypeDaliUartTxSTATUS;

typedef struct {                                                                // times are initialized/updated by DaliUart.c, read-only by DaliUart_HAL.c
  TypeDaliUartTxSTATUS status;
  uint32_t sendData;                                                              // bigger than 8 bit to distroy answer frame by sendig 9 bit
  uint32_t sendBit;
  uint32_t sendDelay_us;
} TypeDaliUartTx;

typedef struct {                                                                // times are initialized/updated by DaliUart.c, read-only by DaliUart_HAL.c
  TypeDaliUartSTATUS status;
  uint8_t failureCounter_FTT;                                                      ///< \brief Counter to detect a system failure.
  TypeDaliUartRx rx;
  TypeDaliUartTx tx;
} TypeDaliUart;
//extern TypeDaliUart daliUart;

