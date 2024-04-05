//------------------------------------------------------------------------------
///   \file   IlluminanceMeasurementServer.c
///   \since  2016-11-11
///   \brief  Definition of callback-functions from illuminance measurement-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup IlluminanceMeasurementServer  IlluminanceMeasurement Cluster Server
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
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xffff, __VA_ARGS__);}

// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

//---------- Public functions --------------------------------------------------

/** @brief Callback for DALI sensor value
 *
 * This function returns whether or not group names are supported.
 *
 * @param endpoint Endpoint  Ver.: always
 */
void IlluminanceMeasurementServer_ActualLightCallback(uint8_t endpoint, uint16_t light) {
// conversion to zigbee format is missing !!!
  uint16_t illuminance = light;
  uint16_t illuminance_floor = 0;
  EmberStatus status = emberAfReadManufacturerSpecificServerAttribute(endpoint,
                              ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                              ZCL_ILLUM_MEASURED_FLOOR_ATTRIBUTE_ID,
                              OSRAM_MANUFACTURER_CODE,
                              (uint8_t*)&illuminance_floor,
                              sizeof(illuminance_floor));
  if (status == EMBER_ZCL_STATUS_SUCCESS) {
	  if (illuminance < illuminance_floor) {
		  illuminance = illuminance_floor;
	  }
  }
  TRACE(true, "IllumMeasurementClusterrClientAttributeChange ep:%u Value:%u", endpoint, illuminance);
  status = emberAfWriteServerAttribute(endpoint,
                              ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                              ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID,
                              (uint8_t *) &illuminance,
                              ZCL_INT16U_ATTRIBUTE_TYPE);

}

/** @brief Get Reverse Wink Threshold
 *
 * This function returns the value of the Reverse Wink Threshold
 *
 * @param endpoint Endpoint  Ver.: always
 */
uint16_t IlluminanceMeasurementServer_GetReverseWinkThreshold(uint8_t endpoint) {
  uint16_t reverseWinkThreshold = 0;
  EmberStatus status = emberAfReadManufacturerSpecificServerAttribute(endpoint,
                              ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                              ZCL_REVERSE_WINK_THRESHOLD_ATTRIBUTE_ID,
                              OSRAM_MANUFACTURER_CODE,
                              (uint8_t*) &reverseWinkThreshold,
                              sizeof(reverseWinkThreshold));
  return reverseWinkThreshold;
}

EmberAfStatus IlluminanceMeasurementServer_SetLuxCalibrationMultplier(uint8_t endpoint, uint32_t multiplier) {
  EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(endpoint,
                              ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                              ZCL_LUX_CALIB_MULT_ATTRIBUTE_ID,
                              OSRAM_MANUFACTURER_CODE,
                              (uint8_t*) &multiplier,
                              ZCL_INT32U_ATTRIBUTE_TYPE);
  TRACE((status != EMBER_ZCL_STATUS_SUCCESS), "Failed to Write LuxCalibrationMultplier to %l status: %u", multiplier, status);
  return status;
}

uint32_t IlluminanceMeasurementServer_GetLuxCalibrationMultplier(uint8_t endpoint) {
  uint32_t luxCalibrationMultiplier = 0x00010000;
  EmberAfStatus status = emberAfReadManufacturerSpecificServerAttribute(endpoint,
                              ZCL_ILLUM_MEASUREMENT_CLUSTER_ID,
                              ZCL_LUX_CALIB_MULT_ATTRIBUTE_ID,
                              OSRAM_MANUFACTURER_CODE,
                              (uint8_t*) &luxCalibrationMultiplier,
                              sizeof(luxCalibrationMultiplier));
  TRACE((status != EMBER_ZCL_STATUS_SUCCESS), "Failed to Read LuxCalibrationMultplier");
  return luxCalibrationMultiplier;
}

//---------- ember Stack Callbacks----------------------------------------------

/** Illuminance Measurement Cluster Server Init
 *
 * Server Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfIllumMeasurementClusterServerInitCallback(uint8_t endpoint) {
  // Init reporting structure for this cluster
  EmberAfPluginReportingEntry reportingEntry;
  reportingEntry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
  reportingEntry.endpoint = endpoint;
  reportingEntry.clusterId = ZCL_ILLUM_MEASUREMENT_CLUSTER_ID;
  reportingEntry.attributeId = ZCL_ILLUM_MEASURED_VALUE_ATTRIBUTE_ID;
  reportingEntry.mask = CLUSTER_MASK_SERVER;
  reportingEntry.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  reportingEntry.data.reported.minInterval = ZIGBEE_MIN_REPORTING_INTERVAL_S;
  reportingEntry.data.reported.maxInterval = ZIGBEE_MAX_REPORTING_INTERVAL_S;
  reportingEntry.data.reported.reportableChange = 256;
  emberAfPluginReportingConfigureReportedAttribute(&reportingEntry);
}


/// @}
