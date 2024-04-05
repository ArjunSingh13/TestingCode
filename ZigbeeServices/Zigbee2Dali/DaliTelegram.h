//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliTelegram.h
///   \since 2017-11-17
///   \brief declarations for DALI driver telegram layer
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALI_TELEGRAM_H__
#define __DALI_TELEGRAM_H__

// --------- Includes ----------------------------------------------------------

// --------- Macro Definitions -------------------------------------------------

#define DALI_POWER_ON_DELAY_FTT             (uint32_t)(450/2.5+0.5)             // wait until a Dali-ECG must be ready to receive, before ceating a failure (IEC FDIS 62386-101, 4.11.6 System start-up timing)

#define DALI_SEND_NORMAL                    0x00
#define DALI_SEND_TWICE                     0x01
#define DALI_WAIT4ANSWER                    0x02
#define DALI_SAFE_QUERY                     0x04

#define DALI_SA_BROADCAST                   0xFF

#define DALI_DELAY_200MS                    (uint32_t)(200/2.5)
#define DALI_DELAY_350MS                    (uint32_t)(350/2.5)
#define DALI_DELAY_1000MS                   (uint32_t)(1000/2.5)

#define Z2D_REFRESH_MIN_PERIOD_FTT          (uint32_t)(5000/GLOBAL_FastTIME_TICK_ms)

#define DALI_EDITION1   1
#define DALI_EDITION2   8

#define Z2D_DEFAULT_MIN_LEVEL   85
#define UINT8_MASK      0xFF
#define UINT16_MASK     0xFFFF
#define RANDOM_ADDRESS_MASK  0x00FFFFFF
#define SHORT_ADDRESS_MASK   0xFF

// --------- Type Definitions --------------------------------------------------

typedef union {
  uint32_t raw;
  union {
    struct {
      uint32_t command:8;
      uint32_t noDap:1;
      uint32_t address:6;
      uint32_t multicast:1;
      uint32_t res:16;
    } shortAddressed;
    struct {
      uint32_t command:8;
      uint32_t noDap:1;
      uint32_t groupaddress:4;
      uint32_t addressmode:3;
      uint32_t res:16;
    } groupAddressed;
    struct {
      uint32_t command:8;
      uint32_t noDap:1;
      uint32_t address:7;
      uint32_t res:16;
    } broadcast;
    struct {
      uint32_t data:8;
      uint32_t command:8;
      uint32_t res:16;
    } specialCommand;
  } fwf;
  struct {
    uint32_t answer:8;
    uint32_t res:24;
  } bwf;
  union {
    struct {
      uint32_t command:16;
      uint32_t noDap:1;
      uint32_t address:6;
      uint32_t multicast:1;
      uint32_t res:8;
    } shortAddressed;
    struct {
      uint32_t data:8;
      uint32_t command:16;
      uint32_t res:8;
    } specialCommand;
  } idf;
  struct {
    uint32_t test:17;
    uint32_t res:15;
  } osf;
} TypeDaliFrame;

typedef enum {
  DALI_NO_ANSWER,
  DALI_NO_ANSWER_TIMEOUT,
  DALI_ANSWER_OK,
  DALI_ANSWER_TRASH
} TypeDaliTelegramANSWER;

typedef struct {
  TypeDaliTelegramANSWER answerType;
  uint8_t answer;
} TypeDaliTelegramResponse;

typedef void (*TypeDaliCallback)(TypeDaliTelegramResponse response, TypeDaliFrame command);

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

// ------------ INIT -----------------------------------------------------------
void DaliTelegram_Init( void );
void DaliTelegram_Start( void );
uint32_t DaliTelegram_TimeTick(void);

// ------------ DALI SEND & RECEIVE  -------------------------------------------
bool DaliTelegram_IsDaliBusy(void);
bool DaliTelegram_SendCommandWithCallback(TypeDaliFrame command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendDap2AddressWithCallback(uint8_t shortAddress, uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendDap2Address(uint8_t shortAddress, uint8_t level, uint8_t control, uint32_t delay);
bool DaliTelegram_SendDap2GroupWithCallback(uint8_t groupAddress, uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendDap2Group(uint8_t groupAddress, uint8_t level, uint8_t control, uint32_t delay);
bool DaliTelegram_SendDapBroadcastWithCallback(uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendDapBroadcast(uint8_t level, uint8_t control, uint32_t delay);
bool DaliTelegram_SendStdCommand2AddressWithCallback(uint8_t shortAddress, uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendStdCommand2Address(uint8_t shortAddress, uint8_t command, uint8_t control, uint32_t delay);
bool DaliTelegram_SendStdCommand2GroupWithCallback(uint8_t groupAddress, uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendStdCommand2Group(uint8_t groupAddress, uint8_t command, uint8_t control, uint32_t delay);
bool DaliTelegram_SendStdCommandBroadcastWithCallback(uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendStdCommandBroadcast(uint8_t command, uint8_t control, uint32_t delay);
bool DaliTelegram_SendSpecialCommandWithCallback(uint8_t command, uint8_t data, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_SendSpecialCommand(uint8_t command, uint8_t data, uint8_t control, uint32_t delay);
bool DaliTelegram_Send3ByteCommand2AddressWithCallback(uint8_t shortAddress, uint16_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_Send3ByteCommand2Address(uint8_t shortAddress, uint16_t command, uint8_t control, uint32_t delay);
bool DaliTelegram_Send3ByteCommandBroadcastWithCallback(uint16_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_Send3ByteCommandBroadcast(uint16_t command, uint8_t control, uint32_t delay);
bool DaliTelegram_Send3ByteSpecialCommandWithCallback(uint16_t command, uint8_t data, uint8_t control, uint32_t delay, TypeDaliCallback callback);
bool DaliTelegram_Send3ByteSpecialCommand(uint16_t command, uint8_t data, uint8_t control, uint32_t delay);


// ------------ callbacks from stack to application  -------------------------------------------
void DaliTelegram_InputDeviceFrameReceivedCallback(TypeDaliFrame frame);
void DaliTelegram_BusFailCallback(void);
#endif // __DALI_TELEGRAM_H__
