//------------------------------------------------------------------------------
///   \file   IdentifyClient.h
///   \since  2018-06-21
///   \brief  Definition of commands for Identify-cluster client
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \addtogroup IdentifyClusterClient Identify Cluster Client
/// @ingroup Zigbee2DALI
/// @{
//------------------------------------------------------------------------------
#define IDENTIFY_EFFECT_BLINK     0x00
#define IDENTIFY_EFFECT_BREATHE   0x01
#define IDENTIFY_EFFECT_OKAY      0x02
#define IDENTIFY_EFFECT_CHANNEL   0x0b
#define IDENTIFY_EFFECT_FINISH    0xfe
#define IDENTIFY_EFFECT_STOP      0xff

EmberStatus IdentifyClient_TriggerEffect(uint8_t endpoint, uint8_t effectIdentifier, uint8_t effectVariant);
EmberStatus IdentifyClient_TriggerReverseWink(uint8_t endpoint);

/// @}
