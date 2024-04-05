//------------------------------------------------------------------------------
///   \file   ElectricalMeasurementServer.c
///   \since  2017-07-11
///   \brief  Definition of callback-functions and interface functions
///           for Electrical Measurement-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup ElectricalMeasurementServer  ElectricalMeasurement Cluster Server
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

/// @}
