//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file MemoryBank_private.h
///   \since 2018-07-10
///   \brief private defines for MemoryBank Access
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __MEMORYBANKPRIVATE_H__
#define __MEMORYBANKPRIVATE_H__
// --------- Includes ----------------------------------------------------------

#define MEMBANK_BLOCKREAD_SIZE      10
#define MEMBANK_MAX_FAULT_RETRIES    3
// --------- Type Definitions --------------------------------------------------

typedef enum {
  MEMORYBANK_IDLE,
  MEMORYBANK_READ_START,
  MEMORYBANK_READ_BLOCK_START,
  MEMORYBANK_READ_BLOCK_READLOCATION,
  MEMORYBANK_READ_BLOCK_WAIT4READLOCATIONRESPONSE,
  MEMORYBANK_READ_BLOCK_FINISHED_CHECKLOCATION,
  MEMORYBANK_READ_BLOCK_FINISHED_WAIT4CHECKLOCATIONRESPONSE,
  MEMORYBANK_READ_BLOCK_FINISHED,
  MEMORYBANK_READ_FINISHED,
  MEMORYBANK_READ_FAILED
} TypeMemoryAccessState;

typedef enum {
  DALI_UNKNOWN_TYPE,
  DALI_DRIVER,
  DALI_INPUT_DEVICE,
  DALI_CONTROL_DEVICE
} TypeDaliDeviceType;

typedef struct {
  uint8_t data[256];
  TypeMemoryAccessState state;
  TypeDaliDeviceType deviceType;
  uint8_t address;
  uint8_t bank;
  uint8_t startLocation;
  uint8_t size;
  uint8_t blockStartLocation;
  uint8_t location;
  uint8_t index;
  uint8_t readBlockCount;
  uint8_t faultCount;

} TypeMemoryBankAccess;

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

#endif // __MEMORYBANKPRIVATE_H__
