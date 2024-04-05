//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   EnceliumInterfaceServer.h
///   \since  2016-11-11
///   \brief  Definitions to customize server functions for EnceliumInterface-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
bool EnceliumInterfaceServer_RescanInterface(uint8_t endpoint);
bool EnceliumInterfaceServer_SetInterfaceConfig(uint8_t endpoint , EnceliumInterfaceType interfaceType)
bool EnceliumInterfaceServer_ScanInterface(void);
EmberStatus EnceliumInterfaceServer_RescanInterfaceFinished(uint8_t endpoint);
EmberStatus EnceliumInterfaceServer_SetEndpointType(uint8_t endpoint, uint16_t zigbeeDevicetype, bool enabled);
