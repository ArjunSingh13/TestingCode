//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file DaliTelegram.c
///   \since 2016-11-17
///   \brief DALI driver Telegram layer
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
/// \defgroup Zigbee2DALI
///
/// \defgroup DALI Telegram
/// \ingroup Zigbee2DALI
/// @{
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// --------- Includes ----------------------------------------------------------

#include "GlobalDefs.h"
#include "app/framework/include/af.h"


#include "DaliUart.h"
#include "DaliHAL.h"
#include "DaliTelegram_private.h"
#include "DaliTelegram.h"

// --------- Macro Definitions -------------------------------------------------

#define TRACE(trace, format, ...)
//#define TRACE(trace, ...) if (trace) {emberAfPrintln(0xFFFF, __VA_ARGS__);}

// --------- Type Definitions --------------------------------------------------
typedef struct StructDaliCommand {
  uint32_t command;
  uint8_t sendTwice;
  bool waitForAnswer;
  bool sentOk;
  uint8_t noAnswerRetries;
  TypeDaliCallback callback;
  uint32_t  delay;
} TypeDaliCommand;

typedef enum {
  DALI_STATE_OK,
  DALI_STATE_BUS_FAIL,
  DALI_STATE_BUS_OVERLOAD,
} TypeDaliSTATE;

#ifdef DALI_MULTI_MASTER_TEST
  #define MM_LOG_IDX_MAX 7
#endif

typedef struct{
  TypeDaliSTATE state;
  int32_t frameDelayCounter_FTT;
  uint8_t receiveIdx;
  uint8_t sendIdx;
  TypeDaliCommand sequence[MAX_COMMAND_IDX +1];
  TypeDaliCommand *pActCommand;
#ifdef DALI_MULTI_MASTER_TEST
  uint8_t wrongFramesLength[MM_LOG_IDX_MAX];
  uint32_t wrongFrames[MM_LOG_IDX_MAX];
  bool collisions[MM_LOG_IDX_MAX];
  uint8_t wrongFramesIdx;
  bool collision;
#endif
} TypeDali;

// --------- Local Variables ---------------------------------------------------

// --------- Global Variables --------------------------------------------------

TypeDali daliTelegram;

const TypeDali daliTelegramDEFAULT = {  .state=DALI_STATE_OK, .pActCommand=&(daliTelegram.sequence[0]), .sequence[0].sentOk=true };

const TypeDaliFrame DALI_FRAME_DEFAULT = { .raw = 0 };


#ifdef FADE_TEST
  uint32_t testTimer = 0;
#endif

// --------- Local Function Prototypes -----------------------------------------

void DaliTelegramSendReceive( void );

// --------- Local Functions ---------------------------------------------------

bool DaliTelegramSend2Uart( TypeDaliCommand *command) {
  bool ret;
  if (command->command >> 16) {                       // 3-byte-commands have 3rd byte=(address+1)
    ret = DaliUart_Send3Byte(command->command);
  }
  else {
    ret = DaliUart_SendCommand(command->command);
  }
  if (ret) {
    if ((command->sendTwice <= 1) && (!command->waitForAnswer)) {
      daliTelegram.frameDelayCounter_FTT = (command->delay < DALI_TIME_BEFORE_FORWARD_FTT) ? DALI_TIME_BEFORE_FORWARD_FTT : command->delay;
    }
    else {
      daliTelegram.frameDelayCounter_FTT = DALI_TIME_BEFORE_FORWARD_FTT;
    }
  }
  return ret;
}

void DaliTelegramSendReceive( void ) {
  TypeDaliUartReceive daliUartReceive = DaliUart_Receive();
  switch (daliUartReceive.length) {                                             // check data-length in bit
  case 0:
    if ( daliTelegram.frameDelayCounter_FTT == 0) {
      if ( ( daliTelegram.pActCommand->sentOk == false) || (daliTelegram.pActCommand->waitForAnswer) ) {   // sendTwice handled in Receive: sentOk left false, if sendTwice>0
        bool noAnswer = false;
        if ( (daliTelegram.pActCommand->waitForAnswer) && (daliTelegram.pActCommand->sentOk) ) {
          TRACE(true, "expecting BWF");
          if ( daliTelegram.pActCommand->noAnswerRetries <= 0) {                      // No Answer retries reached
            TRACE(true, "no BWF");
            if(daliTelegram.pActCommand->callback != NULL) {
              TypeDaliTelegramResponse response;
              response.answerType = DALI_NO_ANSWER;
              response.answer = daliUartReceive.data;
              TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};       //command should be changed to TypeDaliFrame
              daliTelegram.pActCommand->callback(response, frame);
            }
            daliTelegram.pActCommand->waitForAnswer = false;
            return;                                                             // !!!! RETURN in case of missing answer (don't re-send again)!!!!
          } // no else: decrement noAnswerRetries only if command is re-send
          noAnswer = true;
        }
        if (DaliTelegramSend2Uart(daliTelegram.pActCommand)) {
          if (noAnswer) { daliTelegram.pActCommand->noAnswerRetries--; }
        }
      }
      else {// scheduler to select next command
        uint8_t sendIdx;
        sendIdx = daliTelegram.sendIdx;
        if ( sendIdx != daliTelegram.receiveIdx) {
          sendIdx++;
          sendIdx &= MAX_COMMAND_IDX;
          TypeDaliCommand* pCommand = &(daliTelegram.sequence[sendIdx]);
          TRACE(true, "--> i:%u %4x", sendIdx, pCommand);
          if (DaliTelegramSend2Uart(pCommand)) {
            daliTelegram.sendIdx = sendIdx;
            daliTelegram.pActCommand = pCommand;                     // update pointer to new command
          } // nothing to do else
          else {
            TRACE(true, "    NOT SENT!");
          }
        }
      } // else to: if ( ( daliTelegram.pActCommand->sentOk != true) || (daliTelegram.pActCommand->waitForAnswer) )
    } // no else to if (daliTelegram.frameDelayCounter_FTT == 0)
    break;
  case Z2D_ANSWER_FRAME_LENGTH:
    daliTelegram.frameDelayCounter_FTT = DALI_SETTLING_BACKWORD_FORWARD_FTT;          // reset counter after receipt of answer frame
    TRACE(true, "BWF: %x",daliUartReceive.data);
    if ((daliTelegram.pActCommand->sentOk) && (daliTelegram.pActCommand->waitForAnswer)) {
      daliTelegram.pActCommand->waitForAnswer = false;
      if(daliTelegram.pActCommand->callback != NULL) {
        TypeDaliTelegramResponse response;
        response.answerType = DALI_ANSWER_OK;
        response.answer = daliUartReceive.data;
        TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};                      //command should be changed to TypeDaliFrame
        daliTelegram.pActCommand->callback(response, frame);
      }
    }
    break;
  case Z2D_ANSWER_TRASH:
    daliTelegram.frameDelayCounter_FTT = DALI_SETTLING_BACKWORD_FORWARD_FTT;          // reset counter after receipt of answer frame
    TRACE(true, "XYZ: %x",daliUartReceive.data);
    if ((daliTelegram.pActCommand->sentOk) && (daliTelegram.pActCommand->waitForAnswer)) {
      daliTelegram.pActCommand->waitForAnswer = false;
      if(daliTelegram.pActCommand->callback != NULL) {
        TypeDaliTelegramResponse response;
        response.answerType = DALI_ANSWER_TRASH;
        response.answer = daliUartReceive.data;
        TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};                      //command should be changed to TypeDaliFrame
        daliTelegram.pActCommand->callback(response, frame);
      }
    }
    break;
  case Z2D_COMMAND_FRAME_LENGTH:
    TRACE(true, "FWF %4x", daliUartReceive.data);
    if (daliTelegram.pActCommand->command == daliUartReceive.data) {                  // if frame was sent correctly ..
      if (daliTelegram.pActCommand->sendTwice > 0) {
        daliTelegram.pActCommand->sendTwice--;
      }
      if (daliTelegram.pActCommand->sendTwice == 0) {
        daliTelegram.pActCommand->sentOk = true;
        if (daliTelegram.pActCommand->waitForAnswer) {
          daliTelegram.frameDelayCounter_FTT = DALI_TIMEOUT_FORWARD_BACKWORD_FTT;     // reset counter for receipt of answer frame
        }
        else if(daliTelegram.pActCommand->callback != NULL) {                         // Do not wait for Answer, so call callback, if there is any.
          TypeDaliTelegramResponse response;
          response.answerType = DALI_NO_ANSWER;
          response.answer = 0;
          TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};           //command should be changed to TypeDaliFrame
          daliTelegram.pActCommand->callback(response, frame);
        }
      }
    }
    else {                                                                      // .. else: re-send this command (if another master did win)
      TRACE(true, "COL %x", daliUartReceive.data);
      if (daliTelegram.pActCommand->sendTwice > 0) {                                  //    in case of send-twice, restart send-twice
        daliTelegram.pActCommand->sendTwice = 2;
      }
    }
    break;
  case Z2D_INPUT_DEVICE_FRAME_LENGTH:                                           // frame from InputDevice
    if (daliTelegram.pActCommand->command == daliUartReceive.data) {                // if frame was sent correctly ..
      if (daliTelegram.pActCommand->sendTwice > 0) {
        daliTelegram.pActCommand->sendTwice--;
      }
      if (daliTelegram.pActCommand->sendTwice == 0) {
        daliTelegram.pActCommand->sentOk = true;
        if (daliTelegram.pActCommand->waitForAnswer) {
          daliTelegram.frameDelayCounter_FTT = DALI_TIMEOUT_FORWARD_BACKWORD_FTT;   // reset counter for receipt of answer frame
        }
        else if(daliTelegram.pActCommand->callback != NULL) {                       // Do not wait for Answer, so call callback, if there is any.
          TypeDaliTelegramResponse response;
          response.answerType = DALI_NO_ANSWER;
          response.answer = 0;
          TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};         //command should be changed to TypeDaliFrame
          daliTelegram.pActCommand->callback(response, frame);
        }
      }
    }
    else {
      TypeDaliFrame frame;
      frame.raw = daliUartReceive.data;
      DaliTelegram_InputDeviceFrameReceivedCallback( frame);
      // .. else: re-send this command (if another master did win)
      if (daliTelegram.pActCommand->sendTwice > 0) {                                //    in case of send-twice, restart send-twice
        daliTelegram.pActCommand->sendTwice = 2;
      }
    }
    daliTelegram.frameDelayCounter_FTT = DALI_TIME_BEFORE_FORWARD_FTT;                // .. and reset counter
    break;
  case Z2D_OSRAM_FRAME_LENGTH:                                                  // production tests and life-time counter
    TRACE(true, "OCF %4x", daliUartReceive.data);
    // DaliOsram_CommandHandler( byte osramAddress, byte osramCommand, byte vendor, bool osramSeq, byte channel);   // handle command ..
    daliTelegram.frameDelayCounter_FTT = DALI_TIME_BEFORE_FORWARD_FTT;                // .. and reset counter

    break;
  default:                                                                      // a damaged command / answer? Then resend command / query
    TRACE(true, "DEF %4x-%x", daliUartReceive.data, daliUartReceive.length);
    if (daliTelegram.pActCommand->sendTwice > 0) {                                    // in case of send-twice, restart send-twice
      daliTelegram.pActCommand->sendTwice = 2;
    }
    else {
      daliTelegram.frameDelayCounter_FTT = DALI_TIME_BEFORE_FORWARD_FTT;              // reset counter if not sendTwice
    }
    if ((daliTelegram.pActCommand->sentOk) && (daliTelegram.pActCommand->waitForAnswer)) {
      daliTelegram.pActCommand->waitForAnswer = false;
      if(daliTelegram.pActCommand->callback != NULL) {
        TypeDaliTelegramResponse response;
        response.answerType = DALI_ANSWER_TRASH;
        response.answer = daliUartReceive.data;
        TypeDaliFrame frame = {.raw = daliTelegram.pActCommand->command};                      //command should be changed to TypeDaliFrame
        daliTelegram.pActCommand->callback(response, frame);
      }
    }
    break;
#ifdef DALI_MULTI_MASTER_TEST
    daliTelegram.wrongFrames[daliTelegram.wrongFramesIdx] = daliUartReceive.data;
    daliTelegram.wrongFramesLength[daliTelegram.wrongFramesIdx] = daliUartReceive.length;
    daliTelegram.collisions[daliTelegram.wrongFramesIdx] = daliTelegram.collision;
    daliTelegram.wrongFramesIdx++;
    daliTelegram.wrongFramesIdx &= MM_LOG_IDX_MAX;
#endif
    break;
  }
#ifdef DALI_MULTI_MASTER_TEST
  if (daliUartReceive.length > 0) {
    daliTelegram.collision = false;
  }
#endif
}



// --------- Global Functions --------------------------------------------------

/* \brief
 * */
void DaliTelegram_Init( void ) {
  DaliUart_Init();                                                              // Initialize all variables and µC-registers
  daliTelegram = daliTelegramDEFAULT;                                             // Initialize complete structure
}

/* \brief
 * */
void DaliTelegram_Start( void ) {
  DaliUart_Start(DALI_FAILURE_TIME_FTT);                                        // start timer, start measuring bus-down time if bus down
}

/* \brief
 * */
uint32_t  DaliTelegram_TimeTick( void ) {
  uint32_t ticks = DaliUart_GetTimeTicks();
  DaliTelegramSendReceive();                                                   // asynchronous call
  if (ticks > 0) {
    TypeDaliUartBusSTATUS daliStatus = DaliUart_FastTimer();
    switch (daliStatus) {
      case DALI_BUS_FAIL:
        DaliTelegram_BusFailCallback();
        break;
      case DALI_BUS_OK:
      case DALI_BUS_DOWN:
      default:
        break;
    }
    //Zigbee2CheckNetworkLeave( busStatus );
    // handle different timers
    if (daliTelegram.frameDelayCounter_FTT > 0) {
      daliTelegram.frameDelayCounter_FTT -= ticks;
      if (daliTelegram.frameDelayCounter_FTT < 0) {  // Can happen if we got more than one tick since last call
        daliTelegram.frameDelayCounter_FTT = 0;
      }
    }

#ifdef FADE_TEST
    testTimer++;
    if ( testTimer == (uint32_t)(10000/2.5+0.5) ) {                             // set level after 1 s
      DaliTelegram_StartFadeTime(1, 85, 0);
    }
    else if ( testTimer == (uint32_t)(12000/2.5+0.5)) {
      DaliTelegram_StartFadeTime(1, 254, (uint16_t)(3.4*10));
    }
    else if ( testTimer == (uint32_t)(17000/2.5+0.5) ) {
      DaliTelegram_StartFadeTime(1, 85, (uint16_t)(17.4*10));
    }
    else if ( testTimer == (uint32_t)(35000/2.5+0.5) ) {
      DaliTelegram_StartFadeTime(1, 254, (uint16_t)((22*60+4)*10));
    }
#endif
  }
  return ticks;
}

bool DaliTelegram_IsDaliBusy(void) {
  bool queueEmpty = (daliTelegram.receiveIdx == daliTelegram.sendIdx);
  return (!queueEmpty);
}

// --   Z i g b e e   S e r v i c e s   ----------------------------------------

/* \brief
 * */

/* \brief
 * */
bool DaliTelegram_SendCommandWithCallback(TypeDaliFrame command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  uint8_t receiveIdx = daliTelegram.receiveIdx;
  receiveIdx++;
  receiveIdx &= MAX_COMMAND_IDX;
  if ( receiveIdx != daliTelegram.sendIdx) {
    TypeDaliCommand *pCommand = &daliTelegram.sequence[receiveIdx];
    pCommand->command         = command.raw ;
    pCommand->callback        = callback;
    pCommand->sendTwice       = (control & DALI_SEND_TWICE) ? 2 : 0;
    pCommand->waitForAnswer   = (control & DALI_WAIT4ANSWER) ? true : false;
    pCommand->noAnswerRetries = (control & DALI_SAFE_QUERY) ? DALI_NO_ANSWER_RETRIES : 0;
    pCommand->sentOk          = false;
    pCommand->delay           = delay;
    daliTelegram.receiveIdx   = receiveIdx;
    return true;
  }
  else {
    return false;
  }
}

/* \brief
 * */
bool DaliTelegram_SendDap2AddressWithCallback(uint8_t shortAddress, uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  if (shortAddress == DALI_SA_BROADCAST) return DaliTelegram_SendDapBroadcastWithCallback(level, control, delay, callback);

  frame.fwf.shortAddressed.multicast = 0;
  frame.fwf.shortAddressed.noDap = 0;
  frame.fwf.shortAddressed.command = level;
  frame.fwf.shortAddressed.address = shortAddress;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendDap2Address(uint8_t shortAddress, uint8_t level, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendDap2AddressWithCallback(shortAddress, level, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendDap2GroupWithCallback(uint8_t groupAddress, uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.fwf.groupAddressed.addressmode = 0;
  frame.fwf.groupAddressed.noDap = 0;
  frame.fwf.groupAddressed.command = level;
  frame.fwf.groupAddressed.groupaddress = groupAddress;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendDap2Group(uint8_t groupAddress, uint8_t level, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendDap2GroupWithCallback(groupAddress, level, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendDapBroadcastWithCallback(uint8_t level, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.fwf.broadcast.address = 0x7F;
  frame.fwf.broadcast.noDap = 0;
  frame.fwf.broadcast.command = level;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendDapBroadcast(uint8_t level, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendDapBroadcastWithCallback(level, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommand2AddressWithCallback(uint8_t shortAddress, uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  if (shortAddress == DALI_SA_BROADCAST) return DaliTelegram_SendStdCommandBroadcastWithCallback(command, control, delay, callback);
  frame.fwf.shortAddressed.multicast = 0;
  frame.fwf.shortAddressed.noDap = 1;
  frame.fwf.shortAddressed.command = command;
  frame.fwf.shortAddressed.address = shortAddress;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommand2Address(uint8_t shortAddress, uint8_t command, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendStdCommand2AddressWithCallback(shortAddress, command, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommand2GroupWithCallback(uint8_t groupAddress, uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.fwf.groupAddressed.addressmode = 0;
  frame.fwf.groupAddressed.noDap = 1;
  frame.fwf.groupAddressed.command = command;
  frame.fwf.groupAddressed.groupaddress = groupAddress;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommand2Group(uint8_t groupAddress, uint8_t command, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendStdCommand2GroupWithCallback(groupAddress, command, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommandBroadcastWithCallback(uint8_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.fwf.broadcast.address = 0x7F;
  frame.fwf.broadcast.noDap = 1;
  frame.fwf.broadcast.command = command;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendStdCommandBroadcast(uint8_t command, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendStdCommandBroadcastWithCallback(command, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_SendSpecialCommandWithCallback(uint8_t command, uint8_t data, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.fwf.specialCommand.command = command;
  frame.fwf.specialCommand.data = data;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_SendSpecialCommand(uint8_t command, uint8_t data, uint8_t control, uint32_t delay) {
  return DaliTelegram_SendSpecialCommandWithCallback(command, data, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteCommand2AddressWithCallback(uint8_t shortAddress, uint16_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.idf.shortAddressed.multicast = 0;
  frame.idf.shortAddressed.noDap = 1;
  frame.idf.shortAddressed.command = command;
  frame.idf.shortAddressed.address = shortAddress;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteCommand2Address(uint8_t shortAddress, uint16_t command, uint8_t control, uint32_t delay) {
  return DaliTelegram_Send3ByteCommand2AddressWithCallback(shortAddress, command, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteCommandBroadcastWithCallback(uint16_t command, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.idf.shortAddressed.multicast = 1;
  frame.idf.shortAddressed.noDap = 1;
  frame.idf.shortAddressed.command = command;
  frame.idf.shortAddressed.address = 0x3F;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteCommandBroadcast(uint16_t command, uint8_t control, uint32_t delay) {
  return DaliTelegram_Send3ByteCommandBroadcastWithCallback(command, control, delay, NULL);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteSpecialCommandWithCallback(uint16_t command, uint8_t data, uint8_t control, uint32_t delay, TypeDaliCallback callback) {
  TypeDaliFrame frame = DALI_FRAME_DEFAULT;

  frame.idf.specialCommand.command = command;
  frame.idf.specialCommand.data = data;
  return DaliTelegram_SendCommandWithCallback(frame, control, delay, callback);
}

/* \brief
 * */
bool DaliTelegram_Send3ByteSpecialCommand(uint16_t command, uint8_t data, uint8_t control, uint32_t delay) {
  return DaliTelegram_Send3ByteSpecialCommandWithCallback(command, data, control, delay, NULL);
}


/// @}
