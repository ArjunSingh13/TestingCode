//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   EndpointServices.h
///   \since  2016-11-11
///   \brief  Declaration of basic functions called by callback-functions from Zigbee-stack
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "Zigbee2Dali.h"
void EndpointServices_Init( void );
#define EndpointServices_Init                 Zigbee2Dali_Init
void EndpointServices_TimeTick(void);
#define EndpointServices_TimeTick             Zigbee2Dali_TimeTick
bool EndpointServices_TriggerNetworkLeave(void);
#define EndpointServices_TriggerNetworkLeave  Zigbee2Dali_TriggerNetworkLeave
