//------------------------------------------------------------------------------
///   \file   IdentifyClient.c
///   \since  2018-06-21
///   \brief  Definition of commands for Identify-cluster client
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup IdentifyClusterClient Identify Cluster Client
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------

#include "GlobalDefs.h"

#include PLATFORM_HEADER

#ifndef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
	#include "stack/include/ember.h"
#endif

#include "app/framework/include/af.h"
#include "IdentifyClient.h"
#include "app/framework/plugin/encelium-interpan/encelium-interpan.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfIdentifyClusterPrintln(__VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

//---------- Public functions --------------------------------------------------

EmberStatus IdentifyClient_TriggerEffect(uint8_t endpoint, uint8_t effectIdentifier, uint8_t effectVariant) {
  emberAfSetCommandEndpoints(endpoint, 1);
  emberAfFillCommandIdentifyClusterTriggerEffect(effectIdentifier, effectVariant)
  EmberApsFrame* apsFrame = emberAfGetCommandApsFrame();
  apsFrame->options |= EMBER_APS_OPTION_RETRY;
  return emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, EMBER_ZIGBEE_COORDINATOR_ADDRESS);
}

EmberStatus IdentifyClient_TriggerReverseWink(uint8_t endpoint){
  EmberStatus status;
  if (emberStackIsUp() && emberGetNodeId() != 0) {
    status = IdentifyClient_TriggerEffect(endpoint, IDENTIFY_EFFECT_BREATHE, 0);
  }
  else {
    status = emberAfPluginEnceliumInterpanBroadcastReverseWink(endpoint);
  }
  return status;
}

//---------- ember Stack Callbacks----------------------------------------------


/// @}

