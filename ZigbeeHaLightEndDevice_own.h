//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file ZigbeeHaLightEndDevice_own.h
///   \since 2017-10-10
///   \brief common cluster independent functions
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// Functions
void ZigbeeHaLightEndDevice_PrepareReset2FactoryDefaults(void);
bool ZigbeeHaLightEndDevice_IsInRepeatermode(void);
void ZigbeeHaLightEndDevice_SetRepeatermode(bool active);
void ZigbeeHaLightEndDevice_LightPointIdentifyStop(uint8_t endpointIndex);
void ZigbeeHaLightEndDevice_LightPointIdentifySetOutput(uint8_t endpointIndex, uint8_t level, uint16_t fadetime);
void ZigbeeHaLightEndDevice_SAKIdentifyStart(uint8_t endpointIndex);
void ZigbeeHaLightEndDevice_SAKIdentifyStop(uint8_t endpointIndex);
void ZigbeeHaLightEndDevice_SAKIdentifySetOutput(uint8_t endpointIndex, uint8_t level, uint16_t fadetime);
