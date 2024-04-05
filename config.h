//------------------------------------------------------------------------------
/// \file config.h
/// \since  2016-12-07
/// \brief target specific definitions
///
//
/// DISCLAIMER:
/// The content of this file is intellectual property of the Osram GmbH. It is
/// confidential and not intended for public release. All rights reserved.
/// Copyright © Osram GmbH
///
//------------------------------------------------------------------------------
#include "GlobalDefs.h"
#include "app/framework/plugin/Osramds-reset2factory/Reset2Factory.h"
#include "Zigbee2Dali.h"
#include "ZigbeeHaLightEndDevice_own.h"

#define APPLICATION_VERSION       0x00
#define MANUFACTURER_NAME         "Osram Sylvania"
#define MODELIDENTIFIER           "NAED 56340-00/00"
#define POWERSOURCE               EMBER_ZCL_POWER_SOURCE_SINGLE_PHASE_MAINS
#define IMAGE_TYPE                0xE01E  // must be unique within the manufacturer
#define FW_VERSION                0x0037  // 0.0.3.7
#define HW_VERSION_MAJOR          0x05   // Reported in Basic Cluster
#define HW_VERSION_MINOR          0x00    // with HW_VERSION_MAJOR used in OTA Cluster
#define ZDO_LEAVE_REJOIN_DELAY    2       // minutes

#if defined(ZIGBEE2DALI_BB)
//-LEDs-------------------------------------------------------------------------
  #define NUMBER_OF_LEDS            0

//-Buttons-------------------------------------------------------------------------
  #define NUMBER_OF_BUTTONS         0

#elif defined(ZIGBEE2DALI_V1) // Arjun : this seems to be enabled

//UI
//  #include "Zigbee2DaliUI.h"
  #define UI_INTERVAL_MS            10    // Interval for the UI Event
//-LEDs-------------------------------------------------------------------------
  #define NUMBER_OF_LEDS            1
  #define LED0                      PORTB_PIN(1)
  #define LED_CONFIG                {.channel={{.portpin = LED0, .activeLow = 0 }}}

//-OutputChannels-------------------------------------------------------------------------
  #define NUMBER_OF_OUTPUT_CHANNELS 1

//-Buttons-------------------------------------------------------------------------
  #define BUTTON_DEBOUNCE           3
  #define NUMBER_OF_BUTTONS         2  // Arjun : Original value 2
  #define BUTTON0_PIN               PORTB_PIN(2)
  #define BUTTON0_CALLBACKS         {.ButtonDown = NULL, .ButtonUp = NULL,\
                                     .ButtonClick = emberAfPluginOsramdsReset2factoryActionButtonShortClick, .ButtonDoubleClick = NULL,\
                                     .ButtonLongStart = emberAfPluginOsramdsReset2factoryActionButtonLongPress, .ButtonLongStop = NULL, \
                                     .ButtonExtraLongStart = NULL, .ButtonExtraLongStop = NULL}
  #define BUTTON0_CONFIG            {.portpin = BUTTON0_PIN, .activeLow = 1, .usePullUpDown = 1, \
                                     .longTimeMs = BUTTON_LONG_PRESSED_TIME, \
                                     .extraLongTimeMs = BUTTON_EXTRALONG_PRESSED_TIME, \
                                     .doubleClickBreak = PUSH_BUTTON_DOUBLE_CLICK_BREAK, \
                                     .callbacks = BUTTON0_CALLBACKS}
  #define BUTTON1_PIN               PORTB_PIN(6)
  #define BUTTON1_CALLBACKS         {.ButtonDown = NULL, .ButtonUp = NULL,\
                                     .ButtonClick = NULL, .ButtonDoubleClick = NULL,\
                                     .ButtonLongStart = NULL, .ButtonLongStop = NULL, \
                                     .ButtonExtraLongStart = NULL, .ButtonExtraLongStop = NULL}
  #define BUTTON1_CONFIG            {.portpin = BUTTON1_PIN, .activeLow = 1, .usePullUpDown = 1, \
                                     .longTimeMs = BUTTON_LONG_PRESSED_TIME, \
                                     .extraLongTimeMs = BUTTON_EXTRALONG_PRESSED_TIME, \
                                     .doubleClickBreak = PUSH_BUTTON_DOUBLE_CLICK_BREAK, \
                                     .callbacks = BUTTON1_CALLBACKS}
  #define BUTTONCONFIG_DEFAULT    {.channel={BUTTON0_CONFIG, BUTTON1_CONFIG}}

  #define IDENTIFY_SAK_EP            {.IdentifyStart=ZigbeeHaLightEndDevice_SAKIdentifyStart, .IdentifyStop=ZigbeeHaLightEndDevice_SAKIdentifyStop, .IdentifySetOutput=ZigbeeHaLightEndDevice_SAKIdentifySetOutput}
  #define IDENTIFY_REPEATER_EP       {.IdentifyStart=ZigbeeHaLightEndDevice_SAKIdentifyStart, .IdentifyStop=ZigbeeHaLightEndDevice_SAKIdentifyStop, .IdentifySetOutput=ZigbeeHaLightEndDevice_SAKIdentifySetOutput}
  #define IDENTIFY_LIGHT_EP1         {.IdentifyStart=NULL, .IdentifyStop=ZigbeeHaLightEndDevice_LightPointIdentifyStop, .IdentifySetOutput=ZigbeeHaLightEndDevice_LightPointIdentifySetOutput}
  #define IDENTIFY_LIGHT_EP2         {.IdentifyStart=NULL, .IdentifyStop=ZigbeeHaLightEndDevice_LightPointIdentifyStop, .IdentifySetOutput=ZigbeeHaLightEndDevice_LightPointIdentifySetOutput}
  #define IDENTIFY_OCCUPANCY_EP      {.IdentifyStart=Zigbee2Dali_IdentifyIllunminanceSensor, .IdentifyStop=NULL, .IdentifySetOutput=NULL}
  #define IDENTIFY_ILLUMINANCE_EP    {.IdentifyStart=Zigbee2Dali_IdentifyOccupancySensor,    .IdentifyStop=NULL, .IdentifySetOutput=NULL}
  #define IDENTIFY_CALLBACKS         {IDENTIFY_SAK_EP, IDENTIFY_REPEATER_EP, IDENTIFY_LIGHT_EP1, IDENTIFY_LIGHT_EP2, IDENTIFY_OCCUPANCY_EP, IDENTIFY_ILLUMINANCE_EP}

#elif defined(SILABS_DEMOKIT)
//UI
  #define UI_INTERVAL_MS            10    // Interval for the UI Event
//-LEDs-------------------------------------------------------------------------
  #define NUMBER_OF_LEDS            2
  #define LED1                      PORTA_PIN(6)
  #define LED1_ACTIVELOW            0
  #define LED2                      PORTA_PIN(7)
  #define LED2_ACTIVELOW            0
  #define LEDHAL_CONFIG             {.channel[0]={.portpin = LED1, .activeLow = LED1_ACTIVELOW }, \
                                     .channel[1]={.portpin = LED2, .activeLow = LED2_ACTIVELOW }}

//-OutputChannels-------------------------------------------------------------------------
  #define NUMBER_OF_OUTPUT_CHANNELS 2

//-Buttons-------------------------------------------------------------------------
  #define BUTTON_DEBOUNCE           3
  #define NUMBER_OF_BUTTONS         2
  #define BUTTON0_PIN               PORTB_PIN(6)
  #define BUTTON0_CALLBACKS         {.ButtonDownShort = NULL, .ButtonDownShortReleased = &Zigbee2DevelopmentBoard_IdentifyRequest, \
                                    .ButtonDownLong = NULL, .ButtonDownLongReleased = &Zigbee2DevelopmentBoard_NetworkLeaveRequest, \
                                    .ButtonDownExtraLong = NULL, .ButtonDownExtraLongReleased = NULL}
  #define BUTTON0_CONFIG            {.portpin = BUTTON0_PIN, .activeLow = 1, .usePullUpDown = 1, \
                                     .longTimeTicks = BUTTON_LONG_PRESSED_TIME, \
                                     .extraLongTimeTicks = BUTTON_EXTRALONG_PRESSED_TIME, \
                                     .callbacks = BUTTON0_CALLBACKS}
  #define BUTTON0_DEFAULT           {.config = BUTTON0_CONFIG, .data = {.stateCounter = 0, .pressedLast = 0, .state = Unpressed}}
  #define BUTTON1_PIN               PORTC_PIN(6)
  #define BUTTON1_CALLBACKS         {.ButtonDownShort = Zigbee2DevelopmentBoard_OccupancyRequest, .ButtonDownShortReleased = Zigbee2DevelopmentBoard_NoOccupancyRequest, \
                                    .ButtonDownLong = Zigbee2DevelopmentBoard_OccupancyRequest, .ButtonDownLongReleased = Zigbee2DevelopmentBoard_NoOccupancyRequest, \
                                    .ButtonDownExtraLong = Zigbee2DevelopmentBoard_OccupancyRequest, .ButtonDownExtraLongReleased = Zigbee2DevelopmentBoard_NoOccupancyRequest }
  #define BUTTON1_CONFIG            {.portpin = BUTTON1_PIN, .activeLow = 1, .usePullUpDown = 1, \
                                     .longTimeTicks = PUSH_BUTTON_LONG_PRESSED_TIME, \
                                     .extraLongTimeTicks = PUSH_BUTTON_EXTRALONG_PRESSED_TIME, \
                                     .callbacks = BUTTON1_CALLBACKS}
  #define BUTTONCONFIG_DEFAULT      {BUTTON0_CONFIG, BUTTON1_CONFIG}

#elif defined(SAMSUNG_DEMOKIT)
//UI
  #define UI_INTERVAL_MS            10    // Interval for the UI Event
//-LEDs-------------------------------------------------------------------------
  #define NUMBER_OF_LEDS            1
  #define LED0                      PORTB_PIN(4)
  #define LED_CONFIG                {.channel={{.portpin = LED0, .activeLow = 1 }}}

//-Buttons-------------------------------------------------------------------------
  #define BUTTON_DEBOUNCE           3
  #define NUMBER_OF_BUTTONS         1
  #define BUTTON0_CALLBACKS         {.ButtonDownShort = NULL, .ButtonDownShortReleased = &IdentifyRequest, \
                                    .ButtonDownLong = NULL, .ButtonDownLongReleased = &NetworkLeaveRequest, \
                                    .ButtonDownExtraLong = NULL, .ButtonDownExtraLongReleased = NULL}
  #define BUTTON0_CONFIG            {.portpin = PORTB_PIN(7), .activeLow = 1, .usePullUpDown = 1, \
                                     .longTimeTicks = BUTTON_LONG_PRESSED_TIME, \
                                     .extraLongTimeTicks = BUTTON_EXTRALONG_PRESSED_TIME, \
                                     .callbacks = BUTTON0_CALLBACKS}
  #define BUTTON0_DEFAULT           {.config = BUTTON0_CONFIG, .data = {.stateCounter = 0, .pressedLast = 0, .state = Unpressed}}
  #define BUTTONHANDLER_DEFAULT     {.config[0]=BUTTON0_DEFAULT}

#else
  #warning "Unknown Device"
#endif

