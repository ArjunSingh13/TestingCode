//------------------------------------------------------------------------------
///   \file   BallastConfigurationServer.c
///   \since  2017-07-11
///   \brief  Definition of callback-functions and interface functions
///           for Electrical Measurement-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup BallastConfigurationServer Ballast-Configuration Cluster Server
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
#include "BallastConfigurationServices.h"

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfBallastConfigurationClusterPrintln(__VA_ARGS__);}
// --------- Local Variables ---------------------------------------------------

// --------- Local Function Prototypes -----------------------------------------

static bool BallastConfigurationServerUint8InRange(uint8_t min, uint8_t max, uint8_t value);
//static void BallastConfigurationServicesReloadLinearizationTable(uint8_t endpoint); // actual not implemented, just direct get Attribute data

// --------- Local Functions ---------------------------------------------------

/** \brief load the linearization table to an internal cache
 * actual not needed
 *
 * \param[in] endpoint  index of the zigBee endpoint
  */
//static void BallastConfigurationServicesReloadLinearizationTable(uint8_t endpoint){
//
//}

/** \brief tests if a value is in the range min <= value <= max
 *
 * \param[in] min limit
 * \param[in] max limit
 * \param[in] value
 * return true or false
 */
static bool BallastConfigurationServerUint8InRange(uint8_t min, uint8_t max, uint8_t value) {
  return ((min <= value) && (value <= max));
}

//---------- Public functions --------------------------------------------------

/** \brief Sets the status of the endpoint in the Ballast Configuration Cluster
 *
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] level  actual Level from output device
 */
void BallastConfigurationServer_SetBallastStatus(uint8_t endpoint, uint8_t ballastStatus) {
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                              ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                              ZCL_BALLAST_STATUS_ATTRIBUTE_ID,
                              (uint8_t *)&ballastStatus,
                              ZCL_BITMAP8_ATTRIBUTE_TYPE);
  TRACE (true, "SetBallastStatus ep:%u, status:%x result:%x", endpoint, ballastStatus, status);
}

/** \brief Sets the physical min Level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] level physical min level of that device
 */
void BallastConfigurationServer_SetPhysicalMinLevel(uint8_t endpoint, uint8_t level) {
  emberAfWriteServerAttribute(endpoint,
                            ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                            ZCL_PHYSICAL_MIN_LEVEL_ATTRIBUTE_ID,
                            (uint8_t *)&level,
                            ZCL_INT8U_ATTRIBUTE_TYPE);
  uint8_t minLevel = BallastConfigurationServer_GetMinLevel(endpoint);
  // update min level to be equal to physical min level
  emberAfWriteServerAttribute(endpoint,
                            ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                            ZCL_MIN_LEVEL_ATTRIBUTE_ID,
                            (uint8_t *)&level,
                            ZCL_INT8U_ATTRIBUTE_TYPE);
}

/** \brief Sets the burning hours of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] hour burning hours reported by the connected ecg
 */
void BallastConfigurationServer_SetLampBurnHours(uint8_t endpoint, uint32_t hours) {
  emberAfWriteServerAttribute(endpoint,
                              ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                              ZCL_LAMP_BURN_HOURS_ATTRIBUTE_ID,
                              (uint8_t *)&hours,     // only 24 Bit value
                              ZCL_INT24U_ATTRIBUTE_TYPE);
}

/** \brief Sets the physical min Level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return phys min level
 */
uint8_t BallastConfigurationServer_GetPhysicalMinLevel(uint8_t endpoint) {
  uint8_t level = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                            ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                            ZCL_PHYSICAL_MIN_LEVEL_ATTRIBUTE_ID,
                            (uint8_t *)&level,
                            sizeof(level));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      level = 1;
      TRACE (true, "ERR:%x reading PhysMinLevel", status);
  }
  return level;
}

/** \brief Gets the min level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return min level
 */
uint8_t BallastConfigurationServer_GetMinLevel(uint8_t endpoint) {
  uint8_t level = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                                      ZCL_MIN_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&level,
                                      sizeof(level));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
	  level = 1;
	  TRACE (true, "ERR:%x reading MinLevel", status);
  }
  return level;
}

/** \brief Gets the max level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return max level
 */
uint8_t BallastConfigurationServer_GetMaxLevel(uint8_t endpoint) {
  uint8_t level = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                                      ZCL_MAX_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&level,
                                      sizeof(level));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
	  level = 254;
	  TRACE (true, "ERR:%x reading MaxLevel", status);
  }
  return level;
}

/** \brief Gets the max level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return max level
 */
uint8_t BallastConfigurationServer_GetPoweronLevel(uint8_t endpoint) {
  uint8_t level = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                                      ZCL_POWER_ON_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&level,
                                      sizeof(level));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
	  level = 254;
	  TRACE (true, "ERR:%x reading PonLevel", status);
  }
  return level;
}

/** \brief Gets the max level of the endpoint in the Ballast Configuration Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return max level
 */
uint8_t BallastConfigurationServer_GetSystemFailureLevel(uint8_t endpoint) {
  uint8_t level = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                                      ZCL_POWER_ON_LEVEL_ATTRIBUTE_ID,    // Just to avoid compile Faults :-)
//                                      ZCL_SYSTEM_FAILURE_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&level,
                                      sizeof(level));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      level = 254;
      TRACE (true, "ERR:%x reading SysFailLevel", status);
  }
  return level;
}

/** \brief calculates output value using the mfg specific linearization tabele
 *
 * \param[in] endpoint
 * \param[in] level
 * return linearized level
 */
uint8_t BallastConfigurationServer_LinearizeLevel(uint8_t endpoint, uint8_t levelIn) {

  #define MAX_LINEARIZATION_VALUE    5000
  #define MAX_LEVEL_VALUE            0xFE

  if (levelIn == 0) return 0;  // Off should stay off...

  uint16_t linearizationTable[10];
  uint32_t levelOut;
  EmberStatus status = emberAfReadManufacturerSpecificServerAttribute(endpoint,
                                                 ZCL_BALLAST_CONFIGURATION_CLUSTER_ID,
                                                 ZCL_LINEARIZATION_VALUE_ATTRIBUTE_ID,
                                                 OSRAM_MANUFACTURER_CODE,
                                                 ((uint8_t *) &linearizationTable[0]) + 1,   // second byte is overwritten with lenght byte
                                                 18);

  linearizationTable[0] = 0;
  if (status != EMBER_SUCCESS) return levelIn;   // If there is no linearization table return input value
  // Figure out which table entries the value lies between.
  uint16_t tableEntryWidth =  100 * MAX_LEVEL_VALUE / 8;                  // 3175. Scale by 100 as 31.75 will be casted.
  uint16_t interval = ((uint32_t)levelIn * 100 / tableEntryWidth) + 1;   // Find after which table entry the value is (index 0 is skipped)

  if (interval < 9) {
    // Scale 0..MAX_LINEARIZATION_VALUE to 0..255
    uint32_t intervalPosition =  ((uint32_t)levelIn * 100) % tableEntryWidth;
    int32_t tableEntryDiff = (int32_t)linearizationTable[interval+1] - linearizationTable[interval];
    levelOut = linearizationTable[interval] + ((tableEntryDiff * intervalPosition) / tableEntryWidth) + 1;
  }
  else {
      // special case - at end of table
    levelOut = linearizationTable[9];
  }

  levelOut = (MAX_LEVEL_VALUE * levelOut) / MAX_LINEARIZATION_VALUE;
  if (levelOut > MAX_LEVEL_VALUE) {
    levelOut = MAX_LEVEL_VALUE;
  }
  TRACE(true, "BallastConfigurationServices_LinearizationLevel In:%u Out:%u", levelIn, levelOut);
  return (uint8_t) levelOut;

}

/** @brief Ballast Configuration Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * \param[in] endpoint Endpoint that is being initialized  Ver.: always
 * \param[in] attributeId Attribute that changed  Ver.: always
 */
void emberAfBallastConfigurationClusterServerAttributeChangedCallback(uint8_t endpoint, EmberAfAttributeId attributeId) {
  switch(attributeId) {
    case ZCL_MIN_LEVEL_ATTRIBUTE_ID:
      BallastConfigurationServices_SetMinLevel(endpoint, BallastConfigurationServer_GetMinLevel(endpoint));
      break;
    case ZCL_MAX_LEVEL_ATTRIBUTE_ID:
      BallastConfigurationServices_SetMaxLevel(endpoint, BallastConfigurationServer_GetMaxLevel(endpoint));
      break;
    case ZCL_POWER_ON_LEVEL_ATTRIBUTE_ID:
      BallastConfigurationServices_SetPoweronLevel(endpoint, BallastConfigurationServer_GetPoweronLevel(endpoint));
      BallastConfigurationServices_SetSystemFailureLevel(endpoint, BallastConfigurationServer_GetPoweronLevel(endpoint));
      break;
    case ZCL_LINEARIZATION_VALUE_ATTRIBUTE_ID:
      //BallastConfigurationServicesReloadLinearizationTable(endpoint);
      break;
    default:
      break;
  }
}

//

/** @brief Ballast Configuration Cluster Server Pre Attribute Changed
 *
 * Server Pre Attribute Changed Tests Range of attributes
 *
 * \param[in] endpoint Endpoint that is being initialized  Ver.: always
 * \param[in] attributeId Attribute to be changed  Ver.: always
 * \param[in] attributeType Attribute type  Ver.: always
 * \param[in] size Attribute size  Ver.: always
 * \param[in] value Attribute value  Ver.: always
 */
EmberAfStatus emberAfBallastConfigurationClusterServerPreAttributeChangedCallback(uint8_t endpoint,
                                                                                  EmberAfAttributeId attributeId,
                                                                                  EmberAfAttributeType attributeType,
                                                                                  uint8_t size,
                                                                                  uint8_t *value) {
  EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
  switch(attributeId) {
    case ZCL_MIN_LEVEL_ATTRIBUTE_ID:
      if (attributeType == ZCL_INT8U_ATTRIBUTE_TYPE) {
        uint8_t min = BallastConfigurationServer_GetPhysicalMinLevel(endpoint);
        uint8_t max = BallastConfigurationServer_GetMaxLevel(endpoint);
        status = (BallastConfigurationServerUint8InRange(min, max, *value)) ? EMBER_ZCL_STATUS_SUCCESS : EMBER_ZCL_STATUS_INVALID_VALUE;
      }
      break;
    case ZCL_MAX_LEVEL_ATTRIBUTE_ID:
      if (attributeType == ZCL_INT8U_ATTRIBUTE_TYPE) {
        uint8_t min = BallastConfigurationServer_GetMinLevel(endpoint);
        status = (BallastConfigurationServerUint8InRange(min, 254, *value)) ? EMBER_ZCL_STATUS_SUCCESS : EMBER_ZCL_STATUS_INVALID_VALUE;
      }
      break;
    case ZCL_POWER_ON_LEVEL_ATTRIBUTE_ID:
      if (attributeType == ZCL_INT8U_ATTRIBUTE_TYPE) {
        uint8_t min = BallastConfigurationServer_GetMinLevel(endpoint);
        uint8_t max = BallastConfigurationServer_GetMaxLevel(endpoint);
        status = (BallastConfigurationServerUint8InRange(min, max, *value)) ? EMBER_ZCL_STATUS_SUCCESS : EMBER_ZCL_STATUS_INVALID_VALUE;
      }
      break;
    default:
      break; // Nothing to test
  }
  return status;
}


/// @}
