//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   EnceliumInterfaceServices.h
///   \since  2016-11-11
///   \brief  Definitions to customize server functions for EnceliumInterface-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Zigbee2Dali/Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
#define EnceliumInterfaceServer_StartAddressing        Zigbee2Dali_StartAddressing
#define EnceliumInterfaceServer_InitDevices            Zigbee2Dali_InitDevices
