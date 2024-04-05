//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliDeviceHandling.c
///   \since 2017-05-08
///   \brief Services for all Dali Addressing and DeviceScan
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \defgroup DALIdevice DALI Addressing
/// \ingroup Zigbee2DALI
/// @{
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// --------- Includes ----------------------------------------------------------
#include <stdio.h>
#include "GlobalDefs.h"
#include "app/framework/include/af.h"
#include "app/framework/plugin/osramds-types/osramds-types.h"
#include "app/framework/plugin/osramds-basic-server/BasicServer.h"
#include "app/framework/util/attribute-storage.h"
#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)                               // The Encelium plugins are used
  #include "EnceliumInterfaceServer.h"
#endif
#include "IlluminanceMeasurementServices.h"
#include "DaliCommands.h"
#include "DaliTelegram.h"
#include "Zigbee2Dali.h"
//#include "daliConversions.h"
#include "DaliDeviceHandling.h"
#include "MemoryBank.h"
#include "DaliDeviceHandlingPrivate.h"
#include "ZigbeeHaLightEndDevice_own.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF, __VA_ARGS__);}

#if 1
#define useptr
#endif

// --------- Type Definitions --------------------------------------------------

// --------- Local Variables ---------------------------------------------------

const TypeDaliSensorScalingFactor scalingFactorTable[] = {
    // <GTIN (Hex)>,                   <scaling factor (*0x1000)> <scaling factor>,  <sensor name>,                 <GTIN>
    { {0x03, 0xAF, 0xA3, 0xA9, 0x20, 0xB4}, 0x00063333 },      // 6.2             DALI LS/PD CI                  4052899930292
    { {0x03, 0xAF, 0xA3, 0x9B, 0x9A, 0x72}, 0x00063333 },      // 6.2             DALI LS/PD LI                  4052899043954
    { {0x03, 0xAF, 0xA3, 0xA9, 0xF4, 0xE0}, 0x00096666 },      // 9.4             DALI LS/PD LI UF (ultra flat)  4052899984608
    { {0x03, 0xA5, 0x42, 0x92, 0x6B, 0xC5}, 0x00038000 },      // 3.5             Sensor Coupler                 4008321379269
    { {0x03, 0xAF, 0xA3, 0x9D, 0x18, 0x60}, 0x0005999A },      // 5.6             Sensorcoupler PS (HF)          4052899141728
    { {0x03, 0xA5, 0x42, 0x98, 0x72, 0x34}, 0x000F6666 },      // 15.4            DALI HIGHBAY ADAPTER           4008321774132
};


// --------- Global Variables --------------------------------------------------

TypeDaliDeviceHandling daliDeviceHandling = { .state = DEVICE_HANDLING_IDLE,
                                              .ecgParams = ECG_INIT_PARAMS_DEFAULT,
                                              .sensorParams = SENSOR_INIT_PARAMS_DEFAULT,
                                              .emergencyParams = EMERGENCY_INIT_PARAMS_DEFAULT,
                                              .stateChangeDelay =  STARTUP_DELAY};
TypeInputVerifyState inputTypeVerifyState =   INPUT_VERIFY_START;
uint8_t instanceCount = 0;

// --------- Local Function Prototypes -----------------------------------------
static int8_t DaliDeviceHandlingGetNextFreeAddress(void);
static int8_t DaliDeviceHandlingGetUsedAddress(uint8_t index);
static void DaliDeviceHandlingCleanDeviceMask(void);
static void DaliDeviceHandlingAddNewDevice(uint8_t shortAddress);
static uint8_t DaliDeviceHandlingGetFreeBallastIndex(void);
static bool DeviceHandlingHaveExistingInputDeviceRandomAddress(void);
static bool DeviceHandlingSetExistingInputDeviceRandomAddress(void);
static void DeviceHandlingSetInputDeviceSearchAddress(void);
static void DeviceHandlingSetSearchAddress(void);
static bool DeviceHandlingHaveExistingRandomAddresses(void);
static bool DeviceHandlingSetExistingRandomAddress(void);
static bool DaliDeviceHandlingVerifygShortAddressResponse(uint8_t index, TypeDaliTelegramResponse response, TypeDaliFrame command);
static void DaliDeviceResponseCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) ;
static EmberAfStatus DaliDeviceHandling_SetDevicesInfo(uint8_t endpoint, TypeDaliDeviceInfo *deviceInfo);
static EmberStatus DaliDeviceHandling_SetEndpointType(uint8_t endpoint, uint16_t zigbeeDevicetype, bool enabled);

void (*DeviceHandlingSensorInitialise) (uint8_t data);
void (*DeviceHandlingSensorReset) (void);
void (*DeviceHandlingSensorRandomize) (void);
void (*DeviceHandlingSensorSetRandomH) (uint8_t data);
void (*DeviceHandlingSensorSetRandomM) (uint8_t data);
void (*DeviceHandlingSensorSetRandomL) (uint8_t data);
void (*DeviceHandlingSensorCompare) (uint8_t safeQuery);
void (*DeviceHandlingSensorPrgShortAdd) (uint8_t addr);
void (*DeviceHandlingSensorVerifyShortAdd) (uint8_t addr);
void (*DeviceHandlingSensorQueryInputType) (uint8_t addr);
void (*DeviceHandlingSensorTerminate) (void);
void (*DeviceHandlingSensorWithdraw) (void);
void (*DeviceHandlingSensorVerifyType) (uint8_t addr, TypeDaliTelegramResponse resp);
void (*DeviceHandlingSensorReadMemoryBank) (void);
void (*DeviceHandlingSensorConfigureInputDevice) (uint8_t addr);

// --------- Local Functions ---------------------------------------------------
void DeviceHandlingOsramSensorInitialise(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_INITIALISE, data, DALI_SEND_TWICE, 0);
}

void DeviceHandlingDaliSensorInitialise(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_INITIALISE, data, DALI_SEND_TWICE, 0);
}

void DeviceHandlingOsramSensorReset(void) {
  DaliTelegram_Send3ByteCommandBroadcast(SENSOR_RESET, DALI_SEND_TWICE, DALI_DELAY_200MS);
}

void DeviceHandlingDaliSensorReset(void) {
  DaliTelegram_Send3ByteCommandBroadcast(DALI_SENSOR_RESET, DALI_SEND_TWICE, 0);
}

void DeviceHandlingOsramSensorRandomize(void) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_RANDOMIZE, 0x00, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_RANDOMIZE, 0x00, DALI_SEND_NORMAL, DALI_DELAY_200MS);
}

void DeviceHandlingDaliSensorRandomize(void) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_RANDOMIZE, 0x00, DALI_SEND_TWICE, 0);
}

void DeviceHandlingOsramSensorSetRandomH(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRH, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorSetRandomH(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SEARCHADDRH, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorSetRandomM(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRM, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorSetRandomM(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SEARCHADDRM, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorSetRandomL(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRL, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorSetRandomL(uint8_t data) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SEARCHADDRL, data, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorCompare(uint8_t safeQuery) {
  DaliTelegram_Send3ByteSpecialCommandWithCallback(SENSOR_COMPARE, 0x00, DALI_WAIT4ANSWER | safeQuery, 0, DaliDeviceResponseCallback);
}

void DeviceHandlingDaliSensorCompare(uint8_t safeQuery) {
  DaliTelegram_Send3ByteSpecialCommandWithCallback(DALI_SENSOR_COMPARE, 0x00, DALI_WAIT4ANSWER | safeQuery, 0, DaliDeviceResponseCallback);
}

void DeviceHandlingOsramSensorPrgShortAdd(uint8_t addr) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_PGM_SHORT_ADDRESS, (addr << 1) | 0x01, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorPrgShortAdd(uint8_t addr) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_PGM_SHORT_ADDRESS, addr, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorVerifyShortAdd(uint8_t addr) {
  DaliTelegram_Send3ByteSpecialCommandWithCallback(SENSOR_VERIFY_SHORT_ADDR, (addr << 1) | 0x01, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
}

void DeviceHandlingDaliSensorVerifyShortAdd(uint8_t addr) {
  DaliTelegram_Send3ByteSpecialCommandWithCallback(DALI_SENSOR_VERIFY_SHORT_ADDR, addr, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
}

void DeviceHandlingOsramSensorQueryInputType(uint8_t addr) {
  DaliTelegram_Send3ByteCommand2AddressWithCallback(addr, SENSOR_QUERY_INPUT_TYPE, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
}

void DeviceHandlingDaliSensorQueryInputType(uint8_t addr) {
  switch (inputTypeVerifyState) {
  case INPUT_VERIFY_START:
    DaliTelegram_Send3ByteCommand2AddressWithCallback(addr, DALI_SENSOR_QUERY_INSTANCES, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
    break;
  case INPUT_VERIFY_INPUT_TYPE:
    if(instanceCount > 0) {
      DaliTelegram_Send3ByteCommand2AddressWithCallback(addr, ((instanceCount-1) << 8 | DALI_SENSOR_QUERY_INSTANCE_TYPE), DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
      instanceCount--;
    }
    else {
      daliDeviceHandling.state = DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0;
    }
    break;
  }
}

void DeviceHandlingOsramSensorTerminate(void){
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorTerminate(void){
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorWithdraw(void) {
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_WITHDRAWN, 0x00, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingDaliSensorWithdraw(void) {
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_WITHDRAWN, 0x00, DALI_SEND_NORMAL, 0);
}

void DeviceHandlingOsramSensorVerifyType(uint8_t addr, TypeDaliTelegramResponse response) {
  if(response.answerType == DALI_ANSWER_OK) {
    DaliDeviceHandlingAddNewDevice(addr);
    daliDeviceHandling.sensorParams.active = true;
    switch(response.answer) {
    case 0x02:   // occupancy sensor
      daliDeviceHandling.index = 0;
      daliDeviceHandling.sensorParams.address[0] = addr;
      daliDeviceHandling.sensorParams.searchAddress[0].raw = daliDeviceHandling.actualSearchAddress.raw;
      break;
    case 0x03:  // photosensor
      daliDeviceHandling.index = 1;
      daliDeviceHandling.sensorParams.address[1] = addr;
      daliDeviceHandling.sensorParams.searchAddress[1].raw = daliDeviceHandling.actualSearchAddress.raw;
      break;
    default:
      TRACE(true,"Unknown InputDeviceType SA:%u Te:%u", addr, response.answer);
      // Nothing to do
      break;
    }
  }
  else {
    TRACE(true,"Input Device:%u NOT verified", addr);
  }
  daliDeviceHandling.state = DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0;
}

void DeviceHandlingDaliSensorVerifyType(uint8_t addr, TypeDaliTelegramResponse response) {
  if(response.answerType == DALI_ANSWER_OK) {
    switch (inputTypeVerifyState) {
    case INPUT_VERIFY_START:
      DaliDeviceHandlingAddNewDevice(addr);
      daliDeviceHandling.sensorParams.active = true;
      inputTypeVerifyState++;
      instanceCount = response.answer;
      instanceCount = (instanceCount > 2) ? 2 : instanceCount;
      DeviceHandlingDaliSensorQueryInputType(addr);
      break;
    case INPUT_VERIFY_INPUT_TYPE:
      switch(response.answer) {
      case 0x03:   // occupancy sensor
        daliDeviceHandling.index = 0;
        daliDeviceHandling.sensorParams.address[0] = addr;
        daliDeviceHandling.sensorParams.searchAddress[0].raw = daliDeviceHandling.actualSearchAddress.raw;
        break;
      case 0x04:  // light sensor
        daliDeviceHandling.index = 1;
        daliDeviceHandling.sensorParams.address[1] = addr;
        daliDeviceHandling.sensorParams.searchAddress[1].raw = daliDeviceHandling.actualSearchAddress.raw;
        break;
      default:
        TRACE(true,"Unknown Control Device SA:%u Te:%u", addr, response.answer);
        // Nothing to do
        break;
      }
      DeviceHandlingDaliSensorQueryInputType(addr);
      break;
    }
  }
  else {
    TRACE(true,"Input Device:%u NOT verified", addr);
    daliDeviceHandling.state = DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0;
  }
}

void DeviceHandlingOsramSensorReadMemoryBank(void) {
  MemoryBank_InputDeviceReadMemoryBank(daliDeviceHandling.actualShortAddress, 0, 0, 15);
}

void DeviceHandlingDaliSensorReadMemoryBank(void) {
  MemoryBank_DaliInputDeviceReadMemoryBank(daliDeviceHandling.actualShortAddress, 0, 0, 15);
}

void DeviceHandlingOsramSensorConfigureInputDevice(uint8_t addr) {
}

void DeviceHandlingDaliSensorConfigureInputDevice(uint8_t addr) {
  // Set event scheme 2
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 2, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommand2Address(addr, (0xC3 << 8 | DALI_SENSOR_SET_EVENT_SCHEME), DALI_SEND_TWICE, 0);
  // Enable only "movement trigger" event for occupancy sensor (instance type 3)
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 8, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR1, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR2, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommand2Address(addr, (0xC3 << 8 | DALI_SENSOR_SET_EVENT_FILTER), DALI_SEND_TWICE, 0);
  // dead timer 200
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 200, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommand2Address(addr, (0xC4 << 8 | DALI_SENSOR_SET_DEADTIME_TIMER), DALI_SEND_TWICE, 0);
  // report timer 0
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommand2Address(addr, (0xC4 << 8 | DALI_SENSOR_SET_REPORT_TIMER), DALI_SEND_TWICE, 0);
  // Disable events from the light sensor (instance type 4)
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR1, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR2, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommand2Address(addr, (0xC4 << 8 | DALI_SENSOR_SET_EVENT_FILTER), DALI_SEND_TWICE, 0);
}

/** \brief Get the next free address
 *
 * \return next free short address, 0 if no available
 */
static int8_t DaliDeviceHandlingGetNextFreeAddress(void) {
  for(int i = 0; i < 8; i++) {  // search for free place in devices
    if(daliDeviceHandling.newDeviceMask [i] != 0xFF) {
      for(int j = 0; j < 8; j++) {
        if ((daliDeviceHandling.newDeviceMask [i] & (1 << j)) == 0) {
           return ((i * 8) + j);
        }
      }
    }
  }
  return -1;
}

/** \brief Get short address of device
 *
 *
 * \param[in] index of the device
 * \return short address of addressed device
 */
static int8_t DaliDeviceHandlingGetUsedAddress(uint8_t index) {
  uint8_t count = 0;
  for(int i = 0; i < 8; i++) {  // search for free place in devices
    if(daliDeviceHandling.newDeviceMask [i] != 0x00) {
      for(int j = 0; j < 8; j++) {
        if ((daliDeviceHandling.newDeviceMask [i] & (1 << j)) != 0) {
          if(count == index) {
            return ((i * 8) + j);
          }
          else {
            count++;
          }
        }
      }
    }
  }
  return -1;
}

/* \brief
 * Clean the deviceMask
 * */
static void DaliDeviceHandlingCleanDeviceMask(void) {
  for(int i = 0; i < 8; i++)
    daliDeviceHandling.newDeviceMask [i] = 0;
}

/* \brief
 *  Add device to the device mask
 *
 * \param[in] address of the device
 * */
static void DaliDeviceHandlingAddNewDevice(uint8_t shortAddress) {
  daliDeviceHandling.newDeviceMask[shortAddress / 8] |= (1 << (shortAddress % 8));
  daliDeviceHandling.numOfDevices++;
}

/* \brief
 *  Get index of unused ballast index
 *
 * \ return index, 0xff if not found
 * */
static uint8_t DaliDeviceHandlingGetFreeBallastIndex(void) {
  for(uint8_t i = 0; i < 2; i++) {
    if (  (daliDeviceHandling.ecgParams.searchAddress[i].raw == RANDOM_ADDRESS_MASK)
        &&(daliDeviceHandling.ecgParams.address[i] == SHORT_ADDRESS_MASK)) {
      return i;
    }
  }
  return 0xFF;
}

/* \brief
 *  Get index of ballast with given short address
 *
 * \param[in] address of the device *
 * \ return index, 0xff if not found
 * */
static uint8_t DaliDeviceHandlingGetBallastIndex(uint8_t address) {
  for(uint8_t i = 0; i < 2; i++) {
    if (daliDeviceHandling.ecgParams.address[i] == address) {
      return i;
    }
  }
  return 0xFF;
}


/** @brief set search address for input devices
 *
 * only the bytes are send which changes since the last transmission
 * to save time
 *
 */
static void DeviceHandlingSetInputDeviceSearchAddress(void) {
  if (  (daliDeviceHandling.lastSearchAddress.bytes.high != daliDeviceHandling.actualSearchAddress.bytes.high)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
#ifdef useptr
    DeviceHandlingSensorSetRandomH(daliDeviceHandling.actualSearchAddress.bytes.high);
#else
    DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRH, daliDeviceHandling.actualSearchAddress.bytes.high, DALI_SEND_NORMAL, 0);
#endif
  }
  if (  (daliDeviceHandling.lastSearchAddress.bytes.mid != daliDeviceHandling.actualSearchAddress.bytes.mid)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
#ifdef useptr
    DeviceHandlingSensorSetRandomM(daliDeviceHandling.actualSearchAddress.bytes.mid);
#else
    DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRM, daliDeviceHandling.actualSearchAddress.bytes.mid, DALI_SEND_NORMAL, 0);
#endif
  }
  if (  (daliDeviceHandling.lastSearchAddress.bytes.low != daliDeviceHandling.actualSearchAddress.bytes.low)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
#ifdef useptr
    DeviceHandlingSensorSetRandomL(daliDeviceHandling.actualSearchAddress.bytes.low);
#else
    DaliTelegram_Send3ByteSpecialCommand(SENSOR_SEARCHADDRL, daliDeviceHandling.actualSearchAddress.bytes.low, DALI_SEND_NORMAL, 0);
#endif
  }
  daliDeviceHandling.lastSearchAddress.raw = daliDeviceHandling.actualSearchAddress.raw;
}

/** @brief set search address for ballasts
 *
 * only the bytes are send which changes since the last transmission
 * to save time
 *
 */
static void DeviceHandlingSetSearchAddress(void) {
  if (  (daliDeviceHandling.lastSearchAddress.bytes.high != daliDeviceHandling.actualSearchAddress.bytes.high)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SEARCHADDRH, daliDeviceHandling.actualSearchAddress.bytes.high, DALI_SEND_NORMAL, 0);
  }
  if (  (daliDeviceHandling.lastSearchAddress.bytes.mid != daliDeviceHandling.actualSearchAddress.bytes.mid)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SEARCHADDRM, daliDeviceHandling.actualSearchAddress.bytes.mid, DALI_SEND_NORMAL, 0);
  }
  if (  (daliDeviceHandling.lastSearchAddress.bytes.low != daliDeviceHandling.actualSearchAddress.bytes.low)
      || (daliDeviceHandling.lastSearchAddress.bytes.res != 0)) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SEARCHADDRL, daliDeviceHandling.actualSearchAddress.bytes.low, DALI_SEND_NORMAL, 0);
  }
  daliDeviceHandling.lastSearchAddress.raw = daliDeviceHandling.actualSearchAddress.raw;
}


/** @brief look if there are known ballasts
 *
 * \return false if no devices are known
 */
static bool DeviceHandlingHaveExistingRandomAddresses(void) {
  for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    if (daliDeviceHandling.ecgParams.searchAddress[daliDeviceHandling.index].raw != RANDOM_ADDRESS_MASK) {
      return true;   // There is at least one known device
    } // nothing else
  }
  return false;
}

/** @brief set search address random address of known ballasts
 *
 * \return false if no more devices are known
 */
static bool DeviceHandlingSetExistingRandomAddress(void) {
  while (daliDeviceHandling.index < ZIGBEE2DALI_ECG_MAX) {
    if (daliDeviceHandling.ecgParams.searchAddress[daliDeviceHandling.index].raw != RANDOM_ADDRESS_MASK) {
      daliDeviceHandling.ecgParams.address[daliDeviceHandling.index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.lastSearchAddress.raw = 0xFFFFFFFF;
      daliDeviceHandling.actualSearchAddress.raw = daliDeviceHandling.ecgParams.searchAddress[daliDeviceHandling.index].raw;
      DeviceHandlingSetSearchAddress();
      return true;
    } // if(randomAddress == RANDOM_ADDRESS_MASK)
    else {
      daliDeviceHandling.ecgParams.searchAddress[daliDeviceHandling.index].raw = RANDOM_ADDRESS_MASK;
      daliDeviceHandling.ecgParams.address[daliDeviceHandling.index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.index++;
    }
  }
  return false;
}

/** @brief look if there are known ballasts
 *
 * \return false if no devices are known
 */
static bool DeviceHandlingHaveExistingInputDeviceRandomAddress(void) {
  for(int i = 0; i < (ZIGBEE2DALI_LIGHT_SENSOR_MAX + ZIGBEE2DALI_PRESENCE_SENSOR_MAX); i++) {
    if (daliDeviceHandling.sensorParams.searchAddress[i].raw != RANDOM_ADDRESS_MASK) {
      return true; // There is at least one known device
    } // nothing else
  }
  return false;
}

/** @brief set search address random address of known input device
 *
 * \return false if no more devices are known
 */
static bool DeviceHandlingSetExistingInputDeviceRandomAddress(void) {
  while (daliDeviceHandling.index < (ZIGBEE2DALI_LIGHT_SENSOR_MAX + ZIGBEE2DALI_PRESENCE_SENSOR_MAX)) {
    if (daliDeviceHandling.sensorParams.searchAddress[daliDeviceHandling.index].raw != RANDOM_ADDRESS_MASK) {
      daliDeviceHandling.sensorParams.address[daliDeviceHandling.index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.lastSearchAddress.raw = 0xFFFFFFFF;
      daliDeviceHandling.actualSearchAddress.raw = daliDeviceHandling.sensorParams.searchAddress[daliDeviceHandling.index].raw;
      DeviceHandlingSetInputDeviceSearchAddress();
      return true;
    } // if(randomAddress == RANDOM_ADDRESS_MASK)
    else {
      daliDeviceHandling.sensorParams.searchAddress[daliDeviceHandling.index].raw = RANDOM_ADDRESS_MASK;
      daliDeviceHandling.sensorParams.address[daliDeviceHandling.index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.index++;
    }
  }
  return false;
}

/** @brief verify ballast handler
 *
 * \param[in] response  DALI response
 * \param[in] command   DALI command which was sent
 * \return true if verify is ok
 */
static bool DaliDeviceHandlingVerifygShortAddressResponse(uint8_t index, TypeDaliTelegramResponse response, TypeDaliFrame command){
  int8_t shortAddress = (command.fwf.specialCommand.data >> 1) & 0x3F;
  if ((response.answerType == DALI_ANSWER_OK) && (response.answer == 0xff)) {
    TRACE(true,"Gear verified idx:%u verified at addr:%u", index, shortAddress);
    DaliDeviceHandlingAddNewDevice(shortAddress);
    if (index < 2) {
      daliDeviceHandling.ecgParams.address[index] = shortAddress;
      daliDeviceHandling.ecgParams.searchAddress[index].raw = daliDeviceHandling.actualSearchAddress.raw;
    }
    DaliTelegram_SendSpecialCommand(DALI_CMD_WITHDRAWN, 0x00, DALI_SEND_NORMAL, 0);
    //DaliTelegram_SendStdCommand2Address(shortAddress, DALI_CMD_RESET, DALI_SEND_TWICE, 0);
    return true;
  }
  else {
    TRACE(true,"Verify failed idx:%u Delete Address addr:%u", index, shortAddress);
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommand2Address(shortAddress, DALI_CMD_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
    if (index < 2) {
      daliDeviceHandling.ecgParams.address[index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.ecgParams.searchAddress[index].raw = RANDOM_ADDRESS_MASK;
    }
    return false;
  }
}

/** @brief verify InputDevice handler
 *
 * \param[in] response  DALI response
 * \param[in] command   DALI command which was sent
 * \return true if verify is ok
 */
static bool DaliDeviceHandlingVerifygInputDeviceShortAddressResponse(uint8_t index, TypeDaliTelegramResponse response, TypeDaliFrame command){
  int8_t shortAddress = (command.fwf.specialCommand.data >> 1) & 0x3F;
  if ((response.answerType == DALI_ANSWER_OK) && (response.answer == 0xff)) {
    TRACE(true,"Input device verified idx:%u verified at addr:%u", index, shortAddress);
    DaliDeviceHandlingAddNewDevice(shortAddress);
    if (index < 2) {
      daliDeviceHandling.sensorParams.address[index] = shortAddress;
      daliDeviceHandling.sensorParams.searchAddress[index].raw = daliDeviceHandling.actualSearchAddress.raw;
    }
#ifdef useptr
    DeviceHandlingSensorWithdraw();
#else
    DaliTelegram_Send3ByteSpecialCommand(SENSOR_WITHDRAWN, 0x00, DALI_SEND_NORMAL, 0);
#endif
    return true;
  }
  else {
    TRACE(true,"Input device verify failed idx:%u Delete Address addr:%u", index, shortAddress);
    DaliTelegram_Send3ByteSpecialCommand(SENSOR_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0); // TODO: Z2D update for DALI2 context
    DaliTelegram_Send3ByteCommand2Address(shortAddress, SENSOR_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
    daliDeviceHandling.sensorParams.active = false;
    if (index < 2) {
      daliDeviceHandling.sensorParams.address[index] = SHORT_ADDRESS_MASK;
      daliDeviceHandling.sensorParams.searchAddress[index].raw = RANDOM_ADDRESS_MASK;
    }
    return false;
  }
}

/** @brief main command answer handler
 *
 * \param[in] response  DALI response
 * \param[in] command   DALI command which was sent
 */
static void DaliDeviceResponseCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  switch(daliDeviceHandling.state) {
    case DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_BALLAST_RESULT:
      TRACE(true, "DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_BALLAST_RESULT");
      DaliDeviceHandlingVerifygShortAddressResponse(daliDeviceHandling.index, response, command);
      daliDeviceHandling.index++;
      daliDeviceHandling.state = DEVICE_HANDLING_SET_EXISTING_BALLAST_RANDOM_ADDRESS;
      break;
    case DEVICE_HANDLING_WAIT4_COMPARE_NEWBALLAST_RESULT:
      if(response.answerType == DALI_NO_ANSWER) {
        if (daliDeviceHandling.testingBit == 24) {  // No answer do SearchAdress 0xFFFFFF so terminate adressing
          daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_NEW_BALLAST_SEARCH;
          return;
        }
        else {
          daliDeviceHandling.actualSearchAddress.raw |= (1 << daliDeviceHandling.testingBit);
        }
      }
      if (daliDeviceHandling.testingBit == 0) {
        DeviceHandlingSetSearchAddress();
        daliDeviceHandling.state = DEVICE_HANDLING_PROGRAM_BALLAST_SHORTADDR;
      }
      else {
        daliDeviceHandling.testingBit--;
        daliDeviceHandling.actualSearchAddress.raw &= ~(1 << daliDeviceHandling.testingBit);
        DeviceHandlingSetSearchAddress();
        DaliTelegram_SendSpecialCommandWithCallback(DALI_CMD_COMPARE, 0x00, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
      }
      break;
    case DEVICE_HANDLING_WAIT4_VERIFY_NEWBALLAST_RESULT: {
        uint8_t index = DaliDeviceHandlingGetFreeBallastIndex();
        DaliDeviceHandlingVerifygShortAddressResponse(index, response, command);
        daliDeviceHandling.state = DEVICE_HANDLING_INIT_BALLAST_SEARCH;
      }
      break;
    case  DEVICE_HANDLING_WAIT4_BALLAST_DALIVERSION_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        if (response.answer >= DALI_EDITION2) {
          daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICE_EDITION2;
        }
      }
      daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_DEVICETYPE;
      break;
    case  DEVICE_HANDLING_WAIT4_BALLAST_DEVICETYPE_RESULT:
      daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_PHYSMIN;
      if (response.answerType == DALI_ANSWER_OK) {
        switch(response.answer) {
          case 255:
            daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_NEXTDEVICETYPE;
            break;
          case 0:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE0;
            break;
          case 1:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE1;
            break;
          case 6:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE6;
            break;
          case 7:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE7;
            break;
          case 8:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE8;
            break;
        }
      }
      break;
    case  DEVICE_HANDLING_WAIT4_BALLAST_NEXTDEVICETYPE_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_NEXTDEVICETYPE;
        switch(response.answer) {
          case 0:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE0;
            break;
          case 1:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE1;
            break;
          case 6:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE6;
            break;
          case 7:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE7;
            break;
          case 8:
            daliDeviceHandling.tempDeviceFeatureMask |= DALI_DEVICETYPE8;
            break;
          case 254:
            daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_PHYSMIN;
            break;
        }
      }
      else {
        daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_PHYSMIN;
      }
      break;
    case  DEVICE_HANDLING_WAIT4_BALLAST_PHYSMIN_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.tempPhysMinLevel  = response.answer;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST;
      break;
    case  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.colorTempPhysCoolestMirek  = 0x00FF & response.answer;
        daliDeviceHandling.colorTempPhysCoolestMirek = daliDeviceHandling.colorTempPhysCoolestMirek << 8;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB;
      break;
    case  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.colorTempPhysCoolestMirek  |= 0x00FF & response.answer;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST;
      break;
    case  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_RESULT:
        if (response.answerType == DALI_ANSWER_OK) {
          daliDeviceHandling.colorTempPhysWarmestMirek = 0x00FF & response.answer;
          daliDeviceHandling.colorTempPhysWarmestMirek = daliDeviceHandling.colorTempPhysWarmestMirek << 8;
        }
        daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB;
        break;
    case  DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.colorTempPhysWarmestMirek |= 0x00FF & response.answer;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_GET_MEMBANK0;
      break;
    case DEVICE_HANDLING_WAIT4_INPUTDEVICE_QUERY_STATUS_RESULT:
      if (response.answerType == DALI_ANSWER_OK) {
        daliDeviceHandling.sensorParams.deviceTypeMask[0] = 1;
        Zigbee2Dali_SetSensorMode(1);
        DeviceHandlingSensorInitialise = DeviceHandlingOsramSensorInitialise;
        DeviceHandlingSensorReset = DeviceHandlingOsramSensorReset;
        DeviceHandlingSensorRandomize = DeviceHandlingOsramSensorRandomize;
        DeviceHandlingSensorSetRandomH = DeviceHandlingOsramSensorSetRandomH;
        DeviceHandlingSensorSetRandomM = DeviceHandlingOsramSensorSetRandomM;
        DeviceHandlingSensorSetRandomL = DeviceHandlingOsramSensorSetRandomL;
        DeviceHandlingSensorCompare = DeviceHandlingOsramSensorCompare;
        DeviceHandlingSensorPrgShortAdd = DeviceHandlingOsramSensorPrgShortAdd;
        DeviceHandlingSensorVerifyShortAdd = DeviceHandlingOsramSensorVerifyShortAdd;
        DeviceHandlingSensorQueryInputType = DeviceHandlingOsramSensorQueryInputType;
        DeviceHandlingSensorTerminate = DeviceHandlingOsramSensorTerminate;
        DeviceHandlingSensorWithdraw = DeviceHandlingOsramSensorWithdraw;
        DeviceHandlingSensorVerifyType = DeviceHandlingOsramSensorVerifyType;
        DeviceHandlingSensorReadMemoryBank = DeviceHandlingOsramSensorReadMemoryBank;
        DeviceHandlingSensorConfigureInputDevice = DeviceHandlingOsramSensorConfigureInputDevice;
      }
      else {
        TRACE(true, "Looking for DALI 2 Sensor");
        daliDeviceHandling.sensorParams.deviceTypeMask[0] = 2;
        Zigbee2Dali_SetSensorMode(2);
        DeviceHandlingSensorInitialise = DeviceHandlingDaliSensorInitialise;
        DeviceHandlingSensorReset = DeviceHandlingDaliSensorReset;
        DeviceHandlingSensorRandomize = DeviceHandlingDaliSensorRandomize;
        DeviceHandlingSensorSetRandomH = DeviceHandlingDaliSensorSetRandomH;
        DeviceHandlingSensorSetRandomM = DeviceHandlingDaliSensorSetRandomM;
        DeviceHandlingSensorSetRandomL = DeviceHandlingDaliSensorSetRandomL;
        DeviceHandlingSensorCompare = DeviceHandlingDaliSensorCompare;
        DeviceHandlingSensorPrgShortAdd = DeviceHandlingDaliSensorPrgShortAdd;
        DeviceHandlingSensorVerifyShortAdd = DeviceHandlingDaliSensorVerifyShortAdd;
        DeviceHandlingSensorQueryInputType = DeviceHandlingDaliSensorQueryInputType;
        DeviceHandlingSensorTerminate = DeviceHandlingDaliSensorTerminate;
        DeviceHandlingSensorWithdraw = DeviceHandlingDaliSensorWithdraw;
        DeviceHandlingSensorVerifyType = DeviceHandlingDaliSensorVerifyType;
        DeviceHandlingSensorReadMemoryBank = DeviceHandlingDaliSensorReadMemoryBank;
        DeviceHandlingSensorConfigureInputDevice = DeviceHandlingDaliSensorConfigureInputDevice;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_START_SEARCHING_EXISTING_INPUTDEVICE;
      break;
    case DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_INPUTDEVICE_RESULT:
      TRACE(true, "DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_INPUTDEVICE_RESULT");
      DaliDeviceHandlingVerifygInputDeviceShortAddressResponse(daliDeviceHandling.index, response, command);
      daliDeviceHandling.index++;
      daliDeviceHandling.state = DEVICE_HANDLING_SET_EXISTING_INPUTDEVICE_RANDOM_ADDRESS;
      break;
    case DEVICE_HANDLING_WAIT4_COMPARE_NEW_INPUTDEVICE_RESULT:
      if(response.answerType == DALI_NO_ANSWER) {
        if (daliDeviceHandling.testingBit == 24) {  // No answer do SearchAdress 0xFFFFFF so terminate adressing
          daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_NEW_INPUTDEVICE_SEARCH;
          return;
        }
        else {
          daliDeviceHandling.actualSearchAddress.raw |= (1 << daliDeviceHandling.testingBit);
        }
      }
      if (daliDeviceHandling.testingBit == 0) {
        DeviceHandlingSetInputDeviceSearchAddress();
        daliDeviceHandling.state = DEVICE_HANDLING_PROGRAM_INPUTDEVICE_SHORTADDR;
      }
      else {
        daliDeviceHandling.testingBit--;
        daliDeviceHandling.actualSearchAddress.raw &= ~(1 << daliDeviceHandling.testingBit);
        DeviceHandlingSetInputDeviceSearchAddress();
#ifdef useptr
        DeviceHandlingSensorCompare(0);
#else
        DaliTelegram_Send3ByteSpecialCommandWithCallback(SENSOR_COMPARE, 0x00, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
#endif
      }
      break;
    case DEVICE_HANDLING_WAIT4_QUERY_INPUTDEVICE_TYPE_RESULT: {
        uint8_t shortaddress = command.idf.shortAddressed.address;
#ifdef useptr
          DeviceHandlingSensorVerifyType(shortaddress, response);
#else
        if(response.answerType == DALI_ANSWER_OK) {
          DaliDeviceHandlingAddNewDevice(shortaddress);
          daliDeviceHandling.sensorParams.active = true;
          switch(response.answer) {
            case 0x02:   // occupancy sensor
              daliDeviceHandling.index = 0;
              daliDeviceHandling.sensorParams.address[0] = shortaddress;
              daliDeviceHandling.sensorParams.searchAddress[0].raw = daliDeviceHandling.actualSearchAddress.raw;
              break;
            case 0x03:  // photosensor
              daliDeviceHandling.index = 1;
              daliDeviceHandling.sensorParams.address[1] = shortaddress;
              daliDeviceHandling.sensorParams.searchAddress[1].raw = daliDeviceHandling.actualSearchAddress.raw;
             break;
            default:
              TRACE(true,"Unknown InputDeviceType SA:%u Te:%u", shortaddress, response.answer);
              // Nothing to do
              break;
          }
        }
        else {
          TRACE(true,"Input Device:%u NOT verified", shortaddress);
        }
        daliDeviceHandling.state = DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0;
#endif
      }
      break;
    default:
      TRACE(true, "!!! DaliDeviceResponseCallback but not expecting one");
      break;
  }
}

static EmberAfStatus DaliDeviceHandling_SetDevicesInfo(uint8_t endpoint, TypeDaliDeviceInfo *deviceInfo) {
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
  TypeZigbeeString modelIdentifier;
  modelIdentifier.length = sprintf(modelIdentifier.str, "%02X-%02X-%02X-%02X-%02X-%02X",
                                   deviceInfo->gtin.bytes[0], deviceInfo->gtin.bytes[1], deviceInfo->gtin.bytes[2],
                                   deviceInfo->gtin.bytes[3], deviceInfo->gtin.bytes[4], deviceInfo->gtin.bytes[5]);
  emberAfPluginOsramdsBasicServerSetModelIdentifier(endpoint, modelIdentifier);
  TypeZigbeeString swBuildId;
  swBuildId.length = sprintf(swBuildId.str, "%d.%d", deviceInfo->version.byte.high, deviceInfo->version.byte.low);
  return emberAfPluginOsramdsBasicServerSetSwBuildId(endpoint, swBuildId);
#else
  return EMBER_ZCL_STATUS_SUCCESS;
#endif
}

static EmberStatus DaliDeviceHandling_SetEndpointType(uint8_t endpoint, uint16_t zigbeeDevicetype, bool enabled) {
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
  EnceliumInterfaceServer_SetEndpointType(endpoint, zigbeeDevicetype, enabled);
#else
  if(enabled && 0xFF == emberAfIndexFromEndpoint(endpoint)) {
    emberAfSetActiveEndpoint(endpoint);
  }
#endif
  return EMBER_SUCCESS;
}

// --------- Global Functions --------------------------------------------------

uint32_t DaliDeviceHandling_FindLuxCalibrationMultiplier(void){
  uint32_t luxCalibrationMultiplier = 0x00010000;
  for (int i = 0; i < (sizeof(scalingFactorTable)/sizeof(TypeDaliSensorScalingFactor)); i++) {
    if (0 == memcmp(&scalingFactorTable[i].gtin, daliDeviceHandling.sensorInfo[1].gtin.bytes, 6)) {
      luxCalibrationMultiplier = scalingFactorTable[i].scalingFactor;
    }
  }
  return luxCalibrationMultiplier;
}

/** \brief start addressing
 *
 * 
 */
void DaliDeviceHandling_StartDeviceAddressing(void) {
    daliDeviceHandling.state = DEVICE_HANDLING_START_ADDRESSING;
    daliDeviceHandling.timeout = (uint32_t)((1000*30)/2.5);
}

/** \brief Check if addressing is still running
 *
 * \return true if addressing is runnung
 */
bool DaliDeviceHandling_IsAddressing(void) {
  return (daliDeviceHandling.state != DEVICE_HANDLING_IDLE);
}

/** \brief Addressing state machine
 *
 */
void DaliDeviceHandling_Tick(void) {
  int8_t shortAddress;

  if (daliDeviceHandling.stateChangeDelay) {
      daliDeviceHandling.stateChangeDelay-- ;
      return;
  }

  if (daliDeviceHandling.timeout-- == 0) {
    daliDeviceHandling.state = DEVICE_HANDLING_ADDRESSING_CANCEL;    // Cancel if infinite
  }
  switch(daliDeviceHandling.state) {
    case DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_BALLAST_RESULT:      // Nothing to do here when waiting for an answer....
    case DEVICE_HANDLING_WAIT4_COMPARE_NEWBALLAST_RESULT:
    case DEVICE_HANDLING_WAIT4_VERIFY_NEWBALLAST_RESULT:
    case DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_INPUTDEVICE_RESULT:
    case DEVICE_HANDLING_WAIT4_BALLAST_DALIVERSION_RESULT:
    case DEVICE_HANDLING_WAIT4_BALLAST_DEVICETYPE_RESULT:
    case DEVICE_HANDLING_WAIT4_BALLAST_NEXTDEVICETYPE_RESULT:
    case DEVICE_HANDLING_WAIT4_BALLAST_PHYSMIN_RESULT:
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_RESULT:
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB_RESULT:
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_RESULT:
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB_RESULT:
    case DEVICE_HANDLING_WAIT4_INPUTDEVICE_QUERY_STATUS_RESULT:
    case DEVICE_HANDLING_WAIT4_COMPARE_NEW_INPUTDEVICE_RESULT:
    case DEVICE_HANDLING_WAIT4_QUERY_INPUTDEVICE_TYPE_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_TEST_EXECUTION_TIMEOUT_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_BATTERY_CHARGE_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_LEVEL_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_MODE_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_FEATURES_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_FAILURE_STATUS_RESULT:
    case DEVICE_HANDLING_WAIT4_EMERGENCY_STATUS_RESULT:
    case DEVICE_HANDLING_IDLE:
      break;
    case DEVICE_HANDLING_START_ADDRESSING:
      TRACE(true, "DEVICE_HANDLING_START_ADDRESSING")
      daliDeviceHandling.numOfDevices = 0;
      daliDeviceHandling.index = 0;
      DaliDeviceHandlingCleanDeviceMask();
      TypeEcgParams ecgInitParams = ECG_INIT_PARAMS_DEFAULT;
      daliDeviceHandling.ecgParams = ecgInitParams;
      TypeSensorParams sensorInitParams = SENSOR_INIT_PARAMS_DEFAULT;
      daliDeviceHandling.sensorParams = sensorInitParams;
      TypeEmergencyParams emergencyInitParams = EMERGENCY_INIT_PARAMS_DEFAULT;
      daliDeviceHandling.emergencyParams = emergencyInitParams;
      daliDeviceHandling.state = DEVICE_HANDLING_DELETE_ADDRESSES;
      // Maybe some initialization stuff
      break;
    case DEVICE_HANDLING_DELETE_ADDRESSES:
      TRACE(true, "DEVICE_HANDLING_DELETE_ADDRESSES")
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
      daliDeviceHandling.state = DEVICE_HANDLING_DELAY_SET_SHORT_ADDRESS;
      daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      break;
    case DEVICE_HANDLING_DELAY_SET_SHORT_ADDRESS:
      TRACE(true, "DEVICE_HANDLING_DELAY_SET_SHORT_ADDRESS")
      DaliTelegram_SendStdCommandBroadcast(DALI_CMD_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0); // TODO: Z2D check
      DaliTelegram_Send3ByteCommandBroadcast(SENSOR_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
      if (DeviceHandlingHaveExistingRandomAddresses()) {
        daliDeviceHandling.state = DEVICE_HANDLING_START_SEARCHING_EXISTING_BALLASTS;
      }
      else {  // no known ballasts, so reset all on the line
          daliDeviceHandling.state = DEVICE_HANDLING_DELAY_RESET_CMD_DALI_BUS;
      }
      daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      break;
    case DEVICE_HANDLING_DELAY_RESET_CMD_DALI_BUS:
      TRACE(true, "DEVICE_HANDLING_DELAY_RESET_CMD_DALI_BUS")
      DaliTelegram_SendStdCommandBroadcast(DALI_CMD_RESET, DALI_SEND_TWICE, 0);
      daliDeviceHandling.state = DEVICE_HANDLING_START_ADDRESSINTG_NEW_BALLAST;
      daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      break;
    case DEVICE_HANDLING_START_SEARCHING_EXISTING_BALLASTS:
      TRACE(true, "DEVICE_HANDLING_START_SEARCHING_EXISTING_BALLASTS")
      DaliTelegram_SendSpecialCommand(DALI_CMD_INITIALISE, 0x00, DALI_SEND_TWICE, 0);
      daliDeviceHandling.state = DEVICE_HANDLING_SET_EXISTING_BALLAST_RANDOM_ADDRESS;
      break;
    case DEVICE_HANDLING_SET_EXISTING_BALLAST_RANDOM_ADDRESS:
      TRACE(true, "DEVICE_HANDLING_SET_EXISTING_BALLAST_RANDOM_ADDRESS")
      if(DeviceHandlingSetExistingRandomAddress()) {
        daliDeviceHandling.state = DEVICE_HANDLING_PROGRAM_EXISTING_BALLAST_SHORTADDR;
      }
      else {
        daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_EXISTING_BALLAST_SEARCH;
      }
      break;
    case DEVICE_HANDLING_PROGRAM_EXISTING_BALLAST_SHORTADDR:
      TRACE(true, "DEVICE_HANDLING_PROGRAM_EXISTING_BALLAST_SHORTADDR")
      shortAddress = DaliDeviceHandlingGetNextFreeAddress();
      if(shortAddress >= 0) {
        TRACE(true, "Existing Gear set to addr :%u", shortAddress);
        DaliTelegram_SendSpecialCommand(DALI_CMD_PGM_SHORT_ADDRESS, (shortAddress << 1) | 0x01, DALI_SEND_NORMAL, 0);
        daliDeviceHandling.state = DEVICE_HANDLING_VERIFY_EXISTING_BALLAST_SHORTADDR;
      }
      else {
        daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_EXISTING_BALLAST_SEARCH;
      }
      daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      break;
    case DEVICE_HANDLING_VERIFY_EXISTING_BALLAST_SHORTADDR:
      shortAddress = DaliDeviceHandlingGetNextFreeAddress();
      if(shortAddress >= 0) {
          DaliTelegram_SendSpecialCommandWithCallback(DALI_CMD_VERIFY_SHORT_ADDR, (shortAddress << 1) | 0x01, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
          daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_BALLAST_RESULT;
      } else {
              daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_EXISTING_BALLAST_SEARCH;
      }
      break;
    case DEVICE_HANDLING_TERMINATE_EXISTING_BALLAST_SEARCH:
      DaliTelegram_SendSpecialCommand(DALI_CMD_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
      daliDeviceHandling.state = DEVICE_HANDLING_START_ADDRESSINTG_NEW_BALLAST;
      break;
    case DEVICE_HANDLING_START_ADDRESSINTG_NEW_BALLAST:
      TRACE(true, "DEVICE_HANDLING_START_ADDRESSINTG_NEW_BALLAST")
      DaliTelegram_SendSpecialCommand(DALI_CMD_INITIALISE, 0xFF, DALI_SEND_TWICE, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_RANDOMIZE, 0x00, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_RANDOMIZE, 0x00, DALI_SEND_NORMAL, DALI_DELAY_200MS);
      daliDeviceHandling.state = DEVICE_HANDLING_INIT_BALLAST_SEARCH;
      daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      break;
    case DEVICE_HANDLING_INIT_BALLAST_SEARCH:
      TRACE(true, "DEVICE_HANDLING_INIT_BALLAST_SEARCH")
      daliDeviceHandling.testingBit = 24;
      daliDeviceHandling.lastSearchAddress.raw = 0xFFFFFFFF;
      daliDeviceHandling.actualSearchAddress.raw = 0x00FFFFFF;
      DeviceHandlingSetSearchAddress();
      DaliTelegram_SendSpecialCommandWithCallback(DALI_CMD_COMPARE, 0x00, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_COMPARE_NEWBALLAST_RESULT;
      break;
    case DEVICE_HANDLING_PROGRAM_BALLAST_SHORTADDR: {
        TRACE(true, "DEVICE_HANDLING_PROGRAM_BALLAST_SHORTADDR")
        int8_t shortAddress = DaliDeviceHandlingGetNextFreeAddress();
        if(shortAddress >= 0) {
          TRACE(true, "New Gear set to addr :%u", shortAddress);
          DaliTelegram_SendSpecialCommand(DALI_CMD_PGM_SHORT_ADDRESS, (shortAddress << 1) | 0x01, DALI_SEND_NORMAL, 0);
          DaliTelegram_SendSpecialCommandWithCallback(DALI_CMD_VERIFY_SHORT_ADDR, (shortAddress << 1) | 0x01, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
          daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_VERIFY_NEWBALLAST_RESULT;
        }
        else {
          daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_NEW_BALLAST_SEARCH;
        }
        daliDeviceHandling.stateChangeDelay = DEFAULT_STATE_CHANGE_DELAY;
      }
      break;
    case DEVICE_HANDLING_TERMINATE_NEW_BALLAST_SEARCH:
      TRACE(true, "DEVICE_HANDLING_TERMINATE_NEW_BALLAST_SEARCH")
      DaliTelegram_SendSpecialCommand(DALI_CMD_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
      daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_ADDRESSING_FINISHED;
      break;
    case DEVICE_HANDLING_BALLAST_ADDRESSING_FINISHED:
      TRACE(true, "DEVICE_HANDLING_BALLAST_ADDRESSING_FINISHED")
      if (daliDeviceHandling.numOfDevices > ZIGBEE2DALI_ECG_MAX) {  // more than ZIGBEE2DALI_ECG_MAX, so broadcast mode
        daliDeviceHandling.ecgParams.broadcastMode = true;
        for(uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
          daliDeviceHandling.ecgParams.searchAddress[i].raw = RANDOM_ADDRESS_MASK;
          daliDeviceHandling.ecgParams.address[i] = SHORT_ADDRESS_MASK;
          daliDeviceHandling.ecgParams.deviceTypeMask[i] = 0x00;
        }
        daliDeviceHandling.ecgParams.address[0] = DALI_SA_BROADCAST;
      }
      else {
        daliDeviceHandling.ecgParams.broadcastMode = false;
      }
      daliDeviceHandling.index = 0;
      daliDeviceHandling.deviceFeatureMask = DALI_FEATURE_ALL;
      daliDeviceHandling.physMinLevel = 1;
      daliDeviceHandling.state = DEVICE_HANDLING_START_IDENTIFY_BALLAST;
     break;
    case DEVICE_HANDLING_START_IDENTIFY_BALLAST:
      TRACE(true, "DEVICE_HANDLING_START_IDENTIFY_BALLAST")
      daliDeviceHandling.tempDeviceFeatureMask = 0;
      daliDeviceHandling.state = DEVICE_HANDLING_GET_BALLAST_DALIVERSION;
      daliDeviceHandling.actualShortAddress = DaliDeviceHandlingGetUsedAddress(daliDeviceHandling.index);
      if (daliDeviceHandling.actualShortAddress < 0) {
        daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_IDENTIFICATION_FINISHED;
      }
      break;
    case DEVICE_HANDLING_GET_BALLAST_DALIVERSION:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_VERSION_NUMBER, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_BALLAST_DALIVERSION_RESULT;
      break;
    case DEVICE_HANDLING_GET_BALLAST_DEVICETYPE:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_DEVICE_TYPE, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_BALLAST_DEVICETYPE_RESULT;
      break;
    case DEVICE_HANDLING_GET_BALLAST_NEXTDEVICETYPE:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_NEXT_DEVICE_TYPE, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_BALLAST_NEXTDEVICETYPE_RESULT;
      break;
    case DEVICE_HANDLING_GET_BALLAST_PHYSMIN:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_PHYS_MIN_LEVEL, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_BALLAST_PHYSMIN_RESULT;
      break;
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST:
      if(daliDeviceHandling.tempDeviceFeatureMask & DALI_DEVICETYPE8) {
        DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, DALI_QUERY_COLOR_TEMP_PHYS_COOLEST, DALI_SEND_NORMAL, 0);
        DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
        DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_COLOR_VALUE, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
        daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_RESULT;
        break;
      }
      daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_GET_MEMBANK0;
      break;
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_CONTENT_DTR0, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSCOOLEST_LSB_RESULT;
      break;
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST:
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, DALI_QUERY_COLOR_TEMP_PHYS_WARMEST, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_COLOR_VALUE, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_RESULT;
      break;
    case DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB:
      DaliTelegram_SendStdCommand2AddressWithCallback(daliDeviceHandling.actualShortAddress, DALI_QUERY_CONTENT_DTR0, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_GET_COLOR_TEMP_PHYSWARMEST_LSB_RESULT;
      break;
    case DEVICE_HANDLING_BALLAST_GET_MEMBANK0:
      MemoryBank_DriverReadMemoryBank(daliDeviceHandling.actualShortAddress, 0, 0, 15);
      daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_GET_MEMBANK0_WAIT4FINISHED;
      break;
    case DEVICE_HANDLING_BALLAST_GET_MEMBANK0_WAIT4FINISHED:
      if (MemoryBank_ReadRequestDone()) {
        uint8_t * buffer = MemoryBank_GetBuffer();
        if ((buffer != NULL)&& (daliDeviceHandling.index < ZIGBEE2DALI_ECG_MAX)) {
          memcpy(daliDeviceHandling.ecgInfo[daliDeviceHandling.index].gtin.bytes, buffer + 3, 6);  // GTIN starts at location 3
          daliDeviceHandling.ecgInfo[daliDeviceHandling.index].version.byte.high = *(buffer +  9);
          daliDeviceHandling.ecgInfo[daliDeviceHandling.index].version.byte.low  = *(buffer + 10);
        }
        MemoryBank_FreeHandler();
        daliDeviceHandling.state = DEVICE_HANDLING_BALLAST_IDENTIFICATION_FINISHED;
      }
      else {
        MemoryBank_Tick();
      }
      break;
    case DEVICE_HANDLING_BALLAST_IDENTIFICATION_FINISHED:
      if (daliDeviceHandling.numOfDevices > ZIGBEE2DALI_ECG_MAX) {
        if(daliDeviceHandling.physMinLevel < daliDeviceHandling.tempPhysMinLevel) {
          daliDeviceHandling.physMinLevel = daliDeviceHandling.tempPhysMinLevel;
        }
        daliDeviceHandling.deviceFeatureMask |= daliDeviceHandling.tempDeviceFeatureMask; // To find the maximum common features
      }
      else {
        uint8_t index = DaliDeviceHandlingGetBallastIndex(daliDeviceHandling.actualShortAddress);
        if (index < ZIGBEE2DALI_ECG_MAX) {
          daliDeviceHandling.ecgParams.deviceTypeMask[index] = daliDeviceHandling.tempDeviceFeatureMask;
          Zigbee2Dali_SetDeviceType1Support(index, daliDeviceHandling.tempDeviceFeatureMask & DALI_DEVICETYPE1);
          Zigbee2Dali_SetDeviceType8Support(index, daliDeviceHandling.tempDeviceFeatureMask & DALI_DEVICETYPE8);
          Zigbee2Dali_SetPhysicalMinLevel(index, daliDeviceHandling.tempPhysMinLevel);
          if(daliDeviceHandling.tempDeviceFeatureMask & DALI_DEVICETYPE8) {
            Zigbee2Dali_SetColorTempPhysMin(index, daliDeviceHandling.colorTempPhysCoolestMirek);
            Zigbee2Dali_SetColorTempPhysMax(index, daliDeviceHandling.colorTempPhysWarmestMirek);
          }
        }
      }
      if (++daliDeviceHandling.index < daliDeviceHandling.numOfDevices) {
        daliDeviceHandling.state = DEVICE_HANDLING_START_IDENTIFY_BALLAST;
      }
      else {
        if (daliDeviceHandling.numOfDevices > ZIGBEE2DALI_ECG_MAX) {
          daliDeviceHandling.ecgParams.deviceTypeMask[0] = daliDeviceHandling.deviceFeatureMask;
          Zigbee2Dali_SetPhysicalMinLevel(0, daliDeviceHandling.physMinLevel);
        }
        daliDeviceHandling.state = DEVICE_HANDLING_INPUTDEVICE_QUERY_STATUS;
      }
      break;
    case DEVICE_HANDLING_INPUTDEVICE_QUERY_STATUS:
      DaliTelegram_Send3ByteCommandBroadcastWithCallback(SENSOR_QUERY_STATUS, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_INPUTDEVICE_QUERY_STATUS_RESULT;
      break;
    case DEVICE_HANDLING_START_SEARCHING_EXISTING_INPUTDEVICE:
      TRACE(true, "DEVICE_HANDLING_START_SEARCHING_EXISTING_INPUTDEVICE")
      daliDeviceHandling.index = 0;
      daliDeviceHandling.numOfDevices = 0;
      DaliDeviceHandlingCleanDeviceMask();

#ifndef useptr
      // switch operating mode to Osram proprietary mode before proceeding
      DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR0, 0xB2, DALI_SEND_NORMAL, 0);
      DaliTelegram_Send3ByteSpecialCommand(DALI_SENSOR_SETDTR1, 0xB2, DALI_SEND_NORMAL, 0);
      DaliTelegram_Send3ByteCommandBroadcast(DALI_SENSOR_SET_OPERATING_MODE, DALI_SEND_TWICE, 0);
#endif
      if (DeviceHandlingHaveExistingInputDeviceRandomAddress()) {
#ifdef useptr
        DeviceHandlingSensorInitialise(0x00); // TODO: Z2D check if this works
#else
        DaliTelegram_Send3ByteSpecialCommand(SENSOR_INITIALISE, 0x00, DALI_SEND_TWICE, 0);
#endif
        daliDeviceHandling.state = DEVICE_HANDLING_SET_EXISTING_INPUTDEVICE_RANDOM_ADDRESS;
      }
      else {
#ifdef useptr
        DeviceHandlingSensorReset();
        daliDeviceHandling.stateChangeDelay = (uint32_t)((1000)/2.5);
#else
        DaliTelegram_Send3ByteCommandBroadcast(SENSOR_RESET, DALI_SEND_TWICE, DALI_DELAY_200MS);
#endif
        daliDeviceHandling.state = DEVICE_HANDLING_START_ADDRESSINTG_NEW_INPUTDEVICE;
      }
      break;
    case DEVICE_HANDLING_SET_EXISTING_INPUTDEVICE_RANDOM_ADDRESS:
      TRACE(true, "DEVICE_HANDLING_SET_EXISTING_INPUTDEVICE_RANDOM_ADDRESS")
      if(DeviceHandlingSetExistingInputDeviceRandomAddress()) {
        daliDeviceHandling.state = DEVICE_HANDLING_PROGRAM_EXISTING_INPUTDEVICE_SHORTADDR;
      }
      else {
        daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_EXISTING_INPUTDEVICE_SEARCH;
      }
      break;
    case DEVICE_HANDLING_PROGRAM_EXISTING_INPUTDEVICE_SHORTADDR:
      TRACE(true, "DEVICE_HANDLING_PROGRAM_EXISTING_INPUTDEVICE_SHORTADDR")
      shortAddress = DaliDeviceHandlingGetNextFreeAddress();
      if(shortAddress >= 0) {
        TRACE(true, "Existing Gear set to addr :%u", shortAddress);
#ifdef useptr
        DeviceHandlingSensorPrgShortAdd(shortAddress);
        DeviceHandlingSensorVerifyShortAdd(shortAddress);
#else
        DaliTelegram_Send3ByteSpecialCommand(SENSOR_PGM_SHORT_ADDRESS, (shortAddress << 1) | 0x01, DALI_SEND_NORMAL, 0); // TODO: Z2D
        DaliTelegram_Send3ByteSpecialCommandWithCallback(SENSOR_VERIFY_SHORT_ADDR, (shortAddress << 1) | 0x01, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
#endif
        daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_VERIFY_EXISTING_INPUTDEVICE_RESULT;
      }
      else {
        daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_EXISTING_INPUTDEVICE_SEARCH;
      }
      break;
    case DEVICE_HANDLING_TERMINATE_EXISTING_INPUTDEVICE_SEARCH:
#ifdef useptr
      DeviceHandlingSensorTerminate();
#else
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_START_ADDRESSINTG_NEW_INPUTDEVICE;
      break;
    case DEVICE_HANDLING_START_ADDRESSINTG_NEW_INPUTDEVICE:
      TRACE(true, "DEVICE_HANDLING_START_ADRESSING_NEW_INPUTDEVICE")
#ifdef useptr
      DeviceHandlingSensorInitialise(0xFF);
      DeviceHandlingSensorRandomize();
#else
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_INITIALISE, 0xFF, DALI_SEND_TWICE, 0);
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_RANDOMIZE, 0x00, DALI_SEND_NORMAL, 0);
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_RANDOMIZE, 0x00, DALI_SEND_NORMAL, DALI_DELAY_200MS);
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_INIT_INPUTDEVICE_SEARCH;
      break;
    case DEVICE_HANDLING_INIT_INPUTDEVICE_SEARCH:
      TRACE(true, "DEVICE_HANDLING_INIT_INPUTDEVICE_SEARCH")
      daliDeviceHandling.testingBit = 24;
      daliDeviceHandling.lastSearchAddress.raw = 0xFFFFFFFF;
      daliDeviceHandling.actualSearchAddress.raw = 0x00FFFFFF;
      DeviceHandlingSetInputDeviceSearchAddress();
#ifdef useptr
      DeviceHandlingSensorCompare(DALI_SAFE_QUERY);
#else
      DaliTelegram_Send3ByteSpecialCommandWithCallback(SENSOR_COMPARE, 0x00, DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, DaliDeviceResponseCallback);
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_COMPARE_NEW_INPUTDEVICE_RESULT;
      break;
    case DEVICE_HANDLING_PROGRAM_INPUTDEVICE_SHORTADDR: {
        TRACE(true, "DEVICE_HANDLING_PROGRAM_INPUTDEVICE_SHORTADDR")
        int8_t shortAddress = DaliDeviceHandlingGetNextFreeAddress();
        if(shortAddress >= 0) {
          daliDeviceHandling.actualShortAddress = shortAddress;
          TRACE(true, "New InputDevice to addr :%u", shortAddress);
#ifdef useptr
          DeviceHandlingSensorPrgShortAdd(shortAddress);
          DeviceHandlingSensorQueryInputType(shortAddress);
#else
          DaliTelegram_Send3ByteSpecialCommand(SENSOR_PGM_SHORT_ADDRESS, (shortAddress << 1) | 0x01, DALI_SEND_NORMAL, 0);
          DaliTelegram_Send3ByteCommand2AddressWithCallback(shortAddress, SENSOR_QUERY_INPUT_TYPE, DALI_WAIT4ANSWER, 0, DaliDeviceResponseCallback);
#endif
          daliDeviceHandling.state = DEVICE_HANDLING_WAIT4_QUERY_INPUTDEVICE_TYPE_RESULT;
        }
        else {
          daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_NEW_INPUTDEVICE_SEARCH;
        }
      }
      break;
    case DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0:
#if 0
      daliDeviceHandling.state = DEVICE_HANDLING_TERMINATE_NEW_INPUTDEVICE_SEARCH;
#else
#ifdef useptr
      DeviceHandlingSensorReadMemoryBank();
#else
      MemoryBank_InputDeviceReadMemoryBank(daliDeviceHandling.actualShortAddress, 0, 0, 15);
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0_WAIT4FINISHED;
#endif
      break;
    case DEVICE_HANDLING_GET_INPUT_DEVICE_MEMBANK0_WAIT4FINISHED:
      if (MemoryBank_ReadRequestDone()) {
        uint8_t * buffer = MemoryBank_GetBuffer();
        if ((buffer != NULL)&& (daliDeviceHandling.index < ZIGBEE2DALI_ECG_MAX)) {
          memcpy(daliDeviceHandling.sensorInfo[daliDeviceHandling.index].gtin.bytes, buffer + 3, 6);  // GTIN starts at location 3
          daliDeviceHandling.sensorInfo[daliDeviceHandling.index].version.byte.high = *(buffer +  9);
          daliDeviceHandling.sensorInfo[daliDeviceHandling.index].version.byte.low  = *(buffer + 10);
        }
        MemoryBank_FreeHandler();
#ifdef useptr
    DeviceHandlingSensorWithdraw();
#else
        DaliTelegram_Send3ByteSpecialCommand(SENSOR_WITHDRAWN, 0x00, DALI_SEND_NORMAL, 0);
#endif
        daliDeviceHandling.state = DEVICE_HANDLING_CONFIGURE_INPUTDEVICE;
      }
      else {
        MemoryBank_Tick();
      }
      break;
    case DEVICE_HANDLING_CONFIGURE_INPUTDEVICE:
      DeviceHandlingSensorConfigureInputDevice(daliDeviceHandling.actualShortAddress);
      daliDeviceHandling.state = DEVICE_HANDLING_INIT_INPUTDEVICE_SEARCH;
      break;
    case DEVICE_HANDLING_TERMINATE_NEW_INPUTDEVICE_SEARCH:
#ifdef useptr
      DeviceHandlingSensorTerminate();
#else
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_ADDRESSING_FINISHED;
      break;
    case DEVICE_HANDLING_ADDRESSING_CANCEL:
#ifdef useptr
      DeviceHandlingSensorTerminate();
#else
      DaliTelegram_Send3ByteSpecialCommand(SENSOR_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
#endif
      DaliTelegram_SendSpecialCommand(DALI_CMD_TERMINATE, 0x00, DALI_SEND_NORMAL, 0);
      // No break, Go to Finished state
    case DEVICE_HANDLING_ADDRESSING_FINISHED:
      halCommonSetToken(TOKEN_ECG_INIT_PARAMS, &daliDeviceHandling.ecgParams);
      halCommonSetToken(TOKEN_SENSOR_INIT_PARAMS, &daliDeviceHandling.sensorParams);
      halCommonSetToken(TOKEN_ECG_INFO, &daliDeviceHandling.ecgInfo);
      halCommonSetToken(TOKEN_SENSOR_INFO, &daliDeviceHandling.sensorInfo);
      DaliDeviceHandling_InitDevices();
#if 0 // TODO this should be called after stack is initialized
      if(daliDeviceHandling.sensorParams.active) {
        uint32_t luxCalibrationMultiplier = 0x00010000;
        luxCalibrationMultiplier = DaliDeviceHandling_FindLuxCalibrationMultiplier(daliDeviceHandling.sensorInfo[1].gtin.bytes);  // GTIN start at location 0x03
        IlluminanceMeasurementServer_SetLuxCalibrationMultplier(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, luxCalibrationMultiplier);
      }
#endif
      daliDeviceHandling.state = DEVICE_HANDLING_IDLE;
      break;
    default:
      daliDeviceHandling.state = DEVICE_HANDLING_IDLE;
     break;
  }
}

/** \brief Test command to force broadcast mode. System reset required after enabling.
 *
 */
void DaliDeviceHandling_TestBroadcastMode(void) {
  daliDeviceHandling.ecgParams.broadcastMode = TRUE;
  halCommonSetToken(TOKEN_ECG_INIT_PARAMS, &daliDeviceHandling.ecgParams);
} 

/** \brief Start addressing state machine at initialization
 *  Init devices with data stored in Tokens
 */
bool DaliDeviceHandling_InitDevices(void) {
  bool initDone = true;
  if (ZigbeeHaLightEndDevice_IsInRepeatermode()) {
    TypeEcgParams ecgInitParams = ECG_INIT_PARAMS_DEFAULT;
    daliDeviceHandling.ecgParams = ecgInitParams;
    TypeSensorParams sensorInitParams = SENSOR_INIT_PARAMS_DEFAULT;
    daliDeviceHandling.sensorParams = sensorInitParams;
    uint32_t deviceConfig = DEVICE_CONFIG_DEFAULT;
    daliDeviceHandling.deviceConfig = deviceConfig;
  }
  else {
    halCommonGetToken(&daliDeviceHandling.ecgParams, TOKEN_ECG_INIT_PARAMS);
    halCommonGetToken(&daliDeviceHandling.sensorParams, TOKEN_SENSOR_INIT_PARAMS);
    halCommonGetToken(&daliDeviceHandling.ecgInfo, TOKEN_ECG_INFO);
    halCommonGetToken(&daliDeviceHandling.sensorInfo, TOKEN_SENSOR_INFO);
    halCommonGetToken(&daliDeviceHandling.deviceConfig, TOKEN_DEVICE_CONFIG);

    if(daliDeviceHandling.ecgParams.address[0] == SHORT_ADDRESS_MASK &&
       daliDeviceHandling.ecgParams.address[1] == SHORT_ADDRESS_MASK &&
       daliDeviceHandling.sensorParams.address[0] == SHORT_ADDRESS_MASK &&
       daliDeviceHandling.sensorParams.address[1] == SHORT_ADDRESS_MASK) {
        TRACE(true, "Start addressing on factory reset device.");
        initDone = false; // don't return here so that a default endpoint is set in the code below when no device is found
    }
  }
  if(!daliDeviceHandling.ecgParams.broadcastMode) {
    TRACE(true, "BROADCAST MODE OFF");
    DaliDeviceHandlingCleanDeviceMask();
    Zigbee2Dali_SetBroadcastMode(false);
    for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if (daliDeviceHandling.ecgParams.address[i] != SHORT_ADDRESS_MASK) {
        bool colorSupport = daliDeviceHandling.ecgParams.deviceTypeMask[i] & DALI_DEVICETYPE8;

        bool emergencySupport = daliDeviceHandling.ecgParams.deviceTypeMask[i] & DALI_DEVICETYPE1;
        if (emergencySupport) {
          if (daliDeviceHandling.emergencyParams.active == false) {
            TRACE(true, "EMERGENCY DT1 SUPPORT DETECTED");
            daliDeviceHandling.emergencyParams.active = true;
            Zigbee2Dali_SetEmergencyAddress(daliDeviceHandling.ecgParams.address[i]);
            Zigbee2Dali_SetDeviceType1Support(i, emergencySupport);
          }
        }
        if (!(daliDeviceHandling.ecgParams.deviceTypeMask[i] & (DALI_DEVICETYPE6|DALI_DEVICETYPE7|DALI_DEVICETYPE0)) && !colorSupport) { // Don't enable ballast endpoint unless present
          TRACE(true, "Skip ballast for emergency driver");
          Zigbee2Dali_DeactivateBallast(i);
          DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i, 0x0101, false);
          Zigbee2Dali_SetEndpointForDeviceIndex(ZIGBEE2DALI_EMERGENCY_ENDPOINT, i);
          continue;
        }
        DaliDeviceHandlingAddNewDevice(daliDeviceHandling.ecgParams.address[i]);
        Zigbee2Dali_SetBallastAddress(i, daliDeviceHandling.ecgParams.address[i]);
        Zigbee2Dali_SetDaliVersion(i, (daliDeviceHandling.ecgParams.deviceTypeMask[i] & DALI_DEVICE_EDITION2) ? DALI_EDITION2 :DALI_EDITION1);
        Zigbee2Dali_SetDeviceType8Support(i, colorSupport);
        TRACE(true, "COLOR SUPPORT %sDETECTED, EP%u", colorSupport ? "" : "NOT ", ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i);
        DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i, colorSupport ? 0x010C : 0x0101, true);
        Zigbee2Dali_SetEndpointForDeviceIndex(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i, i);
        Zigbee2Dali_StartDevice(i);
        TRACE(true, "Device %d, FeatureMask 0x%4X", i, daliDeviceHandling.ecgParams.deviceTypeMask[i]);
        DaliDeviceHandling_SetDevicesInfo(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i, &daliDeviceHandling.ecgInfo[i]);
      }
      else {
        Zigbee2Dali_DeactivateBallast(i);
        DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + i, 0x0101, false);
        Zigbee2Dali_SetEndpointForDeviceIndex(0, i);
      }
    }
  }
  if (daliDeviceHandling.emergencyParams.active) {
    DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_EMERGENCY_ENDPOINT, 0xF008, true);
    Zigbee2Dali_EmergencyInit();
  }
  else {
    DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_EMERGENCY_ENDPOINT, 0xF008, false);
  }
  if(daliDeviceHandling.ecgParams.broadcastMode ||
      (daliDeviceHandling.ecgParams.address[0] == SHORT_ADDRESS_MASK &&
       daliDeviceHandling.ecgParams.address[1] == SHORT_ADDRESS_MASK &&
       !daliDeviceHandling.sensorParams.active &&
       !(daliDeviceHandling.deviceConfig & PUSHBUTTON_ENDPOINT_ACTIVE))) {
    TRACE(true, "BROADCAST MODE ON");
    bool colorSupport = daliDeviceHandling.ecgParams.deviceTypeMask[0] & DALI_DEVICETYPE8;
    Zigbee2Dali_SetBroadcastMode(true);
    Zigbee2Dali_SetDaliVersion(0, (daliDeviceHandling.ecgParams.deviceTypeMask[0] & DALI_DEVICE_EDITION2) ? DALI_EDITION2 :DALI_EDITION1);
    Zigbee2Dali_SetBallastAddress(0, DALI_SA_BROADCAST);
    Zigbee2Dali_SetDeviceType8Support(0, colorSupport);
    Zigbee2Dali_StartDevice(0);
    TRACE(true, "Device 0, FeatureMask 0x%4X", daliDeviceHandling.ecgParams.deviceTypeMask[0]);
    Zigbee2Dali_DeactivateBallast(1);
    Zigbee2Dali_SetEndpointForDeviceIndex(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT, 0);
    DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + 0, colorSupport ? 0x010C : 0x0101, true); // arjun original true last param.
    DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + 1, 0x0101, false);
    DaliDeviceHandling_SetDevicesInfo(ZIGBEE2DALI_LIGHT_OUT_ENDPOINT + 0, &daliDeviceHandling.ecgInfo[0]);
  }
  if (daliDeviceHandling.sensorParams.active) {
    // currently assuming only 1 sensor is connected & stored at index 0
    Zigbee2Dali_SetPresenceSensorAddress(0, daliDeviceHandling.sensorParams.address[0]);
    Zigbee2Dali_SetLightSensorAddress(0, daliDeviceHandling.sensorParams.address[1]);
    Zigbee2Dali_SetSensorMode(daliDeviceHandling.sensorParams.deviceTypeMask[0]);
  }
  else {
    Zigbee2Dali_DeactivateSensor(0);
  }
  DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT,    0x0106, daliDeviceHandling.sensorParams.active);
  DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT, 0x0107, daliDeviceHandling.sensorParams.active);
  if(daliDeviceHandling.sensorParams.active) {
    DaliDeviceHandling_SetDevicesInfo(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT, &daliDeviceHandling.sensorInfo[0]);
    DaliDeviceHandling_SetDevicesInfo(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, &daliDeviceHandling.sensorInfo[1]);
  }
#if defined(ZIGBEE2DALI_PUSHBUTTON_ENDPOINT)
  DaliDeviceHandling_SetEndpointType(ZIGBEE2DALI_PUSHBUTTON_ENDPOINT, 0x0104, daliDeviceHandling.deviceConfig & PUSHBUTTON_ENDPOINT_ACTIVE);
#endif
  return initDone;
}
/// @}

