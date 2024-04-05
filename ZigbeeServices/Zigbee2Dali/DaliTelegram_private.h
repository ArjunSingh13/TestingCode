//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliTelegram_private.h
///   \since 201-11-17
///   \brief private declarations for DALI driver telegram layer
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALI_TELEGRAM_PRIVATE_H__
#define __DALI_TELEGRAM_PRIVATE_H__

// --------- Macro Definitions -------------------------------------------------

#ifndef MAX_COMMAND_IDX                                                         // needed only for test purpose-includes
#define MAX_COMMAND_IDX    31                                                   // max index to the circular command buffer. Used as mask to limit the access index to valid range
#endif // MAX_COMMAND_IDX

#define DALI_TIME_BEFORE_FORWARD_FTT        (uint32_t)((15+17*0.833)/2.5+0.5)   // command delay after previous frame (> 13,5 ms, s. 8.1.2, table 17 of FDIS - iec62386-101.pdf)
#define DALI_TIMEOUT_FORWARD_BACKWORD_FTT   (uint32_t)((15.5+9*0.833)/2.5+10.5)  // timeout-time when waiting for an answer (including frame length. S. 8.1.2, table 17 of FDIS - iec62386-101.pdf)
#define DALI_SETTLING_BACKWORD_FORWARD_FTT  (uint32_t)((15)/2.5+0.5)            // command delay after previous frame (> 13,5 ms, s. 8.1.2, table 17 of FDIS - iec62386-101.pdf)

#define DALI_NO_ANSWER_RETRIES              2                                   // 1 query with 2 retries => 3 times send

// defines for network-leave pattern
#define Z2D_BUS_SPAN_MAX_FTT                (uint32_t)(1000/GLOBAL_FastTIME_TICK_ms)
#define Z2D_BUS_SPAN_MIN_FTT                (uint32_t)(200/GLOBAL_FastTIME_TICK_ms)
#define Z2D_BUS_TIMEOUT_FTT                 (uint32_t)(3000/GLOBAL_FastTIME_TICK_ms)
#define Z2D_BUS_DOWN_CYCLES                 4                                           // number of down-cycles to command network leave

#define Z2D_INPUT_DEVICE_FRAME_LENGTH   24                                      // definition of all frames ..
#define Z2D_COMMAND_FRAME_LENGTH        16
#define Z2D_ANSWER_FRAME_LENGTH         8
#define Z2D_OSRAM_FRAME_LENGTH          (0x80+17)                               // .. handled by Z2D
#define Z2D_ANSWER_TRASH                0xFF


#endif // __DALI_TELEGRAM_PRIVATE_H__
