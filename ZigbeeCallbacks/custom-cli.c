/******************** (C) COPYRIGHT 2016 -  OSRAM AG *********************************************************************
* File Name         : custom-cli.c
* Description       : custom specific CommandLineInterface extension
* Initial Version   : MS
* Created           :
* Modified by       :
*
**************************************************************************************************************************/

#include "app/framework/include/af.h"
#include "app/framework/cli/custom-cli.h"
//#include "client-api.h"
#include "GlobalDefs.h"
#include "app/framework/plugin/osramds-basic-server/BasicServer.h"
#include "IdentifyClient.h"
#include "EnceliumInterfaceServer.h"
#define MAXIMUM_POLLING_DEVICES         10
#define ATTRIBUTE_LARGEST (33)

typedef struct {
  uint16_t  node;
  uint8_t   endpoint;
} TypeDeviceEndpointAddress;

typedef struct {
  uint32_t  numberOfEntries;
  uint32_t  activeIndex;
  uint32_t  actualCycles;
  uint32_t  delayTime;
  uint32_t  cycles;
  TypeDeviceEndpointAddress adress[MAXIMUM_POLLING_DEVICES];
} TypePolling;

#define CUSTOM_CLI_BUFFER_SIZE    255

uint8_t customCliBuffer[CUSTOM_CLI_BUFFER_SIZE];
uint16_t customCliBufferLen;
EmberEventControl customCliEvent;


const TypePolling CustomCliPolling_DEFAULT = { .numberOfEntries = 0, .activeIndex = 0, .actualCycles = 0, .cycles = 0};
TypePolling CustomCliPolling;// = CustomCliPolling_DEFAULT;




void ReadRfInfo(void);
void PollingClearList(void);
void PollingAddList(void);
void PollingPoll(void);
void PollingLearnList(void);
void PollingStop(void);
void PollingList(void);
void ChangeEndpointActiveState(void);
void RefreshInterfaceConfig(void);
void ReadMfgSpecAttribute(void);
void WriteMfgSpecAttribute(void);
void IdentifyTriggerEfect(void);
#if 1	// DKo - added for testing convenience. For some reason I couldn't generate the standard CLI interface via SimplicityStudio.
void MoveToLevelTest(void);
void MoveToColorTemperatureTest(void);
#endif
void EnableBroadcastMode(void);

extern EmberCommandEntry emberAfEcomCommands[];
extern void Reset2factoryDoReset2FactoryDefaults(void);
void EmergencyCmd(void);


EmberCommandEntry emberAfCustomCommands[] = {
  { "read-rf-info",     ReadRfInfo, "vu" },
  { "poll-clear",       PollingClearList, "" },
  { "poll-add",         PollingAddList, "vu" },
  { "poll-list",        PollingList, "" },
  { "poll",             PollingPoll, "vv" },
  { "poll-learn",       PollingLearnList, "v" },
  { "poll-stop",        PollingStop, "" },
  { "chg-ep-type",      ChangeEndpointActiveState, "uvu" },
  { "refresh-config",   RefreshInterfaceConfig, "u" },
  { "read-mfgspec",     ReadMfgSpecAttribute,  "uvvv"},
  { "write-mfgspec",    WriteMfgSpecAttribute, "uvvvub"},
  { "identify-trigger", IdentifyTriggerEfect, "uuu"},
#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)
  emberCommandEntrySubMenu("ecom", emberAfEcomCommands, "ECOM library debug commands"),
#endif
#if 1	// DKo - added for testing convenience. For some reason I couldn't generate the standard CLI interface via SimplicityStudio.
  { "move-to-level",    MoveToLevelTest, "uuv"},
  { "move-to-ct",       MoveToColorTemperatureTest, "uvv"},
#endif
  { "bcastmode",        EnableBroadcastMode, "u"},
  { "freset",           Reset2factoryDoReset2FactoryDefaults, ""},
  { "emerg",            EmergencyCmd, "u"},
  emberCommandEntryTerminator()
};


void ReadAttributeRfInfo(uint16_t node, uint8_t endpoint) {

 // uint16_t attributeId = 0x0000;

#ifdef ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID
  emberAfFillExternalManufacturerSpecificBuffer(ZCL_GLOBAL_COMMAND | ZCL_MANUFACTURER_SPECIFIC_MASK | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER,
                                                ZCL_ZIGBEE_RF_QUALITY_CLUSTER_ID,
                                                OSRAM_MANUFACTURER_CODE,
                                                ZCL_READ_ATTRIBUTES_COMMAND_ID,
                                                "vvvv",
                                                0x0000, 0x0001, 0x0002, 0x0003);

  emberAfSetCommandEndpoints(1, endpoint);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, node);
#endif
}



void PollNext(void) {

  uint16_t node = CustomCliPolling.adress[CustomCliPolling.activeIndex].node;
  uint8_t endpoint = CustomCliPolling.adress[CustomCliPolling.activeIndex].endpoint;
  ReadAttributeRfInfo(node, endpoint);

  CustomCliPolling.activeIndex++;
  if (CustomCliPolling.activeIndex >= CustomCliPolling.numberOfEntries) {
    CustomCliPolling.activeIndex = 0;
    CustomCliPolling.actualCycles++;
    if (  (CustomCliPolling.actualCycles >= CustomCliPolling.cycles)
       &&(CustomCliPolling.cycles != 0)){
      emberEventControlSetInactive(customCliEvent);
      return;
    }
  }
  emberEventControlSetDelayMS(customCliEvent,MS2TICKS(CustomCliPolling.delayTime));
}

void customCliEventHandler(void){
  PollNext();
}

void PollingList(void){
  emberAfGuaranteedPrintln("Polling list:");
  for(uint32_t i = 0; i < CustomCliPolling.numberOfEntries; i++) {
    emberAfGuaranteedPrintln("  Node=0x%2x EP=%3d",
        CustomCliPolling.adress[i].node,
        CustomCliPolling.adress[i].endpoint);
  }
 emberAfGuaranteedPrintln("");
}

void ReadRfInfo(void){
  uint16_t node = (uint16_t) emberUnsignedCommandArgument(0);
  uint8_t endpoint = (uint8_t) emberUnsignedCommandArgument(1);

  ReadAttributeRfInfo(node, endpoint);
}

void PollingClearList(void){
  CustomCliPolling.numberOfEntries = 0;
}

void PollingAddList(void){
  if (CustomCliPolling.numberOfEntries < MAXIMUM_POLLING_DEVICES) {
    CustomCliPolling.adress[CustomCliPolling.numberOfEntries].node = (uint16_t) emberUnsignedCommandArgument(0);
    CustomCliPolling.adress[CustomCliPolling.numberOfEntries].endpoint = (uint8_t) emberUnsignedCommandArgument(1);
    CustomCliPolling.numberOfEntries++;
  }
}

void PollingPoll(void){
  CustomCliPolling.activeIndex = 0;
  CustomCliPolling.actualCycles = 0;
  CustomCliPolling.delayTime = emberUnsignedCommandArgument(0);
  CustomCliPolling.cycles = emberUnsignedCommandArgument(1);

  PollNext();
}

void PollingLearnList(void){

}

void PollingStop(void){
  emberEventControlSetInactive(customCliEvent);
}

void ChangeEndpointActiveState(void){
  uint8_t endpoint = (uint8_t) emberUnsignedCommandArgument(0);
  uint16_t devicetype = emberUnsignedCommandArgument(1);
  bool active = (0 != (uint8_t) emberUnsignedCommandArgument(2));
  EnceliumInterfaceServer_SetEndpointType(endpoint, devicetype, active);
}

void RefreshInterfaceConfig(void) {
 uint8_t interfaceId = (uint8_t) emberUnsignedCommandArgument(0);
 EnceliumInterfaceServer_RescanInterface(interfaceId);
}

void ReadMfgSpecAttribute(void) {
  {
    EmberAfStatus status;
    uint8_t endpoint = (uint8_t)emberUnsignedCommandArgument(0);
    EmberAfClusterId cluster = (EmberAfClusterId)emberUnsignedCommandArgument(1);
    EmberAfAttributeId attribute = (EmberAfAttributeId)emberUnsignedCommandArgument(2);
    uint16_t manufacturerCode = (uint16_t)emberUnsignedCommandArgument(3);
    uint8_t data[ATTRIBUTE_LARGEST];
    EmberAfAttributeMetadata *attributeData;

    emberAfCorePrintln("%p: ep: %d, cl: 0x%2X, attr: 0x%2X",
                     "read",
                     endpoint,
                     cluster,
                     attribute);
    attributeData = emberAfLocateAttributeMetadata(endpoint,
                                                   cluster,
                                                   attribute,
                                                   CLUSTER_MASK_SERVER,
                                                   manufacturerCode);

    if (attributeData != NULL) {
      status = emberAfReadManufacturerSpecificServerAttribute(endpoint,
                                  cluster,
                                  attribute,
                                  manufacturerCode,
                                  data,
                                  sizeof(data));
    }
    else {
      status = EMBER_ZCL_STATUS_FAILURE;
    }
    if (status == EMBER_ZCL_STATUS_SUCCESS) {
      switch (attributeData->attributeType) {
        case ZCL_LONG_CHAR_STRING_ATTRIBUTE_TYPE:
        case ZCL_CHAR_STRING_ATTRIBUTE_TYPE:
          emberAfCorePrintString(data);
          break;
        case ZCL_OCTET_STRING_ATTRIBUTE_TYPE:
        case ZCL_LONG_OCTET_STRING_ATTRIBUTE_TYPE:
          for (int i = 0; i < attributeData->size; i++) {
            emberAfCorePrint("0x%x ", *(data+i));
          }
          break;
        default:
          emberAfCorePrintBuffer(data, emberAfGetDataSize(attributeData->attributeType), true);
          break;
      }
      emberAfCorePrintln("");
    } else {
      emberAfCorePrintln("%p: read: 0x%x", "Error", status);
    }
  }

}

void WriteMfgSpecAttribute(void) {
  //uint8_t i;
  EmberAfStatus status;
  uint8_t data[ATTRIBUTE_LARGEST];

  uint8_t  endpoint  = (uint8_t)emberUnsignedCommandArgument(0);
  uint16_t cluster   = (uint16_t)emberUnsignedCommandArgument(1);
  uint16_t attribute = (uint16_t)emberUnsignedCommandArgument(2);
  uint16_t manufacturerCode = (uint16_t)emberUnsignedCommandArgument(3);
  uint8_t  dataType  = (uint8_t)emberUnsignedCommandArgument(4);

  emberAfCorePrintln("%p: ep: %d, cl: 0x%2X, attr: 0x%2X dtype: 0x%X",
                   "write",
                   endpoint,
                   cluster,
                   attribute,
                   dataType);

  // If the data type is a string, automatically prepend a length to the data;
  // otherwise, just copy the raw bytes.
  MEMSET(data, 0, ATTRIBUTE_LARGEST);
  if (emberAfIsThisDataTypeAStringType(dataType)) {
    data[0] = emberCopyStringArgument(5,
                                      data + 1,
                                      ATTRIBUTE_LARGEST - 1,
                                      false);
  } else {
    emberCopyStringArgument(5,
                            data,
                            emberAfGetDataSize(dataType),
                            false);
  }

  status = emberAfWriteManufacturerSpecificServerAttribute(endpoint,
                                 cluster,
                                 attribute,
                                 manufacturerCode,
                                 data,
                                 dataType);
  emberAfCorePrint("write %u", status);
  emberAfCorePrintln("");
}

void IdentifyTriggerEfect(void) {
  uint8_t  endpoint  = (uint8_t)emberUnsignedCommandArgument(0);
  uint8_t  effectIdentifier  = (uint8_t)emberUnsignedCommandArgument(1);
  uint8_t  effectVariant  = (uint8_t)emberUnsignedCommandArgument(2);

  IdentifyClient_TriggerEffect(endpoint, effectIdentifier, effectVariant);

}

#if 1	// DKo - added for testing convenience. For some reason I couldn't generate the standard CLI interface via SimplicityStudio.
void MoveToLevelTest(void) {
	uint8_t  endpoint  		  = (uint8_t) emberUnsignedCommandArgument(0);
	uint8_t  level  		  = (uint8_t) emberUnsignedCommandArgument(1);
	uint16_t transitionTime   = (uint16_t)emberUnsignedCommandArgument(2);
	EmberAfStatus status = LevelControlServerMoveToLevel(endpoint, level, transitionTime, true);
	emberAfCorePrintln("move to level status %X", status);
}

void MoveToColorTemperatureTest(void) {
	uint8_t  endpoint  		  = (uint8_t) emberUnsignedCommandArgument(0);
	uint16_t colorTemperature = (uint16_t)emberUnsignedCommandArgument(1);
	uint16_t transitionTime   = (uint16_t)emberUnsignedCommandArgument(2);
	EmberAfStatus status = ColorControlServerMoveToColorTemp(endpoint, colorTemperature, transitionTime);
	emberAfCorePrintln("move to CT status %X", status);
}
#endif

void EnableBroadcastMode(void) {
	boolean enable = (boolean) emberUnsignedCommandArgument(0);
	extern void DaliDeviceHandling_TestBroadcastMode(void);
	DaliDeviceHandling_TestBroadcastMode();
  // user will need to reset device to make this effective
}

void EmergencyCmd(void) {
  uint8_t mode =  emberUnsignedCommandArgument(0);
  switch (mode) {
    case 1:
      emberAfEnceliumEmergencyClusterStartTestCallback(0);
      break;
    case 2:
      emberAfEnceliumEmergencyClusterStartTestCallback(1);
      break;
    case 3:
      emberAfEnceliumEmergencyClusterStopTestCallback();
      break;
    case 4:
      emberAfEnceliumEmergencyClusterStartIdentifyCallback();
      break;
    default:
      break;
  }
}

