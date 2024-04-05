//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file MemoryBank.h
///   \since 2018-07-10
///   \brief Services for MemoryBank Access
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __MEMORYBANK_H__
#define __MEMORYBANK_H__
// --------- Includes ----------------------------------------------------------

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------
EmberStatus MemoryBank_DriverReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size);
EmberStatus MemoryBank_InputDeviceReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size);
EmberStatus MemoryBank_DaliInputDeviceReadMemoryBank(uint8_t shortAddress, uint8_t bank, uint8_t start, uint8_t size);
bool MemoryBank_ReadRequestDone(void);
void MemoryBank_FreeHandler(void);
uint8_t *MemoryBank_GetBuffer(void);
void MemoryBank_Tick(void);

#endif // __MEMORYBANK_H__
