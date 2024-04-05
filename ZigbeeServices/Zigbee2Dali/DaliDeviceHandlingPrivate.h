//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliDeviceHandlingPrivate.h
///   \since 2017-05-08
///   \brief Interface for all Dali Addressing and DeviceScan
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALIDEVICEHANDLINGPRIVATE_H__
#define __DALIDEVICEHANDLINGPRIVATE_H__

// --------- Includes ----------------------------------------------------------

// --------- Macro Definitions -------------------------------------------------
#define DALI_DEVICE_EDITION2                        0x00000001L
#define DALI_DEVICETYPE1                            0x00010000L
#define DALI_DEVICETYPE6                            0x00020000L
#define DALI_DEVICETYPE8                            0x00040000L
#define DALI_DEVICETYPE7							0x00080000L
#define DALI_DEVICETYPE0							0x00100000L
#define DALI_FEATURE_ALL                            0x001F0001L

#define STARTUP_DELAY                               10000
#define DEFAULT_STATE_CHANGE_DELAY                  1000
// --------- Type Definitions --------------------------------------------------

typedef enum {
  DEVICE_HANDLING_IDLE,
  DEVICE_HANDLING_START_ADDRESSING,
  DEVICE_HANDLING_DELETE_ADDRESSES,
  DEVICE_HANDLING_START_SEARCHING_EXISTING_BALLASTS,
  DEVICE_HANDLING_SET_EXISTING_BALLAST_RANDOM_ADDRESS,
  DEVICE_HANDLING_PROGRAM_EXISTING_BALLAST_SHORTADDR,
  DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_BALLAST_RESULT,
  DEVICE_HANDLING_TERMINATE_EXISTING_BALLAST_SEARCH,
  DEVICE_HANDLING_START_ADDRESSINTG_NEW_BALLAST,
  DEVICE_HANDLING_INIT_BALLAST_SEARCH,
  DEVICE_HANDLING_WAIT4_COMPARE_NEWBALLAST_RESULT,
  DEVICE_HANDLING_PROGRAM_BALLAST_SHORTADDR,
  DEVICE_HANDLING_WAIT4_VERIFY_NEWBALLAST_RESULT,
  DEVICE_HANDLING_TERMINATE_NEW_BALLAST_SEARCH,
  DEVICE_HANDLING_BALLAST_ADDRESSING_FINISHED,
  DEVICE_HANDLING_START_IDENTIFY_BALLAST,
  DEVICE_HANDLING_GET_BALLAST_DALIVERSION,
  DEVICE_HANDLING_WAIT4_BALLAST_DALIVERSION_RESULT,
  DEVICE_HANDLING_GET_BALLAST_DEVICETYPE,
  DEVICE_HANDLING_WAIT4_BALLAST_DEVICETYPE_RESULT,
  DEVICE_HANDLING_GET_BALLAST_NEXTDEVICETYPE,
  DEVICE_HANDLING_WAIT4_BALLAST_NEXTDEVICETYPE_RESULT,
  DEVICE_HANDLING_GET_BALLAST_PHYSMIN,
  DEVICE_HANDLING_WAIT4_BALLAST_PHYSMIN_RESULT,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_RESULT,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB_RESULT,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_RESULT,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB,
  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB_RESULT,
  DEVICE_HANDLING_BALLAST_GET_MEMBANK0,
  DEVICE_HANDLING_BALLAST_GET_MEMBANK0_WAIT4FINISHED,
  DEVICE_HANDLING_BALLAST_IDENTIFICATION_FINISHED,
  DEVICE_HANDLING_INPUTDEVICE_QUERY_STATUS,
  DEVICE_HANDLING_WAIT4_INPUTDEVICE_QUERY_STATUS_RESULT,
  DEVICE_HANDLING_START_SEARCHING_EXISTING_INPUTDEVICE,
  DEVICE_HANDLING_SET_EXISTING_INPUTDEVICE_RANDOM_ADDRESS,
  DEVICE_HANDLING_PROGRAM_EXISTING_INPUTDEVICE_SHORTADDR,
  DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_INPUTDEVICE_RESULT,
  DEVICE_HANDLING_TERMINATE_EXISTING_INPUTDEVICE_SEARCH,
  DEVICE_HANDLING_START_ADDRESSINTG_NEW_INPUTDEVICE,
  DEVICE_HANDLING_CONFIGURE_INPUTDEVICE,
  DEVICE_HANDLING_INIT_INPUTDEVICE_SEARCH,
  DEVICE_HANDLING_WAIT4_COMPARE_NEW_INPUTDEVICE_RESULT,
  DEVICE_HANDLING_PROGRAM_INPUTDEVICE_SHORTADDR,
  DEVICE_HANDLING_WAIT4_QUERY_INPUTDEVICE_TYPE_RESULT,
  DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0,
  DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0_WAIT4FINISHED,
  DEVICE_HANDLING_TERMINATE_NEW_INPUTDEVICE_SEARCH,
  DEVICE_HANDLING_GET_EMERGENCY_TEST_EXECUTION_TIMEOUT,
  DEVICE_HANDLING_WAIT4_EMERGENCY_TEST_EXECUTION_TIMEOUT_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_BATTERY_CHARGE,
  DEVICE_HANDLING_WAIT4_EMERGENCY_BATTERY_CHARGE_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_LEVEL,
  DEVICE_HANDLING_WAIT4_EMERGENCY_LEVEL_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_MODE,
  DEVICE_HANDLING_WAIT4_EMERGENCY_MODE_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_FEATURES,
  DEVICE_HANDLING_WAIT4_EMERGENCY_FEATURES_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_FAILURE_STATUS,
  DEVICE_HANDLING_WAIT4_EMERGENCY_FAILURE_STATUS_RESULT,
  DEVICE_HANDLING_GET_EMERGENCY_STATUS,
  DEVICE_HANDLING_WAIT4_EMERGENCY_STATUS_RESULT,
  DEVICE_HANDLING_ADDRESSING_CANCEL,
  DEVICE_HANDLING_ADDRESSING_FINISHED,
  DEVICE_HANDLING_DELAY_SET_SHORT_ADDRESS,
  DEVICE_HANDLING_DELAY_RESET_CMD_DALI_BUS,
  DEVICE_HANDLING_VERIFY_EXISTING_BALLAST_SHORTADDR
} TypeDeviceHandlingState;

typedef struct {
    uint8_t bytes[6];
} TypeGtin;

typedef union {
    uint16_t raw;
    struct {
      uint8_t low;
      uint8_t high;
    } byte;
} TypeDaliDeviceVersion;

typedef struct {
  bool broadcastMode;
  uint8_t address[2];
  uint32_t deviceTypeMask[2];
  TypeSearchAddress searchAddress[2];
} TypeEcgParams;

typedef struct {
  TypeGtin gtin;
  TypeDaliDeviceVersion version;
} TypeDaliDeviceInfo;

typedef struct {
  bool active;
  uint8_t address[2];
  uint32_t deviceTypeMask[2];
  TypeSearchAddress searchAddress[2];
} TypeSensorParams;

typedef struct {
  bool active;
  uint8_t address;
  uint32_t deviceTypeMask;
  TypeSearchAddress searchAddress;
} TypeEmergencyParams;

typedef struct {
  uint8_t numOfDevices;
  uint8_t newDeviceMask[8];
  int8_t  actualShortAddress;
  TypeDeviceHandlingState state;
  TypeSearchAddress lastSearchAddress;
  TypeSearchAddress actualSearchAddress;
  uint8_t testingBit;
  int8_t index;
  TypeEcgParams ecgParams;
  TypeDaliDeviceInfo ecgInfo[2];
  TypeSensorParams sensorParams;
  TypeDaliDeviceInfo sensorInfo[2];
  uint32_t  tempDeviceFeatureMask;
  uint32_t  deviceFeatureMask;
  uint8_t  physMinLevel;
  uint8_t  tempPhysMinLevel;
  uint16_t colorTempPhysCoolestMirek;
  uint16_t colorTempPhysWarmestMirek;
  uint32_t  timeout;
  uint32_t deviceConfig;
  TypeEmergencyParams emergencyParams;
  TypeDaliDeviceInfo emergencyInfo;
  uint16_t stateChangeDelay;
} TypeDaliDeviceHandling;

typedef struct {
    TypeGtin gtin;
    uint32_t scalingFactor;
} TypeDaliSensorScalingFactor;

typedef enum {
  INPUT_VERIFY_START,
  INPUT_VERIFY_INPUT_TYPE
}TypeInputVerifyState;

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

#endif // __DALIDEVICEHANDLINGPRIVATE_H__
