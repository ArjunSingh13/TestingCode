/*
 * app_tokens_def.h
 *
 *  Created on: 04.10.2017
 *      Author: f.mamo
 */

// mfg test tokens
#include "../mfgtest-dut/tokens_def.h"

#define CREATOR_ECG_INIT_PARAMS           0x4801
#define NVM3KEY_ECG_INIT_PARAMS           0x4801

#define CREATOR_SENSOR_INIT_PARAMS        0x4802
#define NVM3KEY_SENSOR_INIT_PARAMS        0x4802

#define CREATOR_LINEARIZATION_TABLE_10    0x4803
#define NVM3KEY_LINEARIZATION_TABLE_10    0x4803

#define CREATOR_LINEARIZATION_TABLE_11    0x4804
#define NVM3KEY_LINEARIZATION_TABLE_11    0x4804

#define CREATOR_DEVICE_CONFIG             0x4805
#define NVM3KEY_DEVICE_CONFIG             0x4805

#define CREATOR_ECG_INFO                  0x4806
#define NVM3KEY_ECG_INFO                  0x4806

#define CREATOR_SENSOR_INFO               0x4807
#define NVM3KEY_SENSOR_INFO               0x4807

#define CREATOR_EMERGENCY_INIT_PARAMS     0x4808
#define NVM3KEY_EMERGENCY_INIT_PARAMS     0x4808

#define CREATOR_EMERGENCY_INFO            0x4809
#define NVM3KEY_EMERGENCY_INFO            0x4809

// Default Values
#define ECG_INIT_PARAMS_DEFAULT           {0, 0xFF, 0xFF, 0xFF, 0xFF}
#define SENSOR_INIT_PARAMS_DEFAULT        {0, 0xFF, 0xFF, 0xFF, 0xFF}
#define EMERGENCY_INIT_PARAMS_DEFAULT     {0, 0xFF, 0xFF, 0x00FFFFFF}
#define DEVICE_CONFIG_DEFAULT             0x00000000
#define LINEARIZATION_TABLE_DEFAULT       {0,625,1250,1875,2500,3125,3750,4375,5000}
#define DALI_DEVICE_INFO_DEFAULT          {.gtin={0, 0, 0, 0, 0, 0}, .version=0}

#ifdef DEFINETYPES
typedef union {
  uint32_t raw;
  struct {
    uint32_t low:8;
    uint32_t mid:8;
    uint32_t high:8;
    uint32_t res:8;
  } bytes;
} TypeSearchAddress;

typedef struct {
  bool broadcastMode;
  uint8_t address[2];
  uint32_t deviceTypeMask[2];
  TypeSearchAddress searchAddress[2];
} typeEcgInitParams;

typedef struct {
  bool active;
  uint8_t address[2];
  uint32_t deviceTypeMask[2];
  TypeSearchAddress searchAddress[2];
} typeSensorInitParams;

typedef struct {
  bool active;
  uint8_t address;
  uint32_t deviceTypeMask;
  TypeSearchAddress searchAddress;
} typeEmergencyInitParams;

typedef struct {
  uint16_t address[9];
} typeLinearizationTable;

typedef struct {
  uint8_t gtin[6];
  uint16_t version;
} typeDaliInfo;

typedef struct {
  typeDaliInfo ecgInfo[2];
} typeEcgInfo;

typedef struct {
  typeDaliInfo sensorInfo[2];
} typeSensorInfo;

typedef typeDaliInfo typeEmergencyInfo;

#endif

#ifdef DEFINETOKENS
DEFINE_BASIC_TOKEN(ECG_INIT_PARAMS,        typeEcgInitParams,      ECG_INIT_PARAMS_DEFAULT)
DEFINE_BASIC_TOKEN(SENSOR_INIT_PARAMS,     typeSensorInitParams,   SENSOR_INIT_PARAMS_DEFAULT)
DEFINE_BASIC_TOKEN(LINEARIZATION_TABLE_10, typeLinearizationTable, LINEARIZATION_TABLE_DEFAULT)
DEFINE_BASIC_TOKEN(LINEARIZATION_TABLE_11, typeLinearizationTable, LINEARIZATION_TABLE_DEFAULT)
DEFINE_BASIC_TOKEN(DEVICE_CONFIG,          uint32_t,               DEVICE_CONFIG_DEFAULT)
DEFINE_BASIC_TOKEN(ECG_INFO,               typeEcgInfo,            {0,0,0,0,0,0, 0, 0,0,0,0,0,0, 0})
DEFINE_BASIC_TOKEN(SENSOR_INFO,            typeSensorInfo,         {0,0,0,0,0,0, 0, 0,0,0,0,0,0, 0})
DEFINE_BASIC_TOKEN(EMERGENCY_INIT_PARAMS,  typeEmergencyInitParams,EMERGENCY_INIT_PARAMS_DEFAULT)
DEFINE_BASIC_TOKEN(EMERGENCY_INFO,         typeEmergencyInfo,      {0,0,0,0,0,0, 0})
#endif // DEFINETOKENS

#define APPTOKENS_SET2DEFAULTS() do { \
   typeEcgInitParams ecgInitParams = ECG_INIT_PARAMS_DEFAULT; \
   halCommonSetToken(TOKEN_ECG_INIT_PARAMS, &ecgInitParams); \
   typeSensorInitParams sensorInitParams = ECG_INIT_PARAMS_DEFAULT; \
   halCommonSetToken(TOKEN_SENSOR_INIT_PARAMS, &sensorInitParams); \
   typeLinearizationTable linearizationTableInit = LINEARIZATION_TABLE_DEFAULT; \
   halCommonSetToken(TOKEN_LINEARIZATION_TABLE_10, &linearizationTableInit); \
   halCommonSetToken(TOKEN_LINEARIZATION_TABLE_11, &linearizationTableInit); \
   uint32_t deviceConfig = DEVICE_CONFIG_DEFAULT; \
   halCommonSetToken(TOKEN_DEVICE_CONFIG, &deviceConfig); \
   typeEcgInfo ecgInfo = {.ecgInfo[0]=DALI_DEVICE_INFO_DEFAULT, .ecgInfo[1]=DALI_DEVICE_INFO_DEFAULT}; \
   halCommonSetToken(TOKEN_ECG_INFO, &ecgInfo); \
   typeSensorInfo sensorInfo = {.sensorInfo[0]=DALI_DEVICE_INFO_DEFAULT, .sensorInfo[1]=DALI_DEVICE_INFO_DEFAULT};\
   halCommonSetToken(TOKEN_SENSOR_INFO, &sensorInfo); \
   typeEmergencyInitParams emergencyInitParams = EMERGENCY_INIT_PARAMS_DEFAULT; \
   halCommonSetToken(TOKEN_EMERGENCY_INIT_PARAMS, &emergencyInitParams); \
   typeEmergencyInfo emergencyInfo = DALI_DEVICE_INFO_DEFAULT; \
   halCommonSetToken(TOKEN_EMERGENCY_INFO, &emergencyInfo); \
} while(false)
