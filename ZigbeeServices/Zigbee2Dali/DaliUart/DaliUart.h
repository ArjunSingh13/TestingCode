//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   DaliUart.h
///   \since  2016-10-27
///   \brief  Interface for module DaliUart
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//                                      M o d u l e   D e f i n i t i o n s
//
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//   C o m p i l e r   s w i t c h e s
//------------------------------------------------------------------------------------------------------------------------------------------------------------
#define DALI_READ_BACK                                                          ///< \brief if defined, frames are read back while sending

//------------------------------------------------------------------------------------------------------------------------------------------------------------
//   D a l i U a r t - I n t e r f a c e:   C o n s t a n t s
//------------------------------------------------------------------------------------------------------------------------------------------------------------

#define DALI_Te_us            		(uint16_t)(417)                             ///< \brief length of half-bit in µs (Dali baudrate is fixed to 1200 Baud).
#define DALI_TxBIT_ANSWER         0x80                                          ///< \brief defines the number of bits in Tx-answer frame (excluding start bit)
#define DALI_TxBIT_COMMAND        0x8000                                        ///< \brief defines the number of bits in Tx-command frame (excluding start bit)
#define DALI_RxBIT_OSRAM          17                                            ///< \brief number of bits in OSRAM-Rx-frame (excluding start bit)

#define DALI_Te_MIN_us            (uint16_t)(DALI_Te_us/2)                      ///< \brief minimum length of pulse or pause to be treated as 1Te
#define DALI_BIT_MIN_us           (uint16_t)(583)                               ///< \brief minimum length of pulse or pause to be treated as 2Te: middle of grey area (500 us - 666 us)
#define DALI_BIT_MAX_us           (uint16_t)(1100)                              ///< \brief maximum length of pulse or pause to be accepted as 2Te: middle of grey area (1000 us - 1200 us)

#define DALI_STOPBIT_MIN_us       (uint16_t)(2000)                              ///< \brief Nominal 4*Te=1668, IEC62386-101, Ed. 2, 8.2.1: 1400 < stop < 2400
#define DALI_OSRAM_STOPBIT_MIN_us  (uint16_t)(1765)                             ///< \brief Receiver timing: 4,23*Te
#define DALI_OSRAM_STOPBIT_MAX_us  (uint16_t)(6.5*DALI_Te_us)                   ///< \brief Receiver timing: 5,75*Te = 2400 acc. to spec., but SCAMPI applies ca. 2500!

#define DALI_TX_TIMEOUT_us        (uint16_t)(13000)                             ///< \brief Timeout, if AnswerFrame is not started/finalized
#define DALI_FAILURE_START_us     (uint16_t)(42500)                             ///< \brief according to IEC62386-101, Ed. 2, 3.8 + 4.11.1
#define DALI_FAILURE_TIME_FTT     (uint8_t)(((500.0-42.5)/GLOBAL_FastTIME_TICK_ms)+1)  // SystemFailure-Time = 500 ms

#define DALI_ANSWER_DELAY_us      (uint16_t)(DALI_Te_us*15)                     ///< \brief answer delay of ballast to command: 15 Te (SCAMPI needs >=13 Te)
#define DALI_COMMAND_DELAY_us     (uint16_t)(14000)                             ///< \brief settling time between any frame and a command frame (>= 13500 us)
#define DALI_DELAY_us           (uint16_t)(14000)

//------------------------------------------------------------------------------------------------------------------------------------------------------------
// D a l i U a r t - I n t e r f a c e:   F u n c t i o n s
//------------------------------------------------------------------------------------------------------------------------------------------------------------

void DaliUart_Init( void );
void DaliUart_Start( uint8_t failureTime_FTT );                                 // called after init or in case of power recovery (all channels affected)
void DaliUart_Stop( void );                                                     // called in case of power down (all channels affected)

uint8_t DaliUart_GetTimeTicks( void );

typedef enum {
  DALI_BUS_OK = 0,
  DALI_BUS_DOWN,                                                                // level is active for longer than DALI_FAILURE_START_us
  DALI_BUS_FAIL,                                                                // level is active for longer than DALI_FAILURE_TIME_FTT
} TypeDaliUartBusSTATUS;
TypeDaliUartBusSTATUS DaliUart_FastTimer( void );

// Rx
// --------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct {
  uint8_t length;                                                               // bits received. In case of a special frame (e.g. Osram-frame, 0x80 is added to the length)
  uint32_t data;
} TypeDaliUartReceive;
TypeDaliUartReceive DaliUart_Receive( void );
bool DaliUart_RxUpdate( void );

// Tx
// -------------------------------------------------------------------------------------------------------------------------------------------------------
void DaliUart_SendFrame( uint32_t sendData, uint32_t sendLength, uint32_t sendDelay_us );  // universal send command
bool DaliUart_SendCommand( uint16_t sendData );                                 // 16-bit manchester frame
void DaliUart_SendAnswer( uint8_t data);
bool DaliUart_Send3Byte( uint32_t sendData );                                   // Address DaliInput device
#ifdef DALI_TEST
void DaliUart_SendBit( void );                                                  // TestFrame
#endif
