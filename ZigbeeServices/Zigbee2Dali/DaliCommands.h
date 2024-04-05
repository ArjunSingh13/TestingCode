//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliCommands.h
///   \since 2016-11-17
///   \brief List of DaliCommands to be included in DaliSequences.h for easier setting of CommandSequences
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALICOMMANDS_H__
#define __DALICOMMANDS_H__

// --------- Includes ----------------------------------------------------------

// --------- Macro Definitions -------------------------------------------------

#define DALI_MASK                   0xFF

// special commands (without data)
#define DALI_CMD_TERMINATE          0xA1
#define DALI_CMD_RANDOMIZE          0xA7
#define DALI_CMD_COMPARE            0xA9
#define DALI_CMD_WITHDRAWN          0xAB
#define DALI_CMD_PING               0xAD                                      // only used to indicate: loop stopped

#define DALI_CMD_SETDTR0            0xA3                                      // add value (low byte)
#define DALI_CMD_INITIALISE         0xA5                                      // add device-addressing(low byte)
#define DALI_CMD_SETDTR1            0xC3                                      // add value (low byte)
#define DALI_CMD_SETDTR2            0xC5                                      // add value (low byte)
#define DALI_CMD_SEARCHADDRH        0xB1
#define DALI_CMD_SEARCHADDRM        0xB3
#define DALI_CMD_SEARCHADDRL        0xB5
#define DALI_CMD_PGM_SHORT_ADDRESS  0xB7
#define DALI_CMD_VERIFY_SHORT_ADDR  0xB9
#define DALI_CMD_ENABLE_DEVICETYPE  0xC1

// addressed commands
#define DALI_CMD_OFF                0x00
#define DALI_CMD_UP                 0x01
#define DALI_CMD_DOWN               0x02
#define DALI_CMD_STEP_UP            0x03
#define DALI_CMD_STEP_DOWN          0x04
#define DALI_CMD_RECALL_MAX         0x05
#define DALI_CMD_RECALL_MIN         0x06
#define DALI_CMD_ONandSTEP_UP       0x07
#define DALI_CMD_STEP_DOWNandOFF    0x08

// DT1 Commands
#define DALI_CMD_DT1_INHIBIT					          0xE1
#define DALI_CMD_DT1_RESET_INHIBIT				      0xE2
#define DALI_CMD_DT1_START_FUNCTION_TEST		    0xE3
#define DALI_CMD_DT1_START_DURATION_TEST		    0xE4
#define DALI_CMD_DT1_STOP_TEST					        0xE5
#define DALI_CMD_DT1_RESET_FUNC_TEST_DONE_FLAG  0xE6
#define DALI_CMD_DT1_RESET_DUR_TEST_DONE_FLAG   0xE7
#define DALI_CMD_DT1_STORE_DTR_AS_LEVEL			    0xE9
#define DALI_CMD_DT1_STORE_DTR_AS_DELAY_TIME_H  0xEA
#define DALI_CMD_DT1_STORE_DTR_AS_DELAY_TIME_L  0xEB
#define DALI_CMD_DT1_STORE_DTR_AS_FUNC_TEST_INT 0xEC
#define DALI_CMD_DT1_STORE_DTR_AS_DUR_TEST_INT  0xED
#define DALI_CMD_DT1_STORE_DTR_AS_TEST_EXEC_TMO	0xEE
#define DALI_CMD_DT1_STORE_DTR_AS_PROLONG		    0xEF
#define DALI_CMD_DT1_START_IDENTIFICATION       0xF0
#define DALI_CMD_DT1_QUERY_BATTERY_CHARGE       0xF1
#define DALI_CMD_DT1_QUERY_LEVEL                0xF6
#define DALI_CMD_DT1_QUERY_MODE                 0xFA
#define DALI_CMD_DT1_QUERY_FEATURES				      0xFB
#define DALI_CMD_DT1_QUERY_FAILURE_STATUS		    0xFC
#define DALI_CMD_DT1_QUERY_STATUS               0xFD

// DT8 Commands
#define DALI_CMD_DT8_ACTIVATE               0xE2
#define DALI_CMD_DT8_SET_TEMP_COLOR         0xE7
#define DALI_CMD_DT8_TEMP_STEP_COOL         0xE8
#define DALI_CMD_DT8_TEMP_STEP_WARM         0xE9
#define DALI_CMD_DT8_TEMP_STORE_LIMIT       0xF2

// DT8 queries
#define DALI_QUERY_DT8_COLOUR_STATUS        0xF8
#define DALI_QUERY_DT8_COLOUR_TYPE_FEATURES 0xF9
#define DALI_QUERY_DT8_COLOUR_VALUE         0xFA


#define DALI_CMD_RESET              0x20
// send-twice-commands (set DTR0 before sending!):
#define DALI_CMD_IDENTIFY           0x25
#define DALI_CMD_SET_MAX_LEVEL      0x2A
#define DALI_CMD_SET_MIN_LEVEL      0x2B
#define DALI_CMD_SET_SysFAIL_LEVEL  0x2C
#define DALI_CMD_SET_POWER_ON_LEVEL 0x2D
#define DALI_CMD_SET_FADE_TIME      0x2E
#define DALI_CMD_SET_FADE_RATE      0x2F
#define DALI_CMD_SET_ExtFADE_TIME   0x30
#define DALI_CMD_SET_SHORT_ADDRESS  0x80


// queries
#define DALI_QUERY_STATUS           0x90
#define DALI_QUERY_GEAR             0x91
#define DALI_QUERY_LAMP_FAILURE     0x92
#define DALI_QUERY_LAMP_ON          0x93
#define DALI_QUERY_NO_ADDRESS       0x96                                      // only broadcast query (to all unaddressed) makes sense (or group-addressed)
#define DALI_QUERY_VERSION_NUMBER   0x97
#define DALI_QUERY_DEVICE_TYPE      0x99
#define DALI_QUERY_PHYS_MIN_LEVEL   0x9A
#define DALI_QUERY_POWER_FAILURE    0x9B                                      // always broadcast
#define DALI_QUERY_OPERATING_MODE   0x9E                                      // not used: QUERY MANUFACTURER SPECIFIC MODE to allow dedicated OperatingModes
#define DALI_QUERY_ACT_LEVEL        0xA0
#define DALI_QUERY_MAX_LEVEL        0xA1
#define DALI_QUERY_MIN_LEVEL        0xA2
#define DALI_QUERY_NEXT_DEVICE_TYPE 0xA7
#define DALI_READ_MEMORY_LOCATION   0xC5
#define DALI_QUERY_CONTENT_DTR0     0x98
#define DALI_QUERY_CONTENT_DTR1     0x9C
#define DALI_QUERY_CONTENT_DTR2     0x9D

#define DALI_QUERY_COLOR_VALUE                  0xFA
#define DALI_QUERY_COLOR_TEMP                   0x02
#define DALI_QUERY_COLOR_TEMP_PHYS_COOLEST      0x81
#define DALI_QUERY_COLOR_TEMP_PHYS_WARMEST      0x83


//----InputDevices----------------------------------------------------------------------------------------------------------------------------------------------------------------
// Addressing
// special commands (without data)
#define SENSOR_TERMINATE            0xA1FF
#define SENSOR_RANDOMIZE            0xA7FF
#define SENSOR_COMPARE              0xA9FF
#define SENSOR_WITHDRAWN            0xABFF

#define SENSOR_SETDTR0              0xA3FF                                      // add value (low byte)
#define SENSOR_INITIALISE           0xA5FF                                      // add device-addressing(low byte)
#define SENSOR_SETDTR1              0xC3FF                                      // add value (low byte)
#define SENSOR_SETDTR2              0xC5FF                                      // add value (low byte)
#define SENSOR_SEARCHADDRH          0xB1FF
#define SENSOR_SEARCHADDRM          0xB3FF
#define SENSOR_SEARCHADDRL          0xB5FF
#define SENSOR_PGM_SHORT_ADDRESS    0xB7FF
#define SENSOR_VERIFY_SHORT_ADDR    0xB9FF


// Sensor data
#define SENSOR_IDENTIFY_DEVICES     0xC00C
#define SENSOR_RESET                0xC020
#define SENSOR_QUERY_INPUT_TYPE     0xC099
#define SENSOR_QUERY_LIGHT_HIGH     0xC101                                    // 0xC1: Read hight byte + 0x01: channel 1
#define SENSOR_QUERY_LIGHT_LOW      0xC201
#define SENSOR_QUERY_CONTENT_DTR0   0xC098
#define SENSOR_QUERY_CONTENT_DTR1   0xC09C
#define SENSOR_QUERY_CONTENT_DTR2   0xC09D
#define SENSOR_QUERY_STATUS         0xC090
#define SENSOR_READ_MEMORY_LOCATION 0xC0C5
#define SENSOR_SET_SHORT_ADDRESS    0xC080

#define SENSOR_EVENT_PIR            0x8201

// DALI device types
#define DALI_DEVICE_TYPE_DT1        0x01
#define DALI_DEVICE_TYPE_DT8        0x08

// DALIv2 input device special commands


#define DALI_SENSOR_TERMINATE       0xC100
#define DALI_SENSOR_INITIALISE      0xC101                                      // add device-addressing(low byte)
#define DALI_SENSOR_RANDOMIZE       0xC102
#define DALI_SENSOR_COMPARE         0xC103
#define DALI_SENSOR_WITHDRAWN       0xC104
#define DALI_SENSOR_SEARCHADDRH     0xC105
#define DALI_SENSOR_SEARCHADDRM     0xC106
#define DALI_SENSOR_SEARCHADDRL     0xC107
#define DALI_SENSOR_PGM_SHORT_ADDRESS    0xC108
#define DALI_SENSOR_VERIFY_SHORT_ADDR    0xC109
#define DALI_SENSOR_SETDTR0         0xC130
#define DALI_SENSOR_SETDTR1         0xC131
#define DALI_SENSOR_SETDTR2         0xC132                                      // add value (low byte)

// Sensor data
#define DALI_SENSOR_IDENTIFY_DEVICES     0xFE00
#define DALI_SENSOR_RESET                0xFE10
#define DALI_SENSOR_SET_SHORT_ADDRESS    0xFE14
#define DALI_SENSOR_QUERY_INSTANCES      0xFE35
#define DALI_SENSOR_QUERY_CONTENT_DTR0   0xFE36
#define DALI_SENSOR_QUERY_CONTENT_DTR1   0xFE37
#define DALI_SENSOR_QUERY_CONTENT_DTR2   0xFE38
#define DALI_SENSOR_READ_MEMORY_LOCATION 0xFE3C
#define DALI_SENSOR_SET_REPORT_TIMER     0x0030
#define DALI_SENSOR_SET_DEADTIME_TIMER   0x0032
#define DALI_SENSOR_SET_EVENT_SCHEME     0x0067
#define DALI_SENSOR_SET_EVENT_FILTER     0x0068
#define DALI_SENSOR_QUERY_INSTANCE_TYPE  0x0080
#define DALI_SENSOR_QUERY_INPUT_VALUE    0x008C
#define DALI_SENSOR_QUERY_INPUT_VALUE_LATCH      0x008D

#define SENSOR_EVENT_PIR            0x8201


// DALIv2 input device standard commands
#define DALI_SENSOR_SET_OPERATING_MODE 0xFE18

// --------- Type Definitions --------------------------------------------------

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

#endif // __DALICOMMANDS_H__
