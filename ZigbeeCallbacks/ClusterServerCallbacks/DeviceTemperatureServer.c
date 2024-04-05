//------------------------------------------------------------------------------
///   \file   DeviceTemperatureServer.c
///   \since  2017-07-11
///   \brief  Definition of callback-functions and interface functions
///           for Device Temperature-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup DeviceTemperatureServer  DeviceTemperature Cluster Server
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

// --------- Macro Definitions -------------------------------------------------
#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfDeviceTemperatureClusterPrintln(__VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

//---------- Public functions --------------------------------------------------

//---------- ember Stack Callbacks----------------------------------------------

/// @}
