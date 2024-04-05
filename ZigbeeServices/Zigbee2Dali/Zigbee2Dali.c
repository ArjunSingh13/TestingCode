//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file Zigbee2Dali.c
///   \since 2016-11-17
///   \brief Services for all Zigbee clusters which need Dali communication
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \defgroup Zigbee2DALI
/// \brief All aspects of the ZigBee and DALI component of this solution are contained here
///
/// In addition to respective components of each protocol, conversion of parameters
/// from ZigBee to DALI and vice versa is found in the @ref LevelControlClusterServer
/// submodule.
/// \defgroup DALI DALI commands
/// \ingroup Zigbee2DALI
/// @{
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// --------- Includes ----------------------------------------------------------

#include "GlobalDefs.h"
#include "app/framework/include/af.h"

#include "IlluminanceMeasurementServices.h"
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)                               // The Encelium plugins are used
  #include "EnceliumInterfaceServer.h"
#endif
#include "OccupancySensingServices.h"
#include "LevelControlServices.h"
#include "BallastConfigurationServices.h"
#include "ColorControlServices.h"
#include "app/framework/plugin/osramds-basic-server/BasicServer.h"
#include "app/framework/plugin/ota-client/ota-client.h"
#include "IdentifyClient.h"

#include "DaliTelegram.h"
#include "DaliCommands.h"                                                       // list of Dali commands available
#include "Zigbee2Dali.h"
#include "Zigbee2DaliPrivate.h"
//#include "daliConversions.h"
#include "DaliDeviceHandling.h"
#include "math.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF, __VA_ARGS__);}
#define DALI_SENSOR_MAX_LUX             1000

// --------- Type Definitions --------------------------------------------------

// --------- Local Variables ---------------------------------------------------

// --------- Global Variables --------------------------------------------------

TypeZigbee2Dali zigbee2Dali;

const TypeZigbee2Dali zigbee2DaliDEFAULT = {  .mode=DALI_MODE_INIT,
                                              .sensor.active=false, .sensor.addressPD=0x80, .sensor.addressLight=0x80, .sensor.errorCounter=0, .sensor.lightSensor.lastValue = 2000,
                                              .networkService.leave=false,
                                              .powerOnCounter_FTT=DALI_POWER_ON_DELAY_FTT,
                                              .emergency.batteryCharge=0xFF, .emergency.level=0xFF, .emergency.mode=0xFF, .emergency.failureStatus=0xFF, .emergency.status=0xFF
};
const TypeDevice zigbee2DaliDeviceDEFAULT = { .active = false, .mode = DEVICE_MODE_INIT, .light.targetLevel = UINT8_MASK, .actLevel = UINT8_MASK, .address = 0x80,
                                              .minLevel = UINT8_MASK, .colorTempMin = UINT16_MASK, .colorTempMax = UINT16_MASK, .errorCounter=0, .fadeTime = 0xFF, .actMirek = UINT16_MASK};

#ifdef FADE_TEST
  uint32_t testTimer = 0;
#endif

// --------- Local Function Prototypes -----------------------------------------

void Zigbee2DaliModeControl(uint32_t ticks);
void Zigbee2DaliDevice(uint32_t ticks);
void Zigbee2DaliSensor(uint32_t ticks);
void Zigbee2DaliEmergency(uint32_t ticks);
static void Zigbee2DaliInitAttributes(void);
void Zigbee2DaliSetEmergencyTestExecutionTimeout(uint8_t value);

//Command callbacks
void Zigbee2DaliQueryLevelResponseCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliSensorHighCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliSensorLowCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliQueryStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliUpdateTargetLevelCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliQueryBallastCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliQueryLampFailureCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliUpdateColorTempFirstCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliUpdateColorTempSecondCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyBatteryChargeCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyLevelCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyModeCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyFeaturesCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyFailureStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);
void Zigbee2DaliEmergencyStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command);

// Helpers for interfacing DALI structs to Zigbee Endpoints
static uint8_t Zigbee2DaliGetDeviceIndexFromEndpoint(uint8_t endpoint);
static uint8_t Zigbee2DaliGetEndpointFromDeviceIndex(uint8_t index);
static bool Zigbee2DaliIsLightEndpoint(uint8_t endpoint);

void Zigbee2DaliStartFadeTimeEd1(uint8_t endpoint, uint8_t level, uint16_t fadeTime_0sx);
void Zigbee2DaliStartFadeTimeEd2(uint8_t endpoint, uint8_t level, uint16_t fadeTime_0sx);
void Zigbee2DaliCalcActLevel(TypeDevice *pDevice);
uint8_t Zigbee2DaliCalcFadeTarget(TypeDevice *pDevice);


// DevicePolling
static TypeRefreshMODE Zigbee2DaliNextRefreshMode(TypeZigbee2Dali *pZigbee2Dali);
static void Zigbee2DaliQueryStatus(TypeDevice *pDevice);
static void Zigbee2DaliQueryLampFailure(TypeDevice *pDevice);
static void Zigbee2DaliUpdateTargetLevel(TypeDevice *pDevice);
static void Zigbee2DaliRefresh(TypeZigbee2Dali *zigbee2Dali, TypeDevice *pDevice);

uint8_t ZigbeeDiv32by32 (uint32_t dividend, uint32_t divisor);

// --------- Local Functions ---------------------------------------------------

void Zigbee2DaliModeControl(uint32_t ticks) {
  switch (zigbee2Dali.mode) {
  case DALI_MODE_INIT:
    Zigbee2Dali_InitDevices();
    zigbee2Dali.mode = DALI_MODE_START;
    break;
    case DALI_MODE_START:
      // Actualize Zigbee Endpoints
      Zigbee2DaliInitAttributes();
      zigbee2Dali.mode = DALI_MODE_NORMAL;
      break;
    case DALI_MODE_DEVICE_ADDRESSING:
      if (DaliDeviceHandling_IsAddressing()) {
        DaliDeviceHandling_Tick();
      }
      else {
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
        EnceliumInterfaceServer_RescanInterfaceFinished(ZIGBEE2DALI_SAK_ENDPOINT);
#endif
        zigbee2Dali.mode = DALI_MODE_START;
        for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
          if(zigbee2Dali.device[i].active) {
            zigbee2Dali.device[i].mode = DEVICE_MODE_START;                     // zigbee2Dali.device[i].mode changes from START to NORMAL
          }
          else {
            zigbee2Dali.device[i].mode = DEVICE_MODE_INACTIVE;                  // zigbee2Dali.device[i].mode changes from START to INACTIVE
          }
        }
      }
      break;
    case DALI_MODE_NORMAL:
      Zigbee2DaliNextRefreshMode(&zigbee2Dali);
      Zigbee2DaliDevice(ticks);
      if (zigbee2Dali.sensor.active) {
        Zigbee2DaliSensor(ticks);
      }
      if (zigbee2Dali.emergency.active) {
        Zigbee2DaliEmergency(ticks);
      }
      break;
    case DALI_MODE_MEMBANK:
      //MemoryBank_Tick();
      break;
    case DALI_MODE_BUS_FAILURE:
      // add code here
      break;
    case DALI_MODE_MFG_TEST:
      break;
    default:
      break;
  }
}



void Zigbee2DaliSensor(uint32_t ticks) {
  if (zigbee2Dali.sensor.pollTime_FTT > 0) {
    zigbee2Dali.sensor.pollTime_FTT -= ticks;
  }
  else {
    zigbee2Dali.sensor.pollTime_FTT = Z2D_LIGHT_POLL_TIME_FTT;
    if(zigbee2Dali.sensor.mode == 2) {
      DaliTelegram_Send3ByteCommand2AddressWithCallback(zigbee2Dali.sensor.addressLight, (0xC4 << 8 | DALI_SENSOR_QUERY_INPUT_VALUE), DALI_WAIT4ANSWER, 0, Zigbee2DaliSensorHighCallback);
    } else {
      DaliTelegram_Send3ByteCommand2AddressWithCallback(zigbee2Dali.sensor.addressLight, SENSOR_QUERY_LIGHT_HIGH, DALI_WAIT4ANSWER, 0, Zigbee2DaliSensorHighCallback);
  }
  }
}

void Zigbee2DaliEmergency(uint32_t ticks) {
  if (zigbee2Dali.emergency.pollTime_FTT > 0) {
    zigbee2Dali.emergency.pollTime_FTT -= ticks;
  }
  else {
    if(DaliTelegram_IsDaliBusy()) { return; } // postpone is DALI is busy

    const uint8_t daliCommand[] = {
        DALI_CMD_DT1_QUERY_FEATURES,
        DALI_CMD_DT1_QUERY_BATTERY_CHARGE,
        DALI_CMD_DT1_QUERY_LEVEL,
        DALI_CMD_DT1_QUERY_MODE,
        DALI_CMD_DT1_QUERY_FAILURE_STATUS,
        DALI_CMD_DT1_QUERY_STATUS,
    };
    TypeDaliCallback const callback[] = {
        Zigbee2DaliEmergencyFeaturesCallback,
        Zigbee2DaliEmergencyBatteryChargeCallback,
        Zigbee2DaliEmergencyLevelCallback,
        Zigbee2DaliEmergencyModeCallback,
        Zigbee2DaliEmergencyFailureStatusCallback,
        Zigbee2DaliEmergencyStatusCallback,
    };
    if (zigbee2Dali.emergency.errorCounter >= Z2D_OFFLINE_FAILURE_COUNTER) {
      zigbee2Dali.emergency.errorCounter = Z2D_OFFLINE_FAILURE_COUNTER;
      emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_EMERGENCY_ENDPOINT, true);
    }
    else {
      emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_EMERGENCY_ENDPOINT, false);
    }

    static uint8_t state = 0;
    DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommand2AddressWithCallback(zigbee2Dali.emergency.address, daliCommand[state], DALI_WAIT4ANSWER | DALI_SAFE_QUERY, 0, callback[state]);
    if (++state > 5)
      state = 1;
    zigbee2Dali.emergency.pollTime_FTT = Z2D_EMERGENCY_POLL_TIME_FTT;
  }
}

void Zigbee2DaliFadeStarted(TypeDaliTelegramResponse response, TypeDaliFrame command){
  if(!zigbee2Dali.ecgBroadcastMode) {
    if (command.fwf.shortAddressed.multicast) return;
    for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if (zigbee2Dali.device[i].address == command.fwf.shortAddressed.address) {
        zigbee2Dali.device[i].light.fading = true;
      }
    }
  }
  else {
    zigbee2Dali.device[0].light.fading = true;
  }
}

void Zigbee2DaliDevice(uint32_t ticks) {
  TypeDevice *pDevice;
  for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    pDevice = &(zigbee2Dali.device[i]);
    switch (zigbee2Dali.device[i].mode) {
    case DEVICE_MODE_INIT:
      break;                                                                    // should never be reached: zigbee2Dali.device[i].mode changes from INIT to START / FAILURE before Zigbee2DaliDevice() is called
    case DEVICE_MODE_START:                                                     // zigbee2Dali.device[i].mode changes from START to NORMAL
      pDevice->mode = DEVICE_MODE_NORMAL;
      if (pDevice->active) {
        LevelControlServer_RefreshCurrentOutputLevel(Zigbee2DaliGetEndpointFromDeviceIndex(i));
      }
      break;
    case DEVICE_MODE_FAILURE:
    case DEVICE_MODE_NORMAL:
      if (pDevice->active) {
         Zigbee2DaliRefresh(&zigbee2Dali, pDevice);
      }
      break;
    case DEVICE_MODE_IDENTIFY:
      if ( pDevice->identify.delay_FTT > 0 ) {
        pDevice->identify.delay_FTT--;
      }
      else {
        if(pDevice->daliVersion >= DALI_EDITION2) {
          pDevice->identify.delay_FTT = DALI_TIME_IDENTIFY_RESEND_FTT;              // re-send after this time
          DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_IDENTIFY, DALI_SEND_TWICE, 0);
        }
        else {
          pDevice->identify.delay_FTT = DALI_TIME_IDENTIFY_TOGGLE_FTT;              // toggle
          pDevice->identify.on = !pDevice->identify.on;
          if (pDevice->identify.on) {
            DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_RECALL_MAX, DALI_SEND_NORMAL, 0);
          }
          else {
            uint8_t command = (pDevice->minLevel < 254) ? DALI_CMD_RECALL_MIN : DALI_CMD_OFF; // for handling non dimmable devices
            DaliTelegram_SendStdCommand2Address(pDevice->address, command, DALI_SEND_NORMAL, 0);
          }
        }
      }
      break;
    case DEVICE_MODE_FADING:
      if ( pDevice->light.fadeCounter_FTT > 0 ) {
        if ( (pDevice->light.fading) ) {                                        // start counting down after DAPC was sent
          if(pDevice->light.fadeCounter_FTT > ticks) { pDevice->light.fadeCounter_FTT -= ticks; }
          else { pDevice->light.fadeCounter_FTT = 0; }                          // don't give fadeCounter_FTT any chance to roll-back b/c of compiler issues

          if ( pDevice->light.delayCounter_FTT > 0) {
            pDevice->light.delayCounter_FTT -= ticks;                           // starts counting down after DAPC(1stTarget) is sent successfully
          }
          else {
            if ( pDevice->light.delay_FTT > 0 ) {                               // if reload-value is 0: only command[3]-target must be calculated
              uint8_t newTarget;
              uint32_t firstDelay_0sx = pDevice->light.delay_FTT * (uint32_t)(52429);    // 52429 = 2^21/(100/2.5)
              firstDelay_0sx >>= 21;
              if(firstDelay_0sx != pDevice->light.fadeTimeDali_0sx) {           // the initial case where delay_FTT is the remainder from the supported fade time
                pDevice->light.fadeTimeDali_0sx_next += firstDelay_0sx + pDevice->light.fadeTimeDali_0sx;
              }
              else {
                pDevice->light.fadeTimeDali_0sx_next += pDevice->light.fadeTimeDali_0sx;
              }
              newTarget = Zigbee2DaliCalcFadeTarget(pDevice);
              newTarget = (i<<9)+ newTarget;

              uint32_t dali_FTT;
              dali_FTT = pDevice->light.fadeTimeDali_0sx * (uint32_t)(100 / 2.5);
              pDevice->light.delayCounter_FTT = dali_FTT;
              pDevice->light.delay_FTT = dali_FTT;
              DaliTelegram_SendDap2AddressWithCallback(pDevice->address, newTarget, DALI_SEND_NORMAL, 0, Zigbee2DaliFadeStarted);
            } //if ( pDevice->light.delay_FTT > 0 )
          } // else
        } // if ( (pDevice->light.fading) )
      } // if (pDevice->light.fadeCounter_FTT > 0)
      else {
        //DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_ACT_LEVEL, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryLevelResponseCallback);
        pDevice->actLevel = pDevice->light.targetLevel;
        pDevice->light.fading = false;
        pDevice->mode = DEVICE_MODE_NORMAL;
      }
      break;
    case DEVICE_MODE_INACTIVE:
      break;
    default:                                                                    // invalid mode, should never be reached!
      pDevice->mode = DEVICE_MODE_NORMAL;
      break;
    } // switch zigbee2Dali.device[i].mode
  } // for all ECG
}


void Zigbee2DaliQueryLevelResponseCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    if (command.fwf.shortAddressed.multicast) return;
    for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if (zigbee2Dali.device[i].address == command.fwf.shortAddressed.address) {
        zigbee2Dali.device[i].actLevel = response.answer;
        //uint8_t linearLevel = Dali2Zigbee(response.answer); // maybe we set DT6 to linear, then we have to decide here translate or not
        //LevelControlServer_ActualLevelCallback(Zigbee2DaliGetEndpointFromDeviceIndex(i), linearLevel);
      }
    }
  }
}

void Zigbee2DaliSensorHighCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.sensor.errorCounter = 0;
    if(zigbee2Dali.sensor.mode == 2) {
    zigbee2Dali.sensor.lightSensor.actValue.bytes.high = response.answer; // TODO: Z2D check math here
    DaliTelegram_Send3ByteCommand2AddressWithCallback(zigbee2Dali.sensor.addressLight, (0xC4 << 8 | DALI_SENSOR_QUERY_INPUT_VALUE_LATCH), DALI_WAIT4ANSWER, 0, Zigbee2DaliSensorLowCallback);
    } else {
      zigbee2Dali.sensor.lightSensor.actValue.bytes.high = response.answer;
      DaliTelegram_Send3ByteCommand2AddressWithCallback(zigbee2Dali.sensor.addressLight,  SENSOR_QUERY_LIGHT_LOW, DALI_WAIT4ANSWER, 0, Zigbee2DaliSensorLowCallback);
    }
  }
  else {
    if(++zigbee2Dali.sensor.errorCounter >= Z2D_OFFLINE_FAILURE_COUNTER) {
      emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, true);
      emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT, true);
      IlluminanceMeasurementServer_ActualLightCallback(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, 0xFFFF); // Set the lightvalue to invalid
      zigbee2Dali.sensor.errorCounter = Z2D_OFFLINE_FAILURE_COUNTER;
    }
  }
}

void Zigbee2DaliSensorLowCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    int32_t delta;
    int32_t avgDelta;
    uint16_t reverseWinkThreshold = IlluminanceMeasurementServer_GetReverseWinkThreshold(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT);
    emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, false);       // Clear fault if high and low byte are received
    emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT, false);
    if(zigbee2Dali.sensor.mode == 2) {
      zigbee2Dali.sensor.lightSensor.actValue.bytes.low = response.answer; // TODO: Z2D Check math here.
    } else {
      zigbee2Dali.sensor.lightSensor.actValue.bytes.low = response.answer;
    }
    delta = zigbee2Dali.sensor.lightSensor.actValue.full - zigbee2Dali.sensor.lightSensor.lastValue;
    avgDelta = (delta + zigbee2Dali.sensor.lightSensor.lastDelta) / 2;
    TRACE(true, "Reverse Wink Filter: Last:%u Act:%u lDelta:%d delta:%d Threshold:%u",
                zigbee2Dali.sensor.lightSensor.lastValue,
                zigbee2Dali.sensor.lightSensor.actValue.full,
                zigbee2Dali.sensor.lightSensor.lastDelta,
                delta,
                reverseWinkThreshold);
    if(avgDelta > reverseWinkThreshold) {
      if(!zigbee2Dali.sensor.lightSensor.reverseWinkTriggered) {
        zigbee2Dali.sensor.lightSensor.reverseWinkTriggered = true;
        IdentifyClient_TriggerReverseWink(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT);
        TRACE(true, "Trigger Reverse Wink");
      }
    }
    else {
      zigbee2Dali.sensor.lightSensor.reverseWinkTriggered = false;
    }
    zigbee2Dali.sensor.lightSensor.lastValue = zigbee2Dali.sensor.lightSensor.actValue.full;
    zigbee2Dali.sensor.lightSensor.lastDelta = delta;

    // Scale measurement;
    uint16_t zigbeeIlluminance;
    if(zigbee2Dali.sensor.mode == 2) {
      //DALI2 has the MSB of the input value left justified. Since we only need relative values for this, don't make
      // a bad assumption and try to align it with a fixed value. The sensor could have a different resolution.
      //log10(2^16) = 4.81648. Max 16-bit dynamic range for result would be 13606 * log10(2^16). 10000 is fine.
      // using a +1.0 to slide the range out of the problematic region of log10().
      float logIlluminance = 10000 * log10((float)(zigbee2Dali.sensor.lightSensor.actValue.full) + 1.0);
      zigbeeIlluminance = (uint16_t)logIlluminance; //mathematically must be between 0 and 48165 so it fits.
      TRACE(true, "LightSensor: raw:%u zigbee %u", zigbee2Dali.sensor.lightSensor.actValue.full, zigbeeIlluminance)
    } else {
      uint32_t luxCalibrationMultiplier = DaliDeviceHandling_FindLuxCalibrationMultiplier();
      float scaledIlluminance = ((float) zigbee2Dali.sensor.lightSensor.actValue.full / 256) * ((float) luxCalibrationMultiplier / 0x10000);
      float convertedIlluminance = 10000 * log10(scaledIlluminance + 1);
      if (convertedIlluminance > 65534) convertedIlluminance = 65534.0;  // just limit it here 0xfffe means invalid.
      zigbeeIlluminance =  (uint16_t) convertedIlluminance;
      TRACE(true, "LightSensor: raw:%u scaled:%u zigbee %u", zigbee2Dali.sensor.lightSensor.actValue.full, (uint16_t) scaledIlluminance, zigbeeIlluminance)
    }
    IlluminanceMeasurementServer_ActualLightCallback(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, zigbeeIlluminance);
  }
}

void Zigbee2DaliQueryStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  TypeDevice *pDevice;
  if ((command.fwf.shortAddressed.multicast) || (zigbee2Dali.ecgBroadcastMode))return;
  for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    pDevice = &(zigbee2Dali.device[i]);
    if( (pDevice->address == command.fwf.shortAddressed.address)
        && (pDevice->active)) {
      uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(i);
      if(response.answerType == DALI_ANSWER_OK) { // at least one device present, therefore no fault
        pDevice->errorCounter = 0;
        pDevice->ecgStatus = response.answer;
        BallastConfigurationServer_SetBallastStatus(endpoint, (pDevice->ecgStatus & 0x03));
        emberAfPluginOsramdsBasicServerSetHardwareFault(endpoint, false);
        if (pDevice->mode == DEVICE_MODE_FAILURE) pDevice->mode = DEVICE_MODE_NORMAL;
      }
      else {
        if (++pDevice->errorCounter >= Z2D_OFFLINE_FAILURE_COUNTER) {
          pDevice->errorCounter = Z2D_OFFLINE_FAILURE_COUNTER;
          pDevice->mode = DEVICE_MODE_FAILURE;
          emberAfPluginOsramdsBasicServerSetHardwareFault(endpoint, true);
        }
        else {
          DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_STATUS, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryStatusCallback);
        }
      }
    } // if address is the same
  } // for each ECG
}

void Zigbee2DaliUpdateTargetLevelCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    if (command.fwf.shortAddressed.multicast) return;
    for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if ((zigbee2Dali.device[i].address == DALI_SA_BROADCAST) ||
          (zigbee2Dali.device[i].address == command.fwf.shortAddressed.address)) {
        if(zigbee2Dali.device[i].actLevel == response.answer) {
          break;
        }
        else {
          if (zigbee2Dali.device[i].actLevel != UINT8_MASK) {
            DaliTelegram_SendDap2Address(zigbee2Dali.device[i].address, zigbee2Dali.device[i].actLevel, DALI_SEND_NORMAL, 0);
          }
        }
      } // if address is the same
    } // for each ECG
  } // DALI_ANSWER_OK
}

void Zigbee2DaliQueryBallastCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if (zigbee2Dali.ecgBroadcastMode && command.fwf.shortAddressed.multicast) {
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(0);
    TypeDevice *pDevice = &(zigbee2Dali.device[0]);
    if(response.answerType != DALI_NO_ANSWER) { // at least one device present, therefore no fault
      pDevice->errorCounter = 0;
      if (pDevice->mode == DEVICE_MODE_FAILURE) pDevice->mode = DEVICE_MODE_NORMAL;
      emberAfPluginOsramdsBasicServerSetHardwareFault(endpoint, false);
    }
    else {
      if (++pDevice->errorCounter >= Z2D_OFFLINE_FAILURE_COUNTER) {
        pDevice->errorCounter = Z2D_OFFLINE_FAILURE_COUNTER;
        pDevice->mode = DEVICE_MODE_FAILURE;
        emberAfPluginOsramdsBasicServerSetHardwareFault(endpoint, true);
      }
      else {
        DaliTelegram_SendStdCommandBroadcastWithCallback(DALI_QUERY_GEAR, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryBallastCallback);
      }
    }
  }
}

void Zigbee2DaliQueryLampFailureCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if (zigbee2Dali.ecgBroadcastMode && command.fwf.shortAddressed.multicast) {
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(0);
    TypeDevice *pDevice = &(zigbee2Dali.device[0]);
    if(response.answerType == DALI_NO_ANSWER) {
      uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(0);
      pDevice->ecgStatus = 0x00;    // &= ~0x02;  actual only lampfault in bcast mode
      BallastConfigurationServer_SetBallastStatus(endpoint, pDevice->ecgStatus);
    }
    else {
      pDevice->ecgStatus = 0x02;   // actual only lampfault in bcast mode
      BallastConfigurationServer_SetBallastStatus(endpoint, pDevice->ecgStatus);
    }
  }
}

void Zigbee2DaliUpdateColorTempFirstCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    if(command.fwf.shortAddressed.multicast) return;
    for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if( (zigbee2Dali.device[i].address == DALI_SA_BROADCAST ) ||
          (zigbee2Dali.device[i].address == command.fwf.shortAddressed.address) ) {
        if( ((zigbee2Dali.device[i].actMirek & 0xFF00) >> 8) == response.answer ) { // if MSB of color temp matches actual value query the LSB
          DaliTelegram_SendStdCommand2AddressWithCallback(zigbee2Dali.device[i].address, DALI_QUERY_CONTENT_DTR0, DALI_WAIT4ANSWER, 0, Zigbee2DaliUpdateColorTempSecondCallback);
        }
        else {
          if(zigbee2Dali.device[i].actMirek != UINT16_MASK) {
            DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, (uint8_t)((zigbee2Dali.device[i].actMirek >> 8 ) & 0x00FF), DALI_SEND_NORMAL, 0);
            DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, (uint8_t)(zigbee2Dali.device[i].actMirek), DALI_SEND_NORMAL, 0);
            DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
            DaliTelegram_SendStdCommand2Address(zigbee2Dali.device[i].address, DALI_CMD_DT8_SET_TEMP_COLOR, DALI_SEND_NORMAL, 0);
            DaliTelegram_SendDap2Address(zigbee2Dali.device[i].address, zigbee2Dali.device[i].actLevel, DALI_SEND_NORMAL, 0);
          }
        }
      } // if address is the same
    } // for each ECG
  } // DALI_ANSWER_OK
}

void Zigbee2DaliUpdateColorTempSecondCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    if(command.fwf.shortAddressed.multicast) return;
    for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
      if( (zigbee2Dali.device[i].address == DALI_SA_BROADCAST ) ||
          (zigbee2Dali.device[i].address == command.fwf.shortAddressed.address) ) {
        if( abs((zigbee2Dali.device[i].actMirek & 0xFF) - response.answer) < 10 ) { // 10 is tolerance for conversion approximation errors
          break;
        }
        else {
          if(zigbee2Dali.device[i].actMirek != UINT16_MASK) {
            DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, (uint8_t)((zigbee2Dali.device[i].actMirek >> 8 ) & 0x00FF), DALI_SEND_NORMAL, 0);
            DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, (uint8_t)(zigbee2Dali.device[i].actMirek), DALI_SEND_NORMAL, 0);
            DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
            DaliTelegram_SendStdCommand2Address(zigbee2Dali.device[i].address, DALI_CMD_DT8_SET_TEMP_COLOR, DALI_SEND_NORMAL, 0);
            DaliTelegram_SendDap2Address(zigbee2Dali.device[i].address, zigbee2Dali.device[i].actLevel, DALI_SEND_NORMAL, 0);
          }
        }
      } // if address is the same
    } // for each ECG
  } // DALI_ANSWER_OK
}

#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
static uint8_t indexToEndpoint[ZIGBEE2DALI_ECG_MAX];
#endif

void Zigbee2Dali_SetEndpointForDeviceIndex(uint8_t endpoint, uint8_t index) {
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
  indexToEndpoint[index] = endpoint;
#endif
}

static uint8_t Zigbee2DaliGetDeviceIndexFromEndpoint(uint8_t endpoint) {
  uint8_t index = 0xFF;
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
  for (index = 0; index < ZIGBEE2DALI_ECG_MAX; index++) {
    if(endpoint == ZIGBEE2DALI_EMERGENCY_ENDPOINT &&  zigbee2Dali.device[index].deviceType1Supported){ // we have one device index for DT1 and DT6 of a device.
      return index;
    } else if (indexToEndpoint[index] == endpoint) {
      return index;
    }
  }
#else
  index = emberAfIndexFromEndpoint(endpoint);
#endif
  return index;
}

static uint8_t Zigbee2DaliGetEndpointFromDeviceIndex(uint8_t index) {
  uint8_t endpoint = 0x00;

  if( index < ZIGBEE2DALI_ECG_MAX) {
#if defined(ZCL_USING_ENCELIUM_INTERFACE_CLUSTER_SERVER)
    endpoint = indexToEndpoint[index];
#else
    endpoint = emberAfEndpointFromIndex(index);
#endif
  }
  // nothing else (endpoint is initialized)
  return endpoint;
}

static bool Zigbee2DaliIsLightEndpoint(uint8_t endpoint) {
  bool ret = false;
  uint8_t index = Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint);
  if (index < ZIGBEE2DALI_ECG_MAX) {
    ret = zigbee2Dali.device[index].active;
  }
  return ret;
}

static TypeRefreshMODE Zigbee2DaliNextRefreshMode(TypeZigbee2Dali *pZigbee2Dali) {
  pZigbee2Dali->refresh.refreshTimer_FTT++;

  if( pZigbee2Dali->refresh.refreshTimer_FTT % Z2D_REFRESH_MIN_PERIOD_FTT != 0) {
    pZigbee2Dali->refresh.state = REFRESH_MODE_IDLE;
    return REFRESH_MODE_IDLE;
  }

  if(DaliTelegram_IsDaliBusy()) {
    pZigbee2Dali->refresh.refreshTimer_FTT--;                                   // postpone triggering a state call
    pZigbee2Dali->refresh.state = REFRESH_MODE_IDLE;
    return REFRESH_MODE_IDLE;
  }

  pZigbee2Dali->refresh.state = pZigbee2Dali->refresh.nextState;
  uint8_t nextState = ((++pZigbee2Dali->refresh.state) % REFRESH_MODE_COUNT);
  nextState = (nextState == REFRESH_MODE_IDLE) ? ++nextState : nextState;       // skip idle state
  pZigbee2Dali->refresh.state = (TypeRefreshMODE)( nextState );                 // get the next state
  if(!pZigbee2Dali->ecgBroadcastMode) {                                         // in broadcast mode we skip fetching target level and go to query ballast state
    if( pZigbee2Dali->refresh.state == REFRESH_MODE_QUERY_LAMPFAILURE) {
      pZigbee2Dali->refresh.state++;
      if (pZigbee2Dali->refresh.state == REFRESH_MODE_COUNT) {
        pZigbee2Dali->refresh.state = (TypeRefreshMODE)(REFRESH_MODE_IDLE + 1); // skip counter enum
      }
    }
  }
  pZigbee2Dali->refresh.nextState = pZigbee2Dali->refresh.state;
  return pZigbee2Dali->refresh.state;
}

void Zigbee2DaliStartFadeTimeEd1(uint8_t endpoint, uint8_t level, uint16_t fadeTime_0sx) {   // ?return void, because an error cannot be forwarded by the calling function
  TypeLightControl *pLightControl;
  uint32_t sendDelay_0sx = 0;                                                   // delay between 1st FadeCommand and next (final) FadeCommand
  uint16_t fadeTimeDali_0sx = 0;
  uint8_t lightCommand;
  uint8_t daliFadeTime;

  //lightCommand = Zigbee2Dali(level);
  lightCommand = level;

  uint8_t index = Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint);
  TRACE(true, "StartFade Ed1 EP:%u index:%u level:%u Fadetime:%u", endpoint, index, lightCommand, fadeTime_0sx);
  if (index == 0xFF) {
    TRACE(true, "Invalid index!");
    return;
  }
  TypeDevice *pDevice = &(zigbee2Dali.device[index]);
  switch (pDevice->mode) {
  case DEVICE_MODE_INIT:                                                        // if no answer from this device yet: RETURN
  case DEVICE_MODE_FAILURE:                                                     // if no device with this address available: RETURN
    return;
  case DEVICE_MODE_FADING:                                                      // if this device is already fading ..
    if (pDevice->light.targetLevel == lightCommand ) {                          // if already fading to target level
      return;
    }
    Zigbee2DaliCalcActLevel(pDevice);                                           // start from actual target
    break;
  case DEVICE_MODE_START:
  case DEVICE_MODE_NORMAL:
  case DEVICE_MODE_IDENTIFY:
  case DEVICE_MODE_INACTIVE:
  default:
    break;
  }

  pDevice->mode = DEVICE_MODE_FADING;
  pLightControl = &(pDevice->light);
  pLightControl->targetLevel = lightCommand;

  if (fadeTime_0sx <= 996) {
    if     (fadeTime_0sx <   5) {
      daliFadeTime = 0;
    }
    else if(fadeTime_0sx <   8) {   // 700ms
      daliFadeTime = 1;
    }
    else if(fadeTime_0sx <  12) {   // 1000ms
        daliFadeTime = 2;
    }
    else if(fadeTime_0sx <  17) {   // 1400ms
      daliFadeTime = 3;
    }
    else if(fadeTime_0sx <  24) {   // 2000ms
      daliFadeTime = 4;
    }
    else if(fadeTime_0sx <  34) {   // 2800ms
      daliFadeTime = 5;
    }
    else if(fadeTime_0sx <  48) {   // 4000ms
      daliFadeTime = 6;
    }
    else if(fadeTime_0sx <  68) {   // 5700ms
      daliFadeTime = 7;
    }
    else if(fadeTime_0sx <  96) {   // 8000ms
      daliFadeTime = 8;
    }
    else if(fadeTime_0sx < 136) {   // 11300ms
      daliFadeTime = 9;
    }
    else if(fadeTime_0sx < 193) {   // 16000ms
      daliFadeTime = 10;
    }
    else if(fadeTime_0sx < 273) {   // 22600ms
      daliFadeTime = 11;
    }
    else if(fadeTime_0sx < 386) {   // 32000ms
      daliFadeTime = 12;
    }
    else if(fadeTime_0sx < 576) {   // 45300ms
      daliFadeTime = 13;
    }
    else if(fadeTime_0sx < 772) {   // 64000ms
      daliFadeTime = 14;
    }
    else {                       // 90500ms
      daliFadeTime = 15;
      fadeTimeDali_0sx = 905;
    }
  }
  else {
    sendDelay_0sx = fadeTime_0sx % 905;
    daliFadeTime = 15;
    fadeTimeDali_0sx = 905;
  }

  if (sendDelay_0sx > 0) {
    if ( fadeTime_0sx > (uint16_t)(2 * 60 * 10) ) {                             // multiple fading enabled for more than 2 minutes of fade time
      pLightControl->delay_FTT = sendDelay_0sx * (uint32_t)(100/GLOBAL_FastTIME_TICK_ms+0.5);
    }
    else {
      pLightControl->delay_FTT = 0;
    }

    pLightControl->delayCounter_FTT = sendDelay_0sx * (uint32_t)(100/GLOBAL_FastTIME_TICK_ms+0.5);
    pLightControl->fadeTimeDali_0sx_next = 0;
    pLightControl->fadeTimeDali_0sx = fadeTimeDali_0sx;
  }
  else {
    pLightControl->delayCounter_FTT = 0;
    pLightControl->delay_FTT = 0;
  }

  if(pDevice->fadeTime != daliFadeTime) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, daliFadeTime, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_FADE_TIME, DALI_SEND_TWICE, 0);
    pDevice->fadeTime = daliFadeTime;
  }
  if(pLightControl->delay_FTT > 0) {                                            // calculate first target
    pLightControl->fadeTimeDali_0sx_next = fadeTimeDali_0sx;
    lightCommand = Zigbee2DaliCalcFadeTarget(pDevice);
    DaliTelegram_SendDap2AddressWithCallback(pDevice->address, lightCommand, DALI_SEND_NORMAL, 0, Zigbee2DaliFadeStarted);
    pLightControl->fadeTimeDali_0sx_next = 0;
  }
  else {
    DaliTelegram_SendDap2AddressWithCallback(pDevice->address, lightCommand, DALI_SEND_NORMAL, 0, Zigbee2DaliFadeStarted);
  }
  pDevice->light.fading = false;

  pLightControl->fadeTimeZigbee_0sx = fadeTime_0sx;
  pLightControl->fadeCounter_FTT = (uint32_t)((fadeTime_0sx * 100 )/GLOBAL_FastTIME_TICK_ms);
  if (pLightControl->fadeCounter_FTT == 0) {pLightControl->fadeCounter_FTT = 1;}
  pLightControl->fadeTime_FTT = pLightControl->fadeCounter_FTT;
}

void Zigbee2DaliStartFadeTimeEd2(uint8_t endpoint, uint8_t level, uint16_t fadeTime_0sx) {   // return void, because an error cannot be forwarded by the calling function
  TypeLightControl *pLightControl;
  uint8_t daliExtendedFadeTime;
  uint32_t sendDelay_0sx = 0;                                                   // delay between 1st FadeCommand and next (final) FadeCommand
  uint16_t fadeTimeDali_0sx = 0;
  uint8_t lightCommand;

  //lightCommand = Zigbee2Dali(level);
  lightCommand = level;

  uint8_t index = Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint);
  TRACE(true, "StartFade Ed2 EP:%u index:%u level:%u Fadetime:%u", endpoint, index, lightCommand, fadeTime_0sx);
  if (index == 0xFF) {
    TRACE(true, "Invalid index!");
    return;
  }
  TypeDevice *pDevice = &(zigbee2Dali.device[index]);
  switch (pDevice->mode) {
  case DEVICE_MODE_INIT:                                                        // if no answer from this device yet: RETURN
  case DEVICE_MODE_FAILURE:                                                     // if no device with this address available: RETURN
    return;
  case DEVICE_MODE_FADING:                                                      // if this device is already fading ..
    if (pDevice->light.targetLevel == lightCommand) {                           // if already fading to target level
      return;
    }
    Zigbee2DaliCalcActLevel(pDevice);                                           // start from actual target
    break;
  case DEVICE_MODE_START:
  case DEVICE_MODE_NORMAL:
  case DEVICE_MODE_IDENTIFY:
  case DEVICE_MODE_INACTIVE:
  default:
    break;
  }

  pDevice->mode = DEVICE_MODE_FADING;

  pLightControl = &(pDevice->light);
  pLightControl->targetLevel = lightCommand;
  if (fadeTime_0sx <= 19) {                                                     // use Dali-FadeTimes x*100 ms
    if (fadeTime_0sx == 0) {
      daliExtendedFadeTime = 0;
      sendDelay_0sx = 0;
    }
    else if (fadeTime_0sx <= 16) {
      daliExtendedFadeTime = 0x10 + (fadeTime_0sx -1);                          // daliFadeMultiplier=0x10 (defines the fadeTime base value! S. Table 7 – Extended fade time - multiplier),  + daliFadeBase (defines the multiplier of the fadeTime base value! S. Table 6 – Extended fade time - base value)
      sendDelay_0sx = 0;
    }
    else {
      daliExtendedFadeTime = 0x1F;                                              // 100 ms * 16
      fadeTimeDali_0sx = 16;
      sendDelay_0sx = fadeTime_0sx - fadeTimeDali_0sx;
    }
  }
  else if (fadeTime_0sx <= 199) {                                               // 2 s .. 19.9 s:
    uint16_t fadeTime_s = (uint16_t)(((uint32_t)fadeTime_0sx * 0x0000CCCD) >> 19);  // 0xCCCD = 2^19/10, rounded up
    if ( fadeTime_s > 16) { fadeTime_s = 16;}
    fadeTimeDali_0sx = fadeTime_s *10;
    sendDelay_0sx = fadeTime_0sx - fadeTimeDali_0sx;
    daliExtendedFadeTime = 0x20 + (fadeTime_s-1);                               // use Dali-FadeTime .. and calculate 1st target, if sendDelay_0sx > 0 afterwards
  }
  else if (fadeTime_0sx <= 1799) {                                              // 20 s .. 179.9 s < 3 min: x * 10 s
    uint16_t fadeTime_10s = (uint16_t)(((uint32_t)fadeTime_0sx * 0x0000147B) >> 19);  // 0x147BD = 2^19/100, rounded up
    if ( fadeTime_10s > 16) { fadeTime_10s = 16;}
    fadeTimeDali_0sx = fadeTime_10s *100;
    sendDelay_0sx = fadeTime_0sx - fadeTimeDali_0sx;
    daliExtendedFadeTime = 0x30 + (fadeTime_10s-1);                             // use Dali-FadeTime .. and calculate 1st target, if sendDelay_0sx > 0 afterwards
  }
  else if (fadeTime_0sx <= (uint16_t)(17.9*60*10) ) {                           // 3 min .. 17.9 min: x * 1 min
    uint16_t fadeTime_min = (uint16_t)(((uint32_t)fadeTime_0sx * 0x0000036A) >> 19);  // 0x036A = 2^19/600, rounded up
    if ( fadeTime_min > 16) { fadeTime_min = 16;}
    fadeTimeDali_0sx = fadeTime_min *600;
    sendDelay_0sx = fadeTime_0sx - fadeTimeDali_0sx;                            // 0 or 0.1 s .. 0.9 s ( fadeTimeDali_0sx <=16) or 0.1 .. 1.9 ( fadeTimeDali_0sx >16)
    daliExtendedFadeTime = 0x40 + (fadeTime_min-1);                             // use Dali-FadeTime .. and calculate 1st target, if sendDelay_0sx > 0 afterwards
  }
  else {                                                                        // 18 min .. (0xFFFE*0.1s)= 109,225 min
    fadeTimeDali_0sx = 16 *600;                                                 // calculate for maximum Dali-fadeTime
    sendDelay_0sx = fadeTime_0sx - fadeTimeDali_0sx;                            // 1 min <= 1st sendDelay_0sx < 2 min. Following sendDelay: 1 min
    daliExtendedFadeTime = 0x4F;                                                // use Dali-FadeTime 16 minutes ..
  }
  // send commands

  if (sendDelay_0sx > 0) {                                                      // if fadeRateZigbee != FadeRateDali a 1st command to intermediate target and a 2nd command to target is necessary
    if ( fadeTime_0sx > (uint16_t)(17.9 * 60 * 10) ) {                          // multiple fading enabled for more than 17.9 minutes of fade time
      pLightControl->delay_FTT = sendDelay_0sx * (uint16_t)(100/GLOBAL_FastTIME_TICK_ms+0.5);
    }
    else {
      pLightControl->delay_FTT = 0;
    }
    pLightControl->delayCounter_FTT = sendDelay_0sx * (uint16_t)(100/GLOBAL_FastTIME_TICK_ms+0.5);
    pLightControl->fadeTimeDali_0sx_next = 0;
    pLightControl->fadeTimeDali_0sx = fadeTimeDali_0sx;
  }
  else {
    pLightControl->delayCounter_FTT = 0;
    pLightControl->delay_FTT = 0;
  }
  pLightControl->fadeTimeZigbee_0sx = fadeTime_0sx;                             // needed to calculate actual level, in case fade is interrupted.
  if(pDevice->fadeTime != daliExtendedFadeTime) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, daliExtendedFadeTime, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_ExtFADE_TIME, DALI_SEND_TWICE, 0);
    pDevice->fadeTime = daliExtendedFadeTime;
  }
  if(pLightControl->delay_FTT > 0) {                                            // calculate first target
    pLightControl->fadeTimeDali_0sx_next = fadeTimeDali_0sx;
    lightCommand = Zigbee2DaliCalcFadeTarget(pDevice);
    DaliTelegram_SendDap2AddressWithCallback(pDevice->address, lightCommand, DALI_SEND_NORMAL, 0, Zigbee2DaliFadeStarted);
    pLightControl->fadeTimeDali_0sx_next = 0;
  }
  else {
    DaliTelegram_SendDap2AddressWithCallback(pDevice->address, lightCommand, DALI_SEND_NORMAL, 0, Zigbee2DaliFadeStarted);
  }
  pDevice->light.fading = true;
  pLightControl->fadeCounter_FTT = fadeTime_0sx * (uint16_t)(100/GLOBAL_FastTIME_TICK_ms);
  if (pLightControl->fadeCounter_FTT == 0) {pLightControl->fadeCounter_FTT = 1;}
  pLightControl->fadeTime_FTT = pLightControl->fadeCounter_FTT;
}

void Zigbee2DaliCalcActLevel(TypeDevice *pDevice) {
  uint32_t fadeTime = pDevice->light.fadeTime_FTT - pDevice->light.fadeCounter_FTT; // <= 2^16 *40!
  fadeTime = (fadeTime != 0) ? fadeTime : 40;                                   // if 0 it means fadeCounter_FTT has not been decremented yet
  fadeTime *= (uint32_t)(1638);                                                 // 1638.4 = 2^16/(100/2.5)
  fadeTime += (1<<15);
  fadeTime >>= 16;
  pDevice->light.fadeTimeDali_0sx_next = fadeTime;
  pDevice->actLevel = Zigbee2DaliCalcFadeTarget(pDevice);
}

uint8_t Zigbee2DaliCalcFadeTarget(TypeDevice *pDevice) {
  TypeLightControl *pLightControl = &(pDevice->light);

  uint8_t actLevel = pDevice->actLevel;                                         // start calculating of next target: load start level ..
  uint8_t target = pLightControl->targetLevel;                                  // .. and target (low byte of command

  if ( actLevel == 0 )      { actLevel = pDevice->minLevel; }                   // if actLevel = 0: limit to minLevel
  if ( target == 0 )        { target = pDevice->minLevel; }                     // if target = 0: limit to minLevel
  if ( target == 255 )      { target = 254; }
  if ( (actLevel == target) || (actLevel == (target+1)) || (actLevel == (target-1)) ) {
    return target;
  }
  else {
    int8_t nextStep;
    int16_t nextTarget;
    if ( actLevel < target ) {
      uint32_t dividend = target - actLevel;
      dividend *= pLightControl->fadeTimeDali_0sx_next;
      nextStep = ZigbeeDiv32by32 (dividend, pLightControl->fadeTimeZigbee_0sx);
      if ( nextStep == 0)  {nextStep = 1;}                                      // to avoid no fading, if actLevel = nextTarget, therefore set to >0
      nextTarget = nextStep;
    }
    else {
      uint32_t dividend = actLevel - target;
      dividend *= pLightControl->fadeTimeDali_0sx_next;
      nextStep = ZigbeeDiv32by32 (dividend, pLightControl->fadeTimeZigbee_0sx);
      if ( nextStep == 0)  {nextStep = -1;}                                     // to avoid no fading, if actLevel = nextTarget, therefore set to >0
      nextTarget = -nextStep;
    }
    nextTarget += actLevel;
    return nextTarget;
  }
}

static void Zigbee2DaliQueryStatus(TypeDevice *pDevice) {
  if(!zigbee2Dali.ecgBroadcastMode) {
    DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_STATUS, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryStatusCallback);
  }
  else {
    DaliTelegram_SendStdCommandBroadcastWithCallback(DALI_QUERY_GEAR, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryBallastCallback);
  }
}

static void Zigbee2DaliUpdateTargetLevel(TypeDevice *pDevice) {
  if (pDevice->mode == DEVICE_MODE_FAILURE) return;    // Nothing to do if offline
  if(!zigbee2Dali.ecgBroadcastMode) {
    DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address,
                                                    DALI_QUERY_ACT_LEVEL,
                                                    (pDevice->mode == DEVICE_MODE_FAILURE) ? DALI_WAIT4ANSWER : (DALI_WAIT4ANSWER),
                                                    0,
                                                    Zigbee2DaliUpdateTargetLevelCallback);
  }
  else{
    if (zigbee2Dali.device[0].actLevel != UINT8_MASK) {
      DaliTelegram_SendDapBroadcast(zigbee2Dali.device[0].actLevel, DALI_SEND_NORMAL, 0);
    } // no else
  }
}

static void Zigbee2DaliQueryLampFailure(TypeDevice *pDevice) {
  if (pDevice->mode == DEVICE_MODE_FAILURE) return;    // Nothing to do if offline
  DaliTelegram_SendStdCommandBroadcastWithCallback(DALI_QUERY_LAMP_FAILURE, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryLampFailureCallback);
}

static void Zigbee2DaliUpdateColorTemp(TypeDevice *pDevice) {
  if(pDevice->mode == DEVICE_MODE_FAILURE) return;    // Nothing to do if offline
  if(!zigbee2Dali.ecgBroadcastMode) {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0x02, DALI_SEND_NORMAL, 0); // 0x02 sets DTR to color temperature query attribute
    DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_COLOR_VALUE, DALI_WAIT4ANSWER, 0, Zigbee2DaliUpdateColorTempFirstCallback);
  }
  else {
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, (uint8_t)((zigbee2Dali.device[0].actMirek >> 8 ) & 0x00FF), DALI_SEND_NORMAL, 0);
    DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, (uint8_t)(zigbee2Dali.device[0].actMirek), DALI_SEND_NORMAL, 0);
    DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendStdCommandBroadcast(DALI_CMD_DT8_SET_TEMP_COLOR, DALI_SEND_NORMAL, 0);
    DaliTelegram_SendDapBroadcast(zigbee2Dali.device[0].actLevel, DALI_SEND_NORMAL, 0);
  }
}

static void Zigbee2DaliRefresh(TypeZigbee2Dali *zigbee2Dali, TypeDevice *pDevice) {
  if(zigbee2Dali != NULL) {
    switch (zigbee2Dali->refresh.state) {
    case REFRESH_MODE_QUERY_STATUS:
      if(pDevice != NULL) {
        Zigbee2DaliQueryStatus(pDevice);
      }
      break;
    case REFRESH_MODE_UPDATE_TARGET_LEVEL:
      if(pDevice != NULL) {
          Zigbee2DaliUpdateTargetLevel(pDevice);
      }
      break;
    case REFRESH_MODE_QUERY_LAMPFAILURE:
      if(pDevice != NULL) {
          Zigbee2DaliQueryLampFailure(pDevice);
      }
      break;
    case REFRESH_MODE_QUERY_COLOR_TEMP:
        if(pDevice->deviceType8Supported) { // in broadcast mode this is set if at least 1 device supports it
            Zigbee2DaliUpdateColorTemp(pDevice);
        }
        break;
    case REFRESH_MODE_IDLE:
    default:
      break;
    }
  } // no else
}

static void Zigbee2DaliInitAttributes(void) {
  for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    if(  ( zigbee2Dali.device[i].mode == DEVICE_MODE_START )
       &&(zigbee2Dali.device[i].active)) {
      uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(i);

      Zigbee2Dali_InitPhysicalMinLevel(i);
      uint8_t minLevel = BallastConfigurationServer_GetMinLevel(endpoint);
      TRACE(true, "minlevel %u, zigbeedevice minLevel %u", minLevel, zigbee2Dali.device[i].minLevel);
      zigbee2Dali.device[i].minLevel = minLevel;
      uint8_t maxLevel = BallastConfigurationServer_GetMaxLevel(endpoint);
      uint8_t poweronLevel = BallastConfigurationServer_GetPoweronLevel(endpoint);
      uint8_t sysFailureLevel = BallastConfigurationServer_GetSystemFailureLevel(endpoint);

      Zigbee2Dali_SetMinLevel(endpoint, minLevel);
      Zigbee2Dali_SetMaxLevel(endpoint, maxLevel);
      if(zigbee2Dali.device[i].deviceType8Supported) {
        Zigbee2Dali_InitColorTempPhysMin(i);
        Zigbee2Dali_InitColorTempPhysMax(i);
        uint16_t startUpColorTemp = ColorControlServer_GetStartUpColorTemp(endpoint);
        uint16_t physMinColorTemp = ColorControlServer_GetColorTempPhysMin(endpoint);
        uint16_t physMaxColorTemp = ColorControlServer_GetColorTempPhysMax(endpoint);

        Zigbee2Dali_SetColorTemp(endpoint, startUpColorTemp); //Set the temporary color, will be saved during the SetPowerOnLevel
        zigbee2Dali.device[i].actMirek = startUpColorTemp;
        zigbee2Dali.device[i].colorTempMin = physMinColorTemp;
        zigbee2Dali.device[i].colorTempMax = physMaxColorTemp;
      }
      Zigbee2Dali_SetPoweronLevel(endpoint, poweronLevel);
      Zigbee2Dali_SetSystemFailureLevel(endpoint, sysFailureLevel);
      Zigbee2Dali_SetFadeTime(endpoint, 0);
    } // no else
  }
  if(zigbee2Dali.sensor.active) {
    uint32_t luxCalibrationMultiplier = DaliDeviceHandling_FindLuxCalibrationMultiplier();
    IlluminanceMeasurementServer_SetLuxCalibrationMultplier(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, luxCalibrationMultiplier);
  }
}
// --   M a t h s   ------------------------------------------------------------

uint8_t ZigbeeDiv32by32 (uint32_t dividend, uint32_t divisor) {
  uint32_t tempDividend = dividend << 8;
  uint32_t tempDivisor = divisor << 16;
  uint32_t result = 0;
  int32_t diff;
  for (uint8_t i=9; i>0; i--) {                                                 // accuracy of 8 bit is sufficient
    result <<= 1;
    diff = tempDividend - tempDivisor;
    if (diff >= 0) {
      tempDividend = diff;
      result++;
    }
    tempDivisor >>= 1;                                                          // shift at last loop ..
  }
  if (tempDividend >= tempDivisor) {                                             // .. needed to round up at >= 0.5
    result++;
  } // no else
  return (uint8_t)result;
}

// --------- Global Functions --------------------------------------------------

/* \brief
 * */
void Zigbee2Dali_Init( void ) {
  DaliTelegram_Init();                                                              // Initialize all variables and µC-registers
  zigbee2Dali = zigbee2DaliDEFAULT;                                             // Initialize complete structure
}

/* \brief
 * */
void Zigbee2Dali_Start( void ) {
#ifdef FADE_TEST
  zigbee2Dali.device[0].mode = DEVICE_MODE_NORMAL;
#endif
  for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    zigbee2Dali.device[i] = zigbee2DaliDeviceDEFAULT;
  }
  DaliTelegram_Start();
}

/* \brief
 * */
void Zigbee2Dali_StartMfgTest( void ) {
  zigbee2Dali.mode = DALI_MODE_MFG_TEST;
}

/* \brief
 * */
uint32_t Zigbee2Dali_TimeTick( void ) {


  uint32_t ticks = DaliTelegram_TimeTick();
#if 1
  static int count = 1;
  if (count){//(ticks > 0) {
      count = 0;
    Zigbee2DaliModeControl(ticks);
  }
#endif
  return ticks;
}

// --   Z i g b e e   S e r v i c e s   ----------------------------------------

/* \brief
 * */
bool Zigbee2Dali_IsInitializing(void) {
  return ((zigbee2Dali.mode == DALI_MODE_INIT) || (zigbee2Dali.mode == DALI_MODE_DEVICE_ADDRESSING));
}

/* \brief
 * */
void Zigbee2Dali_DeleteAllAddresses(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommandBroadcast(DALI_CMD_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
  DaliTelegram_Send3ByteSpecialCommand(SENSOR_SETDTR0, 0xFF, DALI_SEND_NORMAL, 0);
  DaliTelegram_Send3ByteCommandBroadcast(SENSOR_SET_SHORT_ADDRESS, DALI_SEND_TWICE, 0);
  for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    zigbee2Dali.device[i] = zigbee2DaliDeviceDEFAULT;
  }

 // Do same for controls
  zigbee2Dali.mode = DALI_MODE_INIT;
}


// --   D A L I  A d d r e s s i n g   & I d e n t i f i c a t i o n  ----------

/* \brief
 * */
void Zigbee2Dali_SetBallastAddress(int32_t index, uint8_t address) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    zigbee2Dali.device[index].address = address;
    zigbee2Dali.device[index].active = true;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetBroadcastMode(bool mode) {
  zigbee2Dali.ecgBroadcastMode = mode;
}

/* \brief
 * */
void Zigbee2Dali_DeactivateBallast(uint8_t index) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
      zigbee2Dali.device[index].active = false;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetEmergencyAddress(uint8_t address) {
  zigbee2Dali.emergency.address = address;
  zigbee2Dali.emergency.active = true;
  TRACE(true, "emergency type set at address %u", address);
}

/* \brief
 * */
bool Zigbee2Dali_isAddressNew(uint8_t address) {
  bool isNew = true;
  for(uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    if(address == zigbee2Dali.device[i].address && zigbee2Dali.device[i].active) {
      isNew = false;
    }
  }
  return isNew;
}

/* \brief
 * */
void Zigbee2Dali_SetDaliVersion(int32_t index, uint8_t version) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    zigbee2Dali.device[index].daliVersion = version;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetDeviceType1Support(int32_t index, bool supported) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    zigbee2Dali.device[index].deviceType1Supported = supported;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetDeviceType6Support(int32_t index, bool supported) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    zigbee2Dali.device[index].deviceType6Supported = supported;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetDeviceType8Support(int32_t index, bool supported) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    zigbee2Dali.device[index].deviceType8Supported = supported;
  }
}


void Zigbee2Dali_InitPhysicalMinLevel(int32_t index) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    if(zigbee2Dali.device[index].minLevel == UINT8_MASK) return;
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    BallastConfigurationServer_SetPhysicalMinLevel(endpoint, zigbee2Dali.device[index].minLevel);
  }
}

void Zigbee2Dali_InitColorTempPhysMin(int32_t index) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    if(zigbee2Dali.device[index].colorTempMin == UINT16_MASK) return;
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    ColorControlServer_SetColorTempPhysMin(endpoint, zigbee2Dali.device[index].colorTempMin);
  }
}
void Zigbee2Dali_InitColorTempPhysMax(int32_t index) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    if(zigbee2Dali.device[index].colorTempMax == UINT16_MASK) return;
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    ColorControlServer_SetColorTempMax(endpoint, zigbee2Dali.device[index].colorTempMax);
  }
}

/* \brief
 * */
void Zigbee2Dali_SetPhysicalMinLevel(int32_t index, uint8_t level) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    zigbee2Dali.device[index].minLevel = level;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetColorTempPhysMin(int32_t index, uint16_t colorTempMired) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    zigbee2Dali.device[index].colorTempMin = colorTempMired;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetColorTempPhysMax(int32_t index, uint16_t colorTempMired) {
  if (index < ZIGBEE2DALI_ECG_MAX) {
    uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(index);
    zigbee2Dali.device[index].colorTempMax = colorTempMired;
  }
}

/* \brief
 * */
void Zigbee2Dali_StartDevice(int32_t index) {
  if (   (index < ZIGBEE2DALI_ECG_MAX)
      && (zigbee2Dali.device[index].mode != DEVICE_MODE_NORMAL)                // skip if device is already in normal state
      && (zigbee2Dali.device[index].active)) {
      zigbee2Dali.device[index].mode = DEVICE_MODE_START;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetSensorMode(uint8_t mode) {
  zigbee2Dali.sensor.mode = mode;
}

/* \brief
 * */
void Zigbee2Dali_SetLightSensorAddress(int32_t index, uint8_t address) {
  if (index < ZIGBEE2DALI_LIGHT_SENSOR_MAX) {
    zigbee2Dali.sensor.addressLight = address;
    zigbee2Dali.sensor.active = true;
    zigbee2Dali.sensor.errorCounter = 0;
  }
}

/* \brief
 * */
void Zigbee2Dali_SetPresenceSensorAddress(int32_t index, uint8_t address) {
  if (index < ZIGBEE2DALI_PRESENCE_SENSOR_MAX) {
    zigbee2Dali.sensor.addressPD = address;
    zigbee2Dali.sensor.active = true;
  }
}

/* \brief
 * */
void Zigbee2Dali_DeactivateSensor(int32_t index) {
  if (index < ZIGBEE2DALI_PRESENCE_SENSOR_MAX) {
    zigbee2Dali.sensor.active = false;
  }
}

// --   N e t w o r k   L e a v e   --------------------------------------------

/* \brief
 * */
bool Zigbee2Dali_TriggerNetworkLeave ( void ) {
  emberAfCorePrintln("Zigbee2Dali 11\r\n");
  bool leaveNetworkCommand = zigbee2Dali.networkService.leave;
  zigbee2Dali.networkService.leave = false;
  return leaveNetworkCommand;
}

// --   I d e n t i f y   D e v i c e   ----------------------------------------

/* \brief
 * */
void Zigbee2Dali_Identify(uint8_t endpoint, bool identifyStart) {
  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  if (!pDevice->active) return;

  if ((pDevice->mode == DEVICE_MODE_INIT) || (pDevice->mode == DEVICE_MODE_FAILURE)) {
    return;
  }
  if (identifyStart) {
    TRACE(true, "Identify-Start EP %u", endpoint);
    TRACE((pDevice->mode == DEVICE_MODE_IDENTIFY), "   already in Identify", endpoint);

    if (pDevice->mode != DEVICE_MODE_IDENTIFY) {
      pDevice->mode = DEVICE_MODE_IDENTIFY;
      pDevice->identify.on = true;
      if(pDevice->daliVersion >= DALI_EDITION2) {
        pDevice->identify.delay_FTT = DALI_TIME_IDENTIFY_RESEND_FTT;              // re-send after this time
        DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_IDENTIFY, DALI_SEND_TWICE, 0);
      }
      else {
        pDevice->identify.delay_FTT = DALI_TIME_IDENTIFY_TOGGLE_FTT;              // re-send after this time
        DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_RECALL_MAX, DALI_SEND_NORMAL, 0);
      }
    } // no else: already active
  }
  else {
    TRACE(true, "Identify-Stop EP %u", endpoint);
    TRACE((pDevice->mode != DEVICE_MODE_IDENTIFY), "   not in Identify", endpoint);
    if (pDevice->mode == DEVICE_MODE_IDENTIFY ) {                               // mode changed, after DALI_STOP_IDENTIFY was sent successfully
      if(pDevice->daliVersion >= DALI_EDITION2) {
        DaliTelegram_SendDap2Address(pDevice->address, 0xFF, 0, DALI_SEND_NORMAL);
      }
      pDevice->mode = DEVICE_MODE_NORMAL;
      LevelControlServer_RefreshCurrentOutputLevel(endpoint);
    }
  }
}

// --   L i g h t   C o n t r o l   --------------------------------------------

/* \brief
 * */
void Zigbee2Dali_StopFade(uint8_t endpoint) {
  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  if (!pDevice->active) return;
  TRACE(true, "StopFade EP, %u", endpoint);
  if (pDevice->mode != DEVICE_MODE_FADING) {
    TRACE(true, "   not fading", endpoint);
    return;
  }
  DaliTelegram_SendDap2Address(pDevice->address, 0xFF, DALI_SEND_NORMAL,0 );
  DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_ACT_LEVEL, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryLevelResponseCallback);
  pDevice->light.fading = false;
  pDevice->mode = DEVICE_MODE_NORMAL;
}

/* \brief
 * */
void Zigbee2Dali_StopColorTemp(uint8_t endpoint) {
  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  if (!pDevice->active) return;
  TRACE(true, "Stop Color Temperature move EP, %u", endpoint);
  DaliTelegram_SendDap2Address(pDevice->address, 0xFF, DALI_SEND_NORMAL,0 );
  //DaliTelegram_SendStdCommand2AddressWithCallback(pDevice->address, DALI_QUERY_ACT_LEVEL, DALI_WAIT4ANSWER, 0, Zigbee2DaliQueryLevelResponseCallback);
}

/* \brief
 * */
void Zigbee2Dali_StartFadeTime(uint8_t endpoint, uint8_t level, uint16_t fadeTime_0sx) {   // ?return void, because an error cannot be forwarded by the calling function
  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  if (!pDevice->active) return;
  if (level < pDevice->minLevel) {
    TRACE(true, "Zigbee2Dali_StartFadeTime minLevel, %u", pDevice->minLevel);
    level = (level == 0) ? 0 : pDevice->minLevel;
  }
  if (pDevice->daliVersion < DALI_EDITION2) {
    Zigbee2DaliStartFadeTimeEd1(endpoint, level, fadeTime_0sx);
  }
  else {
    Zigbee2DaliStartFadeTimeEd2(endpoint, level, fadeTime_0sx);
  }
}

/* \brief
 * */
void Zigbee2Dali_StartColorTemperatureFadeTime(uint8_t endpoint, uint16_t mired, uint16_t fadeTime_0sx) {
  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  if (!pDevice->active) return;
  if (pDevice->daliVersion >= DALI_EDITION1) {
      uint16_t limitedMired = mired;
      limitedMired = pDevice->colorTempMax < mired ? pDevice->colorTempMax : mired;
      limitedMired = pDevice->colorTempMin > mired ? pDevice->colorTempMin : mired;
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, (uint8_t)((limitedMired >> 8 ) & 0x00FF), DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, (uint8_t)(limitedMired & 0x00FF), DALI_SEND_NORMAL, 0);
      DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
      DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_DT8_SET_TEMP_COLOR, DALI_SEND_NORMAL, 0);
      pDevice->mode = DEVICE_MODE_NORMAL; // this is to force restarting the fading process
      Zigbee2Dali_StartFadeTime(endpoint, (pDevice->light.targetLevel), fadeTime_0sx);
      //Zigbee2Dali_SetColorTemp(endpoint, limitedMired); //We just set it, don't do it again!
      pDevice->actMirek = limitedMired;
  }
}

/* \brief
 * */
void Zigbee2Dali_StartColorFadeTime(uint8_t endpoint, uint16_t x, uint16_t y, uint16_t fadeTime_0sx) {
    uint16_t mired = XYtoColorTempMired(x, y);
    Zigbee2Dali_StartColorTemperatureFadeTime(endpoint, mired, fadeTime_0sx);
}

/* \brief
 * */
void Zigbee2Dali_StartFadeRate(uint8_t endpoint, int16_t fadeRate_Inc_s, bool switchOff) {
  uint8_t lightCommand;
  int16_t levelDiff;
  uint16_t fadeTime_0sx;

  if ( !Zigbee2DaliIsLightEndpoint(endpoint) ) return;                    // if endpoint is not valid: RETURN
  uint8_t index = Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint);
  TypeDevice *pDevice = &(zigbee2Dali.device[index]);
  if (!pDevice->active) return;

  if ( fadeRate_Inc_s == 0) {
    return;
  }
  else if  ( fadeRate_Inc_s > 255 ) {
    fadeRate_Inc_s = 255;
  }
  else if  ( fadeRate_Inc_s < (-255) ) {
    fadeRate_Inc_s = -255;
  }

  if ( fadeRate_Inc_s > 0  ) {
    if ( pDevice->actLevel >= 254) {
      return;                                                                   // if already at limit: RETURN
    }
    else {
      lightCommand = 254;
    }
  }
  else {
    if ( pDevice->actLevel<= pDevice->minLevel ) {
      if (switchOff) {
        lightCommand = 0;
      }
      else {
        return;                                                                 // if already at limit: RETURN
      }
    }
    else {
      if (switchOff) {
        lightCommand = 0;
      }
      else {
        lightCommand = pDevice->minLevel;
      }
    }
  }
  levelDiff = (lightCommand - pDevice->actLevel);                               // diff
  levelDiff *= 10;                                                              // diff in 0.1 Inc ..
  fadeTime_0sx = levelDiff / fadeRate_Inc_s;
                                                                                // .. fadeRate in 0.1 Inc / 0.1 s: result in 0.1 s
  //lightCommand = Dali2Zigbee(lightCommand);
  Zigbee2Dali_StartFadeTime(endpoint, lightCommand, fadeTime_0sx);
}

/* \brief
 * */
void Zigbee2Dali_StartAddressing(bool withDelete) {
  DaliDeviceHandling_StartDeviceAddressing();
  if(withDelete)
    zigbee2Dali = zigbee2DaliDEFAULT;
  zigbee2Dali.mode = DALI_MODE_DEVICE_ADDRESSING;
  for (uint8_t i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
	if(withDelete) {
	  zigbee2Dali.device[i] = zigbee2DaliDeviceDEFAULT;
	}
    zigbee2Dali.device[i].mode = DEVICE_MODE_START;
  }
}

/* \brief
 * */
bool Zigbee2Dali_InitDevices(void) {
  return DaliDeviceHandling_InitDevices();
}

/* \brief
 * */
void Zigbee2Dali_SetMinLevel(uint8_t endpoint, uint8_t level) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, level, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_MIN_LEVEL, DALI_SEND_TWICE, 0);
}

/* \brief
 * */
void Zigbee2Dali_SetMaxLevel(uint8_t endpoint, uint8_t level) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, level, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_MAX_LEVEL, DALI_SEND_TWICE, 0);

}

/* \brief
 * */
void Zigbee2Dali_SetPoweronLevel(uint8_t endpoint, uint8_t level) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, level, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_POWER_ON_LEVEL, DALI_SEND_TWICE, 0);
}

/* \brief
 * */
void Zigbee2Dali_SetSystemFailureLevel(uint8_t endpoint, uint8_t level) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, level, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_SysFAIL_LEVEL, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_SetFadeTime(uint8_t endpoint, uint8_t fadeTime) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, fadeTime, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_SET_FADE_TIME, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_SetColorTemp(uint8_t endpoint, uint16_t mired) {
  if(!Zigbee2DaliIsLightEndpoint(endpoint)) return;
  TypeDevice *pDevice = &(zigbee2Dali.device[Zigbee2DaliGetDeviceIndexFromEndpoint(endpoint)]);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR1, (uint8_t)((mired >> 8 ) & 0x00FF), DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, (uint8_t)(mired & 0x00FF), DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT8, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(pDevice->address, DALI_CMD_DT8_SET_TEMP_COLOR, DALI_SEND_NORMAL, 0);
  //This sets the temporary color temperature register; will be used by the next command (SetARC or SetPowerOn)
}

void Zigbee2Dali_IdentifyIllunminanceSensor(uint8_t endpointIndex){
  DaliTelegram_Send3ByteCommand2Address(zigbee2Dali.sensor.addressLight, SENSOR_IDENTIFY_DEVICES, DALI_SEND_NORMAL, 0);
}

void Zigbee2Dali_IdentifyOccupancySensor(uint8_t endpointIndex){
  DaliTelegram_Send3ByteCommand2Address(zigbee2Dali.sensor.addressPD, SENSOR_IDENTIFY_DEVICES, DALI_SEND_NORMAL, 0);
}

void Zigbee2Dali_EmergencyDisableAutoTest(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, 0, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_DELAY_TIME_H, DALI_SEND_TWICE, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_DELAY_TIME_L, DALI_SEND_TWICE, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_FUNC_TEST_INT, DALI_SEND_TWICE, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_DUR_TEST_INT, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyStartFunctionTest(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_RESET_FUNC_TEST_DONE_FLAG, DALI_SEND_TWICE, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_START_FUNCTION_TEST, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyStartDurationTest(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_RESET_DUR_TEST_DONE_FLAG, DALI_SEND_TWICE, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_START_DURATION_TEST, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyStopTest(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STOP_TEST, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyInhibit(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_INHIBIT, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyResetInhibit(void) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_RESET_INHIBIT, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyProlongBy(uint8_t timeInHalfMinutes) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, timeInHalfMinutes, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_PROLONG, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencyStartIdentify(void) {
  DaliTelegram_SendDap2Address(zigbee2Dali.emergency.address, 254, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_START_IDENTIFICATION, DALI_SEND_TWICE, 0);
}

void Zigbee2Dali_EmergencySetTestExecutionTimeout(uint8_t timeOut) {
  // NOTE: In WM, this is sent twice, but DALI spec does not say it should be sent twice.
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, timeOut, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_TEST_EXEC_TMO, DALI_SEND_TWICE, 0);
  Zigbee2DaliSetEmergencyTestExecutionTimeout(timeOut);	// write-only, so update immediately
}

void Zigbee2Dali_EmergencySetLevel(uint8_t level) {
  DaliTelegram_SendSpecialCommand(DALI_CMD_SETDTR0, level, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendSpecialCommand(DALI_CMD_ENABLE_DEVICETYPE, DALI_DEVICE_TYPE_DT1, DALI_SEND_NORMAL, 0);
  DaliTelegram_SendStdCommand2Address(zigbee2Dali.emergency.address, DALI_CMD_DT1_STORE_DTR_AS_LEVEL, DALI_SEND_TWICE, 0);
}

void Zigbee2DaliSetEmergencyTestExecutionTimeout(uint8_t value) {
  zigbee2Dali.emergency.testExecutionTimeout = value;
  EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                         ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                         ZCL_TEST_EXECUTION_TIMEOUT_ATTRIBUTE_ID,
                                                                         OSRAM_MANUFACTURER_CODE,
                                                                         (uint8_t *) &zigbee2Dali.emergency.testExecutionTimeout,
                                                                         ZCL_INT8U_ATTRIBUTE_TYPE);
}

void Zigbee2DaliEmergencyBatteryChargeCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    if(response.answer != zigbee2Dali.emergency.batteryCharge) {
      zigbee2Dali.emergency.batteryCharge = response.answer;
      EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                             ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                             ZCL_BATTERY_CHARGE_ATTRIBUTE_ID,
                                                                             OSRAM_MANUFACTURER_CODE,
                                                                             (uint8_t *) &zigbee2Dali.emergency.batteryCharge,
                                                                             ZCL_INT8U_ATTRIBUTE_TYPE);
      TRACE(true, "emergency %s: %u", "batteryCharge", response.answer);
    }
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "batteryCharge", response.answerType);
  }
}

void Zigbee2DaliEmergencyLevelCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    if(response.answer != zigbee2Dali.emergency.level) {
      zigbee2Dali.emergency.level = response.answer;
      EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                             ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                             ZCL_EMERGENCY_LEVEL_ATTRIBUTE_ID,
                                                                             OSRAM_MANUFACTURER_CODE,
                                                                             (uint8_t *) &zigbee2Dali.emergency.level,
                                                                             ZCL_INT8U_ATTRIBUTE_TYPE);
      TRACE(true, "emergency %s: %u", "level", response.answer);
    }
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "level", response.answerType);
  }
}

void Zigbee2DaliEmergencyModeCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    if(response.answer != zigbee2Dali.emergency.mode) {
      zigbee2Dali.emergency.mode = response.answer;
      EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                             ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                             ZCL_EMERGENCY_MODE_ATTRIBUTE_ID,
                                                                             OSRAM_MANUFACTURER_CODE,
                                                                             (uint8_t *) &zigbee2Dali.emergency.mode,
                                                                             ZCL_BITMAP8_ATTRIBUTE_TYPE);
      TRACE(true, "emergency %s: 0x%X", "mode", response.answer);
    }
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "mode", response.answerType);
  }
}

void Zigbee2DaliEmergencyFeaturesCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    zigbee2Dali.emergency.features = response.answer;
    EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                           ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                           ZCL_EMERGENCY_FEATURES_ATTRIBUTE_ID,
                                                                           OSRAM_MANUFACTURER_CODE,
                                                                           (uint8_t *) &zigbee2Dali.emergency.features,
                                                                           ZCL_BITMAP8_ATTRIBUTE_TYPE);
    TRACE(true, "emergency %s: 0x%X", "features", response.answer);
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "features", response.answerType);
  }
}

void Zigbee2DaliEmergencyFailureStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    if(response.answer != zigbee2Dali.emergency.failureStatus) {
      zigbee2Dali.emergency.failureStatus = response.answer;
      EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                             ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                             ZCL_FAILURE_STATUS_ATTRIBUTE_ID,
                                                                             OSRAM_MANUFACTURER_CODE,
                                                                             (uint8_t *) &zigbee2Dali.emergency.failureStatus,
                                                                             ZCL_BITMAP8_ATTRIBUTE_TYPE);
      TRACE(true, "emergency %s: 0x%X", "failureStatus", response.answer);
    }
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "failureStatus", response.answerType);
  }
}

void Zigbee2DaliEmergencyStatusCallback(TypeDaliTelegramResponse response, TypeDaliFrame command) {
  if(response.answerType == DALI_ANSWER_OK) {
    zigbee2Dali.emergency.errorCounter = 0;
    if(response.answer != zigbee2Dali.emergency.status) {
      zigbee2Dali.emergency.status = response.answer;
      EmberAfStatus status = emberAfWriteManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                                             ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                                             ZCL_EMERGENCY_STATUS_ATTRIBUTE_ID,
                                                                             OSRAM_MANUFACTURER_CODE,
                                                                             (uint8_t *) &zigbee2Dali.emergency.status,
                                                                             ZCL_BITMAP8_ATTRIBUTE_TYPE);
      TRACE(true, "emergency %s: 0x%X", "status", response.answer);
    }
  }
  else {
    zigbee2Dali.emergency.errorCounter++;
    TRACE(true, "Could not read emergency %s: %u", "status", response.answerType);
  }
}

void Zigbee2Dali_EmergencyInit(void) {
  Zigbee2Dali_EmergencyDisableAutoTest();
}

// --------- Callbacks from DALI driver --------------------------------------------------

void DaliTelegram_InputDeviceFrameReceivedCallback(TypeDaliFrame frame) {
  switch (frame.idf.shortAddressed.command) {
    case SENSOR_EVENT_PIR:
      if (frame.idf.shortAddressed.address == zigbee2Dali.sensor.addressPD) {
        OccupancySensingServer_PresenceDetectedCallback(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT);           // forward content ..
      }
      break;
    default:
      if((frame.idf.shortAddressed.command & 0x01) &&
         (frame.idf.shortAddressed.address == zigbee2Dali.sensor.addressPD) &&
         zigbee2Dali.sensor.mode == 2) // TODO: Z2D check if we are in DALI2 mode
      {
        OccupancySensingServer_PresenceDetectedCallback(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT);           // forward content ..
        TRACE(true, "DaliTelegram_InputDeviceFrameReceivedCallback sa 0x%X cmd 0x%X", frame.idf.shortAddressed, frame.idf.shortAddressed.command);
      }
      else
        zigbee2Dali.sensor.pollTime_FTT = 0;  // trigger a sensor read
      break;
  }
}

void DaliTelegram_BusFailCallback(void){
  for(int i = 0; i < ZIGBEE2DALI_ECG_MAX; i++) {
    TypeDevice *pDevice = &(zigbee2Dali.device[i]);
    if(pDevice->active) {
      uint8_t endpoint = Zigbee2DaliGetEndpointFromDeviceIndex(i);
      pDevice->mode = DEVICE_MODE_FAILURE;
      emberAfPluginOsramdsBasicServerSetHardwareFault(endpoint, true);
    }
  }
  if (zigbee2Dali.sensor.active) {
    emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_LIGHT_SENSOR_ENDPOINT, true);
    emberAfPluginOsramdsBasicServerSetHardwareFault(ZIGBEE2DALI_PRESENCE_SENSOR_ENDPOINT, true);
  }
}



/// @}
