//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file daliConversions.h
///   \since 2017-04-20
///   \brief Conversion from Zigbee (linear) to DALI (log)
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __DALICONVERSIONS_H__
#define __DALICONVERSIONS_H__

// --------- Includes ----------------------------------------------------------

#include "Stdint.h"
#include "Stdbool.h"

// --------- Macro Definitions -------------------------------------------------
#define XY_NORMALIZER       65536

// --------- Type Definitions --------------------------------------------------

// --------- External Variable Declaration -------------------------------------

// --------- Global Functions --------------------------------------------------

uint8_t Zigbee2Dali(uint8_t level );
uint8_t Dali2Zigbee(uint8_t level );
bool ColorTemptoXY(uint16_t colorTempMired, uint16_t *currentX, uint16_t *currentY);
uint16_t XYtoColorTempMired(uint16_t currentX, uint16_t currentY);

#endif // __DALICONVERSIONS_H__
