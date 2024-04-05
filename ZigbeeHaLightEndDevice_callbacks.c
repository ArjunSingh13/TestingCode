//------------------------------------------------------------------------------
///   \file   ZigbeeHaLightEndDevice_callbacks.c
///   \since  2016-11-11
///   \brief  Definition of non cluster related callback-functions from stack
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup ZigbeeHaLightEndDeviceCallbacks Non Cluster related Callbacks
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------

#include "GlobalDefs.h"
#include "config.h"

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h" // emberAfResetAttributes()
#include "app/util/common/form-and-join.h"
#include "app/framework/plugin/ezmode-commissioning/ez-mode.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "app/framework/plugin/encelium-commissioning/ecom-interpan-types.h"
#include "app/framework/plugin/encelium-interface/encelium-interface.h"
#include "app/framework/plugin/encelium-identify/encelium-identify.h"
#include "app/framework/plugin/encelium-interface/encelium-interface.h"
#include "app/framework/plugin/encelium-commissioning/encelium-commissioning.h"
#include "app/framework/plugin/encelium-interpan/encelium-interpan.h"
#include "app/framework/plugin/osramds-basic-server/BasicServer.h"
#include "app/framework/plugin/osramds-identify-server/IdentifyServer.h"


#include "ZigbeeHaLightEndDevice_own.h"
#include "Services.h"
#include "OnOffServices.h"
#include "NetworkServices.h"
#include "LevelControlServices.h"
#include "EnceliumInterfaceServer.h"
#include "BallastConfigurationServices.h"
#include "startup-mfg-config.h"

typedef enum {
  Unknown = 0,
  PowerOn,
  Restart,
  Addressing,
  Running,
  Reset
} typeDeviceOperationMode;
typedef struct {
  typeDeviceOperationMode mode;
  uint32_t config;
  uint32_t modeTimer;
  struct {
    EmberNetworkStatus State;
    int32_t  retryDelay;
  } network;
} typeDeviceStatus;

typeDeviceStatus deviceStatus = {.mode = Unknown, .config = 0x00000000, .network = {.State = 0xFF, .retryDelay = 0}};
int32_t waitTicks = 0;
uint32_t actTime =0;

static void ZigbeeHaLightEndDeviceCheckDeviceStatus(void);
static void ZigbeeHaLightEndDeviceSetFactoryDefaults(void);



#if 1
static void ZigbeeHaLightEndDeviceCheckDeviceStatus(void) {

  switch (deviceStatus.mode) {
    case PowerOn:
      emberAfPluginEnceliumInterfaceSetEnable(ZIGBEE2DALI_REPEATER_ENDPOINT, ZigbeeHaLightEndDevice_IsInRepeatermode());
      if(   emberAfPluginEnceliumCommissioningNetworkParametersAssigned()
         || ZigbeeHaLightEndDevice_IsInRepeatermode()) {
        deviceStatus.mode = Running;
        emberAfPrintln(0xffff, "PowerOn NetworkParameters Assigned go to Run mode");
      }
      else {
        deviceStatus.mode = Addressing;
        emberAfPrintln(0xffff, "PowerOn NetworkParameters not Assigned so start Addressing");
        EnceliumInterfaceServer_ScanInterface();
      }
      break;
    case Reset:
      break;
    case Running:
      break;
    case Addressing:
      if(!Zigbee2Dali_IsInitializing()) {
        deviceStatus.mode = Running;
      }
      break;
  }
}

static void ZigbeeHaLightEndDeviceSetFactoryDefaults(void){
  deviceStatus.mode = Reset;
  emberAfPrintln(0xffff, "SetFactoryDefaults");
  emberAfClearReportTableCallback();                                      // Clear reporting table
  emberClearBindingTable();
  APPTOKENS_SET2DEFAULTS();
  emberAfResetAttributes(0xFF);

#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)
  emberAfPluginEnceliumCommissioningRestart(eEcomRestartReasonReturnToDefault,0,1);
#else
  halReboot();
#endif
}

bool ZigbeeHaLightEndDevice_IsInRepeatermode(void) {
  return ((deviceStatus.config & REPEATERMODE_ACTIVE) != 0);
}

void ZigbeeHaLightEndDevice_SetRepeatermode(bool active) {
  if (active) {
    deviceStatus.config |= REPEATERMODE_ACTIVE;
  }
  else {
    deviceStatus.config &= ~REPEATERMODE_ACTIVE;
  }
  halCommonSetToken(TOKEN_DEVICE_CONFIG, &deviceStatus.config);
}

// DKo - created to tackle multiple light devices of arbitrary endpoint assignment
static bool isLightPoint(uint8_t endpoint) {
  uint16_t deviceId = emberAfGetDeviceIdForEndpoint(endpoint);
  return (deviceId == 0x0101 || deviceId == 0x010C);
}

void ZigbeeHaLightEndDevice_LightPointIdentifyStop(uint8_t endpointIndex) {
  uint8_t endpoint = emberAfEndpointFromIndex(endpointIndex);
  if (isLightPoint(endpoint))
  LevelControlServer_RefreshCurrentOutputLevel(endpoint);
}

void ZigbeeHaLightEndDevice_LightPointIdentifySetOutput(uint8_t endpointIndex, uint8_t level, uint16_t fadetime) {
  uint8_t endpoint = emberAfEndpointFromIndex(endpointIndex);
  if (isLightPoint(endpoint))
    LevelControlServer_SetOutput(emberAfEndpointFromIndex(endpointIndex), level, fadetime);
}

void ZigbeeHaLightEndDevice_SAKIdentifyStart(uint8_t endpointIndex) {
  if ((endpointIndex ==0) || (endpointIndex == 1)) {  // SAK or Repeater endpoint
    emberAfPluginOsramdsReset2factoryIdentifyStart();
  }
}

void ZigbeeHaLightEndDevice_SAKIdentifyStop(uint8_t endpointIndex) {
  if ((endpointIndex == 0) || (endpointIndex == 1)) {  // SAK or Repeater endpoint
    emberAfPluginOsramdsReset2factoryIdentifyStop();
  }
}

void ZigbeeHaLightEndDevice_SAKIdentifySetOutput(uint8_t endpointIndex, uint8_t level, uint16_t fadetime) {
  if ((endpointIndex == 0) || (endpointIndex == 1)) {  // SAK or Repeater endpoint
    emberAfPluginOsramdsReset2factorySetStatusLed(level);
  }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be
 * notified of changes to the stack status and take appropriate action.  The
 * application should return true if the status has been handled and should
 * not be handled by the application framework.
 *
 * @param status   Ver.: always
 */
bool emberAfStackStatusCallback(EmberStatus status)
{
  // If we go up or down, let the user know, although the down case shouldn't
  // happen.
  emberAfPluginEnceliumCommissioningStackStatusCallback(status);

  switch (status)
  {
  case EMBER_NETWORK_UP:
      emberAfCorePrintln("APP: EMBER_NETWORK_UP");
      emberAfCorePrintln("%u: network up (panId 0x%2x, radio power %d)",
              halCommonGetInt32uMillisecondTick(),
              emberAfGetPanId(), emberGetRadioPower());


    //  UpdateInterfaceToken();


      // Start ,monitoring the neighbor table once it joins to network
   //   emberEventControlSetDelayQS(neighborTableMonitorEventControl, 4*15);

   //   DALI_APP_StackStatusNetworkUpCallback();

      // restart the OTA client
   //   emAfOtaClientStop();
   //   emberAfPluginOtaClientStackStatusCallback(status);


#ifdef __DEXAL_EXHAUST_TEST_
      // Stall for DEXAL Testing
      emberAfPluginEnceliumCommissioningStall(TRUE);
#endif
      break;

  case EMBER_NETWORK_DOWN:

      //emberAfCorePrintln("Time %u: EMBER_NETWORK_DOWN: OT Counter %d",halCommonGetInt32uMillisecondTick(), giOTCounter);

    emberAfCorePrintln("%u: network down", halCommonGetInt32uMillisecondTick());

      /* Set the full brightness */
  //   DALI_APP_StackStatusNetworkDowmCallback();

      // Stopping OTA Client due to stack down
 //     emAfOtaClientStop();

      // stopping neighbor table when out of network
  //    emberEventControlSetInactive(neighborTableMonitorEventControl);
      break;

  case EMBER_PRECONFIGURED_KEY_REQUIRED:
    emberAfCorePrintln("%u: authentication failure", halCommonGetInt32uMillisecondTick());
      break;

  case EMBER_JOIN_FAILED:
      break;
  case EMBER_MOVE_FAILED:
  case EMBER_CANNOT_JOIN_AS_ROUTER:
  case EMBER_NODE_ID_CHANGED:
  case EMBER_PAN_ID_CHANGED:
  case EMBER_CHANNEL_CHANGED:
  case EMBER_NO_BEACONS:
  case EMBER_RECEIVED_KEY_IN_THE_CLEAR:
    emberAfCorePrintln("EMBER_RECEIVED_KEY_IN_THE_CLEAR");
      break;
  case EMBER_NO_NETWORK_KEY_RECEIVED:
    emberAfCorePrintln("EMBER_NO_NETWORK_KEY_RECEIVED");
      break;
  case EMBER_NO_LINK_KEY_RECEIVED:
    emberAfCorePrintln("EMBER_NO_LINK_KEY_RECEIVED");
      break;
  default:
    emberAfCorePrintln("%u: Network status %x", halCommonGetInt32uMillisecondTick(),
              status);
      break;
  }
  return FALSE;
}

/** @brief Main Start
 *
 * This function is called at the start of main after the HAL has been
 * initialized.  The standard main function arguments of argc and argv are
 * passed in.  However not all platforms have support for main() function
 * arguments.  Those that do not are passed NULL for argv, therefore argv should
 * be checked for NULL before using it.  If the callback determines that the
 * program must exit, it should return true.  The value returned by main() will
 * be the value written to the returnCode pointer.  Otherwise the callback
 * should return false to let normal execution continue.
 *
 * @param returnCode   Ver.: always
 * @param argc   Ver.: always
 * @param argv   Ver.: always
 */
bool emberAfMainStartCallback(int* returnCode,
                              int argc,
                              char** argv) {
  if (halGetResetInfo() == 0x04) { // PowerOn Reset
    deviceStatus.mode = PowerOn;
  }
  else {
    deviceStatus.mode = PowerOn;
  }
  Services_Init();
  emberAfPluginOsramdsBasicServerPreInit();
  return false;
}

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup.
 * Any code that you would normally put into the top of the application's
 * main() routine should be put into this function.
        Note: No callback
 * in the Application Framework is associated with resource cleanup. If you
 * are implementing your application on a Unix host where resource cleanup is
 * a consideration, we expect that you will use the standard Posix system
 * calls, including the use of atexit() and handlers for signals such as
 * SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If you use the signal()
 * function to register your signal handler, please mind the returned value
 * which may be an Application Framework function. If the return value is
 * non-null, please make sure that you call the returned function from your
 * handler to avoid negating the resource cleanup of the Application Framework
 * itself.
 *
 */
void emberAfMainInitCallback(void)
{
#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)
  emberAfEnceliumCommissioningForceUpdateNaedValue(ENCELIUM_NAED_VALUE, ENCELIUM_NAED_BATCH, ENCELIUM_NAED_COMPAT);
#endif
  halCommonGetToken(&deviceStatus.config, TOKEN_DEVICE_CONFIG);
  emberAfCorePrintln("Services_Start start");
  Services_Start();
  emberAfCorePrintln("Services_Start end");
 // mfgAtStartup(); // arjun commented this also takes out DALI hal init
  emberAfPrintln(0xFFFF,"FW_VERSION %2X", FW_VERSION);
}


/** @brief AppActions
 *
 * This tick function is periodically invoked from within the Encelium
 * Commissioning library to execute application-specific tasks.
 *
 */boolean emberAfPluginEnceliumCommissioningAppActionsCallback(void){
  extern void emberAfMainTickCallback(void);
  emberAfMainTickCallback();
  return false;
}


 void APP_NetworkStatesChecking(void)
 {
     EmberNetworkStatus networkState = emberNetworkState();

     switch( networkState )
     {
     /** The node is currently attempting to join a network. */
     case EMBER_JOINING_NETWORK:
         break;
     case EMBER_LEAVING_NETWORK:
         break;
     }
 }

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at the
 * end of the main tick execution.
 *
 */
void emberAfMainTickCallback(void) {
  static int32u version = 0xFFFFFFFF;

  APP_NetworkStatesChecking();

  if (version == 0xFFFFFFFF) {
    EmberAfStatus status = emberAfReadClientAttribute(0x01,
                         0x0019,
                         0x0002,
                         (uint8_t*)&version,
                         4);
    emberAfPrintln(0xFFFF,"status %X",status);
    if (status == EMBER_SUCCESS) {
    emberAfPrintln(0xFFFF,"OTA VERSION %4X",version);
#if 0// code added by arjun
  EmberNetworkParameters networkParams;


  MEMSET(&networkParams, 0, sizeof(EmberNetworkParameters));
  emberAfGetFormAndJoinExtendedPanIdCallback(networkParams.extendedPanId);
  networkParams.panId = *(int16u*)emberGetEui64();
  emberAfCorePrintln("emberGetEui64 %lu", emberGetEui64());

  networkParams.radioChannel = 26;
  networkParams.radioTxPower = ECOM_GetRadioPowerForChannelCallback(26);

  status = emberAfFormNetwork(&networkParams);
  if (status != EMBER_SUCCESS) {
     // ecomNwkFindPrintln("Form error 0x%x aborting", status);
      emberAfAppFlush();
      emberAfScanErrorCallback(status);
  }
#endif
    }
  }
#if 1
  uint32_t ticks = Services_TimeTick();
  waitTicks-= ticks;
  if (waitTicks <= 0) {
    waitTicks = (uint32_t)   (400);  // wait 1 s (2.5ms)
    ZigbeeHaLightEndDeviceCheckDeviceStatus();
  } // else to if waitTicks >0
#endif
}

/** @brief Finished
 *
 * This callback is fired when the network-find plugin is finished with the
 * forming or joining process.  The result of the operation will be returned
 * in the status parameter.
 *
 * @param status   Ver.: always
 */
void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
  emberAfPrintln(0xffff, "emberAfPluginNetworkFindFinishedCallback - Status:%4x", status);
}


/** @brief Join
 *
 * This callback is called by the plugin when a joinable network has been
 * found.  If the application returns true, the plugin will attempt to join
 * the network.  Otherwise, the plugin will ignore the network and continue
 * searching.  Applications can use this callback to implement a network
 * blacklist.
 *
 * @param networkFound   Ver.: always
 * @param lqi   Ver.: always
 * @param rssi   Ver.: always
 */
bool emberAfPluginNetworkFindJoinCallback(EmberZigbeeNetwork *networkFound,
                                             uint8_t lqi,
                                             int8_t rssi)
{
  emberAfPrintln(0xffff, "emberAfPluginNetworkFindJoinCallback - PANID%4x RSSI:%u", networkFound->panId, rssi);
  emberAfPluginOsramdsIdentifyServerIdentifyEndpoint(0xFF, NETWORK_JOINED_IDENTIFICATION_TIME);
  return true;
}

/** @brief Configured
 *
 * This callback is called by the Reporting plugin whenever a reporting entry
 * is configured, including when entries are deleted or updated.  The
 * application can use this callback for scheduling readings or measurements
 * based on the minimum and maximum reporting interval for the entry.  The
 * application should return EMBER_ZCL_STATUS_SUCCESS if it can support the
 * configuration or an error status otherwise.  Note: attribute reporting is
 * required for many clusters and attributes, so rejecting a reporting
 * configuration may violate ZigBee specifications.
 *
 * @param entry   Ver.: always
 */
EmberAfStatus emberAfPluginReportingConfiguredCallback(const EmberAfPluginReportingEntry * entry)
{
  return EMBER_ZCL_STATUS_SUCCESS;
}


/** @brief HandleInterpanMessage
 *
 * This callback function is invoked from within the Encelium Commissioning
 * library for custom handling of Encelium Interpan messages.
 *
 * @param sender Source EUI of the message  Ver.: always
 * @param class Encelium Interpan class  Ver.: always
 * @param command Encelium Interpan command  Ver.: always
 * @param interface Encelium Interface ID  Ver.: always
 * @param payload Encelium Interpan message payload  Ver.: always
 * @param auxData Encelium Interpan message auxiliary data  Ver.: always
 * @param ret return status  Ver.: always
 */
boolean emberAfPluginEnceliumCommissioningHandleInterpanMessageCallback(EmberEUI64 sender,
                                                                        int8u class,
                                                                        int16u command,
                                                                        int8u endpoint,
                                                                        void* payload,
                                                                        void* auxData,
                                                                        void* ret)
{
  boolean result = false;
  enceliumInterpanMsg_t* msg = (enceliumInterpanMsg_t*) payload;
  switch (class) {
    case InterpanClass_Control:
      switch (command) {
        // this command is issued by the mapping tool to set the current dimming level of an uncommissioned device.
        case InterpanCommand_SetCurrentLevel:
          emberAfPrintln(0xFFFF, "emberAfPluginEnceliumCommissioningHandleInterpanMessageCallback InterpanCommand_SetCurrentLevel int:%x, Level %u", endpoint, msg->setCurrentLevel.u16Level);
          emberAfPluginOsramdsIdentifyServerCancel(endpoint);
          LevelControlServer_SetOutput(endpoint,
                                       (msg->setCurrentLevel.u16Level + 3) >> 2, 1); // u16Level is within [0,1023]
          break;
        default:
          break;
      }
      break;
    case  InterpanClass_Discovery:
      switch (command) {
        case InterpanCommand_RescanInterface:
          emberAfPrintln(0xFFFF, "emberAfPluginEnceliumCommissioningHandleInterpanMessageCallback InterpanCommand_RescanInterface");
          EnceliumInterfaceServer_RescanInterface(endpoint);
          result = true;
          break;
        case InterpanCommand_SetInterfaceConfig:
          {
            EnceliumInterfaceType type = (EnceliumInterfaceType) msg->setInterfaceConfig.u8InterfaceType;
            EnceliumInterfaceServer_SetInterfaceConfig(endpoint,type);
            // InterfaceConfig is expected to be sent immediately after SetInterfaceConfig.
            emberAfPluginEnceliumInterpanSendInterfaceConfig(endpoint);
            result = true;
          }
          break;
        default:
          break;
      }
    break;
    default:
      break;
  }
  return result;
}

#if defined(EMBER_AF_PLUGIN_ENCELIUM_COMMISSIONING)
/** @brief Event
 *
 * This function is called any time the plugin needs to communicate a particular
 * event up to the application layer.
 *
 * @param event Encelium commissioning event ID  Ver.: always
 * @param interface Interface ID  Ver.: always
 * @param param parameter for additional information  Ver.: always
 */
void emberAfPluginEnceliumCommissioningEventCallback(EmberAfEcomEvent event,
                                                     int8u endpoint,
                                                     int32u param)
{
  switch(event)
    {
    // This event is triggered when an Encelium interpan Identify has been
    // received from the mapping tool.  The application is expected to trigger
    // its Identify behaviour at this point.
    case EMBER_ZCL_ECOM_EVENT_IDENTIFYING_PATTERN:
      emberAfPrintln(0xFFFF, "emberAfPluginEnceliumCommissioningEventCallback EMBER_ZCL_ECOM_EVENT_IDENTIFYING_PATTERN int:%x, Param %x", endpoint, param);
      emberAfPluginOsramdsIdentifyServerStartPattern(endpoint, 0, param);
      if(endpoint == ZIGBEE2DALI_EMERGENCY_ENDPOINT || endpoint == 0xFF) {
          emberAfEnceliumEmergencyClusterStartIdentifyCallback();
          Zigbee2Dali_EmergencyStartFunctionTest();
      }
      break;
    case EMBER_ZCL_ECOM_EVENT_DIMMING_SEQUENCE: {
      int8u* ptr = (int8u*) param;
      int8u steps = *(ptr++);
      //emberAfPluginEnceliumIdentifyStartDimmingSequence(steps, (dimSeqStep_t*) ptr);
      emberAfPrintln(0xFFFF, "emberAfPluginEnceliumCommissioningEventCallback EMBER_ZCL_ECOM_EVENT_DIMMING_SEQUENCE int:%x, Param %x", endpoint, param);
#if 1 // DKo - clicker only sends the pattern on endpoint 2, which was an oversight.
      if (endpoint == 2)
          endpoint = 0xFF;  // broadcast endpoint instead
#endif
      emberAfPluginOsramdsIdentifyServerStartPattern(endpoint, 0, 15);
      if(endpoint == ZIGBEE2DALI_EMERGENCY_ENDPOINT || endpoint == 0xFF) {
          emberAfEnceliumEmergencyClusterStartIdentifyCallback();
          Zigbee2Dali_EmergencyStartFunctionTest();
      }
      break;
    }
    
    // This event is triggered in the event that we have not seen the
    // coordinator in over 5 minutes.  The Encelium standard behaviour is to
    // go to full brightness as a safety measure in this condition.
    case EMBER_ZCL_ECOM_EVENT_PENDING_NETWORK_LOST_START:
        // override with full-brightness (note: bypass attribute writing so
        // that it may be restored on network lost cancellation)
        emberAfPrintln(0xffff, "emberAfPluginEnceliumCommissioningEventCallback- PendingNetworkLost => Light to Pon");
        emberAfPluginOsramdsIdentifyServerCancel(0xFF);
        //LevelControlServer_SetOutput(0xFF, 254, 0);
        for (int i = 0; i < emberAfEndpointCount(); i++) {
            LevelControlServices_StartFadeTime(emAfEndpoints[i].endpoint, BallastConfigurationServer_GetPoweronLevel(emAfEndpoints[i].endpoint), 0);
    }
        break;

    // This event is triggered when the coordinator is heard again from within
    // 1 hour of EMBER_ZCL_ECOM_EVENT_PENDING_NETWORK_LOST_START.  It is
    // considered a "false alarm", and the application may choose to return to
    // its former dim level at this point.  Note that in an Encelium network,
    // the coordinator actively monitors the fixture brightness level, and
    // will issue a command to correct it if need be.
    case EMBER_ZCL_ECOM_EVENT_PENDING_NETWORK_LOST_CANCEL:
#if defined(EMBER_AF_PLUGIN_ENCELIUM_IDENTIFY)
        // restore former brightness level
        emberAfPluginEnceliumIdentifyRestoreAllEndpoints();
#endif
        break;
    default:
        break;
    }
}
#endif

/** @brief Pre ZDO Message Received
*
* This function passes the application an incoming ZDO message and gives the
* appictation the opportunity to handle it. By default, this callback returns
* FALSE indicating that the incoming ZDO message has not been handled and
* should be handled by the Application Framework.
*
* @samparam emberNodeId   Ver.: always
* @samparam apsFrame   Ver.: always
* @samparam message   Ver.: always
* @samparam length   Ver.: always
*/
boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                            EmberApsFrame* apsFrame,
                                            int8u* message,
                                            int16u length)
{
   // on ZDO leave request, leave and rejoin after a specified delay if we are joined
   // to an Encelium network, otherwise leave and return to default
   if(apsFrame->clusterId == LEAVE_REQUEST) {
       EcomRestartReason reason = eEcomRestartReasonReturnToDefault;
       if (emberAfPluginEnceliumCommissioningInEnceliumNetwork())
           reason = eEcomRestartReasonZdoLeave;
       emberAfPluginEnceliumCommissioningRestart(reason, 0, ZDO_LEAVE_REJOIN_DELAY);
       return TRUE;
   }

#ifdef EMBER_AF_PLUGIN_UPDATE_TC_LINK_KEY
   // Encelium Modified
   if (emberAfPluginUpdateTcLinkKeyPreZDOMessageReceivedCallback(emberNodeId,
                                            apsFrame,
                                            message,
                                            length)) {
       return TRUE;
   }
#endif

   return FALSE;
}

/** @brief External Attribute Read
 *
 * Like emberAfExternalAttributeWriteCallback above, this function is called
 * when the framework needs to read an attribute that is not stored within the
 * Application Framework's data structures.
        All of the important
 * information about the attribute itself is passed as a pointer to an
 * EmberAfAttributeMetadata struct, which is stored within the application and
 * used to manage the attribute. A complete description of the
 * EmberAfAttributeMetadata struct is provided in
 * app/framework/include/af-types.h
        This function assumes that the
 * application is able to read the attribute, write it into the passed buffer,
 * and return immediately. Any attributes that require a state machine for
 * reading and writing are not really candidates for externalization at the
 * present time. The Application Framework does not currently include a state
 * machine for reading or writing attributes that must take place across a
 * series of application ticks. Attributes that cannot be read in a timely
 * manner should be stored within the Application Framework and updated
 * occasionally by the application code from within the
 * emberAfMainTickCallback.
        If the application was successfully able to
 * read the attribute and write it into the passed buffer, it should return a
 * value of EMBER_ZCL_STATUS_SUCCESS. Any other return value indicates the
 * application was not able to read the attribute.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeMetadata   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param buffer   Ver.: always
 */
EmberAfStatus emberAfExternalAttributeReadCallback(int8u endpoint,
                                                   EmberAfClusterId clusterId,
                                                   EmberAfAttributeMetadata *attributeMetadata,
                                                   int16u manufacturerCode,
                                                   int8u *buffer,
                                                   uint16_t maxReadLength){
  EmberAfStatus status = EMBER_ZCL_STATUS_FAILURE;

  switch (clusterId){
    case ZCL_BALLAST_CONFIGURATION_CLUSTER_ID:
      if (  (attributeMetadata->attributeId == ZCL_LINEARIZATION_VALUE_ATTRIBUTE_ID)
          &&(manufacturerCode == OSRAM_MANUFACTURER_CODE)) {
        typeLinearizationTable table;
        switch (endpoint) {
          case 10:
            halCommonGetToken(&(table.address[0]), TOKEN_LINEARIZATION_TABLE_10);
            status = EMBER_ZCL_STATUS_SUCCESS;
          break;
          case 11:
            halCommonGetToken(&(table.address[0]), TOKEN_LINEARIZATION_TABLE_11);
            status = EMBER_ZCL_STATUS_SUCCESS;
            break;
          default: // nothing to do
            break;
        }
        if (status == EMBER_ZCL_STATUS_SUCCESS) {
          memcpy(buffer+1, (uint8_t*)&table, 18);
          *buffer = 18;
        }
      }
      break;
    default: // nothing to do
      break;
  }
  return status;

}

//

/** @brief External Attribute Write
 *
 * This function is called whenever the Application Framework needs to write an
 * attribute which is not stored within the data structures of the Application
 * Framework itself. One of the new features in Version 2 is the ability to
 * store attributes outside the Framework. This is particularly useful for
 * attributes that do not need to be stored because they can be read off the
 * hardware when they are needed, or are stored in some central location used by
 * many modules within the system. In this case, you can indicate that the
 * attribute is stored externally. When the framework needs to write an external
 * attribute, it makes a call to this callback.
        This callback is very
 * useful for host micros which need to store attributes in persistent memory.
 * Because each host micro (used with an Ember NCP) has its own type of
 * persistent memory storage, the Application Framework does not include the
 * ability to mark attributes as stored in flash the way that it does for Ember
 * SoCs like the EM35x. On a host micro, any attributes that need to be stored
 * in persistent memory should be marked as external and accessed through the
 * external read and write callbacks. Any host code associated with the
 * persistent storage should be implemented within this callback.
        All of
 * the important information about the attribute itself is passed as a pointer
 * to an EmberAfAttributeMetadata struct, which is stored within the application
 * and used to manage the attribute. A complete description of the
 * EmberAfAttributeMetadata struct is provided in
 * app/framework/include/af-types.h.
        This function assumes that the
 * application is able to write the attribute and return immediately. Any
 * attributes that require a state machine for reading and writing are not
 * candidates for externalization at the present time. The Application Framework
 * does not currently include a state machine for reading or writing attributes
 * that must take place across a series of application ticks. Attributes that
 * cannot be written immediately should be stored within the Application
 * Framework and updated occasionally by the application code from within the
 * emberAfMainTickCallback.
        If the application was successfully able to
 * write the attribute, it returns a value of EMBER_ZCL_STATUS_SUCCESS. Any
 * other return value indicates the application was not able to write the
 * attribute.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeMetadata   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param buffer   Ver.: always
 */
EmberAfStatus emberAfExternalAttributeWriteCallback(int8u endpoint,
                                                    EmberAfClusterId clusterId,
                                                    EmberAfAttributeMetadata *attributeMetadata,
                                                    int16u manufacturerCode,
                                                    int8u *buffer) {

  EmberAfStatus status = EMBER_ZCL_STATUS_FAILURE;

  switch (clusterId){
    case ZCL_BALLAST_CONFIGURATION_CLUSTER_ID:
      if (  (attributeMetadata->attributeId == ZCL_LINEARIZATION_VALUE_ATTRIBUTE_ID)
          &&(manufacturerCode == OSRAM_MANUFACTURER_CODE)) {
        typeLinearizationTable table;
        memcpy(&table, buffer + 1, 18);
        switch (endpoint) {
          case 10:
            halCommonSetToken(TOKEN_LINEARIZATION_TABLE_10, &table);
            status = EMBER_ZCL_STATUS_SUCCESS;
            break;
          case 11:
            halCommonSetToken(TOKEN_LINEARIZATION_TABLE_11, &table);
            status = EMBER_ZCL_STATUS_SUCCESS;
            break;
          default: // nothing to do
            break;
        }
      }
      break;
    default: // nothing to do
      break;
  }
  return status;
}

void emberAfPluginOsramdsReset2factoryReset2DefaultsCallback(void) {
  ZigbeeHaLightEndDeviceSetFactoryDefaults();
}

/** @brief Encelium Emergency Cluster Cluster Start Test
 *
 *
 *
 * @param Type   Ver.: always
 */
bool emberAfEnceliumEmergencyClusterStartTestCallback(uint8_t testType) {
  if (testType == 0) {
    Zigbee2Dali_EmergencyStartFunctionTest();
  }
  else if (testType == 1) {
    Zigbee2Dali_EmergencyStartDurationTest();
  }
  return true;
}

/** @brief Encelium Emergency Cluster Cluster Stop Test
 *
 *
 *
 */
bool emberAfEnceliumEmergencyClusterStopTestCallback(void) {
  Zigbee2Dali_EmergencyStopTest();
  return true;
}

/** @brief Encelium Emergency Cluster Cluster Inhibit
 *
 *
 *
 * @param Enable   Ver.: always
 */
bool emberAfEnceliumEmergencyClusterInhibitCallback(uint8_t enable) {
  if (enable) {
    Zigbee2Dali_EmergencyInhibit();
  } else {
    Zigbee2Dali_EmergencyResetInhibit();
  }
  return true;
}

/** @brief Encelium Emergency Cluster Cluster Prolong Time
 *
 *
 *
 * @param Time   Ver.: always
 */
bool emberAfEnceliumEmergencyClusterProlongTimeCallback(uint8_t time) {
  Zigbee2Dali_EmergencyProlongBy(time);
  return true;
}

/** @brief Encelium Emergency Cluster Cluster Start Identify
 *
 *
 *
 */
bool emberAfEnceliumEmergencyClusterStartIdentifyCallback(void) {
  Zigbee2Dali_EmergencyStartIdentify();
  return true;
}

/** @brief Encelium Emergency Cluster Cluster Server Manufacturer Specific Attribute Changed
 *
 * Server Manufacturer Specific Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 * @param manufacturerCode Manufacturer Code of the attribute that changed
 * Ver.: always
 */
void emberAfEnceliumEmergencyClusterServerManufacturerSpecificAttributeChangedCallback(uint8_t endpoint,
                                                                                       EmberAfAttributeId attributeId,
                                                                                       uint16_t manufacturerCode) {
  EmberStatus status;
  uint8_t value;

  switch(attributeId) {
  case ZCL_TEST_EXECUTION_TIMEOUT_ATTRIBUTE_ID:
    status = emberAfReadManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                            ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                            ZCL_TEST_EXECUTION_TIMEOUT_ATTRIBUTE_ID,
                                                            OSRAM_MANUFACTURER_CODE,
                                                            (uint8_t *) &value,
                                                            1);
    if (status == EMBER_SUCCESS) {
      Zigbee2Dali_EmergencySetTestExecutionTimeout(value);
    }
    break;
  case ZCL_EMERGENCY_LEVEL_ATTRIBUTE_ID:
    status = emberAfReadManufacturerSpecificServerAttribute(ZIGBEE2DALI_EMERGENCY_ENDPOINT,
                                                            ZCL_ENCELIUM_EMERGENCY_CLUSTER_ID,
                                                            ZCL_EMERGENCY_LEVEL_ATTRIBUTE_ID,
                                                            OSRAM_MANUFACTURER_CODE,
                                                            (uint8_t *) &value,
                                                            1);
    if (status == EMBER_SUCCESS) {
      Zigbee2Dali_EmergencySetLevel(value);
    }
    break;
  default:
    break;
  }
}
#endif
/// @}

