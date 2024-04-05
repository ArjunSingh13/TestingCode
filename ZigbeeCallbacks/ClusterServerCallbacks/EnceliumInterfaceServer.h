//------------------------------------------------------------------------------
///   \file   EnceliumInterfaceServer.h
///   \since  2017-09-23
///   \brief  Definition of callback-functions from EnceliumInterface-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//------------------------------------------------------------------------------

#include "app/framework/plugin/encelium-interface/encelium-interface.h"

EmberStatus EnceliumInterfaceServer_RescanInterfaceFinished(uint8_t endpoint);
EmberStatus EnceliumInterfaceServer_SetEndpointType(uint8_t endpoint, uint16_t zigbeeDevicetype, bool enabled);
bool EnceliumInterfaceServer_RescanInterface(uint8_t endpoint);
bool EnceliumInterfaceServer_ScanInterface(void);
bool EnceliumInterfaceServer_SetInterfaceConfig(uint8_t endpoint , EnceliumInterfaceType interfaceType);
