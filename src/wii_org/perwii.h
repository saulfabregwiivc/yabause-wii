/*  Copyright 2008 Theo Berkau

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

#ifndef PERWII_H
#define PERWII_H

#define PERCORE_WIIKBD		2
#define PERCORE_WIICLASSIC 	3

#define MSG_CONNECT	0
#define MSG_DISCONNECT	1
#define MSG_EVENT	2

extern PerInterface_struct PERWiiKeyboard;
extern PerInterface_struct PERWiiClassic;
extern volatile int keystate;

s32 KeyboardMenuCallback(s32 result, void *usrdata);
s32 KeyboardConnectCallback(s32 result,void *usrdata);
int KBDInit(s32 (*initcallback)(s32, void *), s32 (*runcallback)(s32, void *));

s8 classic_analog_val( const expansion_t* exp, bool isX, bool isRjs);

extern inline BOOL stick_left(s8 claX, s8 gcX) { return claX < -60 || gcX < -46; }
extern inline BOOL stick_right(s8 claX, s8 gcX) { return claX > 60 || gcX > 46; }
extern inline BOOL stick_up(s8 claY, s8 gcY) { return claY > 70 || gcY > 54; }
extern inline BOOL stick_down(s8 claY, s8 gcY) { return claY < -70 || gcY < -54; }
#endif
