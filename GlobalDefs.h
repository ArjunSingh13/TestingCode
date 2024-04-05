//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   GlobalDefs.h
///   \since  2016-10-27
///   \brief  Definitions of types and values for global use in Zigbee HaLightEndDevices (Zigbee2Dali converter, Zigbee4OT)
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __GLOBALDEFS_H__
#define __GLOBALDEFS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OSRAM_MANUFACTURER_CODE                         0x110c

#define NETWORK_JOINED_IDENTIFICATION_TIME                  30                  // identify time 30s if joined
#define NETWORK_PERMIT_JOINING_IDENTIFICATION_TIME           3                  // identify 3s as feedback for LeaveNetwork

#define ZIGBEE2DALI_SAK_ENDPOINT                             1
#define ZIGBEE2DALI_REPEATER_ENDPOINT                        2
#define ZIGBEE2DALI_LIGHT_OUT_ENDPOINT                       10
#define ZIGBEE2DALI_ECG_MAX                                  2
#define ZIGBEE2DALI_LIGHT_SENSOR_MAX                         1
#define ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT                    21
#define ZIGBEE2DALI_PRESENCE_SENSOR_MAX                      1
#define ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT                 20
#define ZIGBEE2DALI_EMERGENCY_ENDPOINT                       50

#define ZIGBEE_MIN_REPORTING_INTERVAL_S                      2
#define ZIGBEE_MAX_REPORTING_INTERVAL_S                     60

#define REPEATERMODE_ACTIVE                                 0x00000001L
#define PUSHBUTTON_ENDPOINT_ACTIVE                          0x00000100L

#define MS2TICKS(a)                         (((uint32_t) a * TICKS_PER_S)/1000)
#define TICKS_PER_S                         1000

#define GLOBAL_FastTIME_TICK_ms               (2.500)                           // Basic system time slice in [ms]
#define GLOBAL_SlowTIME_TICK_ms               (10.0*GLOBAL_FastTIME_TICK_ms)

#define FINDING_AND_BINDING_DELAY_MS          3000

//#define FADE_TEST
#define DALI_MULTI_MASTER
//#define DALI_MULTI_MASTER_TEST

//extern const TypeVersion fwVersion;
//extern TypeZigbeeString swBuildVersion;


// Global definitions, to be used instead of the numbers (avoid bugs by using wrong numbers)
#define GLOBAL_UINT32_MAX     0xFFFFFFFF
#define GLOBAL_UINT16_MAX     0xFFFF
#define GLOBAL_UINT8_MAX      0xFF
#define GLOBAL_INT32_MAX      0x7FFFFFFF
#define GLOBAL_INT16_MAX      0x7FFF
#define GLOBAL_INT8_MAX       0x7F
#define GLOBAL_INT32_MIN      0x80000000
#define GLOBAL_INT16_MIN      0x8000
#define GLOBAL_INT8_MIN       0x80

#endif // __GLOBALDEFS_H__
