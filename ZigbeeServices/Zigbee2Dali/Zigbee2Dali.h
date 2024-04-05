//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file Zigbee2Dali.h
///   \since 2016-11-17
///   \brief Interface to services for all Zigee clusters which need Dali communication
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __ZIGBEE2DALI_H__
#define __ZIGBEE2DALI_H__

// --------- Includes ----------------------------------------------------------

#include "DaliTelegram.h"

// --------- Macro Definitions -------------------------------------------------

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

// ------------ INIT -----------------------------------------------------------
void Zigbee2Dali_Init( void );
void Zigbee2Dali_Start( void );
uint32_t Zigbee2Dali_TimeTick(void);
bool Zigbee2Dali_TriggerNetworkLeave ( void );
bool Zigbee2Dali_IsInitializing(void);
void Zigbee2Dali_DeleteAllAddresses(void);
void Zigbee2Dali_StartMfgTest( void );
bool Zigbee2Dali_InitDevices(void);

// ------------ ZIGBEE SERVICES ------------------------------------------------
void Zigbee2Dali_OnOff(uint8_t endpoint, bool on);
void Zigbee2Dali_Identify(uint8_t endpoint, bool identifyStart);
void Zigbee2Dali_StartFadeTime(uint8_t endpoint, uint8_t lightCommand, uint16_t fadeTime_0sx);
void Zigbee2Dali_StartColorTemperatureFadeTime(uint8_t endpoint, uint16_t mired, uint16_t fadeTime_0sx);
void Zigbee2Dali_StartColorFadeTime(uint8_t endpoint, uint16_t x, uint16_t y, uint16_t fadeTime_0sx);
void Zigbee2Dali_StartFadeRate(uint8_t endpoint, int16_t fadeRate_Inc_s, bool switchOff);
void Zigbee2Dali_StopFade(uint8_t endpoint);
void Zigbee2Dali_StopColorTemp(uint8_t endpoint);
void Zigbee2Dali_StartAddressing(bool withDelete);
void Zigbee2Dali_SetMinLevel(uint8_t endpoint, uint8_t level);
void Zigbee2Dali_SetMaxLevel(uint8_t endpoint, uint8_t level);
void Zigbee2Dali_SetPoweronLevel(uint8_t endpoint, uint8_t level);
void Zigbee2Dali_SetSystemFailureLevel(uint8_t endpoint, uint8_t level);
void Zigbee2Dali_SetFadeTime(uint8_t endpoint, uint8_t fadeTime);
void Zigbee2Dali_SetColorTemp(uint8_t endpoint, uint16_t mired);
void Zigbee2Dali_IdentifyIllunminanceSensor(uint8_t endpointIndex);
void Zigbee2Dali_IdentifyOccupancySensor(uint8_t endpointIndex);

// ------------ DALI SERVICES --------------------------------------------------
void Zigbee2Dali_SetBallastAddress(int32_t index, uint8_t address);
void Zigbee2Dali_SetEmergencyAddress(uint8_t address);
void Zigbee2Dali_SetBroadcastMode(bool mode);
void Zigbee2Dali_SetDaliVersion(int32_t index, uint8_t version);
void Zigbee2Dali_SetDeviceType1Support(int32_t index, bool supported);
void Zigbee2Dali_SetDeviceType6Support(int32_t index, bool supported);
void Zigbee2Dali_SetDeviceType8Support(int32_t index, bool supported);
void Zigbee2Dali_InitPhysicalMinLevel(int32_t index);
void Zigbee2Dali_InitColorTempPhysMin(int32_t index);
void Zigbee2Dali_InitColorTempPhysMax(int32_t index);
void Zigbee2Dali_SetPhysicalMinLevel(int32_t index, uint8_t level);
void Zigbee2Dali_SetColorTempPhysMin(int32_t index, uint16_t colorTempMired);
void Zigbee2Dali_SetColorTempPhysMax(int32_t index, uint16_t colorTempMired);
void Zigbee2Dali_StartDevice(int32_t index);
void Zigbee2Dali_SetSensorMode(uint8_t mode);
void Zigbee2Dali_SetLightSensorAddress(int32_t index, uint8_t address);
void Zigbee2Dali_SetPresenceSensorAddress(int32_t index, uint8_t address);
void Zigbee2Dali_DeactivateBallast(uint8_t index);
void Zigbee2Dali_DeactivateSensor(int32_t index);
bool Zigbee2Dali_isAddressNew(uint8_t address);
void Zigbee2Dali_EmergencyDisableAutoTest(void);
void Zigbee2Dali_EmergencyStartFunctionTest(void);
void Zigbee2Dali_EmergencyStartDurationTest(void);
void Zigbee2Dali_EmergencyStopTest(void);
void Zigbee2Dali_EmergencyInhibit(void);
void Zigbee2Dali_EmergencyResetInhibit(void);
void Zigbee2Dali_EmergencyProlongBy(uint8_t timeInHalfMinutes);
void Zigbee2Dali_EmergencyStartIdentify(void);
void Zigbee2Dali_EmergencySetTestExecutionTimeout(uint8_t timeOut);
void Zigbee2Dali_EmergencySetLevel(uint8_t level);
void Zigbee2Dali_EmergencyInit(void);
void Zigbee2Dali_SetEndpointForDeviceIndex(uint8_t endpoint, uint8_t index);

#endif // __ZIGBEE2DALI_H__
