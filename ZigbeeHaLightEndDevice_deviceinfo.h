//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file HaLightEndDevice_deviceinfo.h
///   \since 2016-11-30
///   \brief Adapting Interface Services for all Zigee clusters which need Dali communication
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define APPLICATION_VERSION   0x00
#define MANUFACTURER_NAME     "Osram Sylvania"
#define MODELIDENTIFIER       "NAED 56340-00/00"
#define POWERSOURCE           EMBER_ZCL_POWER_SOURCE_SINGLE_PHASE_MAINS
#define IMAGE_TYPE            0xE01E  // must be unique within the manufacturer
#define FW_VERSION            0x0012  // 0.0.1.2
#define HW_VERSION_MAJOR      0x05   // Reported in Basic Cluster
#define HW_VERSION_MINOR      0x00    // with HW_VERSION_MAJOR used in OTA Cluster


