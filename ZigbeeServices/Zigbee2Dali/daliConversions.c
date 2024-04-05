//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file daliConversions.c
///   \since 2017-04-20
///   \brief Conversion from Zigbee (linear) to DALI (log)
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// --------- Includes ----------------------------------------------------------

#include "daliConversions.h"

// --------- Macro Definitions -------------------------------------------------

// --------- Type Definitions --------------------------------------------------

// --------- Local Variables ---------------------------------------------------

// --------- Global Variables --------------------------------------------------

const uint8_t zigbee2DaliTable[] =
   {  0,   1,  51,  77,  92, 102, 110, 117, 123, 127, 132, 136, 139, 142, 145, 148,
    151, 153, 155, 157, 159, 161, 163, 165, 166, 168, 169, 171, 172, 173, 175, 176,
    177, 178, 179, 180, 182, 183, 184, 185, 186, 186, 187, 188, 189, 190, 191, 192,
    192, 193, 194, 195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201, 202, 202,
    203, 204, 204, 205, 205, 206, 206, 207, 207, 208, 208, 209, 209, 210, 210, 211,
    211, 212, 212, 213, 213, 214, 214, 214, 215, 215, 216, 216, 217, 217, 217, 218,
    218, 219, 219, 219, 220, 220, 220, 221, 221, 221, 222, 222, 222, 223, 223, 223,
    224, 224, 224, 225, 225, 225, 226, 226, 226, 227, 227, 227, 228, 228, 228, 228,
    229, 229, 229, 230, 230, 230, 230, 231, 231, 231, 232, 232, 232, 232, 233, 233,
    233, 233, 234, 234, 234, 234, 235, 235, 235, 235, 236, 236, 236, 236, 237, 237,
    237, 237, 237, 238, 238, 238, 238, 239, 239, 239, 239, 239, 240, 240, 240, 240,
    240, 241, 241, 241, 241, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 244,
    244, 244, 244, 244, 244, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246,
    247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249,
    249, 250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 251, 252, 252,
    252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254};

const uint8_t dali2ZigbeeTable[] =
    {  0,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,          // as 1 is often interpreted as off dimmed in Zigbee
       2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
       2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
       2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
       3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,
       4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,
       5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   7,
       7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,
      10,  10,  10,  10,  11,  11,  11,  11,  12,  12,  12,  12,  13,  13,  13,  14,
      14,  14,  15,  15,  16,  16,  16,  17,  17,  18,  18,  18,  19,  19,  20,  20,
      21,  21,  22,  23,  23,  24,  24,  25,  26,  26,  27,  28,  28,  29,  30,  31,
      32,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  46,  47,
      48,  49,  51,  52,  53,  55,  56,  58,  59,  61,  63,  64,  66,  68,  70,  72,
      74,  76,  78,  80,  82,  84,  86,  89,  91,  94,  96,  99, 101, 104, 107, 110,
     113, 116, 119, 123, 126, 129, 133, 137, 140, 144, 148, 152, 156, 161, 165, 169,
     174, 179, 184, 189, 194, 199, 205, 210, 216, 222, 228, 235, 241, 248, 254, 254};

struct colorTable
{
    uint16_t kelvin;
    double   x;
    double   y;
}colorTable;

struct colorTable colorTempBBL[17] = {
    {2000, 0.52669, 0.41331},
    {2105, 0.51541, 0.41465},
    {2222, 0.50338, 0.41525},
    {2353, 0.49059, 0.41498},
    {2500, 0.47701, 0.41368},
    {2677, 0.463, 0.41121},   // 373 - 2680
    {2857, 0.446, 0.40742},
    {3077, 0.43156, 0.40216},
    {3333, 0.41502, 0.39535},
    {3636, 0.39792, 0.38690},
    {4000, 0.38045, 0.37676}, // 250 - 4000
    {4444, 0.36276, 0.36496},
    {5000, 0.34510, 0.35162},
    {5714, 0.32775, 0.33690},
    {6667, 0.31101, 0.32116}, // 150 - 6666
    {8000, 0.29518, 0.30477},
    {10000, 0.28063, 0.28828}};

// --------- Local Function Prototypes -----------------------------------------

// --------- Local Functions ---------------------------------------------------

// --------- Global Functions --------------------------------------------------

uint8_t Zigbee2Dali(uint8_t level )
{
  if(level < sizeof(zigbee2DaliTable))
    return zigbee2DaliTable[level];
  else
    return 254;
}


uint8_t Dali2Zigbee(uint8_t level )
{
  if(level < sizeof(dali2ZigbeeTable))
    return dali2ZigbeeTable[level];
  else
    return 254;
}

/** \brief Convert Color Temperature Mired to XY Values
 */
bool ColorTemptoXY(uint16_t colorTempMired, uint16_t *currentX, uint16_t *currentY) {
    uint16_t x0, x1, y0, y1;
    double dist;
    uint16_t colorTempKelvin = (uint16_t) (1000000 / colorTempMired);
    uint8_t tableSize = sizeof(colorTempBBL) / sizeof(colorTable);

    // check min and max values before finding in table
    if(colorTempKelvin < colorTempBBL[0].kelvin){
        *currentX = (uint16_t)( colorTempBBL[0].x * XY_NORMALIZER );
        *currentY = (uint16_t)( colorTempBBL[0].y * XY_NORMALIZER );
        return true;
    }
    if(colorTempKelvin > colorTempBBL[tableSize-1].kelvin){
        *currentX = (uint16_t)( colorTempBBL[tableSize-1].x * XY_NORMALIZER );
        *currentY = (uint16_t)( colorTempBBL[tableSize-1].y * XY_NORMALIZER );
        return true;
    }

    for(int i=0; i < sizeof(colorTempBBL); i++)
    {
        if(colorTempBBL[i].kelvin >= colorTempKelvin){
            dist = ( (double)colorTempBBL[i].kelvin - colorTempKelvin ) / ( colorTempBBL[i].kelvin - colorTempBBL[i-1].kelvin );
            x0 = (uint16_t)( colorTempBBL[i-1].x * XY_NORMALIZER );
            x1 = (uint16_t)( colorTempBBL[i].x * XY_NORMALIZER );
            y0 = (uint16_t)( colorTempBBL[i-1].y * XY_NORMALIZER );
            y1 = (uint16_t)( colorTempBBL[i].y * XY_NORMALIZER );

            *currentX = (uint16_t)((x0 - x1) * dist + x1);
            *currentY = (uint16_t)((y0 - y1) * dist + y1);
            return true;
        }
    }
    return false;
}

/** \brief Convert XY to Color Temperature Mired
 */
uint16_t XYtoColorTempMired(uint16_t currentX, uint16_t currentY) {
    double xScaled, yScaled, dist;
    uint16_t temp0, temp1;
    uint8_t tableSize = sizeof(colorTempBBL) / sizeof(colorTable);
    double colorTempKelvin = 0xFFFF;
    xScaled = (double)currentX / XY_NORMALIZER;
    yScaled = (double)currentY / XY_NORMALIZER;

    // check min and max values before finding in table
    if(xScaled > colorTempBBL[0].x || yScaled > colorTempBBL[0].y){
        return (uint16_t)( 1000000 / colorTempBBL[0].kelvin );
    }
    if(xScaled < colorTempBBL[tableSize-1].x || yScaled < colorTempBBL[tableSize-1].y){
        return (uint16_t)( 1000000 / colorTempBBL[tableSize-1].kelvin );
    }

    for(int i = tableSize-1; i >= 0; i--)
    {
        if(colorTempBBL[i].x >= xScaled){
            dist = ( (double)colorTempBBL[i].x - xScaled ) / ( colorTempBBL[i].x - colorTempBBL[i+1].x );
            temp1 = colorTempBBL[i+1].kelvin;
            temp0 = colorTempBBL[i].kelvin;

            colorTempKelvin = (temp1 - temp0) * dist + temp0;
            break;
        }
    }
    return (1000000 / (uint16_t)colorTempKelvin);
}
