//------------------------------------------------------------------------------
///   \file   LevelControlServer.c
///   \since  2016-11-11
///   \brief  Definition of callback-functions from LevelControl-cluster
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup LevelControlClusterServer Level-Control Cluster Server
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
#include "ZigbeeServices/LevelControlServices.h"
#include "ZigbeeServices/BallastConfigurationServices.h"
#include "ZigbeeServices/ColorControlServices.h"
#include "app/framework/plugin/osramds-identify-server/IdentifyServer.h"
//#include "IdentifyServer.h"

// --------- Macro Definitions -------------------------------------------------

#define LEVEL_CONTROL_CLUSTER_MIN_LEVEL 1
#define LEVEL_CONTROL_CLUSTER_MAX_LEVEL 254

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfLevelControlClusterPrintln(__VA_ARGS__);}

#define INVALID_STORED_LEVEL 0xFFFF

// --------- Local Variables ---------------------------------------------------

typedef struct {
  uint8_t commandId;
  uint8_t moveToLevel;
  bool increasing;
  bool useOnLevel;
  uint8_t onLevel;
  uint16_t storedLevel;
  uint32_t eventDurationMs;
  uint32_t transitionTimeMs;
  uint32_t elapsedTimeMs;
} TypeLevelControlState;

static TypeLevelControlState stateTable[EMBER_AF_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

// --------- Local Function Prototypes -----------------------------------------

static TypeLevelControlState *LevelControlServerGetState(uint8_t endpoint);
static void LevelControlServerSetOnOffValue(uint8_t endpoint, bool onOff);
static bool LevelControlServerGetOnOffValue(uint8_t endpoint);
//LevelControlCommand Implementations
       EmberAfStatus LevelControlServerMoveToLevel(uint8_t endpoint, uint8_t level, uint16_t transitionTime, bool onOff);
static EmberAfStatus LevelControlServerStep (uint8_t stepMode, uint8_t stepSize, uint16_t transitionTime, bool onOff);
static EmberAfStatus LevelControlServerMove (uint8_t moveMode, uint8_t rate, bool onOff);

static void LevelControlServerStartFadeTime(uint8_t endpoint, uint8_t level, uint16_t time);
static void LevelControlServerStartFadeRate(uint8_t endpoint, int16_t fadeRate, bool switchOff);

#if defined(ZCL_USING_LEVEL_CONTROL_CLUSTER_OPTIONS_ATTRIBUTE) \
  && defined(ZCL_USING_COLOR_CONTROL_CLUSTER_SERVER)
static void reallyUpdateCoupledColorTemp(uint8_t endpoint);
#define updateCoupledColorTemp(endpoint) reallyUpdateCoupledColorTemp(endpoint)
#else
#define updateCoupledColorTemp(endpoint)
#endif // LEVEL...OPTIONS_ATTRIBUTE && COLOR...SERVER_TEMP

// --------- Local Functions ---------------------------------------------------

static void schedule(uint8_t endpoint, uint32_t delayMs)
{
  emberAfScheduleServerTickExtended(endpoint,
                                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                    delayMs,
                                    EMBER_AF_LONG_POLL,
                                    EMBER_AF_OK_TO_SLEEP);
}

#if 1	// ENCELIUM: options is ignored in an Encelium network.  For interoperability with HA networks,
		// consider re-instating the circumvented logic.
static bool shouldExecuteIfOff(uint8_t endpoint,
							   uint8_t commandId,
                               uint8_t optionMask,
                               uint8_t optionOverride)
{
	return true;
}
#else
static bool shouldExecuteIfOff(uint8_t endpoint,
                               uint8_t commandId,
                               uint8_t optionMask,
                               uint8_t optionOverride)
{
#ifndef ZCL_USING_LEVEL_CONTROL_CLUSTER_OPTIONS_ATTRIBUTE
  return true;
#endif
  // From 3.10.2.2.8.1 of ZCL7 document 14-0127-20j-zcl-ch-3-general.docx:
  //   "Command execution SHALL NOT continue beyond the Options processing if
  //    all of these criteria are true:
  //      - The command is one of the ‘without On/Off’ commands: Move, Move to
  //        Level, Stop, or Step.
  //      - The On/Off cluster exists on the same endpoint as this cluster.
  //      - The OnOff attribute of the On/Off cluster, on this endpoint, is 0x00
  //        (FALSE).
  //      - The value of the ExecuteIfOff bit is 0."
  if (commandId > ZCL_STOP_COMMAND_ID) {
    return true;
  }

  if (!emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) {
    return true;
  }

  uint8_t options;
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                                    ZCL_OPTIONS_ATTRIBUTE_ID,
                                                    &options,
                                                    sizeof(options));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("Unable to read Options attribute: 0x%X",
                                      status);
    // If we can't read the attribute, then we should just assume that it has its
    // default value.
    options = 0x00;
  }

  bool on;
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_ON_OFF_CLUSTER_ID,
                                      ZCL_ON_OFF_ATTRIBUTE_ID,
                                      (uint8_t *)&on,
                                      sizeof(on));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("Unable to read OnOff attribute: 0x%X",
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
  // in section 3.10.2.2.3.

  // ---------- The following order is important in decission making -------
  // -----------more readable ----------
  //
  if (optionMask == 0xFF && optionOverride == 0xFF) {
    // 0xFF are the default values passed to the command handler when
    // the payload is not present - in that case there is use of option
    // attribute to decide execution of the command
    return READBITS(options, EMBER_ZCL_LEVEL_CONTROL_OPTIONS_EXECUTE_IF_OFF);
  }
  // ---------- The above is to distinguish if the payload is present or not

  if (READBITS(optionMask, EMBER_ZCL_LEVEL_CONTROL_OPTIONS_EXECUTE_IF_OFF)) {
    // Mask is present and set in the command payload, this indicates
    // use the over ride as temporary option
    return READBITS(optionOverride, EMBER_ZCL_LEVEL_CONTROL_OPTIONS_EXECUTE_IF_OFF);
  }
  // if we are here - use the option bits
  return (READBITS(options, EMBER_ZCL_LEVEL_CONTROL_OPTIONS_EXECUTE_IF_OFF));
}
#endif

static void writeRemainingTime(uint8_t endpoint, uint16_t remainingTimeMs)
{
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE
  // Convert milliseconds to tenths of a second, rounding any fractional value
  // up to the nearest whole value.  This means:
  //
  //   0 ms = 0.00 ds = 0 ds
  //   1 ms = 0.01 ds = 1 ds
  //   ...
  //   100 ms = 1.00 ds = 1 ds
  //   101 ms = 1.01 ds = 2 ds
  //   ...
  //   200 ms = 2.00 ds = 2 ds
  //   201 ms = 2.01 ds = 3 ds
  //   ...
  //
  // This is done to ensure that the attribute, in tenths of a second, only
  // goes to zero when the remaining time in milliseconds is actually zero.
  uint16_t remainingTimeDs = (remainingTimeMs + 99) / 100;
  EmberStatus status = emberAfWriteServerAttribute(endpoint,
                                                   ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                                   ZCL_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                                   (uint8_t *)&remainingTimeDs,
                                                   sizeof(remainingTimeDs));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: writing remaining time %x", status);
  }
#endif
}

#if defined(ZCL_USING_LEVEL_CONTROL_CLUSTER_OPTIONS_ATTRIBUTE) \
  && defined(ZCL_USING_COLOR_CONTROL_CLUSTER_SERVER)
static void reallyUpdateCoupledColorTemp(uint8_t endpoint)
{
  uint8_t options;
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                                    ZCL_OPTIONS_ATTRIBUTE_ID,
                                                    &options,
                                                    sizeof(options));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("Unable to read Options attribute: 0x%X",
                                      status);
    return;
  }

  if (READBITS(options, EMBER_ZCL_LEVEL_CONTROL_OPTIONS_COUPLE_COLOR_TEMP_TO_LEVEL)) {
    ColorControlServer_LevelControlCoupledColorTempChangeCallback(endpoint);
  }
}
#endif // LEVEL...OPTIONS_ATTRIBUTE && COLOR...SERVER_TEMP

static void deactivate(uint8_t endpoint)
{
  emberAfDeactivateServerTick(endpoint, ZCL_LEVEL_CONTROL_CLUSTER_ID);
}

/** \brief Get a pointer to the cluster control structure for that endpint
 *
 * \param[in] endpoint
 */
static TypeLevelControlState *LevelControlServerGetState(uint8_t endpoint)
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint,
                                                   ZCL_LEVEL_CONTROL_CLUSTER_ID);
  return (ep == 0xFF ? NULL : &stateTable[ep]);
}

/** \brief Set OnOff state
 *  Set the OnOff state of the OnOff cluster of this endpoint
 *
 * \param[in] endpoint which should be set on/off
 * \param[in] onOff state to set
 */
static void LevelControlServerSetOnOffValue(uint8_t endpoint, bool on)
{
  EmberAfStatus status;
  if (emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) {
    TRACE(true, "Setting on/off to %p due to level change", on ? "ON" : "OFF");
    status = emberAfOnOffClusterSetValueCallback(endpoint,
                                        (on ? ZCL_ON_COMMAND_ID : ZCL_OFF_COMMAND_ID),
                                        true);
    TRACE((status != EMBER_ZCL_STATUS_SUCCESS), "ERR:%x setting  OnOff state", status);
  }
}

/** \brief Get OnOff state
 *  Get the OnOff state of the OnOff cluster of this endpoint
 *
 * \param[in] endpoint which should be set on/off
 * \return current on state of the onOff-cluster
 */
static bool LevelControlServerGetOnOffValue(uint8_t endpoint)
{
  EmberAfStatus status;
  uint8_t onOff = true;                                             //seems easier To have a default state than an Error Handling
  if (emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) {

    status = emberAfReadServerAttribute(endpoint,
                                        ZCL_ON_OFF_CLUSTER_ID,
                                        ZCL_ON_OFF_ATTRIBUTE_ID,
                                        (uint8_t *)&onOff,
                                        sizeof(onOff));
    TRACE((status == EMBER_ZCL_STATUS_SUCCESS), "Getting on/off with %p", onOff ? "ON" : "OFF");
  }
  return onOff;
}

/** \brief Limit output Level
 *
 * \param[in] target level
 * \return    level limited to range
 */
uint8_t LevelControlServerLimitLevel(int16_t level, bool onOff) {
  uint8_t limitedLevel = level;
  if (level >= LEVEL_CONTROL_CLUSTER_MAX_LEVEL) {
    limitedLevel = LEVEL_CONTROL_CLUSTER_MAX_LEVEL;
  }
  else if (level == 0) {
      limitedLevel = onOff ? 0 : LEVEL_CONTROL_CLUSTER_MIN_LEVEL;
  }
  else {
    if (level <= LEVEL_CONTROL_CLUSTER_MIN_LEVEL) {
      limitedLevel = onOff ? 0 : LEVEL_CONTROL_CLUSTER_MIN_LEVEL;
    }
  }
  return limitedLevel;
}

/** \brief Generic MoveToLevel implementation
 *  Shared functionality MovToLevel and MoveToLevelWithOnOff
 *
 * \param[in] level   target level to move to
 * \param[in] transitionTime   in 100ms units
 * \param[in] onOff (true=with onOff, false=no change of OnOff)
 * \return    true if processed without faults
 */
EmberAfStatus LevelControlServerMoveToLevel(uint8_t endpoint, uint8_t level, uint16_t transitionTime, bool onOff) {
  TypeLevelControlState *state = LevelControlServerGetState(endpoint);
  EmberAfStatus status;
  uint8_t currentLevel;

  if (NULL == state) return EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND;

  deactivate(endpoint);

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return status;
  }

  level = LevelControlServerLimitLevel(level, onOff);

  state->commandId = onOff ? ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID : ZCL_MOVE_TO_LEVEL_COMMAND_ID;
  state->moveToLevel = level == 0 ? LEVEL_CONTROL_CLUSTER_MIN_LEVEL : level; // level should not be less than min
  if (onOff || LevelControlServerGetOnOffValue(endpoint)) {
    LevelControlServerSetOnOffValue(endpoint, (level >= LEVEL_CONTROL_CLUSTER_MIN_LEVEL)); // on if (level >= min); off if (level < min)
    if(currentLevel == state->moveToLevel) {
      LevelControlServerStartFadeTime(endpoint, level, transitionTime);
      return EMBER_ZCL_STATUS_SUCCESS;
    }
  }
  state->increasing = currentLevel <= state->moveToLevel ? true : false;

  // If the Transition time field takes the value 0xFFFF, then the time taken
  // to move to the new level shall instead be determined by the On/Off
  // Transition Time attribute.  If On/Off Transition Time, which is an
  // optional attribute, is not present, the device shall move to its new level
  // as fast as it is able.
  if (transitionTime == 0xFFFF) {
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    status = emberAfReadServerAttribute(endpoint,
                                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                        ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
                                        (uint8_t *)&transitionTime,
                                        sizeof(transitionTime));
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfLevelControlClusterPrintln("ERR: reading on/off transition time %x",
                                        status);
      return status;
    }

    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    state->transitionTimeMs = (transitionTime
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
#else //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    // If the Transition Time field is 0xFFFF and On/Off Transition Time,
    // which is an optional attribute, is not present, the device shall move to
    // its new level as fast as it is able.
    state->transitionTimeMs = 0;
    transitionTime = 0;
#endif //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
  }
  else {
    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    state->transitionTimeMs = (transitionTime
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
  }

  if(onOff || LevelControlServerGetOnOffValue(endpoint)) {
    LevelControlServerStartFadeTime(endpoint, level, transitionTime);
  }

  // The duration between events will be the transition time divided by the
  // distance we must move.
  state->eventDurationMs = state->transitionTimeMs / abs(state->moveToLevel - currentLevel);
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Move commands.
  state->useOnLevel = false;

  state->storedLevel = INVALID_STORED_LEVEL;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
  return EMBER_ZCL_STATUS_SUCCESS;
}


/** \brief Generic Step implementation
 *  Shared functionality Step and StepWithOnOff
 *
 * \param[in] stepMode  0=up; 1=down
 * \param[in] stepSize  units of the step
 * \param[in] transitionTime   in 100ms units
 * \param[in] onOff (true=with onOff, false=no change of OnOff)
  * \return    true if processed without faults
 */
static EmberAfStatus LevelControlServerStep (uint8_t stepMode, uint8_t stepSize, uint16_t transitionTime, bool onOff) {
  uint8_t currentLevel;
  int16_t calculatedTargetLevel;
  EmberAfStatus status;
  uint8_t endpoint = emberAfCurrentEndpoint();
  TypeLevelControlState *state = LevelControlServerGetState(endpoint);

  status = emberAfReadServerAttribute(endpoint,
                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                    ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                    (uint8_t *)&currentLevel,
                    sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    return false;
  }
  calculatedTargetLevel = currentLevel;
  if (stepMode == 0x00) {      // up
    calculatedTargetLevel += stepSize;
    state->increasing = true;
  }
  else if (stepMode == 0x01) { // down
    calculatedTargetLevel -= stepSize;
    state->increasing = false;
  }
  else {
    return EMBER_ZCL_STATUS_INVALID_VALUE; // invalid stepMode
  }
  calculatedTargetLevel = LevelControlServerLimitLevel(calculatedTargetLevel, onOff);

  if (onOff || LevelControlServerGetOnOffValue(endpoint)) {
    LevelControlServerSetOnOffValue(endpoint, (calculatedTargetLevel >= LEVEL_CONTROL_CLUSTER_MIN_LEVEL)); // on if (level >= min); off if (level < min)
  }
  state->commandId = onOff ? ZCL_STEP_WITH_ON_OFF_COMMAND_ID : ZCL_STEP_COMMAND_ID;
  state->moveToLevel = calculatedTargetLevel == 0 ? LEVEL_CONTROL_CLUSTER_MIN_LEVEL : calculatedTargetLevel; // level should not be less than min
  if(currentLevel == state->moveToLevel) {
    return EMBER_ZCL_STATUS_SUCCESS;
  }

  // If the Transition Time field is 0xFFFF, the device should move as fast as
  // it is able.
  if (transitionTime == 0xFFFF) {
    state->transitionTimeMs = 0;
    transitionTime = 0;
  }
  else {
    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    state->transitionTimeMs = (transitionTime
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
  }

  LevelControlServerStartFadeTime(endpoint, (uint8_t)calculatedTargetLevel, transitionTime);

  // The duration between events will be the transition time divided by the
  // distance we must move.
  state->eventDurationMs = state->transitionTimeMs / abs(state->moveToLevel - currentLevel);
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Move commands.
  state->useOnLevel = false;

  state->storedLevel = INVALID_STORED_LEVEL;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
  return EMBER_ZCL_STATUS_SUCCESS;
}

/** \brief Generic Move implementation
 *  Shared functionality Move and MoveWithOnOff
 *
 * \param[in] moveMode  direction of move (0 = Up, 1 = down)
 * \param[in] rate  movement in units per second
 * \param[in] onOff (true=with onOff, false=no change of OnOff)
  * \return    true if processed without faults
 */
static EmberAfStatus LevelControlServerMove (uint8_t moveMode, uint8_t rate, bool onOff) {
  uint8_t targetLevel;
  int16_t signedRate;
  EmberAfStatus status;
  uint8_t endpoint = emberAfCurrentEndpoint();
  bool onOffClusterIsOn = LevelControlServerGetOnOffValue(endpoint);
  TypeLevelControlState *state = LevelControlServerGetState(endpoint);
  uint8_t currentLevel;

  deactivate(endpoint);
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&currentLevel,
                                      sizeof(currentLevel));
  if(status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return status;
  }

  if (rate == 0) {
    LevelControlServices_StopFade(endpoint);
    return EMBER_ZCL_STATUS_SUCCESS;        // a rate of 0 means no chance, nothing to do
  }
  signedRate = rate;
  if (moveMode == 0x00) {       // up
    targetLevel = LEVEL_CONTROL_CLUSTER_MAX_LEVEL;
    state->increasing = true;
    if (onOff && !onOffClusterIsOn ) {        // With OnOff and actual off
      LevelControlServerSetOnOffValue(endpoint, true);
    }
  }
  else if (moveMode == 0x01) {  // down
    state->increasing = false;
    signedRate = -signedRate;
    if (onOff) {
      targetLevel = 0;
    }
    else {
      targetLevel = LEVEL_CONTROL_CLUSTER_MIN_LEVEL;
    }
  }
  else {
    return EMBER_ZCL_STATUS_INVALID_VALUE; // invalid stepMode
  }
  if (LevelControlServerGetOnOffValue(endpoint)) {
    LevelControlServerStartFadeRate(endpoint, signedRate, onOff);
    state->commandId = onOff ? ZCL_MOVE_WITH_ON_OFF_COMMAND_ID : ZCL_MOVE_COMMAND_ID;
#if 0
    state->storedLevel = INVALID_STORED_LEVEL;
    emberAfReadServerAttribute(endpoint,
                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                        ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                        (uint8_t *)&state->storedLevel,
                        sizeof(uint8_t));
    state->rate = signedRate;
#endif
    state->moveToLevel = targetLevel;
  }
#if 1
  // If the Rate field is 0xFF, the device should move as fast as it is able.
  // Otherwise, the rate is in units per second.
  if (rate == 0xFF) {
    state->eventDurationMs = 0;
  } else {
    state->eventDurationMs = MILLISECOND_TICKS_PER_SECOND / rate;
  }
  state->transitionTimeMs = abs(targetLevel - currentLevel) * state->eventDurationMs;
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Move commands.
  state->useOnLevel = false;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
#else
  // hack we just set the current level to the target level
  emberAfWriteServerAttribute(endpoint,
                                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                         ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                         (uint8_t *)&targetLevel,
                                         ZCL_INT8U_ATTRIBUTE_TYPE);
#endif
  return EMBER_ZCL_STATUS_SUCCESS;
}

static EmberAfStatus LevelControlServerStop(bool onOff) {
  uint8_t endpoint = emberAfCurrentEndpoint();
  TypeLevelControlState *state = LevelControlServerGetState(endpoint);
  EmberAfStatus status;

  if (state == NULL) {
      status = EMBER_ZCL_STATUS_FAILURE;
  }
  else {
    deactivate(endpoint);
    writeRemainingTime(endpoint, 0);
    LevelControlServices_StopFade(endpoint);
    status = EMBER_ZCL_STATUS_SUCCESS;
  }

  return status;
}

/** \brief Fade in 1 s to current output level
 *  if OnOff cluster is off target level is 0, instead its the current level attribute
 *
 *
 * \param[in] endpoint  index of the zigbee endpoint
 */
void LevelControlServerFadeToCurrentOutputLevel(uint8_t endpoint) {
  uint8_t currentLevel = 254;
  EmberAfStatus status;

  status = emberAfReadServerAttribute(endpoint,
                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                    ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                    (uint8_t *)&currentLevel,
                    sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    return; // endpoint seems not to have a LevelControlCluster
  }

  if(!LevelControlServerGetOnOffValue(endpoint)) {
    currentLevel = 0;
  }
  emberAfLevelControlClusterPrintln("LevelControlCluster FadeToCurrentOutputLevel ep:%u level:%u", endpoint, currentLevel);
  LevelControlServerStartFadeTime(endpoint, currentLevel, 10);
}

/**
 *  \brief Start a fade of the output
 *  Starts a fade to a given target level in a given time
 *
 *
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] level     target level in linear format (zigbee)
 * \param[in] time      fade time in 100ms
 *
 */
void LevelControlServerStartFadeTime(uint8_t endpoint, uint8_t level, uint16_t time) {

  emberAfPluginOsramdsIdentifyServerCancel(endpoint);  // a level command should stop identification
#ifdef ZCL_USING_BALLAST_CONFIGURATION_CLUSTER_LINEARIZATION_VALUE_ATTRIBUTE
  level = BallastConfigurationServer_LinearizeLevel(endpoint, level);
#else
  level = LevelControlServices_CalculateLevel(level);
#endif
  emberAfLevelControlClusterPrintln("LevelControlCluster StartFadeTime ep:%u level:%u time:%u", endpoint, level, time);
  LevelControlServices_StartFadeTime(endpoint, level, time);
}

/**
 *  \brief Start a fade of the output
 *  Starts a fade with a give fade speed
 *
 *
 * \param[in] endpoint     index of the zigbee endpoint
 * \param[in] fadeRate     fade rate in units per s, (> 0 dim up; < 0 dim down)
 * \param[in] switchOff    Allow switching off if level goes beyond min level
 *
 */
void LevelControlServerStartFadeRate(uint8_t endpoint, int16_t fadeRate, bool switchOff) {

  emberAfPluginOsramdsIdentifyServerCancel(endpoint);  // a level command should stpo identification
  emberAfLevelControlClusterPrintln("LevelControlCluster StartFadeRate ep:%u rate:%d off:%u", endpoint, fadeRate, switchOff);
  LevelControlServices_StartFadeRate(endpoint, fadeRate, switchOff);
}

//---------- Public functions --------------------------------------------------

/**
 *  \brief Refresh Output level of cluster
 *  Set output to a certail value
 *
 * \param[in] endpoint  index of the zigbee endpoint
 */
void LevelControlServer_SetOutput(uint8_t endpoint, uint8_t levelIn, uint16_t fadeTime100ms) {
  uint8_t levelOut;
  emberAfLevelControlClusterPrintln("LevelControlServer_SetOutput StartFadeRate ep:%u level:%d time:%u", endpoint, levelIn, fadeTime100ms);
  if (endpoint == 0xFF) {
    for (int i = 0; i < emberAfEndpointCount(); i++) {
#ifdef ZCL_USING_BALLAST_CONFIGURATION_CLUSTER_LINEARIZATION_VALUE_ATTRIBUTE
      levelOut = BallastConfigurationServer_LinearizeLevel(emAfEndpoints[i].endpoint, levelIn);
#else
      levelOut = levelIn;
#endif
      LevelControlServices_StartFadeTime(emAfEndpoints[i].endpoint, levelOut, fadeTime100ms);
    }
  }
  else {
#ifdef ZCL_USING_BALLAST_CONFIGURATION_CLUSTER_LINEARIZATION_VALUE_ATTRIBUTE
    levelOut = BallastConfigurationServer_LinearizeLevel(endpoint, levelIn);
#else
    levelOut = levelIn;
#endif
    LevelControlServices_StartFadeTime(endpoint, levelOut, fadeTime100ms);
  }
}

/**
 *  \brief Refresh Output level of cluster
 *
 *
 * \param[in] endpoint  index of the zigbee endpoint
 */
void LevelControlServer_RefreshCurrentOutputLevel(uint8_t endpoint) {
  if (endpoint == 0xFF) {
    for (int i = 0; i < emberAfEndpointCount(); i++) {
      LevelControlServerFadeToCurrentOutputLevel(emAfEndpoints[i].endpoint);
    }
  }
  else {
    LevelControlServerFadeToCurrentOutputLevel(endpoint);
  }
}

/**
 *  \brief Level Control Cluster Init Callback
 *
 * \param[in] endpoint  index of the zigbee endpoint
 * \param[in] level  actual Level from output device
 */
void LevelControlServer_ActualLevelCallback(uint8_t endpoint, uint8_t level) {
  emberAfWriteServerAttribute(endpoint,
                               ZCL_LEVEL_CONTROL_CLUSTER_ID,
                               ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                               (uint8_t *)&level,
                               ZCL_INT8U_ATTRIBUTE_TYPE);
}

//---------- ember Stack Callbacks----------------------------------------------

/**
 *  Cluster initialisation
 *  Sets the Current Level to the BallastConfiguration PowerOnLevel
 *
 * \param[in] endpoint  index of the zigbee endpoint
 */
void emberAfLevelControlClusterServerInitCallback(uint8_t endpoint)
{
  uint8_t powerOnLevel = BallastConfigurationServer_GetPoweronLevel(endpoint);
  emberAfWriteServerAttribute(endpoint,
                               ZCL_LEVEL_CONTROL_CLUSTER_ID,
                               ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                               (uint8_t *)&powerOnLevel,
                               ZCL_INT8U_ATTRIBUTE_TYPE);
  LevelControlServerSetOnOffValue(endpoint, (powerOnLevel > 0));
}

#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)
/**
 * On receipt of this command, a device SHALL move from its current level in
 * an up or down direction in a continuous fashion, OnOff attribute of the On/Off
 * cluster is on.
 *
 * Up:
 * Increase the output level at the rate given in the Rate field.
 * Down:
 * Decrease the output level at the rate given in the Rate field.
 *
 *
 * \param[in] level   target level to move to
 * \param[in] transitionTime   in 100ms units
  * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveToLevelCallback(uint8_t level,
                                                   uint16_t transitionTime)
{
  EmberAfStatus status;

  TRACE (true, "LevelControlCluster MoveToLevel ep:%u level:%u time:%u", emberAfCurrentEndpoint(), level, transitionTime);
  status = LevelControlServerMoveToLevel(emberAfCurrentEndpoint(), level, transitionTime, false);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#else
/**
 * On receipt of this command, a device SHALL move from its current level in
 * an up or down direction in a continuous fashion, OnOff attribute of the On/Off
 * cluster is on.
 *
 * Up:
 * Increase the output level at the rate given in the Rate field.
 * Down:
 * Decrease the output level at the rate given in the Rate field.
 *
 *
 * \param[in] level   target level to move to
 * \param[in] transitionTime   in 100ms units
  * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveToLevelCallback(uint8_t level,
                                                   uint16_t transitionTime,
                                                   uint8_t optionMask,
                                                   uint8_t optionOverride)
{
  EmberAfStatus status;

  TRACE (true, "LevelControlCluster MoveToLevel ep:%u level:%u time:%u", emberAfCurrentEndpoint(), level, transitionTime);
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), ZCL_MOVE_TO_LEVEL_COMMAND_ID, optionMask, optionOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = LevelControlServerMoveToLevel(emberAfCurrentEndpoint(), level, transitionTime, false);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;
}
#endif

/**
 * On receipt of this command, a device SHALL move from its current level in
 * an up or down direction in a continuous fashion
 * Up:
 * Increase the output level at the rate given in the Rate field.
 * Before increasing CurrentLevel, the OnOff attribute of the On/Off cluster
 * on the same endpoint, if implemented, SHALL be set to On.
 * If the level reaches the maximum allowed for the device, stop.
 * Down:
 * Decrease the output level at the rate given in the Rate field. If
 * the level reaches the minimum allowed for the device, the OnOff attribute
 * of the On/Off cluster on the same endpoint, if implemented, SHALL be set
 * to Off..
 *
 * \param[in] level   target level to move to
 * \param[in] transitionTime   in 100ms units
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveToLevelWithOnOffCallback(uint8_t level,
                                                               uint16_t transitionTime)
{
  EmberAfStatus status;

  TRACE (true, "LevelControlCluster MoveToLevelOnOff ep:%u level:%u time:%u", emberAfCurrentEndpoint(), level, transitionTime);
  status = LevelControlServerMoveToLevel(emberAfCurrentEndpoint(), level, transitionTime, true);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)
/**
 * On receipt of this command, a device SHALL move from its current level in an
 * up or down direction in a continuous fashion
 *
 * Up:
 * Increase the output level at the rate given in the Rate field. If
 * the level reaches the maximum allowed for the device, stop.
 * Down:
 * Decrease the output level at the rate given in the Rate field. If
 * the level reaches the minimum allowed for the device, stop
 *
 * \param[in] moveMode  direction of move (0 = Up, 1 = down)
 * \param[in] rate  movement in units per second
  * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveCallback(uint8_t moveMode,
                                            uint8_t rate)
{
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Move ep:%u mode:%u rate:%u", emberAfCurrentEndpoint(), moveMode, rate);
  status = LevelControlServerMove(moveMode, rate, false);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#else
/**
 * On receipt of this command, a device SHALL move from its current level in an
 * up or down direction in a continuous fashion
 *
 * Up:
 * Increase the output level at the rate given in the Rate field. If
 * the level reaches the maximum allowed for the device, stop.
 * Down:
 * Decrease the output level at the rate given in the Rate field. If
 * the level reaches the minimum allowed for the device, stop
 *
 * \param[in] moveMode  direction of move (0 = Up, 1 = down)
 * \param[in] rate  movement in units per second
  * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveCallback(uint8_t moveMode,
                                            uint8_t rate,
                                            uint8_t optionMask,
                                            uint8_t optionOverride)
{
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Move ep:%u mode:%u rate:%u", emberAfCurrentEndpoint(), moveMode, rate);
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), ZCL_MOVE_COMMAND_ID, optionMask, optionOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = LevelControlServerMove(moveMode, rate, false);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;
}
#endif

/**
 * On receipt of this command, a device SHALL move from its current level in
 * an up or down direction in a continuous fashion
 * Up:
 * Increase the device's level at the rate given in the Rate field.
 * Before increasing CurrentLevel, the OnOff attribute of the On/Off cluster
 * on the same endpoint, if implemented, SHALL be set to On.
 * If the level reaches the maximum allowed for the device, stop.
 * Down:
 * Decrease the device's level at the rate given in the Rate field. If
 * the level reaches the minimum allowed for the device, the OnOff attribute
 * of the On/Off cluster on the same endpoint, if implemented, SHALL be set
 * to Off..
 *
 * \param[in] moveMode  direction of move (0 = Up, 1 = down)
 * \param[in] rate  movement in units per second
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterMoveWithOnOffCallback(uint8_t moveMode, uint8_t rate)
{
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster MoveOnOff ep:%u mode:%u rate:%u", emberAfCurrentEndpoint(), moveMode, rate);
  status = LevelControlServerMove(moveMode, rate, true);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)
/**
 * change the output level from the actual level
 *
 *
 * \param level   Ver.: always
 * \param transitionTime   Ver.: always
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStepCallback(uint8_t stepMode,
                                               uint8_t stepSize,
                                               uint16_t transitionTime) {
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Step ep:%u mode:%u size:%u time:%u", emberAfCurrentEndpoint(), stepMode, stepSize, transitionTime);
  status = LevelControlServerStep( stepMode, stepSize, transitionTime, false);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#else
/**
 * change the output level from the actual level
 *
 *
 * \param level   Ver.: always
 * \param transitionTime   Ver.: always
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStepCallback(uint8_t stepMode,
                                               uint8_t stepSize,
                                               uint16_t transitionTime,
                                               uint8_t optionMask,
                                               uint8_t optionOverride) {
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Step ep:%u mode:%u size:%u time:%u", emberAfCurrentEndpoint(), stepMode, stepSize, transitionTime);
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), ZCL_STEP_COMMAND_ID, optionMask, optionOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = LevelControlServerStep( stepMode, stepSize, transitionTime, false);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;
}
#endif

/**
 * change the output level from the actual level
 *
 * \param[in] stepMode  0=up; 1=down
 * \param[in] stepSize  units of the step
 * \param[in] transitionTime   in 100ms units
  * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStepWithOnOffCallback(uint8_t stepMode,
                                                        uint8_t stepSize,
                                                        uint16_t transitionTime) {
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster StepOnOff ep:%u mode:%u size:%u time:%u", emberAfCurrentEndpoint(), stepMode, stepSize, transitionTime);
  status =  LevelControlServerStep( stepMode, stepSize, transitionTime, true);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#if 0 // ENCELIUM overridden to revert to SDK v1.1 (options parameter dropped)
/**
 *  Stop a running fade
 *
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStopCallback(void) {
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Stop ep:%u", emberAfCurrentEndpoint());
  status = LevelControlServerStop(false);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

#else
/**
 *  Stop a running fade
 *
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStopCallback(uint8_t optionMask,
                                            uint8_t optionOverride) {
  EmberAfStatus status;
  TRACE (true, "LevelControlCluster Stop ep:%u", emberAfCurrentEndpoint());
  if (!shouldExecuteIfOff(emberAfCurrentEndpoint(), ZCL_STOP_COMMAND_ID, optionMask, optionOverride)) {
    status = EMBER_ZCL_STATUS_SUCCESS;
  }
  else {
    status = LevelControlServerStop(false);
  }

  emberAfSendImmediateDefaultResponse(status);
  return true;
}
#endif

/**
 *  Stop a running fade (same as without OnOff)
 *
 * \return command finally processed (true: no action in stack necessary)
 */
bool emberAfLevelControlClusterStopWithOnOffCallback(void) {
  TRACE (true, "LevelControlCluster StopOnOff ep:%u", emberAfCurrentEndpoint());
  EmberAfStatus status= LevelControlServerStop(true);

  emberAfSendImmediateDefaultResponse(status);
  return true;
}

/**
 * This is called by the framework when the on/off cluster initiates a command
 * that must effect a level control change. The implementation assumes that the
 * client will handle any effect on the On/Off Cluster.
 * Follows 07-5123-04 (ZigBee Cluster Library doc), section 3.10.2.1.1.
 * Quotes are from table 3.46.
 *
 * \param[in] endpoint   the endpoint of the device, which is addressed
 * \param[in] newValue   the new on/off state (0=off, 1=on)
 */

void emberAfOnOffClusterLevelControlEffectCallback(int8u endpoint,
                                                   boolean newValue) {
	uint8_t currentLevelCache = LEVEL_CONTROL_CLUSTER_MAX_LEVEL;
	uint16_t currentOnOffTransitionTime = 0xFFFF;
	uint8_t currentOnLevel= 0xFF;
	EmberAfStatus status;
	TypeLevelControlState *state = LevelControlServerGetState(endpoint);

	if (NULL == state) return;

	// "Temporarily store CurrentLevel."
	status = emberAfReadServerAttribute(endpoint,
									  ZCL_LEVEL_CONTROL_CLUSTER_ID,
									  ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
									  (uint8_t *)&currentLevelCache,
									  sizeof(currentLevelCache));
  TRACE ((status != EMBER_ZCL_STATUS_SUCCESS), "ERR:%x reading CurrentLevel for cache", status);

	  // Read the OnLevel attribute.
	#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_LEVEL_ATTRIBUTE
	  status = emberAfReadServerAttribute(endpoint,
	                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
	                                      ZCL_ON_LEVEL_ATTRIBUTE_ID,
	                                      (uint8_t *)&currentOnLevel,
	                                      sizeof(currentOnLevel));
	  TRACE ((status != EMBER_ZCL_STATUS_SUCCESS), "ERR:%x reading OnLevel", status);
	#endif

	  // Read the OnOffTransitionTime attribute.
	#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
	  status = emberAfReadServerAttribute(endpoint,
	                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
	                                      ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
	                                      (uint8_t *)&currentOnOffTransitionTime,
	                                      sizeof(currentOnOffTransitionTime));
    TRACE ((status != EMBER_ZCL_STATUS_SUCCESS), "ERR:%x reading OnOffTransitionTime", status);
	#endif

  currentOnOffTransitionTime = (0xFFFF == currentOnOffTransitionTime) ? 0 : currentOnOffTransitionTime;          // Fade immediate if transition Time is 0xFFFF
  if (newValue) {  // means switch On
    //LevelControlServerStartFadeTime(endpoint, minimumLevelAllowedForTheDevice, 0);                     // "Set CurrentLevel to minimum level allowed for the device."
    // We dont need this step from the Zigbee Plugin because the arcPower then is already off
    if (currentLevelCache != 0)
      currentOnLevel = currentLevelCache;
    if (LEVEL_CONTROL_CLUSTER_MAX_LEVEL < currentOnLevel)
      currentOnLevel = LEVEL_CONTROL_CLUSTER_MAX_LEVEL;
    emberAfLevelControlClusterPrintln("Switch on level: %x", currentOnLevel);
    LevelControlServerStartFadeTime(endpoint, currentOnLevel, currentOnOffTransitionTime);
    state->commandId = ZCL_MOVE_TO_LEVEL_COMMAND_ID;
    state->storedLevel = INVALID_STORED_LEVEL;
    state->moveToLevel = currentOnLevel;
    status = emberAfWriteServerAttribute(endpoint,
                                           ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                           ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                           (uint8_t *)&currentOnLevel,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
    TRACE ((status != EMBER_ZCL_STATUS_SUCCESS), "ERR:%x setting CurrentLevel", status);
  }
  else {         //switch off
    LevelControlServerStartFadeTime(endpoint, 0, currentOnOffTransitionTime);
    state->commandId = ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID;
    state->storedLevel = currentLevelCache;
    state->moveToLevel = 0;

  }
}

void emberAfLevelControlClusterServerTickCallback(uint8_t endpoint)
{
  TypeLevelControlState *state = LevelControlServerGetState(endpoint);
  EmberAfStatus status;
  uint8_t currentLevel;

  if (state == NULL) {
    return;
  }

  state->elapsedTimeMs += state->eventDurationMs;

#if !defined(ZCL_USING_LEVEL_CONTROL_CLUSTER_OPTIONS_ATTRIBUTE) \
  && defined(EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER)
  if (emberAfPluginZllLevelControlServerIgnoreMoveToLevelMoveStepStop(endpoint,
                                                                      state->commandId)) {
    return;
  }
#endif

  // Read the attribute; print error message and return if it can't be read
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (uint8_t *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    writeRemainingTime(endpoint, 0);
    return;
  }

  TRACE(true, "Event: move from %d", currentLevel);

  // adjust by the proper amount, either up or down
  if (state->transitionTimeMs == 0) {
    // Immediate, not over a time interval.
    currentLevel = state->moveToLevel;
  } else if (state->increasing) {
    assert(currentLevel < LEVEL_CONTROL_CLUSTER_MAX_LEVEL);
    assert(currentLevel < state->moveToLevel);
    currentLevel++;
  } else {
    assert(LEVEL_CONTROL_CLUSTER_MIN_LEVEL < currentLevel);
    assert(state->moveToLevel < currentLevel);
    currentLevel--;
  }

  TRACE(true, " to %d ", currentLevel);
  TRACE(true, "(diff %c1)",
                                    state->increasing ? '+' : '-');

  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                       ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                       (uint8_t *)&currentLevel,
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
    writeRemainingTime(endpoint, 0);
    return;
  }

  updateCoupledColorTemp(endpoint);

  // The level has changed, so the scene is no longer valid.
  if (emberAfContainsServer(endpoint, ZCL_SCENES_CLUSTER_ID)) {
    emberAfScenesClusterMakeInvalidCallback(endpoint);
  }

  // Are we at the requested level?
  if (currentLevel == state->moveToLevel) {
    if (state->commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID
        || state->commandId == ZCL_MOVE_WITH_ON_OFF_COMMAND_ID
        || state->commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) {
      LevelControlServerSetOnOffValue(endpoint, (currentLevel != LEVEL_CONTROL_CLUSTER_MIN_LEVEL));
      if (currentLevel == LEVEL_CONTROL_CLUSTER_MIN_LEVEL && state->useOnLevel) {
        status = emberAfWriteServerAttribute(endpoint,
                                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                             (uint8_t *)&state->onLevel,
                                             ZCL_INT8U_ATTRIBUTE_TYPE);
        if (status != EMBER_ZCL_STATUS_SUCCESS) {
          emberAfLevelControlClusterPrintln("ERR: writing current level %x",
                                            status);
        } else {
          updateCoupledColorTemp(endpoint);
        }
      }
    } else {
      if (state->storedLevel != INVALID_STORED_LEVEL) {
        uint8_t storedLevel8u = (uint8_t) state->storedLevel;
        status = emberAfWriteServerAttribute(endpoint,
                                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                             (uint8_t *)&storedLevel8u,
                                             ZCL_INT8U_ATTRIBUTE_TYPE);
        if (status != EMBER_ZCL_STATUS_SUCCESS) {
          emberAfLevelControlClusterPrintln("ERR: writing current level %x",
                                            status);
        } else {
          updateCoupledColorTemp(endpoint);
        }
      }
    }
    writeRemainingTime(endpoint, 0);
  } else {
    writeRemainingTime(endpoint,
                       state->transitionTimeMs - state->elapsedTimeMs);
    schedule(endpoint, state->eventDurationMs);
  }
}
/// @}

