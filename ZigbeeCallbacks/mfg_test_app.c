/******************** (C) COPYRIGHT 2016 -  OSRAM AG *********************************************************************
* File Name         : mfg_test_app.c
* Description       : application part of manufacturing test
* Initial Version   : MS
* Created           : 14.03.2017
* Modified by       :
*
**************************************************************************************************************************/
#include <string.h>
#include PLATFORM_HEADER //compiler/micro specifics, types
#include "hal/micro/micro-common.h"
#include "hal/micro/cortexm3/efm32/micro-common.h"  // arjun, original path was for em35x: hal/micro/cortexm3/micro-common.h
#include "app/framework/include/af.h"
#include "GlobalDefs.h"
#include "config.h"
#include "mfg-config-messages.h"
#include "lib_csv.h"
#include "stack/include/mfglib.h"
#include "raw-mfglib-msg.h"
#include "DaliTelegram.h"
#include "DaliCommands.h"


static char const * pEui64Str;

void appSpecificMfgQueryGearResponse(TypeDaliTelegramResponse response, TypeDaliFrame command);

void rawAppMessageHandler(const char* ourEui64, const char* cmd, const char*frame, uint8_t tokenCount, uint8_t rssi) {

#if 0
  if (strcmp(cmd, MASTER_TO_DUT_READ_BUTTONS_CMD)==0) {
    uint8_t button = (halGpioRead(BUTTON0_PIN)) ? 0 : 1;
    button |= (halGpioRead(BUTTON1_PIN)) ? 0 : 2;
    SEND_RAW_CMD_STRING("M:%s,%s,%u", ourEui64, DUT_TO_MASTER_READ_BUTTONS_RESP,button);
  }
  else if ((strcmp(cmd, MASTER_TO_DUT_LED_CMD)==0) && (tokenCount == 3)) {
    uint8_t led;
    CSV_GetTokenInt8u((char const*)frame, &led, 2, ',');
    if(led & 0x01) halGpioSet(LED0);
    else halGpioClear(LED0);
    SEND_RAW_CMD_STRING("M:%s,%s", ourEui64, DUT_TO_MASTER_LED_RESP);
  }
  else if ((strcmp(cmd, MASTER_TO_DUT_CUSTOM_CMD)==0) && (tokenCount == 3)) {
    uint8_t subcmd;
    CSV_GetTokenInt8u((char const*)frame, &subcmd, 2, ',');
    switch(subcmd) {
      case 1:
        DaliTelegram_SendStdCommandBroadcastWithCallback(DALI_QUERY_GEAR, DALI_WAIT4ANSWER, 0, appSpecificMfgQueryGearResponse);
        pEui64Str = ourEui64;
        break;
      default:
        break;
    }
  }
#endif
}


void appSpecificMfgQueryGearResponse(TypeDaliTelegramResponse response, TypeDaliFrame command) {

  if (response.answerType == DALI_ANSWER_OK) {
    SEND_RAW_CMD_STRING("M:%s,%s,%u", pEui64Str, DUT_TO_MASTER_CUSTOM_RESP, response.answer);
  }
  else {
    SEND_RAW_CMD_STRING("M:%s,%s,-1", pEui64Str, DUT_TO_MASTER_CUSTOM_RESP);
  }
}

void appSpecificMfgTick(void) {
  DaliTelegram_TimeTick();
}

void appSpecificMfgInit(void) {
  DaliTelegram_Init();
  DaliTelegram_Start();

}

void appSpecificMfgOnExit(void) {
}
