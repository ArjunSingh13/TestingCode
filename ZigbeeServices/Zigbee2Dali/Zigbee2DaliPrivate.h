//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file Zigbee2DaliPrivate.h
///   \since 2016-11-17
///   \brief Definitions and typedefs private for Zigbee2Dali
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __ZIGBEE2DALIPRIVATE_H__
#define __ZIGBEE2DALIPRIVATE_H__

// --------- Includes ----------------------------------------------------------

#include "GlobalDefs.h"

// --------- Macro Definitions -------------------------------------------------

#define Z2D_LIGHT_POLL_TIME_FTT             (2000/2.5)                           // query each 500 ms
#define Z2D_OFFLINE_FAILURE_COUNTER         3                                    // 3 * Z2D_LIGHT_POLL_TIME_FTT = 6 secs

#define DALI_TIME_IDENTIFY_TOGGLE_FTT       (uint32_t)(1500/2.5)                // interval for manual identify on Ed1.
#define DALI_TIME_IDENTIFY_RESEND_FTT       (uint32_t)(5000/2.5)                // resend interval for IDENTIFY_START

#define Z2D_EMERGENCY_POLL_TIME_FTT         (1000/2.5)                           // query each 1000 ms

// --------- Type Definitions --------------------------------------------------

typedef struct StructDaliCommand TypeDaliCommand;

typedef struct{
  bool leave;
  uint8_t stateCounter;
  uint16_t busDownCounter_FTT;
  uint16_t busOkCounter_FTT;
} TypeNetworkService;

typedef struct{
  //uint8_t actIdx;
  uint8_t  targetLevel;
  uint32_t delay_FTT;                                                           // reload value (further delays for sending, if more than 2 DAPC-commands are needed)
  uint32_t delayCounter_FTT;                                                    // send delay after 1st target
  uint32_t fadeCounter_FTT;                                                     // downCounter while device fades
  uint32_t fadeTime_FTT;                                                        // fadeTime may be used to calculate actLevel while fading, always has the initial value of fadeCounter_FTT
  uint16_t fadeTimeDali_0sx_next;                                               // total time elapsed in the current fade cycle, used to calculate next fade target level
  uint16_t fadeTimeDali_0sx;                                                    // Time (in x0s) equivalent to the DALI supported fade time
  uint16_t fadeTimeZigbee_0sx;
  bool fading;
} TypeLightControl;

typedef struct{
  bool on;
  uint16_t delay_FTT;
} TypeIdentify;

typedef enum {
  REFRESH_MODE_IDLE,
  REFRESH_MODE_UPDATE_TARGET_LEVEL,
  REFRESH_MODE_QUERY_COLOR_TEMP,
  REFRESH_MODE_QUERY_STATUS,
  REFRESH_MODE_QUERY_LAMPFAILURE,
  REFRESH_MODE_COUNT
} TypeRefreshMODE;

typedef struct{
  TypeRefreshMODE state;
  TypeRefreshMODE nextState;
  uint16_t refreshTimer_FTT;
}TypeRefreshStatus;

typedef enum {
  DEVICE_MODE_INIT,
  DEVICE_MODE_START,
  DEVICE_MODE_NORMAL,
  DEVICE_MODE_FADING,
  DEVICE_MODE_IDENTIFY,
  DEVICE_MODE_FAILURE,
  DEVICE_MODE_INACTIVE
} TypeDeviceMODE;

typedef struct{
  bool active;
  TypeDeviceMODE mode;                                                          // init by Init() to DEVICE_MODE_INIT
  uint8_t daliVersion;
  uint8_t address;
  bool deviceType1Supported;
  bool deviceType6Supported;
  bool deviceType8Supported;
  uint8_t actLevel;                                                             // init by Init() to UINT8_MASK
  uint8_t minLevel;                                                             // init by Init() to ??
  uint8_t fadeTime;                                                             // init by Init() to ??, The supported DALI fade time value
  uint16_t actMirek;
  uint16_t colorTempMin;
  uint16_t colorTempMax;
  TypeLightControl light;                                                       // init to all 0
  TypeIdentify identify;                                                        // init to all 0
  uint8_t ecgStatus;
  int8_t errorCounter;
} TypeDevice;


typedef struct{
  bool active;
  uint8_t mode;
  uint8_t addressPD;
  uint8_t addressLight;
  uint16_t pollTime_FTT;
  struct {
    union {
      uint16_t full;
      struct {
        uint8_t low;
        uint8_t high;
      } bytes;
    } actValue;
    uint16_t lastValue;
    int32_t lastDelta;
    bool reverseWinkTriggered;
  } lightSensor;
  int8_t errorCounter;
} TypeSensor;


typedef struct{
  bool active;
  uint8_t daliVersion;
  uint8_t address;
  uint16_t pollTime_FTT;
  uint8_t testExecutionTimeout;
  uint8_t batteryCharge;
  uint8_t level;
  uint8_t mode;
  uint8_t features;
  uint8_t failureStatus;
  uint8_t status;
  int8_t errorCounter;
} TypeEmergency;


typedef enum {
  DALI_MODE_INIT,
  DALI_MODE_DEVICE_ADDRESSING,
  DALI_MODE_START,
  DALI_MODE_NORMAL,
  DALI_MODE_MEMBANK,
  DALI_MODE_BUS_FAILURE,
  DALI_MODE_MFG_TEST
} TypeDaliMODE;

typedef struct {
//  uint8_t activeDevices;                                                        // init to 0
  bool ecgBroadcastMode;
  TypeDaliMODE mode;
  TypeRefreshStatus refresh;
  TypeDevice device[ZIGBEE2DALI_ECG_MAX];                                       // init by loop in Init() because of flexible number
  TypeSensor sensor;
  TypeNetworkService networkService;                                            // init to all 0, except leave=false
  uint32_t powerOnCounter_FTT;                                                  // wait until a Dali-ECG must be ready to receive, before ceating a failure (IEC FDIS 62386-101, 4.11.6 System start-up timing)
  TypeEmergency emergency;
} TypeZigbee2Dali;


// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

#endif // __ZIGBEE2DALIPRIVATE_H__
