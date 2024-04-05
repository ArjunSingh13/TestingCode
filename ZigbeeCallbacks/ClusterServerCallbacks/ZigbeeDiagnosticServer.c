//------------------------------------------------------------------------------
///   \file   ZigbeeDiagnosticServer.c
///   \since  2016-11-11
///   \brief  Definition of callback-functions from OsramSpecific Diagnostic Cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//------------------------------------------------------------------------------

#include "GlobalDefs.h"
#include "ZigbeeDiagnosticServer.h"

#include PLATFORM_HEADER

#ifndef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
  #include "stack/include/ember.h"
#endif

// copied from HaLightSocDali_callbacks.c (buttonEventHandler())
#include "app/framework/include/af.h"

#define ENDPOINT    1  // no better idea, write all to ep1

#define TRACE(trace, ...) if(trace) {emberAfZigbeeRfQualityClusterPrintln(EMBER_AF_PRINT_ZIGBEE_RF_QUALITY_CLUSTER, __VA_ARGS__);}
//#define TRACE(trace, ...)

#define DIAGNOSTICINFO_DEFAULT {.rxCounter = 0, .rssi_sum = 0, .rssi_min = 127, .rssi_max = -128, .lqi_sum = 0, .lqi_min = 255, .lqi_max = 0}
TypeSignalQuality signalQuality;
const TypeDiagnosticInfo diagnosticInfoDEFAULT = DIAGNOSTICINFO_DEFAULT;

#ifdef ZCL_USING_ZIGBEE_RF_QUALITY_CLUSTER_SERVER
TypeDiagnosticInfo diagnosticInfo;
#endif
#ifdef ZCL_USING_ZIGBEE_RF_QUALITY_CLUSTER_CLIENT
  TypeServerClientDiagnosticInfo diagnosticFieldInfo[MAXIMUM_TESTRF_DEVICES];
#endif



int32_t signedDivision(int32_t dividend, int32_t divisor)
{
  bool neg = false;
  int32_t result;
  if (dividend < 0) {
    neg = !neg;
    dividend = -dividend;
  }
  if (divisor < 0) {
    neg = !neg;
    divisor = -divisor;
  }
  if (divisor == 0) {
    if (neg)  return 0x80000000;
    else      return 0x7FFFFFFF;
  }
  dividend += divisor / 2;      //rounding
  result = dividend / divisor;

  if (neg) result = -result;

  return result;
}

/** @brief Pre Message Received
 *
 * This callback is the first in the Application Framework's message processing
 * chain. The Application Framework calls it when a message has been received
 * over the air but has not yet been parsed by the ZCL command-handling code. If
 * you wish to parse some messages that are completely outside the ZCL
 * specification or are not handled by the Application Framework's command
 * handling code, you should intercept them for parsing in this callback.

 *   This callback returns a Boolean value indicating whether or not the message
 * has been handled. If the callback returns a value of true, then the
 * Application Framework assumes that the message has been handled and it does
 * nothing else with it. If the callback returns a value of false, then the
 * application framework continues to process the message as it would with any
 * incoming message.
        Note:   This callback receives a pointer to an
 * incoming message struct. This struct allows the application framework to
 * provide a unified interface between both Host devices, which receive their
 * message through the ezspIncomingMessageHandler, and SoC devices, which
 * receive their message through emberIncomingMessageHandler.
 *
 * @param incomingMessage   Ver.: always
 */

/*
 boolean emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage) {
  signalQuality.actualRssi = incomingMessage->lastHopRssi;
  signalQuality.actualLqi = incomingMessage->lastHopLqi;
  StatusLedTelegramReceived();
  return false;
}
*/

#ifdef ZCL_USING_ZIGBEE_RF_QUALITY_CLUSTER_SERVER
//
/** @brief Zigbee Diagnostic Cluster Cluster Server Init
 *
 * Server Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfZigbeeRfQualityClusterServerInitCallback(int8u endpoint){
  diagnosticInfo = diagnosticInfoDEFAULT;
}

/** @brief Zigbee Diagnostic Cluster Cluster Reset Statistics
 *
 *
 *
 */
boolean emberAfZigbeeRfQualityClusterResetStatisticsCallback(void){
  diagnosticInfo = diagnosticInfoDEFAULT;
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

/** @brief Zigbee Diagnostic Cluster Cluster Test R F
 *
 *
 *
 * @param argOne   Ver.: always
 */
boolean emberAfZigbeeRfQualityClusterTestRFCallback(int16u argOne){

  EmberStatus status;

  diagnosticInfo.rssi_act   = signalQuality.actualRssi;
  diagnosticInfo.lqi_act = signalQuality.actualLqi;
  diagnosticInfo.rssi_sum += signalQuality.actualRssi;
  diagnosticInfo.lqi_sum += signalQuality.actualLqi;
  diagnosticInfo.rxCounter++;
  int8_t avg_rssi = (int8_t) signedDivision(diagnosticInfo.rssi_sum, diagnosticInfo.rxCounter);
  uint8_t avg_lqi = (uint8_t) signedDivision(diagnosticInfo.lqi_sum, diagnosticInfo.rxCounter);;
  if (diagnosticInfo.rssi_min > signalQuality.actualRssi) diagnosticInfo.rssi_min = signalQuality.actualRssi;
  if (diagnosticInfo.rssi_max < signalQuality.actualRssi) diagnosticInfo.rssi_max = signalQuality.actualRssi;
  if (diagnosticInfo.lqi_min > signalQuality.actualLqi) diagnosticInfo.lqi_min = signalQuality.actualLqi;
  if (diagnosticInfo.lqi_max < signalQuality.actualLqi) diagnosticInfo.lqi_max = signalQuality.actualLqi;
  //Rssi values
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                         ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                         ZCL_ACTUAL_RSSI_ATTRIBUTE_ID,
                                         OSRAM_MANUFACTURER_CODE,
                                         (uint8_t *)&signalQuality.actualRssi,
                                         ZCL_INT8S_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                         ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                         ZCL_AVERAGE_RSSI_ATTRIBUTE_ID,
                                         OSRAM_MANUFACTURER_CODE,
                                         (uint8_t *)&avg_rssi,
                                         ZCL_INT8S_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                         ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                         ZCL_MIN_RSSI_ATTRIBUTE_ID,
                                         OSRAM_MANUFACTURER_CODE,
                                         (uint8_t *)&diagnosticInfo.rssi_min,
                                         ZCL_INT8S_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                         ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                         ZCL_MAX_RSSI_ATTRIBUTE_ID,
                                         OSRAM_MANUFACTURER_CODE,
                                         (uint8_t *)&diagnosticInfo.rssi_max,
                                         ZCL_INT8S_ATTRIBUTE_TYPE);
  //lqi values
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                           ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                           ZCL_ACTUAL_LQI_ATTRIBUTE_ID,
                                           OSRAM_MANUFACTURER_CODE,
                                           (uint8_t *)&signalQuality.actualLqi,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                           ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                           ZCL_AVERAGE_LQI_ATTRIBUTE_ID,
                                           OSRAM_MANUFACTURER_CODE,
                                           (uint8_t *)&avg_lqi,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                           ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                           ZCL_MIN_LQI_ATTRIBUTE_ID,
                                           OSRAM_MANUFACTURER_CODE,
                                           (uint8_t *)&diagnosticInfo.lqi_min,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                           ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                           ZCL_MAX_LQI_ATTRIBUTE_ID,
                                           OSRAM_MANUFACTURER_CODE,
                                           (uint8_t *)&diagnosticInfo.lqi_max,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
    //counter
  status = emberAfWriteManufacturerSpecificServerAttribute(ENDPOINT,
                                           ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                           ZCL_COUNTER_ATTRIBUTE_ID,
                                           OSRAM_MANUFACTURER_CODE,
                                           (uint8_t *)&diagnosticInfo.rxCounter,
                                           ZCL_INT16U_ATTRIBUTE_TYPE);


  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

#endif

#ifdef ZCL_USING_ZIGBEE_RF_QUALITY_CLUSTER_CLIENT

bool RfQualityClusterTestRFAnswerCallback(uint16_t indexOrDestination, uint16_t msgLen, uint8_t *message) {

  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    if (diagnosticFieldInfo[index].client.node == indexOrDestination) {
      diagnosticFieldInfo[index].client.rssi_sum += signalQuality.actualRssi;
      diagnosticFieldInfo[index].client.lqi_sum += signalQuality.actualLqi;
      diagnosticFieldInfo[index].client.rxCounter++;
      if (diagnosticFieldInfo[index].client.rssi_min > signalQuality.actualRssi) diagnosticFieldInfo[index].client.rssi_min = signalQuality.actualRssi;
      if (diagnosticFieldInfo[index].client.rssi_max < signalQuality.actualRssi) diagnosticFieldInfo[index].client.rssi_max = signalQuality.actualRssi;
      if (diagnosticFieldInfo[index].client.lqi_min > signalQuality.actualLqi) diagnosticFieldInfo[index].client.lqi_min = signalQuality.actualLqi;
      if (diagnosticFieldInfo[index].client.lqi_max < signalQuality.actualLqi) diagnosticFieldInfo[index].client.lqi_max = signalQuality.actualLqi;
      return true;
    }
 }
  return false;
  }


bool ZigbeeRfQualityClusterTestRFReadAttributesCallback(uint16_t indexOrDestination, uint16_t msgLen, uint8_t *message)
{


  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    if (diagnosticFieldInfo[index].client.node == indexOrDestination) {
      diagnosticFieldInfo[index].client.rssi_avg = (int8_t) signedDivision(diagnosticFieldInfo[index].client.rssi_sum, diagnosticFieldInfo[index].client.rxCounter);
      diagnosticFieldInfo[index].client.lqi_avg = (uint8_t) signedDivision(diagnosticFieldInfo[index].client.lqi_sum, diagnosticFieldInfo[index].client.rxCounter);

      uint8_t *buffer = message + 3;
      uint8_t bufLen = msgLen -3;
      uint16_t bytesParsed = 0;
      while ((bufLen - bytesParsed) > 0) {
        ReadAttributeStatusRecord *attributeRecord = (ReadAttributeStatusRecord*) (buffer + bytesParsed);
        switch(attributeRecord->attributeId) {
          case ZCL_ACTUAL_RSSI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.rssi_act = (int8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_AVERAGE_RSSI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.rssi_avg = (int8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_MIN_RSSI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.rssi_min = (int8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_MAX_RSSI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.rssi_max = (int8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_ACTUAL_LQI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.lqi_act = (uint8_t)attributeRecord->attributeLocation;
               bytesParsed+= 5;
             }
             break;
          case ZCL_AVERAGE_LQI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.lqi_avg = (uint8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_MIN_LQI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.lqi_min = (uint8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_MAX_LQI_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.lqi_max = (uint8_t)attributeRecord->attributeLocation;
              bytesParsed+= 5;
            }
            break;
          case ZCL_COUNTER_ATTRIBUTE_ID: {
              diagnosticFieldInfo[index].server.rxCounter  =  (uint16_t)attributeRecord->attributeLocation;
              bytesParsed+= 6;
            }
            break;
          default:
            bytesParsed = bufLen; // Don't know what to do, exit loop.
        }
      }
      return true;
    }
  }
  return false;
}

void ZigbeeRfQualityClusterPrintResult(TypeServerClientDiagnosticInfo *data) {

  emberAfZigbeeRfQualityClusterPrint("0x%2x    ", data->server.node);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->server.rssi_avg);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->server.rssi_min);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->server.rssi_max);
  emberAfZigbeeRfQualityClusterPrint("%5d     "  , data->server.rxCounter);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->client.rssi_avg);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->client.rssi_min);
  emberAfZigbeeRfQualityClusterPrint("%5d     ", data->client.rssi_max);
  emberAfZigbeeRfQualityClusterPrint("%5d     "  , data->client.rxCounter);
  emberAfZigbeeRfQualityClusterPrintln("");
}

void ZigbeeRfQualityClusterPrintAllResult() {
  emberAfZigbeeRfQualityClusterPrintln("Source   S-AvgRssi S-MinRssi S-MaxRssi S-Count   C-AvgRssi C-MinRssi C-MaxRssi C-Count");
  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    if (diagnosticFieldInfo[index].client.node != 0xFFFFFFFF) {
      ZigbeeRfQualityClusterPrintResult(&diagnosticFieldInfo[index]);
    }
  }
}

void ZigbeeRfQualityClusterClientResetStatistics(uint16_t indexOrDestination) {
  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    if (diagnosticFieldInfo[index].client.node == indexOrDestination) {
      diagnosticFieldInfo[index].client.rssi_sum = 0;
      diagnosticFieldInfo[index].client.rssi_avg = 0;
      diagnosticFieldInfo[index].client.rssi_min = 127;
      diagnosticFieldInfo[index].client.rssi_max = -127;
      diagnosticFieldInfo[index].client.lqi_sum = 0;
      diagnosticFieldInfo[index].client.lqi_avg = 0;
      diagnosticFieldInfo[index].client.lqi_min = 127;
      diagnosticFieldInfo[index].client.lqi_max = -127;
      diagnosticFieldInfo[index].client.rxCounter = 0;
      diagnosticFieldInfo[index].server.rssi_sum = 0;
      diagnosticFieldInfo[index].server.rssi_avg = 0;
      diagnosticFieldInfo[index].server.rssi_min = 127;
      diagnosticFieldInfo[index].server.rssi_max = -127;
      diagnosticFieldInfo[index].server.lqi_sum = 0;
      diagnosticFieldInfo[index].server.lqi_avg = 0;
      diagnosticFieldInfo[index].server.lqi_min = 127;
      diagnosticFieldInfo[index].server.lqi_max = -127;
      diagnosticFieldInfo[index].server.rxCounter = 0;
    }
  }
}

void ZigbeeRfQualityClusterClientAddDevice(uint16_t indexOrDestination) {
  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    if (diagnosticFieldInfo[index].client.node == 0xFFFFFFFF) {  // insert at first free position
      diagnosticFieldInfo[index].client.node = indexOrDestination;
      diagnosticFieldInfo[index].server.node = indexOrDestination;
      return;
    }
  }
}

void ZigbeeRfQualityClusterClientRemoveAllDevices(void) {
  for (int index = 0; index < MAXIMUM_TESTRF_DEVICES; index++) {
    diagnosticFieldInfo[index].client = diagnosticInfoDEFAULT;
    diagnosticFieldInfo[index].client.node = 0xFFFFFFFF;
    diagnosticFieldInfo[index].server = diagnosticInfoDEFAULT;
    diagnosticFieldInfo[index].server.node = 0xFFFFFFFF;
    }
  }

/** @brief Zigbee Diagnostic Cluster Cluster Client Init
 *
 * Client Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfZigbeeRfQualityClusterClientInitCallback(int8u endpoint){
  ZigbeeRfQualityClusterClientRemoveAllDevices();
}


#endif
