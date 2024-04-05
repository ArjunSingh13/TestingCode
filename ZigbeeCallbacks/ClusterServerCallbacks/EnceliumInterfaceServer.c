//------------------------------------------------------------------------------
///   \file   EnceliumInterfaceServer.c
///   \since  2017-09-23
///   \brief  Definition of callback-functions from EnceliumInterface-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup EnceliumInterfaceServer EnceliumInterface Cluster Server
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------

#include "GlobalDefs.h"
#include <stdio.h>

#include PLATFORM_HEADER

#ifndef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
	#include "stack/include/ember.h"
#endif

// copied from HaLightSocDali_callbacks.c (buttonEventHandler())
#include "app/framework/include/af.h"
#include "app/framework/plugin/encelium-interpan/encelium-interpan.h"

#include "ZigbeeHaLightEndDevice_deviceinfo.h"
#include "EnceliumInterfaceServer.h"
#include "EnceliumInterfaceServices.h"
#include "ZigbeeHaLightEndDevice_own.h"

// --------- Macro Definitions -------------------------------------------------
//#define TRACE(trace, format, ...)
#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF, __VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

//---------- Public functions --------------------------------------------------

/** \brief Rescan Interface (DALI) for devices
 *
 *
 *
 * \param endpoint
 * \param zigbeeDevicetype  Devicetype Id of the endpoint
 * \param enabled           endpoint enabled (means Zigbee endpoint is active)
 * \return command finally processed (true: no action in stack necessary)
 */
EmberStatus EnceliumInterfaceServer_SetEndpointType(uint8_t endpoint, uint16_t zigbeeDevicetype, bool enabled) {

  TRACE(true, "EnceliumInterfaceServer SetEndpointType ep:%u type:0x%2X enabled:%u", endpoint, zigbeeDevicetype, enabled);
  uint8_t endpointIndex = emberAfIndexFromEndpointIncludingDisabledEndpoints(endpoint);

  if(endpointIndex == 0xFF) return EMBER_ZCL_STATUS_INVALID_VALUE;

  emAfEndpoints[endpointIndex].deviceId = zigbeeDevicetype;
  if(enabled) {
    emberAfPluginEnceliumInterfaceSetInterfaceType(endpoint, emberAfPluginEnceliumInterfaceZigBeeDeviceTypeToInterface(zigbeeDevicetype));
  }
  else {
    emberAfPluginEnceliumInterfaceSetInterfaceType(endpoint, ENCELIUM_INTERFACE_TYPE_UNKNOWN);
  }

  return EMBER_ZCL_STATUS_SUCCESS;
}


/** \brief Rescan Interface (DALI) for devices
 *
 *
 *
 * \param endpoint actual 0xff for all   Ver.: always
 * \return status (EMBER_ZCL_STATUS_SUCCESS if ok)
 */
EmberStatus EnceliumInterfaceServer_RescanInterfaceFinished(uint8_t endpoint) {
  EmberStatus status = EMBER_ZCL_STATUS_SUCCESS;
  if (emberStackIsUp()) {
    if (emberGetNodeId() != 0) {
      TRACE(true, "EnceliumInterfaceCluster RescanInterfaceFinished ZCL Message");
      EnceliumInterfaceType ifType[8];  // Actual number of endpoints is hardcoded (also in "uuuuuuuu" in FillBuffer function
      for(int i = 0; i < 8; i++) {
        ifType[i] = emberAfPluginEnceliumInterfaceGetInterfaceType(endpoint);
      }
      emberAfSetCommandEndpoints(endpoint, 1);
      emberAfFillExternalManufacturerSpecificBuffer( (ZCL_CLUSTER_SPECIFIC_COMMAND
                                                    | ZCL_MANUFACTURER_SPECIFIC_MASK
                                                    | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
                                                    ZCL_ENCELIUM_INTERFACE_CLUSTER_ID,
                                                    EMBER_AF_MANUFACTURER_CODE,
                                                    ZCL_INTERFACE_CONFIG_COMMAND_ID,
                                                    "uuuuuuuu",
                                                    ifType[0],
                                                    ifType[1],
                                                    ifType[2],
                                                    ifType[3],
                                                    ifType[4],
                                                    ifType[5],
                                                    ifType[6],
                                                    ifType[7]);
      EmberApsFrame* apsFrame = emberAfGetCommandApsFrame();
      apsFrame->options |= EMBER_APS_OPTION_RETRY;
      status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, EMBER_ZIGBEE_COORDINATOR_ADDRESS);
      TRACE(true, "EnceliumInterfaceCluster Send InterfaceConfig %x-%x-%x-%x-%x-%x-%x", ifType[0], ifType[1], ifType[2], ifType[3], ifType[4], ifType[5], ifType[6]);
    } else {
#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)
      TRACE(true, "EnceliumInterfaceCluster emberAfPluginEnceliumInterpanSendInterfaceConfig ");
      emberAfPluginEnceliumInterpanSendInterfaceConfig(0);
      //ZigbeeHaLightEndDevice_SendInterpanRescanResponse();
#endif
    }
  }
  return status;
}

/** \brief Rescan Interface (DALI) for devices
 *
 *
 *
 * \param endpoint   Ver.: always
 * \return command finally processed (true: no action in stack necessary)
 */
bool EnceliumInterfaceServer_RescanInterface(uint8_t endpoint) {
  TRACE(true, "EnceliumInterfaceCluster RescanInterfac");
  ZigbeeHaLightEndDevice_SetRepeatermode(false);
  emberAfPluginEnceliumInterfaceSetEnable(ZIGBEE2DALI_REPEATER_ENDPOINT, false);
  EnceliumInterfaceServer_StartAddressing(true);
  return true;
}

/** \brief Scan Interface (DALI) for devices
 *
 *
 *
 * \param endpoint   Ver.: always
 * \return command finally processed (true: no action in stack necessary)
 */
bool EnceliumInterfaceServer_ScanInterface(void) {
  TRACE(true, "EnceliumInterfaceCluster ScanInterface");
  EnceliumInterfaceServer_StartAddressing(false);
  return true;
}

/** \brief SetInterfaceConfig
 *
 *  Used to set device in repeater Mode
 *
 * \param endpoint of the enpoint which should change
 * \param interfaceType to change to
 * \return command processed, right endpoint and interfacetype
 */
bool EnceliumInterfaceServer_SetInterfaceConfig(uint8_t endpoint , EnceliumInterfaceType interfaceType) {
  TRACE(true, "EnceliumInterfaceCluster SetInterfaceConfig");
  bool ret = false;
  if (    (endpoint == ZIGBEE2DALI_REPEATER_ENDPOINT)
       && (interfaceType == ENCELIUM_INTERFACE_TYPE_ROUTER)) {
    ZigbeeHaLightEndDevice_SetRepeatermode(true);
    emberAfPluginEnceliumInterfaceSetEnable(ZIGBEE2DALI_REPEATER_ENDPOINT, true);
    TRACE(true, "EnceliumInterfaceCluster Changed to Repeater Mode");
    EnceliumInterfaceServer_InitDevices();
    EnceliumInterfaceServer_RescanInterfaceFinished(ZIGBEE2DALI_SAK_ENDPOINT);
    ret = true;
  }
  return ret;
}

//---------- ember Stack Callbacks----------------------------------------------

/** \brief Encelium Interface Cluster Cluster Rescan Interface
 *
 *
 *
 * \param endpoint   actual not used, just scan all devicesVer.: always
 */
bool emberAfEnceliumInterfaceClusterRescanInterfaceCallback(uint8_t endpoint) {
  EnceliumInterfaceServer_RescanInterface(endpoint);
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

/** \brief Encelium Interface Cluster Cluster Set Interface
 *
 *
 *
 * \param endpoint   Ver.: always
 * \param interfaceType   Ver.: always
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfEnceliumInterfaceClusterSetInterfaceCallback(uint8_t endpoint,
                                                            uint8_t interfaceType) {
  EnceliumInterfaceServer_SetInterfaceConfig(endpoint, interfaceType);

  return true;
}

/// @}

