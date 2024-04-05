//------------------------------------------------------------------------------
///   \file   ColorControlServer.c
///   \since  2017-07-11
///   \brief  Definition of callback-functions and interface functions 
///           for Color Control-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup ColorControlClusterServer ColorControl Cluster Server
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------

#include "GlobalDefs.h"

#include PLATFORM_HEADER

#ifndef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
#include "stack/include/ember.h"
#endif

#include "app/framework/include/af.h"
#include "ZigbeeServices/ColorControlServices.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfColorControlClusterPrintln(__VA_ARGS__);}
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF,__VA_ARGS__);}

#define COLOR_CONTROL_CLUSTER_MIN_COLOR_TEMP 1
#define COLOR_CONTROL_CLUSTER_MAX_COLOR_TEMP 0xFEFF

#ifndef LEVEL_CONTROL_CLUSTER_MIN_LEVEL
#define LEVEL_CONTROL_CLUSTER_MIN_LEVEL 1
#endif
#ifndef LEVEL_CONTROL_CLUSTER_MAX_LEVEL
#define LEVEL_CONTROL_CLUSTER_MAX_LEVEL 254
#endif

#define COLOR_TEMP_CONTROL tempTransitionEventControl
#define COLOR_XY_CONTROL   xyTransitionEventControl
#define COLOR_HSV_CONTROL  hueSatTransitionEventControl

#define UPDATE_TIME_MS 100
// --------- Local Variables ---------------------------------------------------

typedef struct {
  uint8_t commandId;
  uint8_t moveToColourTemp;
  uint16_t storedColourTemp;
  uint16_t storedXValue;
  uint16_t storedYValue;
} TypeColorControlState;

enum {
  MOVE_MODE_STOP     = 0x00,
  MOVE_MODE_UP       = 0x01,
  MOVE_MODE_DOWN     = 0x03
};

enum {
  COLOR_MODE_HSV         = 0x00,
  COLOR_MODE_CIE_XY      = 0x01,
  COLOR_MODE_TEMPERATURE = 0x02
};

enum {
  HSV_TO_HSV         = 0x00,
  HSV_TO_CIE_XY      = 0x01,
  HSV_TO_TEMPERATURE = 0x02,
  CIE_XY_TO_HSV         = 0x10,
  CIE_XY_TO_CIE_XY      = 0x11,
  CIE_XY_TO_TEMPERATURE = 0x12,
  TEMPERATURE_TO_HSV         = 0x20,
  TEMPERATURE_TO_CIE_XY      = 0x21,
  TEMPERATURE_TO_TEMPERATURE = 0x22
};

EmberEventControl tempTransitionEventControl;
EmberEventControl xyTransitionEventControl;
EmberEventControl hueSatTransitionEventControl;

typedef struct {
  uint16_t initialValue;
  uint16_t currentValue;
  uint16_t finalValue;
  uint16_t stepsRemaining;
  uint16_t stepsTotal;
  uint16_t lowLimit;
  uint16_t highLimit;
  uint8_t  endpoint;
} Color16uTransitionState;

static Color16uTransitionState colorXTransitionState;
static Color16uTransitionState colorYTransitionState;

static Color16uTransitionState colorTempTransitionState;

static TypeColorControlState stateTable[EMBER_AF_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

// --------- Local Function Prototypes -----------------------------------------

static TypeColorControlState *ColorControlServerGetState(uint8_t endpoint);
static uint16_t ColorControlServerLimitColorTemp(uint16_t color);
       EmberAfStatus ColorControlServerMoveToColorTemp(uint8_t endpoint, uint16_t colorTemp, uint16_t transitionTime);
static EmberAfStatus ColorControlServerMoveToXYColor(uint8_t endpoint, uint16_t x, uint16_t y, uint16_t transitionTime);
static EmberAfStatus ColorControlServerMoveColorTemp(uint8_t endpoint,
                                                     uint8_t moveMode,
                                                     uint16_t rate,
                                                     uint16_t colorTemperatureMinimum,
                                                     uint16_t colorTemperatureMaximum);
static EmberAfStatus  ColorControlServerStepColorTemp(uint8_t endpoint,
                                                      uint8_t stepMode,
                                                      uint16_t stepSize,
                                                      uint16_t transitionTime,
                                                      uint16_t colorTemperatureMinimum,
                                                      uint16_t colorTemperatureMaximum);
static uint16_t readColorX(uint8_t endpoint);
static uint16_t readColorY(uint8_t endpoint);
static uint16_t readColorTemperatureCoupleToLevelMin(uint8_t endpoint);
static uint8_t readLevelControlCurrentLevel(uint8_t endpoint);
static void writeColorMode(uint8_t endpoint, uint8_t colorMode);

// --------- Local Functions ---------------------------------------------------
static uint8_t readColorMode(uint8_t endpoint)
{
  uint8_t colorMode = 0x02;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID,
                                      (uint8_t *)&colorMode,
                                      sizeof(uint8_t));

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color mode.", status, endpoint);
  }
  return colorMode;
}

static uint16_t readColorX(uint8_t endpoint)
{
  uint16_t colorX = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID,
                                      (uint8_t *)&colorX,
                                      sizeof(uint16_t));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color X.", status, endpoint);
  }

  return colorX;
}

static uint16_t readColorY(uint8_t endpoint)
{
  uint16_t colorY = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID,
                                      (uint8_t *)&colorY,
                                      sizeof(uint16_t));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color Y.", status, endpoint);
  }

  return colorY;
}

static uint16_t readColorTemperatureCoupleToLevelMin(uint8_t endpoint)
{
  uint16_t colorTemperatureCoupleToLevelMin;
  EmberStatus status;

  status =
    emberAfReadServerAttribute(endpoint,
                               ZCL_COLOR_CONTROL_CLUSTER_ID,
                               ZCL_COLOR_CONTROL_TEMPERATURE_LEVEL_MIN_MIREDS_ATTRIBUTE_ID,
                               (uint8_t *)&colorTemperatureCoupleToLevelMin,
                               sizeof(uint16_t));

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    // Not less than the physical min.
    TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading couple color temp min.", status, endpoint);
    colorTemperatureCoupleToLevelMin = ColorControlServer_GetColorTempPhysMin(endpoint);
  }

  return colorTemperatureCoupleToLevelMin;
}

static uint8_t readLevelControlCurrentLevel(uint8_t endpoint)
{
  uint8_t currentLevel;
  EmberStatus status;

  status =
    emberAfReadServerAttribute(endpoint,
                               ZCL_LEVEL_CONTROL_CLUSTER_ID,
                               ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                               (uint8_t *)&currentLevel,
                               sizeof(uint8_t));

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    currentLevel = 0x7F; // midpoint of range 0x01-0xFE
  }

  return currentLevel;
}

static void writeColorMode(uint8_t endpoint, uint8_t colorMode)
{
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_ATTRIBUTE_ID,
                                       (uint8_t *)&colorMode,
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing enhanced color mode.", status, endpoint);
  }
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID,
                                       (uint8_t *)&colorMode,
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing color mode.", status, endpoint);
  }
}

static void writeColorX(uint8_t endpoint, uint16_t colorX)
{
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID,
                                       (uint8_t *)&colorX,
                                       ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing color X.", status, endpoint);
  }
}

static void writeColorY(uint8_t endpoint, uint16_t colorY)
{
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID,
                                       (uint8_t *)&colorY,
                                       ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing color Y.", status, endpoint);
  }
}

static void writeColorTemperature(uint8_t endpoint, uint16_t colorTemperature)
{
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                       (uint8_t *)&colorTemperature,
                                       ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing color temperature.", status, endpoint);
  }
}

static void writeRemainingTime(uint8_t endpoint, uint16_t remainingTime)
{
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  EmberAfStatus status;
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_COLOR_CONTROL_CLUSTER_ID,
                                         ZCL_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                         (uint8_t *)&remainingTime,
                                         ZCL_INT16U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
        TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing remanining time.", status, endpoint);
    }
#endif
}

#if 1	// ENCELIUM: options is ignored in an Encelium network.  For interoperability with HA networks,
		// consider re-instating the circumvented logic.
static bool shouldExecuteIfOff(uint8_t endpoint,
                               uint8_t optionMask,
                               uint8_t optionOverride)
{
	return true;
}
#else
static bool shouldExecuteIfOff(uint8_t endpoint,
                               uint8_t optionMask,
                               uint8_t optionOverride)
{
  // From 5.2.2.2.1.10 of ZCL7 document 14-0129-15f-zcl-ch-5-lighting.docx:
  //   "Command execution SHALL NOT continue beyond the Options processing if
  //    all of these criteria are true:
  //      - The On/Off cluster exists on the same endpoint as this cluster.
  //      - The OnOff attribute of the On/Off cluster, on this endpoint, is 0x00
  //        (FALSE).
  //      - The value of the ExecuteIfOff bit is 0."

  if (!emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) {
    return true;
  }

  uint8_t options;
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                    ZCL_OPTIONS_ATTRIBUTE_ID,
                                                    &options,
                                                    sizeof(options));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("Unable to read Options attribute: 0x%X",
                                      status);
    // If we can't read the attribute, then we should just assume that it has
    // its default value.
    options = 0x00;
  }

  bool on;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_ON_OFF_CLUSTER_ID,
                                      ZCL_ON_OFF_ATTRIBUTE_ID,
                                      (uint8_t *)&on,
                                      sizeof(on));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("Unable to read OnOff attribute: 0x%X",
                                      status);
    return true;
  }
  // The device is on - hence ExecuteIfOff does not matter
  if (on) {
    return true;
  }
  // The OptionsMask & OptionsOverride fields SHALL both be present or both
  // omitted in the command. A temporary Options bitmap SHALL be created from
  // the Options attribute, using the OptionsMask & OptionsOverride fields, if
  // present. Each bit of the temporary Options bitmap SHALL be determined as
  // follows:
  // Each bit in the Options attribute SHALL determine the corresponding bit in
  // the temporary Options bitmap, unless the OptionsMask field is present and
  // has the corresponding bit set to 1, in which case the corresponding bit in
  // the OptionsOverride field SHALL determine the corresponding bit in the
  // temporary Options bitmap.
  //The resulting temporary Options bitmap SHALL then be processed as defined
  // in section 5.2.2.2.1.10.

  // ---------- The following order is important in decision making -------
  // -----------more readable ----------
  //
  if (optionMask == 0xFF && optionOverride == 0xFF) {
    // 0xFF are the default values passed to the command handler when
    // the payload is not present - in that case there is use of option
    // attribute to decide execution of the command
    return READBITS(options, EMBER_ZCL_COLOR_CONTROL_OPTIONS_EXECUTE_IF_OFF);
  }
  // ---------- The above is to distinguish if the payload is present or not

  if (READBITS(optionMask, EMBER_ZCL_COLOR_CONTROL_OPTIONS_EXECUTE_IF_OFF)) {
    // Mask is present and set in the command payload, this indicates
    // use the override as temporary option
    return READBITS(optionOverride, EMBER_ZCL_COLOR_CONTROL_OPTIONS_EXECUTE_IF_OFF);
  }
  // if we are here - use the option attribute bits
  return (READBITS(options, EMBER_ZCL_COLOR_CONTROL_OPTIONS_EXECUTE_IF_OFF));
}
#endif

static void stopAllColorTransitions(void)
{
  TRACE(true,"stopAllColorTransitions: called");
  emberEventControlSetInactive(COLOR_TEMP_CONTROL);
  emberEventControlSetInactive(COLOR_XY_CONTROL);
  emberEventControlSetInactive(COLOR_HSV_CONTROL);
}

void emberAfPluginColorControlServerStopTransition(void)
{
  stopAllColorTransitions();
}

// The specification says that if we are transitioning from one color mode
// into another, we need to compute the new mode's attribute values from the
// old mode.  However, it also says that if the old mode doesn't translate into
// the new mode, this must be avoided.
// I am putting in this function to compute the new attributes based on the old
// color mode.
static void handleModeSwitch(uint8_t endpoint,
                             uint8_t newColorMode)
{
  uint8_t oldColorMode = readColorMode(endpoint);
  uint8_t colorModeTransition;

  if (oldColorMode == newColorMode) {
    return;
  } else {
    writeColorMode(endpoint, newColorMode);
  }

  colorModeTransition = (newColorMode << 4) + oldColorMode;

  // Note:  It may be OK to not do anything here.
  switch (colorModeTransition) {
    case HSV_TO_CIE_XY:
      //emberAfPluginColorControlServerComputePwmFromXyCallback(endpoint);
      break;
    case TEMPERATURE_TO_CIE_XY:
      //emberAfPluginColorControlServerComputePwmFromXyCallback(endpoint);
      break;
    case CIE_XY_TO_HSV:
      //emberAfPluginColorControlServerComputePwmFromHsvCallback(endpoint);
      break;
    case TEMPERATURE_TO_HSV:
      //emberAfPluginColorControlServerComputePwmFromHsvCallback(endpoint);
      break;
    case HSV_TO_TEMPERATURE:
      //emberAfPluginColorControlServerComputePwmFromTempCallback(endpoint);
      break;
    case CIE_XY_TO_TEMPERATURE:
      //emberAfPluginColorControlServerComputePwmFromTempCallback(endpoint);
      break;

    // for the following cases, there is no transition.
    case HSV_TO_HSV:
    case CIE_XY_TO_CIE_XY:
    case TEMPERATURE_TO_TEMPERATURE:
    default:
      return;
  }
}

// Return value of true means we need to stop.
static bool computeNewColor16uValue(Color16uTransitionState *p)
{
  uint32_t newValue32u;

  if (p->stepsRemaining == 0) {
    return false;
  }

  (p->stepsRemaining)--;

  writeRemainingTime(p->endpoint, p->stepsRemaining);

  // handle sign
  if (p->finalValue == p->currentValue) {
    // do nothing
  } else if (p->finalValue > p->initialValue) {
    newValue32u = ((uint32_t) (p->finalValue - p->initialValue));
    newValue32u *= ((uint32_t) (p->stepsRemaining));
    newValue32u /= ((uint32_t) (p->stepsTotal));
    p->currentValue = p->finalValue - ((uint16_t) (newValue32u));
  } else {
    newValue32u = ((uint32_t) (p->initialValue - p->finalValue));
    newValue32u *= ((uint32_t) (p->stepsRemaining));
    newValue32u /= ((uint32_t) (p->stepsTotal));
    p->currentValue = p->finalValue + ((uint16_t) (newValue32u));
  }

  if (p->stepsRemaining == 0) {
    // we have completed our move.
    return true;
  }

  return false;
}

static uint16_t computeTransitionTimeFromStateAndRate(Color16uTransitionState *p,
                                                      uint16_t rate)
{
  uint32_t transitionTime;
  uint16_t max, min;

  if (rate == 0) {
    return 0xFFFF;
  }

  if (p->currentValue > p->finalValue) {
    max = p->currentValue;
    min = p->finalValue;
  } else {
    max = p->finalValue;
    min = p->currentValue;
  }

  transitionTime = max - min;
  transitionTime *= 10;
  transitionTime /= rate;

  if (transitionTime > 0xFFFF) {
    return 0xFFFF;
  }

  return (uint16_t) transitionTime;
}

void xyTransitionEventHandler(void)
{
  bool limitReachedX, limitReachedY;

  // compute new values for X and Y.
  limitReachedX = computeNewColor16uValue(&colorXTransitionState);

  limitReachedY = computeNewColor16uValue(&colorYTransitionState);

  if (limitReachedX || limitReachedY) {
    stopAllColorTransitions();
  } else {
    emberEventControlSetDelayMS(COLOR_XY_CONTROL, UPDATE_TIME_MS);
  }

  // update the attributes
  writeColorX(colorXTransitionState.endpoint,
              colorXTransitionState.currentValue);
  writeColorY(colorXTransitionState.endpoint,
              colorYTransitionState.currentValue);

  emberAfColorControlClusterPrintln("Color X %d Color Y %d",
                                    colorXTransitionState.currentValue,
                                    colorYTransitionState.currentValue);
}

void tempTransitionEventHandler(void)
{
  bool limitReached;

  TRACE(true,"tempTransitionEventHandler: called");

  limitReached = computeNewColor16uValue(&colorTempTransitionState);

  if (limitReached) {
    stopAllColorTransitions();
  } else {
    emberEventControlSetDelayMS(COLOR_TEMP_CONTROL, UPDATE_TIME_MS);
  }

  writeColorTemperature(colorTempTransitionState.endpoint,
                        colorTempTransitionState.currentValue);

  emberAfColorControlClusterPrintln("Color Temperature %d",
                                    colorTempTransitionState.currentValue);
}


/*******************************************************************************/
/** \brief Get a pointer to the cluster control structure for that endpoint
 *
 * \param[in] endpoint
 */
static TypeColorControlState *ColorControlServerGetState(uint8_t endpoint)
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint,
                                                   ZCL_COLOR_CONTROL_CLUSTER_ID);
  return (ep == 0xFF ? NULL : &stateTable[ep]);
}

/** \brief Limit output Color Temperature
 *
 * \param[in] target color temp
 * \return    color temp limited to range
 */
static uint16_t ColorControlServerLimitColorTemp(uint16_t color) {
  if (color <= COLOR_CONTROL_CLUSTER_MAX_COLOR_TEMP && color >= COLOR_CONTROL_CLUSTER_MIN_COLOR_TEMP) {
      return color;
  }
  if (color >= COLOR_CONTROL_CLUSTER_MAX_COLOR_TEMP) {
    return COLOR_CONTROL_CLUSTER_MAX_COLOR_TEMP;
  }
  else {
      return COLOR_CONTROL_CLUSTER_MIN_COLOR_TEMP;
  }
}

static EmberAfStatus ColorControlServerMoveToXYColor(uint8_t endpoint, uint16_t x, uint16_t y, uint16_t transitionTime) {
    TypeColorControlState *state = ColorControlServerGetState(endpoint);

    if (NULL == state) return EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND;

    x = ColorControlServerLimitColorTemp(x);
    y = ColorControlServerLimitColorTemp(y);

    // first action should be to change the attribute
    emberAfWriteServerAttribute(endpoint,
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID,
                                (uint8_t *)&x,
                                ZCL_INT16U_ATTRIBUTE_TYPE);
    emberAfWriteServerAttribute(endpoint,
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID,
                                (uint8_t *)&y,
                                ZCL_INT16U_ATTRIBUTE_TYPE);


    transitionTime = (0xFFFF == transitionTime) ? 0 : transitionTime;    // Fade immediate if transition Time is 0xFFF

    ColorControlServices_StartColorTemperatureXYFadeTime(endpoint, x, y, transitionTime);
    state->commandId = ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID;
    state->storedXValue= x;
    state->storedYValue = y;

    uint8_t colorMode = 0x01;
    emberAfWriteServerAttribute(endpoint,
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID,
                                (uint8_t *)&colorMode,
                                ZCL_INT8U_ATTRIBUTE_TYPE);

    return EMBER_ZCL_STATUS_SUCCESS;
}

static EmberAfStatus ColorControlServerMoveColorTemp(uint8_t endpoint,
                                                     uint8_t moveMode,
                                                     uint16_t rate,
                                                     uint16_t colorTemperatureMinimum,
                                                     uint16_t colorTemperatureMaximum) {
  uint16_t targetColorTemp, transitionTime;
  TypeColorControlState *state = ColorControlServerGetState(endpoint);
  uint16_t tempPhysicalMin = ColorControlServer_GetColorTempPhysMin(endpoint);
  uint16_t tempPhysicalMax = ColorControlServer_GetColorTempPhysMax(endpoint);

  if (colorTemperatureMinimum == 0 ) {
    colorTemperatureMinimum = tempPhysicalMin;
  }
  if (colorTemperatureMaximum == 0 ) {
    colorTemperatureMaximum = tempPhysicalMax;
  }
  colorTemperatureMinimum = colorTemperatureMinimum > tempPhysicalMin ? colorTemperatureMinimum : tempPhysicalMin;
  colorTemperatureMinimum = colorTemperatureMinimum < tempPhysicalMax ? colorTemperatureMinimum : tempPhysicalMax; // a strange case
  colorTemperatureMaximum = colorTemperatureMaximum < tempPhysicalMax ? colorTemperatureMaximum : tempPhysicalMax;
  colorTemperatureMaximum = colorTemperatureMaximum > tempPhysicalMin ? colorTemperatureMaximum : tempPhysicalMin; // a strange case

  stopAllColorTransitions();

  if (rate == 0 && moveMode != MOVE_MODE_STOP) {
    return EMBER_ZCL_STATUS_INVALID_FIELD; // a rate of 0 means do nothing
  }
  switch (moveMode) {
  case MOVE_MODE_UP:
  {
    targetColorTemp = colorTemperatureMaximum;
    break;
  }
  case MOVE_MODE_DOWN:
  {
    targetColorTemp = colorTemperatureMinimum;
    break;
  }
  case MOVE_MODE_STOP:
  {
#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)
    emberAfColorControlClusterStopMoveStepCallback();
#else
    emberAfColorControlClusterStopMoveStepCallback(0,0);
#endif
    return EMBER_ZCL_STATUS_SUCCESS;
    break;
  }
  default:
    return EMBER_ZCL_STATUS_INVALID_VALUE; // invalid stepMode
    break;
  }

  handleModeSwitch(endpoint, COLOR_MODE_TEMPERATURE);

  if(ColorControlServer_GetColorTemp(endpoint) == targetColorTemp) {
    return EMBER_ZCL_STATUS_SUCCESS;
  }

  colorTempTransitionState.initialValue = ColorControlServer_GetColorTemp(endpoint);
  colorTempTransitionState.currentValue = ColorControlServer_GetColorTemp(endpoint);
  colorTempTransitionState.finalValue = targetColorTemp;

  transitionTime = computeTransitionTimeFromStateAndRate(&colorTempTransitionState, rate);
  transitionTime = transitionTime == 0 ? 1 : transitionTime;

  colorTempTransitionState.stepsRemaining = transitionTime;
  colorTempTransitionState.stepsTotal     = transitionTime;
  colorTempTransitionState.endpoint  = endpoint;
  colorTempTransitionState.lowLimit  = colorTemperatureMinimum;
  colorTempTransitionState.highLimit = colorTemperatureMaximum;

  writeRemainingTime(endpoint, transitionTime);

  ColorControlServices_StartColorTemperatureFadeTime(endpoint, targetColorTemp, transitionTime);

  // kick off the state machine:
  emberEventControlSetDelayMS(COLOR_TEMP_CONTROL, UPDATE_TIME_MS);

  return EMBER_ZCL_STATUS_SUCCESS;
}


EmberAfStatus ColorControlServerMoveToColorTemp(uint8_t endpoint, uint16_t colorTemp, uint16_t transitionTime) {
    TypeColorControlState *state = ColorControlServerGetState(endpoint);

    TRACE(true,"ColorControlServerMoveToColorTemp: target=%u",colorTemp)

    if (NULL == state) return EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND;

    if (transitionTime == 0) {
      transitionTime++;
    }

    TRACE(true,"ColorControlServerMoveToColorTemp: transitionTime=%u",transitionTime)

    stopAllColorTransitions();
    handleModeSwitch(endpoint, COLOR_MODE_TEMPERATURE);

    colorTemp = ColorControlServerLimitColorTemp(colorTemp);
    colorTemp = ColorControlServer_GetColorTempPhysMin(endpoint) > colorTemp ? ColorControlServer_GetColorTempPhysMin(endpoint) : colorTemp;
    colorTemp = ColorControlServer_GetColorTempPhysMax(endpoint) < colorTemp ? ColorControlServer_GetColorTempPhysMax(endpoint) : colorTemp;

    TRACE(true,"ColorControlServerMoveToColorTemp: adjusted target=%u",colorTemp)
    // now, kick off the state machine.
    colorTempTransitionState.initialValue = ColorControlServer_GetColorTemp(endpoint);
    colorTempTransitionState.currentValue = ColorControlServer_GetColorTemp(endpoint);
    colorTempTransitionState.finalValue   = colorTemp;
    colorTempTransitionState.stepsRemaining = transitionTime;
    colorTempTransitionState.stepsTotal     = transitionTime;
    colorTempTransitionState.endpoint  = endpoint;
    colorTempTransitionState.lowLimit  = ColorControlServer_GetColorTempPhysMin(endpoint);
    colorTempTransitionState.highLimit = ColorControlServer_GetColorTempPhysMax(endpoint);

    TRACE(true,"ColorControlServerMoveToColorTemp: starting FSM...")

    ColorControlServices_StartColorTemperatureFadeTime(endpoint, colorTemp, transitionTime);
    emberEventControlSetDelayMS(COLOR_TEMP_CONTROL, UPDATE_TIME_MS);

    return EMBER_ZCL_STATUS_SUCCESS;
}

static EmberAfStatus  ColorControlServerStepColorTemp(uint8_t endpoint,
                                                      uint8_t stepMode,
                                                      uint16_t stepSize,
                                                      uint16_t transitionTime,
                                                      uint16_t colorTemperatureMinimum,
                                                      uint16_t colorTemperatureMaximum) {
  uint16_t currentColorTemp;
  int16_t calculatedTargetColorTemp;
  TypeColorControlState *state = ColorControlServerGetState(endpoint);

  uint16_t tempPhysicalMin = ColorControlServer_GetColorTempPhysMin(endpoint);
  uint16_t tempPhysicalMax = ColorControlServer_GetColorTempPhysMax(endpoint);


  if (colorTemperatureMinimum == 0 ) {
    colorTemperatureMinimum = tempPhysicalMin;
  }
  if (colorTemperatureMaximum == 0 ) {
    colorTemperatureMaximum = tempPhysicalMax;
  }
  colorTemperatureMinimum = colorTemperatureMinimum < tempPhysicalMin ? tempPhysicalMin : colorTemperatureMinimum;
  colorTemperatureMinimum = colorTemperatureMinimum > tempPhysicalMax ? tempPhysicalMax : colorTemperatureMinimum;
  colorTemperatureMaximum = colorTemperatureMaximum > tempPhysicalMax ? tempPhysicalMax : colorTemperatureMaximum;
  colorTemperatureMaximum = colorTemperatureMaximum < tempPhysicalMin ? tempPhysicalMin : colorTemperatureMaximum;

  if (transitionTime == 0) {
    transitionTime++;
  }

  stopAllColorTransitions();
  handleModeSwitch(endpoint, COLOR_MODE_TEMPERATURE);

  currentColorTemp = ColorControlServer_GetColorTemp(endpoint);

  calculatedTargetColorTemp = currentColorTemp;
  if (stepMode == MOVE_MODE_UP) {      // up
    calculatedTargetColorTemp += stepSize;
    calculatedTargetColorTemp = calculatedTargetColorTemp > colorTemperatureMaximum ? colorTemperatureMaximum : calculatedTargetColorTemp;
  }
  else if (stepMode == MOVE_MODE_DOWN) { // down
    calculatedTargetColorTemp -= stepSize;
    calculatedTargetColorTemp = calculatedTargetColorTemp < colorTemperatureMinimum ? colorTemperatureMinimum : calculatedTargetColorTemp;
  }
  else {
    return EMBER_ZCL_STATUS_INVALID_VALUE; // invalid stepMode
  }

  calculatedTargetColorTemp = ColorControlServerLimitColorTemp(calculatedTargetColorTemp);

  if(currentColorTemp == calculatedTargetColorTemp) {
    return EMBER_ZCL_STATUS_SUCCESS;
  }
  colorTempTransitionState.initialValue = currentColorTemp;
  colorTempTransitionState.currentValue = currentColorTemp;
  colorTempTransitionState.finalValue = calculatedTargetColorTemp;
  colorTempTransitionState.stepsRemaining = transitionTime;
  colorTempTransitionState.stepsTotal     = transitionTime;
  colorTempTransitionState.endpoint  = endpoint;
  colorTempTransitionState.lowLimit  = colorTemperatureMinimum;
  colorTempTransitionState.highLimit = colorTemperatureMaximum;

  writeRemainingTime(endpoint, transitionTime);

  ColorControlServices_StartColorTemperatureFadeTime(endpoint, calculatedTargetColorTemp, transitionTime);

  // kick off the state machine:
  emberEventControlSetDelayMS(COLOR_TEMP_CONTROL, UPDATE_TIME_MS);

  return EMBER_ZCL_STATUS_SUCCESS;
}

// ------------------------------   Global functions ---------------------------

#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)

/**
 *
 *
 *
 * \param[in] moveMode (0x00 Stop, 0x01 Up, 0x02 Reserved, 0x03 Down)
 * \param[in] rate
 * \param[in] colorTemperatureMinimum in Mireds
 * \param[in] colorTemperatureMaximum in Mireds
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveColorTemperatureCallback(uint8_t moveMode,
                                                            uint16_t rate,
                                                            uint16_t colorTemperatureMinimum,
                                                            uint16_t colorTemperatureMaximum,
                                                            uint8_t optionsMask,
                                                            uint8_t optionsOverride) {
  EmberAfStatus status;

  TRACE (true, "ColorControlCluster MoveColorTemprature ep:%u move mode:%u rate:%u", emberAfCurrentEndpoint(), moveMode, rate);
  status = ColorControlServerMoveColorTemp(emberAfCurrentEndpoint(), moveMode, rate, colorTemperatureMinimum, colorTemperatureMaximum);

  emberAfSendImmediateDefaultResponse(status);
  return true;

}

/**
 *
 *
 *
 * \param[in] colorTemperature in Mireds
 * \param[in] transitionTime in 0.1s
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveToColorTemperatureCallback(uint16_t colorTemperature,
                                                              uint16_t transitionTime,
                                                              uint8_t optionsMask,
                                                              uint8_t optionsOverride) {
    EmberAfStatus status;

    TRACE (true, "ColorControlCluster MoveToColorTemprature ep:%u ColorTemp:%u time:%u", emberAfCurrentEndpoint(), colorTemperature, transitionTime);
    status = ColorControlServerMoveToColorTemp(emberAfCurrentEndpoint(), colorTemperature, transitionTime);

    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 *
 * \param[in] colorX
 * \param[in] colorY
 * \param[in] transitionTime in 0.1s
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveToColorCallback(uint16_t colorX,
                                                   uint16_t colorY,
                                                   uint16_t transitionTime,
                                                   uint8_t optionsMask,
                                                   uint8_t optionsOverride){
    EmberAfStatus status;

    TRACE (true, "ColorControlCluster MoveToColor ep:%u colorX:%u colorY:%u time:%u", emberAfCurrentEndpoint(), colorX, colorY, transitionTime);
    status = ColorControlServerMoveToXYColor(emberAfCurrentEndpoint(), colorX, colorY, transitionTime);

    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/** @brief Color Control Cluster Stop Move Step
 *
 * @param optionsMask
 * @param optionsOverride
 */
bool emberAfColorControlClusterStopMoveStepCallback(void){
  TRACE (true, "ColorControlCluster Stop move ep:%u", emberAfCurrentEndpoint());

  stopAllColorTransitions();
  ColorControlServices_StopColorTemperature(emberAfCurrentEndpoint());

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

/**
 * The Step Color Temperature command allows the color temperature of a lamp to be stepped
 * with a specified step size.
 *
 * \param[in] stepMode
 * \param[in] stepSize
 * \param[in] transitionTime in 0.1s
 * \param[in] colorTemperatureMinimum in Mireds
 * \param[in] colorTemperatureMaximum in Mireds
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterStepColorTemperatureCallback(uint8_t stepMode,
                                                            uint16_t stepSize,
                                                            uint16_t transitionTime,
                                                            uint16_t colorTemperatureMinimum,
                                                            uint16_t colorTemperatureMaximum) {
  EmberAfStatus status;
  status = ColorControlServerStepColorTemp(emberAfCurrentEndpoint(), stepMode, stepSize, transitionTime, colorTemperatureMinimum, colorTemperatureMaximum);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}


#else
/**
 *
 *
 *
 * \param[in] moveMode (0x00 Stop, 0x01 Up, 0x02 Reserved, 0x03 Down)
 * \param[in] rate
 * \param[in] colorTemperatureMinimum in Mireds
 * \param[in] colorTemperatureMaximum in Mireds
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveColorTemperatureCallback(uint8_t moveMode,
                                                            uint16_t rate,
                                                            uint16_t colorTemperatureMinimum,
                                                            uint16_t colorTemperatureMaximum,
                                                            uint8_t optionsMask,
                                                            uint8_t optionsOverride) {
  EmberAfStatus status;

  TRACE (true, "ColorControlCluster MoveColorTemprature ep:%u move mode:%u rate:%u", emberAfCurrentEndpoint(), moveMode, rate);
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), optionsMask, optionsOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = ColorControlServerMoveColorTemp(emberAfCurrentEndpoint(), moveMode, rate, colorTemperatureMinimum, colorTemperatureMaximum);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;

}

/**
 *
 *
 *
 * \param[in] colorTemperature in Mireds
 * \param[in] transitionTime in 0.1s
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveToColorTemperatureCallback(uint16_t colorTemperature,
                                                              uint16_t transitionTime,
                                                              uint8_t optionsMask,
                                                              uint8_t optionsOverride) {
    EmberAfStatus status;

    TRACE (true, "ColorControlCluster MoveToColorTemprature ep:%u ColorTemp:%u time:%u", emberAfCurrentEndpoint(), colorTemperature, transitionTime);
    if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), optionsMask, optionsOverride)) {
      status = EMBER_ZCL_STATUS_SUCCESS;
    }
    else {
      status = ColorControlServerMoveToColorTemp(emberAfCurrentEndpoint(), colorTemperature, transitionTime);
    }

    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 *
 * \param[in] colorX
 * \param[in] colorY
 * \param[in] transitionTime in 0.1s
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterMoveToColorCallback(uint16_t colorX,
                                                   uint16_t colorY,
                                                   uint16_t transitionTime,
                                                   uint8_t optionsMask,
                                                   uint8_t optionsOverride){
    EmberAfStatus status;

    TRACE (true, "ColorControlCluster MoveToColor ep:%u colorX:%u colorY:%u time:%u", emberAfCurrentEndpoint(), colorX, colorY, transitionTime);
    status = ColorControlServerMoveToXYColor(emberAfCurrentEndpoint(), colorX, colorY, transitionTime);

    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/** @brief Color Control Cluster Stop Move Step
 *
 * @param optionsMask
 * @param optionsOverride
 */
bool emberAfColorControlClusterStopMoveStepCallback(uint8_t optionMask,
                                                    uint8_t optionOverride){
  TRACE (true, "ColorControlCluster Stop move ep:%u", emberAfCurrentEndpoint());
  if (shouldExecuteIfOff(emberAfCurrentEndpoint(), optionMask, optionOverride)) {
      stopAllColorTransitions();
      ColorControlServices_StopColorTemperature(emberAfCurrentEndpoint());
    }

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

/**
 * The Step Color Temperature command allows the color temperature of a lamp to be stepped
 * with a specified step size.
 *
 * \param[in] stepMode
 * \param[in] stepSize
 * \param[in] transitionTime in 0.1s
 * \param[in] colorTemperatureMinimum in Mireds
 * \param[in] colorTemperatureMaximum in Mireds
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfColorControlClusterStepColorTemperatureCallback(uint8_t stepMode,
                                                            uint16_t stepSize,
                                                            uint16_t transitionTime,
                                                            uint16_t colorTemperatureMinimum,
                                                            uint16_t colorTemperatureMaximum,
                                                            uint8_t optionsMask,
                                                            uint8_t optionsOverride) {
  EmberAfStatus status;

  TRACE (true, "ColorControlCluster StepColorTemperature ep:%u", emberAfCurrentEndpoint());
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), optionsMask, optionsOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = ColorControlServerStepColorTemp(emberAfCurrentEndpoint(), stepMode, stepSize, transitionTime, colorTemperatureMinimum, colorTemperatureMaximum);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;
}
#endif

/** \brief Gets the start up color temperature of the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return color temperature in mired
 */
uint16_t ColorControlServer_GetStartUpColorTemp(uint8_t endpoint) {
  uint16_t mired = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID,
                                      (uint8_t *)&mired,
                                      sizeof(mired));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      mired = 250; // 4000K
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading start up color temperature.", status, endpoint);
  }
  TRACE(true, "START_UP_COLOR_TEMP_MIREDS: %2X", mired);
  if (mired == 0) {mired = 0xFFFF;} //0 is an invalid startup color, but we defaulted to it set that way.
  if(mired == 0xFFFF) {
    mired = ColorControlServer_GetColorTemp(endpoint);
    TRACE(true, "Last COLOR_TEMP_MIREDS: %2X", mired);
  } else {
	writeColorTemperature(endpoint, mired); //Our current color will be the startup color shortly.
  }
  return mired;
}

/** \brief Gets the color temperature of the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return color temperature in mired
 */
uint16_t ColorControlServer_GetColorTemp(uint8_t endpoint) {
  uint16_t mired = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                      (uint8_t *)&mired,
                                      sizeof(mired));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      mired = 250; // 4000K
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color temperature.", status, endpoint);
  }
  return mired;
}

/** \brief Gets the physical min color temprature of the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return color temperature in mired
 */
uint16_t ColorControlServer_GetColorTempPhysMin(uint8_t endpoint) {
  uint16_t mired = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ATTRIBUTE_ID,
                                      (uint8_t *)&mired,
                                      sizeof(mired));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      mired = 154; // 6490K - Warmest
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color temperature phy min.", status, endpoint);
  }
  return mired;
}

/** \brief Gets the physical max color temprature of the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \return color temperature in mired
 */
uint16_t ColorControlServer_GetColorTempPhysMax(uint8_t endpoint) {
  uint16_t mired = 0;
  EmberAfStatus status;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_COLOR_CONTROL_CLUSTER_ID,
                                      ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ATTRIBUTE_ID,
                                      (uint8_t *)&mired,
                                      sizeof(mired));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      mired = 370; // 2703K - Coolest
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x reading color temperature phy max.", status, endpoint);
  }
  return mired;
}

/** \brief Sets the color temp pysical min in the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] colorTemp physical min
 */
void ColorControlServer_SetColorTempPhysMin(uint8_t endpoint, uint16_t colorTempMired) {
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ATTRIBUTE_ID,
                            (uint8_t *)&colorTempMired,
                            ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing physical min color temperature.", status, endpoint);
  }
}

/** \brief Sets the color temp pysical max in the endpoint in the Color Control Cluster
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] colorTemp physical max
 */
void ColorControlServer_SetColorTempMax(uint8_t endpoint, uint16_t colorTempMired) {
  EmberAfStatus status;
  status = emberAfWriteServerAttribute(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ATTRIBUTE_ID,
                            (uint8_t *)&colorTempMired,
                            ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
      TRACE (true, "ColorControlCluster ERR:%x Endpoint: %x writing physical max color temperature.", status, endpoint);
  }
}

/** @brief Color Control Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfColorControlClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                              EmberAfAttributeId attributeId) {
    switch(attributeId) {
#if 0
      case ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID:
      {
        // update the start up color temp value
        uint16_t mired = ColorControlServer_GetColorTemp(endpoint);
        emberAfWriteServerAttribute(endpoint,
                                    ZCL_COLOR_CONTROL_CLUSTER_ID,
                                    ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID,
                                    (uint8_t *)&mired,
                                    ZCL_INT16U_ATTRIBUTE_TYPE);
        break;
      }
#endif
      default:
        break;
    }
}

void ColorControlServer_LevelControlCoupledColorTempChangeCallback(uint8_t endpoint)
{
  // ZCL 5.2.2.1.1 Coupling color temperature to Level Control
  //
  // If the Level Control for Lighting cluster identifier 0x0008 is supported
  // on the same endpoint as the Color Control cluster and color temperature is
  // supported, it is possible to couple changes in the current level to the
  // color temperature.
  //
  // The CoupleColorTempToLevel bit of the Options attribute of the Level
  // Control cluster indicates whether the color temperature is to be linked
  // with the CurrentLevel attribute in the Level Control cluster.
  //
  // If the CoupleColorTempToLevel bit of the Options attribute of the Level
  // Control cluster is equal to 1 and the ColorMode or EnhancedColorMode
  // attribute is set to 0x02 (color temperature) then a change in the
  // CurrentLevel attribute SHALL affect the ColorTemperatureMireds attribute.
  // This relationship is manufacturer specific, with the qualification that
  // the maximum value of the CurrentLevel attribute SHALL correspond to a
  // ColorTemperatureMired attribute value equal to the
  // CoupleColorTempToLevelMinMireds attribute. This relationship is one-way so
  // a change to the ColorTemperatureMireds attribute SHALL NOT have any effect
  // on the CurrentLevel attribute.
  //
  // In order to simulate the behavior of an incandescent bulb, a low value of
  // the CurrentLevel attribute SHALL be associated with a high value of the
  // ColorTemperatureMireds attribute (i.e., a low value of color temperature
  // in kelvins).
  //
  // If the CoupleColorTempToLevel bit of the Options attribute of the Level
  // Control cluster is equal to 0, there SHALL be no link between color
  // temperature and current level.

  if (!emberAfContainsServer(endpoint, ZCL_COLOR_CONTROL_CLUSTER_ID)) {
    return;
  }

  if (readColorMode(endpoint) == COLOR_MODE_TEMPERATURE) {
    uint16_t tempCoupleMin = readColorTemperatureCoupleToLevelMin(endpoint);
    uint16_t tempPhysMax = ColorControlServer_GetColorTempPhysMax(endpoint);
    uint8_t currentLevel = readLevelControlCurrentLevel(endpoint);

    // Scale color temp setting between the coupling min and the physical max.
    // Note that mireds varies inversely with level: low level -> high mireds.
    // Peg min/MAX level to MAX/min mireds, otherwise interpolate.
    uint16_t newColorTemp;
    if (currentLevel <= LEVEL_CONTROL_CLUSTER_MIN_LEVEL) {
      newColorTemp = tempPhysMax;
    } else if (currentLevel >= LEVEL_CONTROL_CLUSTER_MAX_LEVEL) {
      newColorTemp = tempCoupleMin;
    } else {
      uint32_t tempDelta = (((uint32_t)tempPhysMax - (uint32_t)tempCoupleMin)
                            * currentLevel)
                           / (uint32_t)(LEVEL_CONTROL_CLUSTER_MAX_LEVEL - LEVEL_CONTROL_CLUSTER_MIN_LEVEL + 1);
      newColorTemp = (uint16_t)((uint32_t)tempPhysMax - tempDelta);
    }

    // Apply new color temp.
    ColorControlServerMoveToColorTemp(endpoint, newColorTemp, 0);
  }
}

bool emberAfColorControlClusterStepColorTemperatueCallback(uint8_t stepMode,
                                                           uint16_t stepSize,
                                                           uint16_t transitionTime,
                                                           uint16_t colorTemperatureMinimum,
                                                           uint16_t colorTemperatureMaximum)
{
	return false;
}

void emberAfColorControlClusterServerInitCallback(uint8_t endpoint)
{
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_SERVER
  // 07-5123-07 (i.e. ZCL 7) 5.2.2.2.1.22 StartUpColorTemperatureMireds Attribute
  // The StartUpColorTemperatureMireds attribute SHALL define the desired startup color
  // temperature values a lamp SHAL use when it is supplied with power and this value SHALL
  // be reflected in the ColorTemperatureMireds attribute. In addition, the ColorMode and
  // EnhancedColorMode attributes SHALL be set to 0x02 (color temperature). The values of
  // the StartUpColorTemperatureMireds attribute are listed in the table below.
  // Value                Action on power up
  // 0x0000-0xffef        Set the ColorTemperatureMireds attribute to this value.
  // 0xffff               Set the ColorTemperatureMireds attribue to its previous value.

  // Initialize startUpColorTempMireds to "maintain previous value" value 0xFFFF
  uint16_t startUpColorTemp = 0xFFFF;
  EmberAfStatus status = emberAfReadAttribute(endpoint,
                                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                                              ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID,
                                              CLUSTER_MASK_SERVER,
                                              (uint8_t *)&startUpColorTemp,
                                              sizeof(startUpColorTemp),
                                              NULL);
  if (status == EMBER_ZCL_STATUS_SUCCESS) {
    uint16_t updatedColorTemp = COLOR_CONTROL_CLUSTER_MAX_COLOR_TEMP;
    status = emberAfReadAttribute(endpoint,
                                  ZCL_COLOR_CONTROL_CLUSTER_ID,
                                  ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                  CLUSTER_MASK_SERVER,
                                  (uint8_t *)&updatedColorTemp,
                                  sizeof(updatedColorTemp),
                                  NULL);
    if (status == EMBER_ZCL_STATUS_SUCCESS) {
      uint16_t tempPhysicalMin = ColorControlServer_GetColorTempPhysMin(endpoint);
      uint16_t tempPhysicalMax = ColorControlServer_GetColorTempPhysMax(endpoint);
      if (tempPhysicalMin <= startUpColorTemp && startUpColorTemp <= tempPhysicalMax) {
        // Apply valid startup color temp value that is within physical limits of device.
        // Otherwise, the startup value is outside the device's supported range, and the
        // existing setting of ColorTemp attribute will be left unchanged (i.e., treated as
        // if startup color temp was set to 0xFFFF).
        updatedColorTemp = startUpColorTemp;
        status = emberAfWriteAttribute(endpoint,
                                       ZCL_COLOR_CONTROL_CLUSTER_ID,
                                       ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                       CLUSTER_MASK_SERVER,
                                       (uint8_t *)&updatedColorTemp,
                                       ZCL_INT16U_ATTRIBUTE_TYPE);
        if (status == EMBER_ZCL_STATUS_SUCCESS) {
          // Set ColorMode attributes to reflect ColorTemperature.
          uint8_t updateColorMode = EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE;
          status = emberAfWriteAttribute(endpoint,
                                         ZCL_COLOR_CONTROL_CLUSTER_ID,
                                         ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID,
                                         CLUSTER_MASK_SERVER,
                                         &updateColorMode,
                                         ZCL_ENUM8_ATTRIBUTE_TYPE);
          updateColorMode = EMBER_ZCL_ENHANCED_COLOR_MODE_COLOR_TEMPERATURE;
          status = emberAfWriteAttribute(endpoint,
                                         ZCL_COLOR_CONTROL_CLUSTER_ID,
                                         ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_ATTRIBUTE_ID,
                                         CLUSTER_MASK_SERVER,
                                         &updateColorMode,
                                         ZCL_ENUM8_ATTRIBUTE_TYPE);
        }
      }
    }
  }
#endif
}

/// @}
