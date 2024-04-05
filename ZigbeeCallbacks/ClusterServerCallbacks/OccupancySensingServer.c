//------------------------------------------------------------------------------
///   \file   OccupancySensingServer.c
///   \since  2016-11-11
///   \brief  Definition of callback-functions from occupancy sensing cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup OccupancySensingServer  OccupancySensing Cluster Server
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
#include "app/framework/plugin/reporting/reporting.h"

// --------- Macro Definitions -------------------------------------------------
#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfOccupancySensingClusterPrintln(__VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

static void OccupancySensingServerSetOccupancy(uint8_t endpoint, bool occupied);

// --------- Local Functions ---------------------------------------------------

/** @brief set the occupancy attribute of the occupancy sensing cluster (internal)
 *
 * @param endpoint Endpoint that is being served  Ver.: always
 * @param occupied  new occupancy state  Ver.: always
*/
static void OccupancySensingServerSetOccupancy(uint8_t endpoint, bool occupied)
{
  uint8_t occupancyAttribute = (occupied) ? 0x01 : 0x00;
  emberAfWriteServerAttribute(endpoint,
                              ZCL_OCCUPANCY_SENSING_CLUSTER_ID,
                              ZCL_OCCUPANCY_ATTRIBUTE_ID,
                              (uint8_t *) &occupancyAttribute,
                              ZCL_BITMAP8_ATTRIBUTE_TYPE);
  TRACE(true, "OccupancySensingClusterClientAttributeChange ep:%u Value:%u", endpoint, occupancyAttribute);

}

//---------- Public functions --------------------------------------------------

/** @brief Trigger Presence state
 *
 *  Callback for presence detection interface
 *  maybe DALI or button or HAL
 *
 * @param endpoint Endpoint  Ver.: always
 */
void OccupancySensingServer_PresenceDetectedCallback(uint8_t endpoint) {
  OccupancySensingServerSetOccupancy(endpoint, true);
  uint32_t delayMs;
  uint16_t Occupied2UnoccupiedDelay;
  emberAfCorePrintln("Occupancy Sensor detected \r\n");

  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                      ZCL_OCCUPANCY_SENSING_CLUSTER_ID,
                      ZCL_PIR_OCCUPIED_TO_UNOCCUPIED_DELAY_ATTRIBUTE_ID,
                      (uint8_t *)&Occupied2UnoccupiedDelay,
                      sizeof(Occupied2UnoccupiedDelay));

  delayMs = Occupied2UnoccupiedDelay * MILLISECOND_TICKS_PER_SECOND;
  emberAfScheduleServerTick(endpoint, ZCL_OCCUPANCY_SENSING_CLUSTER_ID, delayMs);

}

//---------- ember Stack Callbacks----------------------------------------------

/** @brief Occupancy Sensing Cluster Server Tick
 *
 * Server Tick Callback
 *
 * @param endpoint Endpoint that is being served  Ver.: always
 */
void emberAfOccupancySensingClusterServerTickCallback(uint8_t endpoint) {
  OccupancySensingServerSetOccupancy(endpoint, false);
  emberAfDeactivateServerTick(endpoint, ZCL_OCCUPANCY_SENSING_CLUSTER_ID);
}

/** @brief Occupancy Sensing Cluster Server Init
 *
 * Server Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfOccupancySensingClusterServerInitCallback(uint8_t endpoint) {
  // Init reporting structure for this cluster
  EmberAfPluginReportingEntry reportingEntry;
  reportingEntry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
  reportingEntry.endpoint = endpoint;
  reportingEntry.clusterId = ZCL_OCCUPANCY_SENSING_CLUSTER_ID;
  reportingEntry.attributeId = ZCL_OCCUPANCY_ATTRIBUTE_ID;
  reportingEntry.mask = CLUSTER_MASK_SERVER;
  reportingEntry.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  reportingEntry.data.reported.minInterval = 0;
  reportingEntry.data.reported.maxInterval = ZIGBEE_MAX_REPORTING_INTERVAL_S;
  reportingEntry.data.reported.reportableChange = 0;
  emberAfPluginReportingConfigureReportedAttribute(&reportingEntry);
}


/// @}

