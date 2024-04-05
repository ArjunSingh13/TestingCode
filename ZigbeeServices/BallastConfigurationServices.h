//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   BallastConfigurationServices.h
///   \since  2016-11-11
///   \brief  Definitions to customize server functions for Ballast Configuration-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #include "Zigbee2Dali/Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
  #define BallastConfigurationServices_SetMinLevel            Zigbee2Dali_SetMinLevel
  #define BallastConfigurationServices_SetMaxLevel            Zigbee2Dali_SetMaxLevel
  #define BallastConfigurationServices_SetPoweronLevel        Zigbee2Dali_SetPoweronLevel
  #define BallastConfigurationServices_SetSystemFailureLevel  Zigbee2Dali_SetSystemFailureLevel
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
  #define BallastConfigurationServices_SetMinLevel(a, b)
  #define BallastConfigurationServices_SetMaxLevel(a, b)
  #define BallastConfigurationServices_SetPoweronLevel(a, b)
  #define BallastConfigurationServices_SetSystemFailureLevel(a, b)
#endif
void BallastConfigurationServer_SetBallastStatus(uint8_t endpoint, uint8_t status);
void BallastConfigurationServer_SetPhysicalMinLevel(uint8_t endpoint, uint8_t level);
void BallastConfigurationServer_SetLampBurnHours(uint8_t endpoint, uint32_t hours);
uint8_t BallastConfigurationServer_GetMinLevel(uint8_t endpoint);
uint8_t BallastConfigurationServer_GetMaxLevel(uint8_t endpoint);
uint8_t BallastConfigurationServer_GetPoweronLevel(uint8_t endpoint);
uint8_t BallastConfigurationServer_GetPhysicalMinLevel(uint8_t endpoint);
uint8_t BallastConfigurationServer_GetSystemFailureLevel(uint8_t endpoint);
uint8_t BallastConfigurationServer_LinearizeLevel(uint8_t endpoint, uint8_t levelIn);
