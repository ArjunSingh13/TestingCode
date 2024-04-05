//------------------------------------------------------------------------------
///   \file   GroupServer.c
///   \since  2016-11-11
///   \brief  Definition of callback-functions from group-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup GroupServer  Group Cluster Server
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------

#include "GlobalDefs.h"

#include PLATFORM_HEADER
//#include "platform\base\hal\micro\led-blink.h"

#ifndef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
	#include "stack/include/ember.h"
#endif

// copied from HaLightSocDali_callbacks.c (buttonEventHandler())
#include "app/framework/include/af.h"
#include "app/util/common/form-and-join.h"
#include "app/framework/plugin/ezmode-commissioning/ez-mode.h"
#include "app/framework/plugin/reporting/reporting.h"
// .. copied

// --------- Macro Definitions -------------------------------------------------
#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfGroupClusterPrintln(__VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

//---------- Public functions --------------------------------------------------

//---------- ember Stack Callbacks----------------------------------------------

/** @brief Group Names Supported
 *
 * This function returns whether or not group names are supported.
 *
 * @param endpoint Endpoint  Ver.: always
 */
bool emberAfPluginGroupsServerGroupNamesSupportedCallback(uint8_t endpoint)
{
  return false;
}

/** @brief Get Group Name
 *
 * This function returns the name of a group with the provided group ID,
 * should it exist.
 *
 * @param endpoint Endpoint  Ver.: always
 * @param groupId Group ID  Ver.: always
 * @param groupName Group Name  Ver.: always
 */
void emberAfPluginGroupsServerGetGroupNameCallback(uint8_t endpoint,
                                                   uint16_t groupId,
                                                   uint8_t * groupName)
{
}

/** @brief Set Group Name
 *
 * This function sets the name of a group with the provided group ID.
 *
 * @param endpoint Endpoint  Ver.: always
 * @param groupId Group ID  Ver.: always
 * @param groupName Group Name  Ver.: always
 */
void emberAfPluginGroupsServerSetGroupNameCallback(uint8_t endpoint,
                                                   uint16_t groupId,
                                                   uint8_t * groupName)
{
}

/// @}
