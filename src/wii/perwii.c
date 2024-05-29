/*  Copyright 2008 Theo Berkau
    Copyright 2008 Romulo

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <stdio.h>
#include <stddef.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include <math.h>
#include "../peripheral.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "perwii.h"
#include "keys.h"

#define PI    3.141592654

s32 kbdfd = -1;
volatile BOOL kbdconnected = FALSE;
extern u16 buttonbits;
PerPad_struct *pad[12];
volatile int keystate;
extern volatile int done;
extern char saves_dir[512];
extern void SNDWiiMuteAudio();
extern void SNDWiiUnMuteAudio();
int wii_ir_x;
int wii_ir_y;
int wii_ir_valid;

inline BOOL stick_left(s8 claX, s8 gcX) { return claX < -60 || gcX < -46; }
inline BOOL stick_right(s8 claX, s8 gcX) { return claX > 60 || gcX > 46; }

inline BOOL stick_up(s8 claY, s8 gcY) { return claY > 70 || gcY > 54; }
inline BOOL stick_down(s8 claY, s8 gcY) { return claY < -70 || gcY < -54; }

static const u32 lookup_button_WII[6] =
 {WPAD_BUTTON_2,
  WPAD_BUTTON_1,
  WPAD_BUTTON_B,
  WPAD_BUTTON_A,
  WPAD_BUTTON_MINUS,
  WPAD_BUTTON_PLUS
 };

u8 num_button_WII[9] =
 {3, 1, 0, 4, 2, 6, 6, 6, 5}; //default: A, B, C, X, Y, Z, L, R, Start

static const u32 lookup_button_CLA[10] =
 {WPAD_CLASSIC_BUTTON_A,
  WPAD_CLASSIC_BUTTON_B,
  WPAD_CLASSIC_BUTTON_X,
  WPAD_CLASSIC_BUTTON_Y,
  WPAD_CLASSIC_BUTTON_MINUS,
  WPAD_CLASSIC_BUTTON_PLUS,
  WPAD_CLASSIC_BUTTON_FULL_R,
  WPAD_CLASSIC_BUTTON_FULL_L,
  WPAD_CLASSIC_BUTTON_ZR,
  WPAD_CLASSIC_BUTTON_ZL,
 };

u8 num_button_CLA[9] =
 {3, 1, 0, 2, 9, 8, 7, 6, 5}; //default: A, B, C, X, Y, Z, L, R, Start

static const u16 lookup_button_GCC[8] =
 {PAD_BUTTON_A,
  PAD_BUTTON_B,
  PAD_BUTTON_X,
  PAD_BUTTON_Y,
  PAD_BUTTON_START,
  PAD_TRIGGER_Z,
  PAD_TRIGGER_R,
  PAD_TRIGGER_L
 };

u8 num_button_GCC[9] =
 {3, 1, 0, 2, 8, 5, 7, 6, 4}; //default: A, B, C, X, Y, Z, L, R, Start


struct
{
   u32 msg;
   u32 unknown;
   u8 modifier;
   u8 unknown2;
   u8 keydata[6];
} kbdevent ATTRIBUTE_ALIGN(32);

s32 KeyboardConnectCallback(s32 result,void *usrdata);

s32 KeyboardPoll(s32 result, void *usrdata)
{
   int i;

   if (kbdconnected)
   {
      switch(kbdevent.msg)
      {
         case MSG_DISCONNECT:
            kbdconnected = FALSE;
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardConnectCallback, NULL);
            break;
         case MSG_EVENT:
            // Hackish, horray!
            pad[0]->padbits[0] = 0xFF;
            pad[0]->padbits[1] = 0xFF;

            for (i = 0; i < 6; i++)
            {
               if (kbdevent.keydata[i] == 0)
                  break;
               PerKeyDown(kbdevent.keydata[i]);
            }
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardPoll, NULL);
            break;
         default: break;
      }
   }

   return 0;
}

s32 KeyboardMenuCallback(s32 result, void *usrdata)
{
   int i;
   int oldkeystate;
   int newkeystate;

   if (kbdconnected)
   {
      switch(kbdevent.msg)
      {
         case MSG_DISCONNECT:
            kbdconnected = FALSE;
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardConnectCallback, NULL);
            break;
         case MSG_EVENT:
            oldkeystate = keystate;
            newkeystate = 0;

            for (i = 0; i < 6; i++)
            {
               if (kbdevent.keydata[i] == 0)
                  break;
               switch(kbdevent.keydata[i])
               {
                  case KEY_UP:
                     if (!(oldkeystate & PAD_BUTTON_UP))
                        newkeystate |= PAD_BUTTON_UP;
                     break;
                  case KEY_DOWN:
                     if (!(oldkeystate & PAD_BUTTON_DOWN))
                        newkeystate |= PAD_BUTTON_DOWN;
                     break;
                  case KEY_ENTER:
                     if (!(oldkeystate & PAD_BUTTON_A))
                        newkeystate |= PAD_BUTTON_A;
                     break;
                  default: break;
               }
            }
            keystate = (oldkeystate ^ newkeystate) & newkeystate;
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardMenuCallback, NULL);
            break;
         default: break;
      }
   }

   return 0;
}

s32 KeyboardConnectCallback(s32 result,void *usrdata)
{
   // Should be the connect msg
   if (kbdevent.msg == MSG_CONNECT)
   {
      IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, usrdata, NULL);
      kbdconnected = TRUE;
   }
   return 0;
}

int KBDInit(s32 (*initcallback)(s32, void *), s32 (*runcallback)(s32, void *))
{
   static char kbdstr[] ATTRIBUTE_ALIGN(32) = "/dev/usb/kbd";

   if ((kbdfd = IOS_Open(kbdstr, IPC_OPEN_NONE)) < 0)
      return -1;

   IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, initcallback, runcallback);
   return 0;
}

int PERKeyboardInit()
{
   PerPortReset();
   pad[0] = PerPadAdd(&PORTDATA1);

//   SetupKeyPush(keypush, KEY_F1, ToggleFPS);
//   SetupKeyPush(keypush, KEY_1, ToggleNBG0);
//   SetupKeyPush(keypush, KEY_2, ToggleNBG1);
//   SetupKeyPush(keypush, KEY_3, ToggleNBG2);
//   SetupKeyPush(keypush, KEY_4, ToggleNBG3);
//   SetupKeyPush(keypush, KEY_4, ToggleRBG0);
//   SetupKeyPush(keypush, KEY_5, ToggleVDP1);

   PerSetKey(KEY_UP, PERPAD_UP, pad[0]);
   PerSetKey(KEY_DOWN, PERPAD_DOWN, pad[0]);
   PerSetKey(KEY_LEFT, PERPAD_LEFT, pad[0]);
   PerSetKey(KEY_RIGHT, PERPAD_RIGHT, pad[0]);
   PerSetKey(KEY_K, PERPAD_A, pad[0]);
   PerSetKey(KEY_L, PERPAD_B, pad[0]);
   PerSetKey(KEY_M, PERPAD_C, pad[0]);
   PerSetKey(KEY_U, PERPAD_X, pad[0]);
   PerSetKey(KEY_I, PERPAD_Y, pad[0]);
   PerSetKey(KEY_O, PERPAD_Z, pad[0]);
   PerSetKey(KEY_X, PERPAD_LEFT_TRIGGER, pad[0]);
   PerSetKey(KEY_Z, PERPAD_RIGHT_TRIGGER, pad[0]);
   PerSetKey(KEY_J, PERPAD_START, pad[0]);

   return KBDInit(KeyboardConnectCallback, KeyboardPoll);
}

void PERKeyboardDeInit()
{
   if (kbdfd > -1)
   {
      IOS_Close(kbdfd);
      kbdfd = -1;
   }
}

int PERKeyboardHandleEvents(void)
{
   return YabauseExec();
}

PerInterface_struct PERWiiKeyboard = {
PERCORE_WIIKBD,
"USB Keyboard Interface",
PERKeyboardInit,
PERKeyboardDeInit,
PERKeyboardHandleEvents
};

//////////////////////////////////////////////////////////////////////////////
// Wii Remote/ Classic Controller
//////////////////////////////////////////////////////////////////////////////

int PERClassicInit(void)	
{
   PerPortReset();
   pad[0] = PerPadAdd(&PORTDATA1);
   pad[1] = PerPadAdd(&PORTDATA2);

   return 0;
}

void PERClassicDeInit(void)	
{
}

s8 classic_analog_val( const expansion_t* exp, bool isX, bool isRjs)
{
    float mag = 0.0;
    float ang = 0.0;

    if( exp->type == WPAD_EXP_CLASSIC )
    {
        if( isRjs )
        {
           mag = exp->classic.rjs.mag;
           ang = exp->classic.rjs.ang;
        } else
        {
           mag = exp->classic.ljs.mag;
           ang = exp->classic.ljs.ang;
        }
    } else
    {
        return 0.0;
    }

    if( mag > 1.0 ) mag = 1.0;
    else if( mag < -1.0 ) mag = -1.0;
    double val = ( isX ?
        mag * sin( PI * ang / 180.0f ) :
        mag * cos( PI * ang / 180.0f ) );

    return (s8)(val * 128.0f);
}

int Check_Buttons_WII(u32 buttonsDown, u8 num_button)
{
   if(num_button==6)
         return 0;

   if(buttonsDown & lookup_button_WII[num_button])
      return 1;
   else
      return 0;
}
int Check_Buttons_CLA(u32 buttonsDown, u8 num_button)
{
   if(num_button==10)
         return 0;

   if(buttonsDown & lookup_button_CLA[num_button])
      return 1;
   else
      return 0;
}
int Check_Buttons_GCC(u16 gcbuttonsDown, u8 num_button, u8 gcS)
{
   if(num_button==9)
         return 0;

   if(num_button==8)
      if(gcS)
         return 1;
       else
         return 0;
   else if(gcbuttonsDown & lookup_button_GCC[num_button])
      return 1;
   else
      return 0;
}

int PERClassicHandleEvents(void)	
{
   u32 buttonsDown;
   u32 buttonsDown1 = WPAD_ButtonsDown(0);
   int i;
   u16 gcbuttonsDown;
   expansion_t exp;
   s8 claX, claY;
   s8 gcX, gcY;
   s8 gcSX, gcSY;
   u8 gcS;

   WPAD_ScanPads();
   PAD_ScanPads();
   WPADData *wd = WPAD_Data(0);

   for (i = 0; i < 2; i++)
   {
      buttonsDown = WPAD_ButtonsHeld(i);
      gcbuttonsDown = PAD_ButtonsHeld(i);
      WPAD_Expansion(i, &exp);
      claX = classic_analog_val(&exp, true, false);
      claY = classic_analog_val(&exp, false, false);
      gcX = PAD_StickX(i);
      gcY = PAD_StickY(i);
      gcSX = PAD_SubStickX(i);
      gcSY = PAD_SubStickY(i);
      gcS = (stick_right(0, gcSX) ||
             stick_left(0, gcSX) ||
             stick_up(0, gcSY) ||
             stick_down(0, gcSY));

   if(i==0 && wd->ir.valid)
   {
       wii_ir_valid = 1;
       wii_ir_x = wd->ir.x;
       wii_ir_y = wd->ir.y;

      if ((buttonsDown & WPAD_BUTTON_MINUS) &&
          (buttonsDown & WPAD_BUTTON_1))
          {
             SNDWiiMuteAudio();
             YabSaveStateSlot(saves_dir,1);
             SNDWiiUnMuteAudio();
          }

      if ((buttonsDown & WPAD_BUTTON_MINUS) &&
          (buttonsDown & WPAD_BUTTON_2))
          {
             SNDWiiMuteAudio();
             YabSaveStateSlot(saves_dir,2);
             SNDWiiUnMuteAudio();
          }

      if ((buttonsDown & WPAD_BUTTON_PLUS) &&
          (buttonsDown & WPAD_BUTTON_1))
          {
             SNDWiiMuteAudio();
             YabLoadStateSlot(saves_dir,1);
             SNDWiiUnMuteAudio();
          }

      if ((buttonsDown & WPAD_BUTTON_PLUS) &&
          (buttonsDown & WPAD_BUTTON_2))
          {
             SNDWiiMuteAudio();
             YabLoadStateSlot(saves_dir,2);
             SNDWiiUnMuteAudio();
          }
      if (buttonsDown1 & WPAD_BUTTON_UP) ToggleNBG0();
      if (buttonsDown1 & WPAD_BUTTON_RIGHT) ToggleNBG1();
      if (buttonsDown1 & WPAD_BUTTON_DOWN) ToggleNBG2();
      if (buttonsDown1 & WPAD_BUTTON_LEFT) ToggleNBG3();
      if (buttonsDown1 & WPAD_BUTTON_A) ToggleRBG0();
      if (buttonsDown1 & WPAD_BUTTON_B) ToggleVDP1();
      if ((buttonsDown & WPAD_BUTTON_1) &&
          (buttonsDown & WPAD_BUTTON_2)) ToggleFPS();
  } else
  {
      if(i==0) wii_ir_valid = 0;

      if (buttonsDown & WPAD_CLASSIC_BUTTON_UP ||
          buttonsDown & WPAD_BUTTON_RIGHT ||
          gcbuttonsDown & PAD_BUTTON_UP ||
          stick_up(claY, gcY))
         PerPadUpPressed(pad[i]);
      else
         PerPadUpReleased(pad[i]);

      if (buttonsDown & WPAD_CLASSIC_BUTTON_DOWN ||
          buttonsDown & WPAD_BUTTON_LEFT ||
          gcbuttonsDown & PAD_BUTTON_DOWN ||
          stick_down(claY, gcY))
         PerPadDownPressed(pad[i]);
      else
         PerPadDownReleased(pad[i]);

      if (buttonsDown & WPAD_CLASSIC_BUTTON_LEFT ||
          buttonsDown & WPAD_BUTTON_UP ||
          gcbuttonsDown & PAD_BUTTON_LEFT ||
          stick_left(claX, gcX))
         PerPadLeftPressed(pad[i]);
      else
         PerPadLeftReleased(pad[i]);

      if (buttonsDown & WPAD_CLASSIC_BUTTON_RIGHT ||
          buttonsDown & WPAD_BUTTON_DOWN ||
          gcbuttonsDown & PAD_BUTTON_RIGHT ||
          stick_right(claX, gcX))
         PerPadRightPressed(pad[i]);
      else
         PerPadRightReleased(pad[i]);

/////
      if (Check_Buttons_WII(buttonsDown, num_button_WII[8]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[8]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[8], gcS))
         PerPadStartPressed(pad[i]);
      else
         PerPadStartReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[0]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[0]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[0], gcS))
         PerPadAPressed(pad[i]);
      else
         PerPadAReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[1]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[1]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[1], gcS))
         PerPadBPressed(pad[i]);
      else
         PerPadBReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[2]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[2]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[2], gcS))
         PerPadCPressed(pad[i]);
      else
         PerPadCReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[3]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[3]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[3], gcS))
         PerPadXPressed(pad[i]);
      else
         PerPadXReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[4]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[4]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[4], gcS))
         PerPadYPressed(pad[i]);
      else
         PerPadYReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[5]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[5]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[5], gcS))
         PerPadZPressed(pad[i]);
      else
         PerPadZReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[6]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[6]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[6], gcS))
         PerPadLTriggerPressed(pad[i]);
      else
         PerPadLTriggerReleased(pad[i]);

      if (Check_Buttons_WII(buttonsDown, num_button_WII[7]) ||
          Check_Buttons_CLA(buttonsDown, num_button_CLA[7]) ||
          Check_Buttons_GCC(gcbuttonsDown, num_button_GCC[7], gcS))
         PerPadRTriggerPressed(pad[i]);
      else
         PerPadRTriggerReleased(pad[i]);
  }

      if (buttonsDown & WPAD_CLASSIC_BUTTON_HOME ||
          buttonsDown & WPAD_BUTTON_HOME || 
          ((gcbuttonsDown & PAD_BUTTON_START) && (gcbuttonsDown & PAD_TRIGGER_Z)))
      {
         done = 1;
         return 0;
      }
   }

   return YabauseExec();
}

PerInterface_struct PERWiiClassic = 
{
PERCORE_WIICLASSIC,
"Wii Remote/Classic Controller",
PERClassicInit,
PERClassicDeInit,
PERClassicHandleEvents
};

