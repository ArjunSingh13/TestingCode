//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file MemoryBank.c
///   \since 2018-07-10
///   \brief Services for MemoryBank Access
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \defgroup DALIdevice DALI Membank Access
/// \ingroup Zigbee2DALI
/// @{
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// --------- Includes ----------------------------------------------------------

#include "GlobalDefs.h"
#include "app/framework/include/af.h"
#include "DaliCommands.h"
#include "DaliTelegram.h"
#include "Zigbee2Dali.h"
#include "daliConversions.h"
//#include "MemoryBank.h"
#include "MemoryBank_Private.h"
#include "ZigbeeHaLightEndDevice_own.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF, __VA_ARGS__);}

// --------- Type Definitions --------------------------------------------------

// --------- Local Variables ---------------------------------------------------

// --------- Global Variables --------------------------------------------------

TypeMemoryBankAccess memBankAccess = { .state = MEMORYBANK_IDLE };

// --------- Local Function Prototypes -----------------------------------------
static EmberStatus MemoryBankReadMemoryBank(TypeDaliDeviceType deviceType, uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size);

// --------- Local Function ----------------------------------------------------

static EmberStatus MemoryBankReadMemoryBank(TypeDaliDeviceType deviceType, uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size) {
  if(memBankAccess.state != MEMORYBANK_IDLE) return-1;
  memBankAccess.address = shortAddress;
  memBankAccess.deviceType = deviceType;
  memBankAccess.bank = bank;
  memBankAccess.startLocation = start;
  memBankAccess.size = size;
  memBankAccess.state = MEMORYBANK_READ_START;
  return 0;
}

static void MemoryBankResponseCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  switch(memBankAccess.state) {
    case MEMORYBANK_READ_BLOCK_WAIT4READLOCATIONRESPONSE:
      if (response.answerType == DALI_ANSWER_OK) {
        memBankAccess.data[memBankAccess.index++] = response.answer;
      }
      else {
        memBankAccess.data[memBankAccess.index++] = 0;
      }
      memBankAccess.readBlockCount++;
      if (  (memBankAccess.readBlockCount < MEMBANK_BLOCKREAD_SIZE)  // block readed
          &&(memBankAccess.index < memBankAccess.size)) {            // read request done
        memBankAccess.state = MEMORYBANK_READ_BLOCK_READLOCATION;
      }
      else {
        memBankAccess.state = MEMORYBANK_READ_BLOCK_FINISHED_CHECKLOCATION;
      }
      break;
    case MEMORYBANK_READ_BLOCK_FINISHED_WAIT4CHECKLOCATIONRESPONSE:
      if(  (response.answerType != DALI_ANSWER_OK)
         ||(  (response.answerType == DALI_ANSWER_OK)
            &&(response.answer != (memBankAccess.blockStartLocation + memBankAccess.readBlockCount)))) {
        memBankAccess.faultCount++;
        if (memBankAccess.faultCount < MEMBANK_MAX_FAULT_RETRIES) {
          memBankAccess.state = MEMORYBANK_READ_BLOCK_START;
        }
        else {
          memBankAccess.state = MEMORYBANK_READ_FAILED;
        }
      }
      else {
        memBankAccess.blockStartLocation +=  memBankAccess.readBlockCount;
        memBankAccess.state = MEMORYBANK_READ_BLOCK_FINISHED;
      }
      break;
    default:
      break;
  }
}

// --------- Global Function ----------------------------------------------------

EmberStatus MemoryBank_DriverReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size) {
  return MemoryBankReadMemoryBank(DALI_DRIVER, shortAddress, bank, start, size);
}

EmberStatus MemoryBank_InputDeviceReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size) {
  return MemoryBankReadMemoryBank(DALI_INPUT_DEVICE, shortAddress, bank, start, size);
}

EmberStatus MemoryBank_DaliInputDeviceReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size) {
  return MemoryBankReadMemoryBank(DALI_CONTROL_DEVICE, shortAddress, bank, start, size);
}

bool MemoryBank_ReadRequestDone(void) {
  return ((memBankAccess.state ==  MEMORYBANK_READ_FINISHED) || (memBankAccess.state ==  MEMORYBANK_READ_FAILED));
}

void MemoryBank_FreeHandler(void) {
  memBankAccess.state = MEMORYBANK_IDLE;
}

uint8_t *MemoryBank_GetBuffer(void){
  return memBankAccess.data;
}

void MemoryBank_Tick(void) {
  switch (memBankAccess.state) {
    case MEMORYBANK_READ_BLOCK_WAIT4READLOCATIONRESPONSE:
    case MEMORYBANK_IDLE:
    case MEMORYBANK_READ_FINISHED:
    case MEMORYBANK_READ_FAILED:
      break;
    case MEMORYBANK_READ_START:
      memBankAccess.blockStartLocation = memBankAccess.startLocation;
      memBankAccess.state = MEMORYBANK_READ_BLOCK_START;
      break;
    case MEMORYBANK_READ_BLOCK_START:
         if(memBankAccess.deviceType == DALI_DRIVER) {
           DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, memBankAccess.blockStartLocation, DALI_SEND_NORMAL, 0);
           DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, memBankAccess.bank, DALI_SEND_NORMAL, 0);
         }
         else if (memBankAccess.deviceType == DALI_CONTROL_DEVICE) {
           DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, memBankAccess.blockStartLocation, DALI_SEND_NORMAL, 0);
           DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR1, memBankAccess.bank, DALI_SEND_NORMAL, 0);
         }
         else {
           DaliTelegram_Send3ByteSpecialCommand(SENSOR_SETDTR0, memBankAccess.blockStartLocation, DALI_SEND_NORMAL, 0);
           DaliTelegram_Send3ByteSpecialCommand(SENSOR_SETDTR1, memBankAccess.bank, DALI_SEND_NORMAL, 0);
         }
         memBankAccess.index = memBankAccess.blockStartLocation - memBankAccess.startLocation;
         memBankAccess.readBlockCount = 0;
         memBankAccess.state = MEMORYBANK_READ_BLOCK_READLOCATION;
      break;
    case MEMORYBANK_READ_BLOCK_READLOCATION:
      if(memBankAccess.deviceType == DALI_DRIVER) {
        DaliTelegram_SendStdCommand2AddressWithCallback(memBankAccess.address,
                                                        DALI_READ_MEMORY_LOCATION,
                                                        DALI_WAIT4ANSWER,
                                                        0,
                                                        MemoryBankResponseCallback);
      }
      else if (memBankAccess.deviceType == DALI_CONTROL_DEVICE) {
        DaliTelegram_Send3ByteCommand2AddressWithCallback(memBankAccess.address,
                                                          DALI_SENSOR_READ_MEMORY_LOCATION,
                                                          DALI_WAIT4ANSWER,
                                                          0,
                                                          MemoryBankResponseCallback);
      }
      else {
        DaliTelegram_Send3ByteCommand2AddressWithCallback(memBankAccess.address,
                                                          SENSOR_READ_MEMORY_LOCATION,
                                                          DALI_WAIT4ANSWER,
                                                          0,
                                                          MemoryBankResponseCallback);
      }
      memBankAccess.state = MEMORYBANK_READ_BLOCK_WAIT4READLOCATIONRESPONSE;
      break;
    case MEMORYBANK_READ_BLOCK_FINISHED_CHECKLOCATION:
      if(memBankAccess.deviceType == DALI_DRIVER) {
        DaliTelegram_SendStdCommand2AddressWithCallback(memBankAccess.address,
                                                        DALI_QUERY_CONTENT_DTR0,
                                                        DALI_WAIT4ANSWER | DALI_SAFE_QUERY,
                                                        0,
                                                        MemoryBankResponseCallback);
      }
      else if (memBankAccess.deviceType == DALI_CONTROL_DEVICE) {
        DaliTelegram_Send3ByteCommand2AddressWithCallback(memBankAccess.address,
                                                          DALI_SENSOR_QUERY_CONTENT_DTR0,
                                                          DALI_WAIT4ANSWER | DALI_SAFE_QUERY,
                                                          0,
                                                          MemoryBankResponseCallback);
      }
      else {
        DaliTelegram_Send3ByteCommand2AddressWithCallback(memBankAccess.address,
                                                          SENSOR_QUERY_CONTENT_DTR0,
                                                          DALI_WAIT4ANSWER | DALI_SAFE_QUERY,
                                                          0,
                                                          MemoryBankResponseCallback);
      }
      memBankAccess.state = MEMORYBANK_READ_BLOCK_FINISHED_WAIT4CHECKLOCATIONRESPONSE;
      break;
    case MEMORYBANK_READ_BLOCK_FINISHED:
       if (memBankAccess.index >= memBankAccess.size) {
         memBankAccess.state = MEMORYBANK_READ_FINISHED;
       }
       else {
         memBankAccess.state = MEMORYBANK_READ_BLOCK_START;
       }
      break;
    default:
      break;
  }
}

/// @}

