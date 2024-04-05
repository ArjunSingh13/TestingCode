//------------------------------------------------------------------------------
///   \file DaliHAL.h
///   \since 2016-09-21
///   \brief Interface to the µC-dependent code of the Manchester Uart
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//------------------------------------------------------------------------------

void DaliHAL_Init(void);                                                  ///< \brief Initialises µC-registers and variables. Returns the status at Dali-terminals. Called from DaliUartInit( void )
uint8_t DaliHAL_Start( void );                                            ///< \brief Starts the Dali-Timer, (Re)starts InputCapture
void DaliHAL_Stop( void );                                                ///< \brief Stops interface functions and sets the µC into power saving state

bool DaliHAL_IsVoltageHigh(void);

uint16_t DaliHAL_GetTimer_us(void);                                             ///< \brief returns the actual Timer in µs

typedef struct {
  uint16_t readTime;
  uint16_t actTime;
  uint8_t actIdx;
} TypeDaliHalCapture;

TypeDaliHalCapture DaliHAL_GetCapture( uint8_t *readIndex );        ///< \brief Read-function to forwards captured times to daliUart. returns the actual captureIndex (last bit=0 means risingEdge, i.e. level is high)

bool DaliHAL_AddTxPattern(uint16_t idleStateTime_us, uint16_t periodTime_us);             ///< \brief Write-function to command outputCompare/PWM from daliUart.

#ifdef HARDWARE_TEST
  bool ButtonHal_IsPressed(uint8_t buttonNumber);
  void LedHal_Switch (bool switchOn);
#endif
