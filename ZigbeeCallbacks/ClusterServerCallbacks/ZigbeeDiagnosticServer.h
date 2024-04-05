//------------------------------------------------------------------------------
///   \file   ZigbeeDiagnosticServer.h
///   \since  2017-02-09
///   \brief  Definition for OsramSpecific Diagnostic Cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//------------------------------------------------------------------------------

#define MAXIMUM_TESTRF_DEVICES         10

typedef struct {
  uint32_t node;
  uint8_t endpoint;
  uint32_t rxCounter;
  int32_t rssi_sum;
  int8_t rssi_min;
  int8_t rssi_max;
  int8_t rssi_avg;
  int8_t rssi_act;
  uint32_t lqi_sum;
  uint8_t lqi_min;
  uint8_t lqi_max;
  uint8_t lqi_avg;
  uint8_t lqi_act;
} TypeDiagnosticInfo;

typedef struct {
  int8_t actualRssi;
  uint8_t actualLqi;
} TypeSignalQuality;


typedef struct {
  TypeDiagnosticInfo  server;
  TypeDiagnosticInfo  client;
} TypeServerClientDiagnosticInfo;

void ZigbeeRfQualityClusterClientResetStatistics(uint16_t indexOrDestination);
void ZigbeeRfQualityClusterPrintResult(TypeServerClientDiagnosticInfo *data);
