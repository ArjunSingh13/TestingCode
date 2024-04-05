//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliDeviceHandling.h
///   \since 2017-05-08
///   \brief Interface for all Dali Addressing and DeviceScan
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALIDEVICEHANDLING_H__
#define __DALIDEVICEHANDLING_H__

// --------- Includes ----------------------------------------------------------

// --------- Macro Definitions -------------------------------------------------

// --------- Type Definitions --------------------------------------------------

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

uint32_t DaliDeviceHandling_FindLuxCalibrationMultiplier(void);
void DaliDeviceHandling_StartDeviceAddressing(void);
bool DaliDeviceHandling_IsAddressing(void);
void DaliDeviceHandling_InitDeviceAddressing(void) ;
bool DaliDeviceHandling_InitDevices(void);
void DaliDeviceHandling_Tick(void);

#endif // __DALIDEVICEHANDLING_H__
