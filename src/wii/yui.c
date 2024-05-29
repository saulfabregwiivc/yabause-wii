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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <ogcsys.h>
#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
//#include <di/di.h>
#include "../cs0.h"
#include "../cs2.h"
#include "../m68kcore.h"
#include "../peripheral.h"
#include "../vidsoft.h"
#include "../vdp2.h"
#include "../yui.h"
#include "../sndsdl.h"
#include "perwii.h"
#include "sndwii.h"
#include "menu.h"

#include "../yui.h"

extern u8 num_button_WII[9];
extern u8 num_button_CLA[9];
extern u8 num_button_GCC[9];

/* Wii includes */
#include "iowii.h"
//#include "syswii.h"

#include <mxml.h>

static mxml_node_t *xml = NULL;
static mxml_node_t *data = NULL;
static mxml_node_t *section = NULL;
static mxml_node_t *item = NULL;

int YuiExec(void);

void InitMenu(void);
void Check_Bios_Exist(void);
void Check_SD_Init(void);
void Check_Read_Dir(void);
void scroll_filelist(s8 delta);
void print_filelist(void);
void scroll_cartlist(s8 delta);
void print_cartlist(void);
void scroll_settinglist(s8 delta);
void print_settinglist(void);

/* Constants */

static int IsPal = 0;
static u32 *xfb[2] = { NULL, NULL };
int fbsel = 0;
static GXRModeObj *rmode = NULL;
volatile int done=0;
volatile int resetemu=0;
int running=1;
static void *_console_buffer = NULL;
void __console_init_ex(void *conbuffer,int tgt_xstart,int tgt_ystart,int tgt_stride,int con_xres,int con_yres,int con_stride);

#ifdef USE_WIIGX
#define DEFAULT_FIFO_SIZE       (256*1024)
GXColor background = {0, 0, 0, 0xff};
GXTexObj texObj;
Mtx GXmodelView2D, view;
Mtx44 perspective;
u32 *wiidispbuffer=NULL;
#endif

#ifdef AUTOLOADPLUGIN
u8 autoload = 0;
u8 returnhomebrew = 0;
//#include "homebrew.h"
//#define TITLE_ID(x,y) (((u64)(x) << 32) | (y))
char Exit_Dol_File[1024];
#endif

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
&PERWiiKeyboard,
&PERWiiClassic,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
NULL
};

#ifdef SCSP_PLUGIN
SCSPInterface_struct *SCSCoreList[] = {
&SCSDummy,
&SCSScsp1,
&SCSScsp2,
NULL
};
#endif

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#else
&SNDDummy,
#endif
&SNDWII,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
#ifdef HAVE_LIBGL //OGL driver on wii does not work
&VIDOGL,
#endif
&VIDSoft,
&VIDSoftOld,
NULL
};

M68K_struct *M68KCoreList[] = {
&M68KDummy,
&M68KC68K,
#ifdef HAVE_Q68
&M68KQ68,
#endif
NULL
};


char yabause_dir[512]="sd:/yabause"; //default
char games_dir[512];
char saves_dir[512];
char carts_dir[512];
char prev_itemnum[512];
static char buppath[512];
static char biospath[512];
static char cartpath[512];
char settingpath[512];
static char bupfilename[512]="/bkram.bin";
static char biosfilename[512]="/bios.bin";
static char isofilename[512]="";
#ifdef AUTOLOADPLUGIN
static char gamefilename[512]="";
#endif
static char settingfilename[512]="/yabause.xml";
static char cartfilename[11][512]={
 "",
 "/actionreplay.bin",
 "/bkram4M.bin",
 "/bkram8M.bin",
 "/bkram16M.bin",
 "/bkram32M.bin",
 "",
 "",
 "",
 "/", // "/ROM16M.bin",
 ""
};
static const char saturnbuttonsname[9][50]={
"Saturn A     = ",
"Saturn B     = ",
"Saturn C     = ",
"Saturn X     = ",
"Saturn Y     = ",
"Saturn Z     = ",
"Saturn L     = ",
"Saturn R     = ",
"Saturn Start = "
};
static const char WIIbuttonsname[7][50]={
"Wii Button 2    ",
"Wii Button 1    ",
"Wii Button B    ",
"Wii Button A    ",
"Wii Button Minus",
"Wii Button Plus ",
"None            ",
};
static const char CLAbuttonsname[11][50]={
"Classic Button A    ",
"Classic Button B    ",
"Classic Button X    ",
"Classic Button Y    ",
"Classic Button Minus",
"Classic Button Plus ",
"Classic Button R    ",
"Classic Button L    ",
"Classic Button ZR   ",
"Classic Button ZL   ",
"None                ",
};
static const char GCCbuttonsname[10][50]={
"GC Button A    ",
"GC Button B    ",
"GC Button X    ",
"GC Button Y    ",
"GC Button Start",
"GC Trigger Z   ",
"GC Trigger R   ",
"GC Trigger L   ",
"GC Right Stick ",
"None           ",
};
static char menuitemWIIbuttons[9][512];
static char menuitemCLAbuttons[9][512];
static char menuitemGCCbuttons[9][512];
static char menuitemwithoutbios[512]="Without Bios (Now With)";
static char menuitemwithbios[512]="With Bios (Now Without)";
static char menuitemoffframeskip[512]="Off Frameskip (Now On)";
static char menuitemonframeskip[512]="On Frameskip (Now Off)";
static char menuitemoffspecialcolor[512]="Off Special Color (Now On)";
static char menuitemonspecialcolor[512]="On Special Color (Now Off)";
static char menuitemsmpcperipheraltiming[512]="Smpc Peripheral Timing:      ";
static char menuitemsmpcothertiming[512]="Smpc Other Timing:     ";
static char menuitemdeclinenumber[512]="Decline Number:   ";
static char menuitemdividenumberclock[512]="Divide Number for Clock:   ";
static char menuitemoffeachbackupram[512]="Use One backup ram (Now Use Each)";
static char menuitemoneachbackupram[512]="Use Each backup ram (Now Use One)";
static char menuitemoffthreadingscsp2[512]="Off Threading SCSP2 (Now On)";
static char menuitemonthreadingscsp2[512]="On Threading SCSP2 (Now Off)";
static char menuitemoffeachsetting[512]="Use One Setting (Now Use Each)";
static char menuitemoneachsetting[512]="Use Each Setting (Now Use One)";
static char menuitemloadsettingeach[512]="Load Settings (Each)";
static char menuitemloadsettingone[512]="Load Settings (One) ";
static char menuitemsavesettingeach[512]="Save Settings (Each)";
static char menuitemsavesettingone[512]="Save Settings (One) ";
static int display_pos=0;
static int display_cnt=0;
static int first_load=1;
static int first_readdir=1;

extern int vdp2width, vdp2height;
int wii_width, wii_height;
extern int wii_ir_x;
extern int wii_ir_y;
extern int wii_ir_valid;

#define FILES_PER_PAGE	10

struct file *filelist = NULL;
u32 nb_files = 0;
#define CART_TYPE_NUM 11
#define SETTING_NUM 14

void gotoxy(int x, int y);
void OnScreenDebugMessage(char *string, ...);
void SetMenuItem();
void DoMenu();
int GameExec();
int LoadCue();
int Settings();
int CartSet();
int CartSetExec();
int BiosWith();
int BiosOnlySet();
int FrameskipOff();
int SpecialColorOn();
int SetTimingMenu();
int SetTiming();
int EachBackupramOn();
int ThreadingSCSP2On();
int EachSettingOn();
int SoundDriver();
int VideoDriver();
int M68KDriver();
int SCSPDriver();
int ConfigureButtons();
int WIIConfigureButtons();
int CLAConfigureButtons();
int GCCConfigureButtons();
int WIISetButtons();
int CLASetButtons();
int GCCSetButtons();
int SoundDriverSetExec();
int VideoDriverSetExec();
int M68KDriverSetExec();
int SCSPDriverSetExec();
int About();
int saveSettings();
int load_settings();

static s32 selected = 0, start = 0;
static s32 selectedcart = 7, startcart = 0;
static s32 selectedsetting = 0, startsetting = 0;
static int submenuflag = 0;
static int bioswith = 0;
static int frameskipoff = 0;
int specialcoloron = 1;
int eachbackupramon = 1;
int threadingscsp2on = 1;
int eachsettingon = 1;

int menuselect=1;
int setmenuselect=0;
int sounddriverselect=2;
int videodriverselect=2;
#ifdef HAVE_Q68
//int m68kdriverselect=2;
int m68kdriverselect=1; // c68k seems be better..
#else
int m68kdriverselect=1;
#endif
int scspdriverselect=2;
int configurebuttonsselect=0;
int WIIconfigurebuttonsselect=0;
int CLAconfigurebuttonsselect=0;
int GCCconfigurebuttonsselect=0;
int settimingselect=0;
int aboutmenuselect=6;
int numsetmenu;
int numcartmenu;
int numsounddriver;
int numvideodriver;
int numm68kdriver;
int numscspdriver;
int numconfigurebuttons;
int numWIIconfigurebuttons;
int numCLAconfigurebuttons;
int numGCCconfigurebuttons;
int numsettimings;
int numaboutmenu;

extern int smpcperipheraltiming;
extern int smpcothertiming;
extern int declinenum;
extern int dividenumclock;

static BOOL flag_mount = FALSE;

extern int fpstoggle;
extern int fps;
u8 digit_font[10][14*18] = {
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void fat_remount()
{
   if(flag_mount)
   {
      fatUnmount("fat:/");
      flag_mount = FALSE;
   }
   fatInitDefault();
   flag_mount = TRUE;
}

void reset()
{
   resetemu=1;
}

void powerdown()
{
   done = 1;
}

#if 0
int DVDStopMotor()
{
   static char dvdstr[] ATTRIBUTE_ALIGN(32) = "/dev/di";
   s32 fd;
   u8 buf[0x20];
   u8 outbuf[0x20];

   if ((fd = IOS_Open(dvdstr,0)) < 0)
      return 0;
	
   ((u32 *)buf)[0x00] = 0xE3000000;
   ((u32 *)buf)[0x01] = 0;
   ((u32 *)buf)[0x02] = 0;

   IOS_Ioctl(fd, buf[0], buf, 0x20, outbuf, 0x20);

   IOS_Close(fd);

   return 1;
}
#endif

#ifdef USE_WIIGX
#ifdef WIICONVERTASM
void convertBufferToRGBA8(u32* src, u16 width, u16 height) {
   u32 bufferSize = (width * height) << 2;

   u16 *dst_ar = (u16 *)(wiidispbuffer);
   u16 *dst_gb = (u16 *)(wiidispbuffer + 8);
   u32 *src1 = (u32 *)(src);
   u32 *src2 = (u32 *)(src + width);
   u32 *src3 = (u32 *)(src + 2*width);
   u32 *src4 = (u32 *)(src + 3*width);
   u32 h,w;
   register u32 tmp0=0,tmp1=0;

   memset((u8 *)wiidispbuffer, 0x00, bufferSize);

   for (h=0; h<height; h+=4)
   {
      for (w=0; w<width; w+=4)
      {
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src1++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src1++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src1++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src1++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src2++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src2++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src2++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src2++),"b"(dst_ar++),"b"(dst_gb++)
            );

            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src3++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src3++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src3++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src3++),"b"(dst_ar++),"b"(dst_gb++)
            );

            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src4++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src4++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src4++),"b"(dst_ar++),"b"(dst_gb++)
            );
            asm volatile(
               "lwzu %0,0(%2);"
               "rotlwi %1,%0,8;"
               "sth %1,0(%3);"
               "srwi %1,%0,8;"
               "sth %1,0(%4)"
               :"=&b"(tmp0),"=&b"(tmp1)
               :"b"(src4++),"b"(dst_ar++),"b"(dst_gb++)
            );

         /* next paired tiles */
         dst_ar += 16;
         dst_gb += 16;
      }

      /* next 4 lines */
      src1 = src4;
      src2 = src1 + width;
      src3 = src2 + width;
      src4 = src3 + width;
   }

   DCFlushRange((u8 *)wiidispbuffer, bufferSize);
}
#else
void convertBufferToRGBA8(u32* src, u16 width, u16 height) {
   u16 i;
   u32 bufferSize = (width * height) << 2;

   u16 *dst_ar = (u16 *)(wiidispbuffer);
   u16 *dst_gb = (u16 *)(wiidispbuffer + 8);
   u32 *src1 = (u32 *)(src);
   u32 *src2 = (u32 *)(src + width);
   u32 *src3 = (u32 *)(src + 2*width);
   u32 *src4 = (u32 *)(src + 3*width);
   u32 pixel,h,w;

   memset((u8 *)wiidispbuffer, 0x00, bufferSize);

   for (h=0; h<height; h+=4)
   {
      for (w=0; w<width; w+=4)
      {
         /* line N (4 pixels) */
         for (i=0; i<4; i++)
         {
            pixel = *src1++;
            *dst_ar++= ((pixel << 8) & 0xff00) | (pixel >> 24);
            *dst_gb++= (pixel >> 8) & 0xffff;
         }

         /* line N + 1 (4 pixels) */
         for (i=0; i<4; i++)
         {
            pixel = *src2++;
            *dst_ar++= ((pixel << 8) & 0xff00) | (pixel >> 24);
            *dst_gb++= (pixel >> 8) & 0xffff;
         }

         /* line N + 2 (4 pixels) */
         for (i=0; i<4; i++)
         {
            pixel = *src3++;
            *dst_ar++= ((pixel << 8) & 0xff00) | (pixel >> 24);
            *dst_gb++= (pixel >> 8) & 0xffff;
         }

         /* line N + 3 (4 pixels) */
         for (i=0; i<4; i++)
         {
            pixel = *src4++;
            *dst_ar++= ((pixel << 8) & 0xff00) | (pixel >> 24);
            *dst_gb++= (pixel >> 8) & 0xffff;
         }

         /* next paired tiles */
         dst_ar += 16;
         dst_gb += 16;
      }

      /* next 4 lines */
      src1 = src4;
      src2 = src1 + width;
      src3 = src2 + width;
      src4 = src3 + width;
   }

   DCFlushRange((u8 *)wiidispbuffer, bufferSize);
}
#endif

void InitGX(void )
{
    // Initialize wii output buffer
    if ((wiidispbuffer = (u32 *)memalign(32, 704 * 512)) == NULL)
       exit(-1);

    // Setup the fifo
    void *gp_fifo = NULL;
    if ((gp_fifo = memalign(32,DEFAULT_FIFO_SIZE)) == NULL)
       exit(-1);
    memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

    GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

    GX_SetCopyClear((GXColor){ 0, 0, 0, 0xff }, GX_MAX_Z24);


    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_ClearVtxDesc();     
    GX_InvVtxCache();       
    GX_InvalidateTexAll(); 

    GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,   GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
    GX_SetNumChans(1);    
    GX_SetNumTexGens(1);  
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    guMtxIdentity(GXmodelView2D);
    guMtxTransApply(GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -100.0F);
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    guOrtho(perspective, 0, rmode->efbHeight, 0, rmode->fbWidth, 0, 1000.0f);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GX_SetColorUpdate(GX_ENABLE);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetClipMode(GX_CLIP_ENABLE);

    f32 yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
    GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
    if (rmode->aa)
       GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
       GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetCullMode(GX_CULL_NONE);
    GX_SetDispCopyGamma(GX_GM_1_0);
}
#endif

int RemoveSettings(void)
{
    char eachxmlfilename[512];
    char *p_c;

    if(eachsettingon && !first_load)
    {
        strcpy(eachxmlfilename, filelist[selected].filename);
        p_c = strrchr(eachxmlfilename, '.');
        *p_c = '\0';
        strcat(eachxmlfilename, ".xml");
        strcpy(settingpath, saves_dir);
        strcat(settingpath, "/");
        strcat(settingpath, eachxmlfilename);
    }
    else
    {
        strcpy(settingpath, yabause_dir);
        strcat(settingpath, settingfilename);
    }
    remove(settingpath);

    menuselect=1;
    Check_Read_Dir();
    return 0;
}

int ResetSettings(void)
{
   bioswith = 0;
   selectedcart = 7;
   videodriverselect = 2;
   sounddriverselect = 2;
   m68kdriverselect = 1;
   scspdriverselect = 2;
   //selected;
   frameskipoff = 0;
   specialcoloron = 1;
   smpcperipheraltiming = 1000;
   smpcothertiming = 1050;
   eachbackupramon = 1;
   declinenum = 15;
   dividenumclock = 1;
   threadingscsp2on = 1;
// buttons
   num_button_WII[0] = 3;
   num_button_WII[1] = 1;
   num_button_WII[2] = 0;
   num_button_WII[3] = 4;
   num_button_WII[4] = 2;
   num_button_WII[5] = 6;
   num_button_WII[6] = 6;
   num_button_WII[7] = 6;
   num_button_WII[8] = 5;
   num_button_CLA[0] = 3;
   num_button_CLA[1] = 1;
   num_button_CLA[2] = 0;
   num_button_CLA[3] = 2;
   num_button_CLA[4] = 9;
   num_button_CLA[5] = 8;
   num_button_CLA[6] = 7;
   num_button_CLA[7] = 6;
   num_button_CLA[8] = 5;
   num_button_GCC[0] = 3;
   num_button_GCC[1] = 1;
   num_button_GCC[2] = 0;
   num_button_GCC[3] = 2;
   num_button_GCC[4] = 8;
   num_button_GCC[5] = 5;
   num_button_GCC[6] = 7;
   num_button_GCC[7] = 6;
   num_button_GCC[8] = 4;

   SetMenuItem();

   menuselect=1;
   return 0;
}

int main(int argc, char **argv)
{
   int retry;

   L2Enhance();
   WPAD_Init();
   PAD_Init();
   SYS_SetResetCallback(reset);
   SYS_SetPowerCallback(powerdown);
	
   usleep(500000);

   //fatInitDefault();

   //for stable mout
   retry = 3;
   while(retry)
   {
      if(fatMountSimple("sd", &__io_wiisd))
      {
        break;
      } else {
        usleep(500000);
      }
      retry--;
   } 
   
   retry = 3;
   while(retry)
   {
      if(fatMountSimple("usb", &__io_usbstorage))
      {
        break;
      } else {
        usleep(500000);
      }
      retry--;
   } 


   if (!opendir(yabause_dir)) {
      strcpy(yabause_dir, "usb:/yabause");
      if (!opendir(yabause_dir)) strcpy(yabause_dir, "sd:/yabause");
   } else {
     strcpy(yabause_dir, "sd:/yabause");
   }

   strcpy(games_dir, yabause_dir);
   strcat(games_dir, "/games");
   strcpy(saves_dir, yabause_dir);
   strcat(saves_dir, "/saves");
   strcpy(carts_dir, yabause_dir);
   strcat(carts_dir, "/carts");

   load_settings(); //first loading global setting
#ifdef AUTOLOADPLUGIN
   if(argc > 3 && argv[1] != NULL && argv[2] != NULL && argv[3] != NULL)
   {
      autoload = 1;
      returnhomebrew = 1;
      strcpy(isofilename, argv[1]);
      strcat(isofilename, "/");
      strcat(isofilename, argv[2]);
      strcpy(gamefilename, argv[2]);
      //strncpy(Exit_Dol_File, argv[3], sizeof(Exit_Dol_File));
      first_load=0;
      load_settings(); //loading each setting if there is ..
   }
#endif
   SetMenuItem();
   DoMenu();

#ifdef AUTOLOADPLUGIN
   if(returnhomebrew)
   {
/*
     WII_LaunchTitle(TITLE_ID(0x00010008,0x57494948));
     LoadHomebrew(Exit_Dol_File);
     AddBootArgument(Exit_Dol_File);
     AddBootArgument("EMULATOR_MAGIC");
     BootHomebrew();
*/
   }
#endif
   exit(0);
   return 0;
}

int YuiExec()
{
   yabauseinit_struct yinit;
   int ret;
   FILE *fp;

   VIDEO_Init();

#ifndef USE_WIIGX

   switch(VIDEO_GetCurrentTvMode()) 
   {
      case VI_NTSC:
         rmode = &TVNtsc240Ds;
	 break;
      case VI_PAL:
         rmode = &TVPal264Ds;
 	 break;
      case VI_MPAL:
	 rmode = &TVMpal480IntDf;
	 break;
      default:
         rmode = &TVNtsc240Ds;
	 break;
   }
#endif



#ifndef USE_WIIGX
   // Allocate two buffers(may not be necessary)
   xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
   xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
   VIDEO_Configure(rmode);
   VIDEO_ClearFrameBuffer (rmode, xfb[0], COLOR_BLACK);
   VIDEO_ClearFrameBuffer (rmode, xfb[1], COLOR_BLACK);
   VIDEO_SetNextFramebuffer(xfb[0]);
   VIDEO_SetBlack(FALSE);
   VIDEO_Flush();
   VIDEO_WaitVSync();
   if(rmode->viTVMode&VI_NON_INTERLACE) 
      VIDEO_WaitVSync();

#endif


#ifdef USE_WIIGX
   wii_width  = rmode->fbWidth;
#else
   wii_width  = rmode->fbWidth / 2;
#endif
   wii_height = rmode->xfbHeight;

   WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
   WPAD_SetVRes(WPAD_CHAN_ALL, wii_width, wii_height);

   memset(&yinit, 0, sizeof(yabauseinit_struct));
   yinit.percoretype = PERCORE_WIICLASSIC;
   yinit.sh2coretype = SH2CORE_INTERPRETER;
   yinit.vidcoretype = videodriverselect;
#ifdef SCSP_PLUGIN
   yinit.scspcoretype = scspdriverselect;
#endif
   yinit.sndcoretype = sounddriverselect;
   yinit.cdcoretype = CDCORE_ISO;
   yinit.m68kcoretype = m68kdriverselect;
   yinit.carttype = selectedcart;
   yinit.regionid = REGION_AUTODETECT;
   strcpy(biospath, yabause_dir);
   strcat(biospath, biosfilename);
   if (!bioswith || ((fp=fopen(biospath, "rb")) == NULL)) {
      yinit.biospath = NULL;
      bioswith = 0;
   } else {
      yinit.biospath = biospath;
      fclose(fp);
   }
   yinit.cdpath = isofilename;
   strcpy(buppath, saves_dir);
   strcat(buppath, bupfilename);
   yinit.buppath = buppath;
   yinit.mpegpath = NULL;
   strcpy(cartpath, carts_dir);
   strcat(cartpath, cartfilename[selectedcart]);
   yinit.cartpath = cartpath;
   yinit.netlinksetting = NULL;
   yinit.videoformattype = IsPal;
   yinit.clocksync = 0;
   yinit.basetime = 0;
   yinit.usethreads = threadingscsp2on;

   // Hijack the fps display
   VIDSoft.OnScreenDebugMessage = OnScreenDebugMessage;

   if ((isofilename[0] == 0) && (!bioswith))
   {
      menuselect=1;
      InitMenu();
      return 0;
   }

   if ((ret = YabauseInit(&yinit)) == 0)
   {
      if(frameskipoff)
         DisableAutoFrameSkip();
      else
         EnableAutoFrameSkip();

      ScspSetFrameAccurate(0);
      YabauseSetDecilineMode(1);

      VIDEO_ClearFrameBuffer(rmode, xfb[fbsel], COLOR_BLACK);

      while(!done)
      {
         if (PERCore->HandleEvents() != 0)
            return -1;
         if (resetemu)
         {
            YabauseReset();
            resetemu = 0;
            SYS_SetResetCallback(reset);
         }
      }
      if(strlen(cdip->itemnum)!=0)
         strcpy(prev_itemnum, cdip->itemnum);
      YabauseDeInit();
#ifdef AUTOLOADPLUGIN
      if(autoload) autoload=0;
#endif
      done=0;
      resetemu=0;
      InitMenu();
   }
   else
   {
      while(!done)
         VIDEO_WaitVSync();
   }

   return 0;
}

void YuiErrorMsg(const char *string)
{
   if (strncmp(string, "Master SH2 invalid opcode", 25) == 0)
   {
      if (!running)
         return;
      running = 0;
      printf("%s\n", string);
   }
}

void YuiSwapBuffers()
{


#ifdef USE_WIIGX

   f32 stretch_width = (f32)wii_width/(f32)vdp2width;
   f32 stretch_height = (f32)wii_height/(f32)vdp2height;
   f32 ref_width = (f32)vdp2width/2.0,
       ref_height = (f32)vdp2height/2.0;

   //Simple pointer of Wii IR for the functions: save & load states,....
   if(wii_ir_valid)
   {
      int i, j;
      int disp_x = (int)((double)wii_ir_x/stretch_width);
      int disp_y = (int)((double)wii_ir_y/stretch_width);
      for(j=-8; j<8; j++)
      {
        for(i=-8; i<8; i++)
        {
           if(!(disp_x+i<0) && !(disp_x+i>=vdp2width) &&
              !(disp_y+j<0) && !(disp_y+j>=vdp2height))
           {
              dispbuffer[disp_x+i+(disp_y+j)*vdp2width]=
              ((j>-4) && (j<4) && (i>-4) && (i<4)) ?
              0x00000000:0xffffffff;
           }
        }
      }
   }
   if(fpstoggle)
   {
      int i, j;
      int num = fps / 10;
      int othercolor = 0;

      if(((abs(0xcc-(dispbuffer[18+20*vdp2width]>>24)))+
          (abs(0x11-((dispbuffer[18+20*vdp2width]>>16)&0xff)))+
          (abs(0x44-((dispbuffer[18+20*vdp2width]>>8)&0xff)))) <
         ((abs(0x00-(dispbuffer[18+20*vdp2width]>>24)))+
          (abs(0xcc-((dispbuffer[18+20*vdp2width]>>16)&0xff)))+
          (abs(0x22-((dispbuffer[18+20*vdp2width]>>8)&0xff)))))
             othercolor = 1;

      for(j=0; j<18; j++)
      {
        for(i=0; i<14; i++)
        {
          if(digit_font[num][j*14+i])
          {
             if(othercolor)
                dispbuffer[4+i+(10+j)*vdp2width]= 0x00cc22ff;
              else
                dispbuffer[4+i+(10+j)*vdp2width]= 0xcc1144ff;
          }
        }
      }
      num = fps % 10;
      for(j=0; j<18; j++)
      {
        for(i=0; i<14; i++)
        {
          if(digit_font[num][j*14+i])
          {
             if(othercolor)
                dispbuffer[18+i+(10+j)*vdp2width]= 0x00cc22ff;
              else
                dispbuffer[18+i+(10+j)*vdp2width]= 0xcc1144ff;
          }
        }
      }
    }


   convertBufferToRGBA8(dispbuffer,vdp2width,vdp2height);
   GX_InvalidateTexAll();
   GX_InitTexObj(&texObj, wiidispbuffer, vdp2width, vdp2height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
   GX_InitTexObjLOD(&texObj,GX_NEAR,GX_NEAR_MIP_NEAR,2.5f,9.0f,0.0f,0,0,GX_ANISO_1);
   GX_LoadTexObj(&texObj, GX_TEXMAP0);

   Mtx model, tmp1, tmp2, mv;
   guMtxIdentity(tmp1);
   guMtxScaleApply(tmp1, tmp1, stretch_width, stretch_height, 1.0);
   guVector axis = (guVector) {0, 0, 1};
   guMtxRotAxisDeg(tmp2, &axis, 0.0);
   guMtxConcat(tmp2, tmp1, model);
   guMtxTransApply(model, model, (f32)wii_width/2.0, (f32)wii_height/2.0, 0.0f);
   guMtxConcat(GXmodelView2D, model, mv);
   GX_LoadPosMtxImm(mv, GX_PNMTX0);
     GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-ref_width, -ref_height, 0.0);
        GX_Color1u32(0xffffffff);
        GX_TexCoord2f32(0.0, 0.0);
        GX_Position3f32(ref_width, -ref_height, 0.0);
        GX_Color1u32(0xffffffff);
        GX_TexCoord2f32(1.0, 0.0);
        GX_Position3f32(ref_width, ref_height, 0.0);
        GX_Color1u32(0xffffffff);
        GX_TexCoord2f32(1.0, 1.0);
        GX_Position3f32(-ref_width, ref_height, 0.0);
        GX_Color1u32(0xffffffff);
        GX_TexCoord2f32(0.0, 1.0);
     GX_End();

   fbsel ^= 1;
   GX_DrawDone();
   GX_InvalidateTexAll();

   GX_CopyDisp(xfb[fbsel],GX_TRUE);
   GX_Flush();

#else
   int i, j;
   u32 *curfb;
   u32 *buf;
   int index;

   fbsel ^= 1;
   curfb = xfb[fbsel];
   buf = dispbuffer;

   for (j = 0; j < wii_height; j++)
   {
      for (i = 0; i < wii_width; i++)
      {
         u8 y, cb, cr;
         u8 r, g, b;

         index = ((int)((double)j*(double)vdp2height/(double)wii_height)*vdp2width) + (int)((double)i*(double)vdp2width/(double)wii_width);

         r = buf[index] >> 24;
         g = buf[index] >> 16;
         b = buf[index] >> 8;

         y  = ( 257 * r + 504 * g +  98 * b +  16000) / 1000;
         cb = (-148 * r - 291 * g + 439 * b + 128000) / 1000; //b-y
         cr = ( 439 * r - 368 * g -  71 * b + 128000) / 1000; //r-y

         curfb[0] = (y << 24) | (cb << 16) | (y << 8) | cr;
         curfb++;
      }
   }

#endif

   VIDEO_SetNextFramebuffer (xfb[fbsel]);
   VIDEO_Flush ();
}

void gotoxy(int x, int y)
{
   printf("\033[%d;%dH", y, x);
}

void OnScreenDebugMessage(char *string, ...)
{
#if 0
   va_list arglist;

   va_start(arglist, string);
   gotoxy(0, 1);
   vprintf(string, arglist);
   printf("\n");
   gotoxy(0, 1);
   va_end(arglist);
#endif
}

int ClearMenu(const unsigned long *bmp)
{
   fbsel ^= 1;
   memcpy (xfb[fbsel], bmp, MENU_SIZE * 2);
   VIDEO_SetNextFramebuffer (xfb[fbsel]);
   VIDEO_Flush ();
   return 0;   
}

typedef struct
{
   char *name;
   int (*func)();
} menuitem_struct;

int MenuNone()
{
   return 0;
}

menuitem_struct menuitem[] = {
{ "Start emulation", YuiExec },
{ "Load ISO/CUE", LoadCue },
{ "Settings", Settings},
{ "Load Settings (Each)", load_settings},
{ "Save Settings (Each)", saveSettings},
{ "Remove Setting File", RemoveSettings},
{ "Reset Settings", ResetSettings},
{ "About", About},
{ "Exit", NULL },
{ NULL, NULL }
};

menuitem_struct setmenuitem[] = {
{ "Cartridge", CartSet},
{ "With Bios (Now Without)", BiosWith},
{ "Bios only", BiosOnlySet},
{ "Sound Driver", SoundDriver},
{ "Video Driver", VideoDriver},
{ "M68K Driver", M68KDriver},
#ifdef SCSP_PLUGIN
{ "SCSP Driver", SCSPDriver},
#else
{ "(dummy)", MenuNone},
#endif
{ "Off Frameskip (Now On)", FrameskipOff},
{ "Configure Buttons", ConfigureButtons},
{ "Off Special Color (Now On)", SpecialColorOn},
{ "Set Timing Parameters", SetTimingMenu},
{ "Use One backup ram (Now Use Each)", EachBackupramOn},
{ "Off Threading SCSP2 (Now On)", ThreadingSCSP2On},
{ "Use One Setting (Now Use Each)", EachSettingOn},
{ NULL, NULL }
};

menuitem_struct cartmenuitem[] = {
{ "None", CartSetExec},
{ "Action Relay", CartSetExec},
{ "Backup RAM  4MBIT", CartSetExec},
{ "Backup RAM  8MBIT", CartSetExec},
{ "Backup RAM 16MBIT", CartSetExec},
{ "Backup RAM 32MBIT", CartSetExec},
{ "DRAM  8MBIT", CartSetExec},
{ "DRAM 32MBIT", CartSetExec},
{ "NETLINK (fake)", MenuNone},
{ "ROM 16MBIT", CartSetExec},
{ "Japanese Modem", CartSetExec},
{ NULL, NULL }
};

menuitem_struct sounddriveritem[] = {
{ "None", SoundDriverSetExec},
#ifdef HAVE_LIBSDL
{ "SDL Driver", SoundDriverSetExec},
#else
{ "(dummy)", MenuNone},
#endif
{ "Wii DMA Driver", SoundDriverSetExec},
{ NULL, NULL }
};

menuitem_struct videodriveritem[] = {
{ "None", VideoDriverSetExec},
#ifdef HAVE_LIBGL
//{ "GX (based on OGL) Driver", VideoDriverSetExec},
{ "NO exist OGL Driver", VideoDriverSetExec},
#else
{ "(dummy)", MenuNone},
#endif
{ "Software Driver", VideoDriverSetExec},
{ "Old Software Driver", VideoDriverSetExec},
{ NULL, NULL }
};

menuitem_struct m68kdriveritem[] = {
{ "(dummy)", MenuNone},
{ "C68K Driver", M68KDriverSetExec},
#ifdef HAVE_Q68
{ "Q68 Driver", M68KDriverSetExec},
#else
{ "(dummy)", MenuNone},
#endif
{ NULL, NULL }
};

menuitem_struct scspdriveritem[] = {
{ "(dummy)", MenuNone},
{ "SCSP1 Driver", SCSPDriverSetExec},
{ "SCSP2 Driver", SCSPDriverSetExec},
{ NULL, NULL }
};

menuitem_struct configurebuttonsitem[] = {
{ "Wii Remote", WIIConfigureButtons},
{ "Classic Controller", CLAConfigureButtons},
{ "Game Cube Controller", GCCConfigureButtons},
{ NULL, NULL }
};

menuitem_struct WIIbuttonsitem[] = {
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "", WIISetButtons},
{ "Return", WIISetButtons},
{ NULL, NULL }
};

menuitem_struct CLAbuttonsitem[] = {
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "", CLASetButtons},
{ "Return", CLASetButtons},
{ NULL, NULL }
};

menuitem_struct GCCbuttonsitem[] = {
{ "A                                    ", GCCSetButtons},
{ "B                                    ", GCCSetButtons},
{ "C                                    ", GCCSetButtons},
{ "X                                    ", GCCSetButtons},
{ "Y                                    ", GCCSetButtons},
{ "Z                                    ", GCCSetButtons},
{ "L                                    ", GCCSetButtons},
{ "R                                    ", GCCSetButtons},
{ "Start                                ", GCCSetButtons},
{ "Return", GCCSetButtons},
{ NULL, NULL }
};

menuitem_struct settimingitem[] = {
{ "Smpc Peripheral Timing:              ", SetTiming},
{ "Smpc Other Timing:              ", SetTiming},
{ "Decline Number:   ", SetTiming},
{ "Divide Number for Clock:   ", SetTiming},
{ "Return", SetTiming},
{ NULL, NULL }
};

menuitem_struct aboutmenuitem[] = {
{ "Yabause, a Sega Saturn Emulator", MenuNone},
{ WIIVERSION, MenuNone},
{ "http://wiibrew.org/wiki/Yabause_Wii", MenuNone},
{ "Original Yabause by Yabause Team", MenuNone},
{ "This is unofficial modified ver.", MenuNone},
{ "You must not ask the Yabause Team about this unofficial Wii version.", MenuNone},
{ "Return", NULL},
{ NULL, NULL }
};

void InitMenu()
{
   VIDEO_Init();
   switch(CONF_GetVideo())
   {
      case CONF_VIDEO_NTSC:
         rmode = &TVNtsc480IntDf;
         break;
      case CONF_VIDEO_PAL:
         if ( CONF_GetEuRGB60() > 0 ) {
            rmode = &TVEurgb60Hz480IntDf;
         } else {
            rmode = &TVPal528IntDf;
            IsPal = 1;
         }
         break;
      case CONF_VIDEO_MPAL:
         rmode = &TVMpal480IntDf;
         break;
      default:
         rmode = VIDEO_GetPreferredMode(NULL);
         break;
   }

   if(xfb[0]==NULL)
   xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
   if(xfb[1]==NULL)
   xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
   VIDEO_Configure(rmode);
   VIDEO_ClearFrameBuffer (rmode, xfb[0], COLOR_BLACK);
   VIDEO_ClearFrameBuffer (rmode, xfb[1], COLOR_BLACK);
   VIDEO_SetNextFramebuffer(xfb[0]);
   VIDEO_SetBlack(FALSE);
   VIDEO_Flush();
   VIDEO_WaitVSync();
   if(rmode->viTVMode&VI_NON_INTERLACE) 
      VIDEO_WaitVSync();
   if(_console_buffer != NULL) free(_console_buffer);
   _console_buffer = malloc(288*185*VI_DISPLAY_PIX_SZ);
   __console_init_ex(_console_buffer,176,193,rmode->fbWidth*VI_DISPLAY_PIX_SZ,288,185,288*VI_DISPLAY_PIX_SZ);
}

void SetMenuItem()
{
   if(bioswith)
      setmenuitem[1].name=menuitemwithoutbios;
   else
      setmenuitem[1].name=menuitemwithbios;

   if(frameskipoff)
     setmenuitem[7].name=menuitemonframeskip;
   else
     setmenuitem[7].name=menuitemoffframeskip;

   if(specialcoloron)
     setmenuitem[9].name=menuitemoffspecialcolor;
   else
     setmenuitem[9].name=menuitemonspecialcolor;

   if(eachbackupramon)
     setmenuitem[11].name=menuitemoffeachbackupram;
   else
     setmenuitem[11].name=menuitemoneachbackupram;

   if(threadingscsp2on)
     setmenuitem[12].name=menuitemoffthreadingscsp2;
   else
     setmenuitem[12].name=menuitemonthreadingscsp2;

   if(eachsettingon)
   {
     setmenuitem[13].name=menuitemoffeachsetting;
     if(!first_load)
     {
       menuitem[3].name=menuitemloadsettingeach;
       menuitem[4].name=menuitemsavesettingeach;
     }
     else
     {
       menuitem[3].name=menuitemloadsettingone;
       menuitem[4].name=menuitemsavesettingone;
     }
   }
   else
   {
     setmenuitem[13].name=menuitemoneachsetting;
     menuitem[3].name=menuitemloadsettingone;
     menuitem[4].name=menuitemsavesettingone;
   }

   sprintf(menuitemsmpcperipheraltiming, "Smpc Peripheral Timing: %5d", smpcperipheraltiming);
   settimingitem[0].name = menuitemsmpcperipheraltiming;

   sprintf(menuitemsmpcothertiming, "Smpc Other Timing: %4d", smpcothertiming);
   settimingitem[1].name = menuitemsmpcothertiming;

   sprintf(menuitemdeclinenumber, "Decline Number: %2d", declinenum);
   settimingitem[2].name = menuitemdeclinenumber;

   sprintf(menuitemdividenumberclock, "Divide Number for Clock: %1d", dividenumclock);
   settimingitem[3].name = menuitemdividenumberclock;
}

void DoMenu()
{
   int i;
   int nummenu=0;
   expansion_t exp;
   s8 claX, claY;
   s8 gcX, gcY;
   u32 buttonsDown;
   u32 gcbuttonsDown;
   u32 buttonsDown1;
   u32 gcbuttonsDown1;

   InitMenu();
#ifdef USE_WIIGX
   InitGX();
#endif

   SetMenuItem();

   for (i = 0; menuitem[i].name != NULL; i++)
      nummenu++;

   for (;;)
   {
      VIDEO_WaitVSync();
      WPAD_ScanPads();
      PAD_ScanPads();
      WPAD_Expansion(0, &exp);
      claX = classic_analog_val(&exp, true, false);
      claY = classic_analog_val(&exp, false, false);
      gcX = PAD_StickX(0);
      gcY = PAD_StickY(0);
      buttonsDown = WPAD_ButtonsHeld(0);
      gcbuttonsDown = PAD_ButtonsHeld(0);
      buttonsDown1 = WPAD_ButtonsDown(0);
      gcbuttonsDown1 = PAD_ButtonsDown(0);

      // Get Wii Remote/Keyboard/etc. presses
      if ((buttonsDown & WPAD_CLASSIC_BUTTON_UP) ||
          (buttonsDown & WPAD_BUTTON_RIGHT) ||
          (gcbuttonsDown & PAD_BUTTON_UP) ||
          stick_up(claY, gcY))
      {
         switch (submenuflag) {
           case 0: //main menu
             menuselect--;
             if (menuselect < 0)
               menuselect = nummenu-1;
             break;
           case 1: //file selection menu
             scroll_filelist(-1);
             break;
           case 2: //setting menu
             scroll_settinglist(-1);
             break;
           case 20: //setting cart menu
             scroll_cartlist(-1);
             break;
           case 23: //sound driver menu
             sounddriverselect--;
             if (sounddriverselect < 0)
               sounddriverselect = numsounddriver-1;
             break;
           case 24: //video driver menu
             videodriverselect--;
             if (videodriverselect < 0)
               videodriverselect = numvideodriver-1;
             break;
           case 25: //m68k driver menu
             m68kdriverselect--;
             if (m68kdriverselect < 0)
               m68kdriverselect = numm68kdriver-1;
             break;
           case 26: //scsp driver menu
             scspdriverselect--;
             if (scspdriverselect < 0)
               scspdriverselect = numscspdriver-1;
             break;
           case 27: //configure buttons menu
             configurebuttonsselect--;
             if (configurebuttonsselect < 0)
               configurebuttonsselect = numconfigurebuttons-1;
             break;
           case 29: //set timing menu
             settimingselect--;
             if (settimingselect < 0)
               settimingselect = numsettimings-1;
             break;
           case 270: //WII configure buttons menu
             WIIconfigurebuttonsselect--;
             if (WIIconfigurebuttonsselect < 0)
               WIIconfigurebuttonsselect = numWIIconfigurebuttons-1;
             break;
           case 271: //CLA configure buttons menu
             CLAconfigurebuttonsselect--;
             if (CLAconfigurebuttonsselect < 0)
               CLAconfigurebuttonsselect = numCLAconfigurebuttons-1;
             break;
           case 272: //GCC configure buttons menu
             GCCconfigurebuttonsselect--;
             if (GCCconfigurebuttonsselect < 0)
               GCCconfigurebuttonsselect = numGCCconfigurebuttons-1;
             break;
         }
      }
      else if ((buttonsDown & WPAD_CLASSIC_BUTTON_DOWN) ||
               (buttonsDown & WPAD_BUTTON_LEFT) ||
               (gcbuttonsDown & PAD_BUTTON_DOWN) ||
               stick_down(claY, gcY))
      {          
         switch (submenuflag) {
           case 0: //main menu
             menuselect++;
             if (menuselect == nummenu)
               menuselect = 0;
             break;
           case 1: //file selection menu
             scroll_filelist(1);
             break;
           case 2: //setting menu
             scroll_settinglist(1);
             break;
           case 20: //setting cart menu
             scroll_cartlist(1);
             break;
           case 23: //sound driver menu
             sounddriverselect++;
             if (sounddriverselect == numsounddriver)
               sounddriverselect = 0;
             break;
           case 24: //video driver menu
             videodriverselect++;
             if (videodriverselect == numvideodriver)
               videodriverselect = 0;
             break;
           case 25: //m68k driver menu
             m68kdriverselect++;
             if (m68kdriverselect == numm68kdriver)
               m68kdriverselect = 0;
             break;
           case 26: //scsp driver menu
             scspdriverselect++;
             if (scspdriverselect == numscspdriver)
               scspdriverselect = 0;
             break;
           case 27: //configure buttons menu
             configurebuttonsselect++;
             if (configurebuttonsselect == numconfigurebuttons)
               configurebuttonsselect = 0;
             break;
           case 29: //set timing menu
             settimingselect++;
             if (settimingselect == numsettimings)
               settimingselect = 0;
             break;
           case 270: //WII configure buttons menu
             WIIconfigurebuttonsselect++;
             if (WIIconfigurebuttonsselect == numWIIconfigurebuttons)
               WIIconfigurebuttonsselect = 0;
             break;
           case 271: //CLA configure buttons menu
             CLAconfigurebuttonsselect++;
             if (CLAconfigurebuttonsselect == numCLAconfigurebuttons)
               CLAconfigurebuttonsselect = 0;
             break;
           case 272: //GCC configure buttons menu
             GCCconfigurebuttonsselect++;
             if (GCCconfigurebuttonsselect == numGCCconfigurebuttons)
               GCCconfigurebuttonsselect = 0;
             break;
         }
      }

      if ((buttonsDown1 & WPAD_CLASSIC_BUTTON_A) ||
          (buttonsDown1 & WPAD_BUTTON_A) ||
          (buttonsDown1 & WPAD_BUTTON_2) ||
          (gcbuttonsDown1 & PAD_BUTTON_A))
      {
          switch (submenuflag) {
             case 0: //main menu
               if (menuitem[menuselect].func)
               {
                 if (menuitem[menuselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 1: //file selection menu
                GameExec();
                break;
             case 2: //setting menu
               if (setmenuitem[selectedsetting].func)
               {
                 if (setmenuitem[selectedsetting].func())
                   return;
               }
               else
                 return;
                 break;
             case 20: //setting cart menu
               if (cartmenuitem[selectedcart].func)
               {
                 if (cartmenuitem[selectedcart].func())
                   return;
               }
               else
                 return;
                 break;
             case 23: //sound driver menu
               if (sounddriveritem[sounddriverselect].func)
               {
                 if (sounddriveritem[sounddriverselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 24: //video driver menu
               if (videodriveritem[videodriverselect].func)
               {
                 if (videodriveritem[videodriverselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 25: //m68k driver menu
               if (m68kdriveritem[m68kdriverselect].func)
               {
                 if (m68kdriveritem[m68kdriverselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 26: //scsp driver menu
               if (scspdriveritem[scspdriverselect].func)
               {
                 if (scspdriveritem[scspdriverselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 27: //configure buttons menu
               if (configurebuttonsitem[configurebuttonsselect].func)
               {
                 if (configurebuttonsitem[configurebuttonsselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 29: //set timing menu
               if (settimingitem[settimingselect].func)
               {
                 if (settimingitem[settimingselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 270: //WII configure buttons menu
               if (WIIbuttonsitem[WIIconfigurebuttonsselect].func)
               {
                 if (WIIbuttonsitem[WIIconfigurebuttonsselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 271: //CLA configure buttons menu
               if (CLAbuttonsitem[CLAconfigurebuttonsselect].func)
               {
                 if (CLAbuttonsitem[CLAconfigurebuttonsselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 272: //GCC configure buttons menu
               if (GCCbuttonsitem[GCCconfigurebuttonsselect].func)
               {
                 if (GCCbuttonsitem[GCCconfigurebuttonsselect].func())
                   return;
               }
               else
                 return;
                 break;
             case 3: //about menu
               if (aboutmenuitem[aboutmenuselect].func)
               {
                 if (aboutmenuitem[aboutmenuselect].func())
                   return;
               }
               else
               {
                 submenuflag = 0;
                 menuselect = 1;
                 //return;
               }
               break;
          }
      }

      // Draw menu
      ClearMenu(menu_bmp);

      printf("\033[2J");
      gotoxy(0, 0);
#ifdef AUTOLOADPLUGIN
      if(autoload) 
        printf("Now Loading Game ... \n %s\n", gamefilename);
      else 
        printf("Now Loading Game List ... \n in %s/games/\n", yabause_dir);
#else
      printf("Now Loading Game List ... \n in %s/games/\n", yabause_dir);
#endif
      if(first_readdir)
      {
         Check_Read_Dir();
         first_readdir = 0;
      }
      printf("\033[2J");
      gotoxy(0, 0);
      switch (submenuflag) {
        case 0: //main menu
          // Draw menu items
          for (i = 0; i < nummenu; i++)
          {
            if (menuselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", menuitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", menuitem[i].name);
            }
          }
          break;
        case 1: //file selection menu
          print_filelist();
          break;
        case 2: //setting menu
          print_settinglist();
          break;
        case 20: //setting cart menu
          print_cartlist();
          break;
        case 23: //sound driver menu
          for (i = 0; i < numsounddriver; i++)
          {
            if (sounddriverselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", sounddriveritem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", sounddriveritem[i].name);
            }
          }
          break;
        case 24: //video driver menu
          for (i = 0; i < numvideodriver; i++)
          {
            if (videodriverselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", videodriveritem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", videodriveritem[i].name);
            }
          }
          break;
        case 25: //m68k driver menu
          for (i = 0; i < numm68kdriver; i++)
          {
            if (m68kdriverselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", m68kdriveritem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", m68kdriveritem[i].name);
            }
          }
          break;
        case 26: //scsp driver menu
          for (i = 0; i < numscspdriver; i++)
          {
            if (scspdriverselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", scspdriveritem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", scspdriveritem[i].name);
            }
          }
          break;
        case 27: //configure buttons menu
          for (i = 0; i < numconfigurebuttons; i++)
          {
            if (configurebuttonsselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", configurebuttonsitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", configurebuttonsitem[i].name);
            }
          }
          break;
        case 29: //set timing menu
          for (i = 0; i < numsettimings; i++)
          {
            if (settimingselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", settimingitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", settimingitem[i].name);
            }
          }
          break;
        case 270: //WII configure buttons menu
          for (i = 0; i < numWIIconfigurebuttons; i++)
          {
            if (WIIconfigurebuttonsselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", WIIbuttonsitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", WIIbuttonsitem[i].name);
            }
          }
          break;
        case 271: //CLA configure buttons menu
          for (i = 0; i < numCLAconfigurebuttons; i++)
          {
            if (CLAconfigurebuttonsselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", CLAbuttonsitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", CLAbuttonsitem[i].name);
            }
          }
          break;
        case 272: //GCC configure buttons menu
          for (i = 0; i < numGCCconfigurebuttons; i++)
          {
            if (GCCconfigurebuttonsselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", GCCbuttonsitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", GCCbuttonsitem[i].name);
            }
          }
          break;
        case 3: //about menu
          for (i = 0; i < numaboutmenu; i++)
          {
            if (aboutmenuselect == i) {
              printf("\033[%d;%dm\033[35m", 30+9, 0);
              printf("%s\033[0m\n", aboutmenuitem[i].name);
            } else {
              printf("\033[%d;%dm", 30+7, 0);
              printf("%s\n", aboutmenuitem[i].name);
            }
          }
          break;
      }
#ifdef AUTOLOADPLUGIN
      if(autoload) YuiExec();
#endif

      //usleep(125000); //for too fast moving by stick....but slow!?
      usleep(110000); //for too fast moving by stick
   }
}

int GameExec()
{
   //submenuflag = 20;
   submenuflag = 0;
   menuselect=0;
   printf("\033[2;0H");
   strcpy(isofilename, games_dir);
   strcat(isofilename, "/");
   strcat(isofilename, filelist[selected].filename);
   if(eachsettingon)
   {
     load_settings();
     SetMenuItem();
   }
   menuselect=0;
   return 0;
}
////////



void Check_SD_Init(void)
{
	s32 ret;
	
	//printf("\t[*] Initializing libfat...");
	fflush(stdout);
	ret = sd_init();
	if (!ret) {
		printf(" ERROR! Return to Wii Menu in 5s...");
		sleep(5);
		//Exit_ToWiiMenu();
		exit(0);
	}
	//printf(" OK!\n");
}

int compare(const void *x, const void *y) {
        return strcasecmp( (char *)(((struct file *)x)->filename), (char *)(((struct file *)y)->filename));
}


void Check_Read_Dir(void)
{
	s32 ret;
	
	//printf("\t[*] Reading SD card...");
	fflush(stdout);
	ret = sd_readdir(games_dir, &filelist, &nb_files);
	if (ret < 0) {
		printf(" ERROR! (ret = %d) Return to Wii Menu in 5s...", ret);
		sleep(5);
		//Exit_ToWiiMenu();
		exit(0);
	}
	//printf(" OK!\n");
	qsort(filelist, nb_files, sizeof(struct file), compare);
}

void scroll_filelist(s8 delta)
{
	s32 index;

        display_pos=0;
        display_cnt=0;

	/* No files */
	if (!nb_files)
		return;

	/* Select next entry */
	selected += delta;

	/* Out of the list? */
	if (selected <= -1)
		selected = nb_files - 1;
	if (selected >= nb_files)
		selected = 0;

	/* List scrolling */
	index = (selected - start);
	if (index >= FILES_PER_PAGE)
		start += (index - FILES_PER_PAGE) + 1;
	if (index <= -1)
		start += index;
}

void print_filelist(void)
{
	u32 cnt;
	u32 len;

	//printf("\t[*] Available files on the SD card:\n\n");

	/* No files */
	if (!nb_files) {
		printf("No files available!\n");
		return;
	}

	for (cnt = start; cnt < nb_files; cnt++) {
		/* Files per page limit */
		if ((cnt - start) >= FILES_PER_PAGE)
			break;

		/* Selected file */
		if (cnt == selected) {
                      len = strlen(filelist[cnt].filename);
                      display_cnt++;
                      if((display_cnt>10) && (display_pos < len-35)) display_pos++;
                      if((display_pos >= len-35) && (display_cnt > len-35+10*2))
                      {
                         display_pos=0;
                         display_cnt=0;
                      }
                      (filelist[cnt].havesetting && eachsettingon) ?
                      printf("\033[%d;%dm\033[45m", 30+9, 0):
                      printf("\033[%d;%dm\033[35m", 30+9, 0);
                      if(len > 35)
		         printf("%.35s\033[0m\033[40m\n", filelist[cnt].filename+display_pos);
                      else
		         printf("%.35s\033[0m\033[40m\n", filelist[cnt].filename);
		} else {
                      (filelist[cnt].havesetting && eachsettingon) ?
                      printf("\033[%d;%dm\033[30m\033[47m", 30+7, 0):
                      printf("\033[%d;%dm", 30+7, 0);
                      printf("%.35s\033[0m\033[40m\n", filelist[cnt].filename);
		}
		fflush(stdout);

	}
}

void scroll_cartlist(s8 delta)
{
        s32 index;

        /* Select next entry */
        selectedcart += delta;

        /* Out of the list? */
        if (selectedcart <= -1)
                selectedcart = CART_TYPE_NUM - 1;
        if (selectedcart >= CART_TYPE_NUM)
                selectedcart = 0;

        /* List scrolling */
        index = (selectedcart - startcart);
        if (index >= FILES_PER_PAGE)
                startcart += (index - FILES_PER_PAGE) + 1;
        if (index <= -1)
                startcart += index;
}

void print_cartlist(void)
{
        u32 cnt;

        for (cnt = startcart; cnt < CART_TYPE_NUM; cnt++) {
                /* Files per page limit */
                if ((cnt - startcart) >= FILES_PER_PAGE)
                        break;

                /* Selected file */
                if (cnt == selectedcart) {
                      printf("\033[%d;%dm\033[35m", 30+9, 0);
                      printf("%s\033[0m\n", cartmenuitem[cnt].name);
                } else {
                      printf("\033[%d;%dm", 30+7, 0);
                      printf("%s\n", cartmenuitem[cnt].name);
                }
                fflush(stdout);
        }
}

void scroll_settinglist(s8 delta)
{
        s32 index;

        /* Select next entry */
        selectedsetting += delta;

        /* Out of the list? */
        if (selectedsetting <= -1)
                selectedsetting = SETTING_NUM - 1;
        if (selectedsetting >= SETTING_NUM)
                selectedsetting = 0;

        /* List scrolling */
        index = (selectedsetting - startsetting);
        if (index >= FILES_PER_PAGE)
                startsetting += (index - FILES_PER_PAGE) + 1;
        if (index <= -1)
                startsetting += index;
}

void print_settinglist(void)
{
        u32 cnt;

        for (cnt = startsetting; cnt < SETTING_NUM; cnt++) {
                /* Files per page limit */
                if ((cnt - startsetting) >= FILES_PER_PAGE)
                        break;

                /* Selected file */
                if (cnt == selectedsetting) {
                      printf("\033[%d;%dm\033[35m", 30+9, 0);
                      printf("%s\033[0m\n", setmenuitem[cnt].name);
                } else {
                      printf("\033[%d;%dm", 30+7, 0);
                      printf("%s\n", setmenuitem[cnt].name);
                }
                fflush(stdout);
        }
}

////////
int LoadCue()
{
   submenuflag = 1;
   printf("\033[2;0H");
   gotoxy(0, 0);
   first_load=0;

   return 0;
}


int Settings()
{
   int i;
   submenuflag = 2;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numsetmenu=0;
   for (i = 0; setmenuitem[i].name != NULL; i++)
      numsetmenu++;

   return 0;
}

int CartSet()
{
   int i;
   submenuflag = 20;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numcartmenu=0;
   for (i = 0; cartmenuitem[i].name != NULL; i++)
      numcartmenu++;

   return 0;
}

int CartSetExec()
{
   submenuflag = 0;
   menuselect=0;
   return 0;
}

int BiosWith()
{
   submenuflag = 0;
   menuselect=0;
   if(bioswith)
   {
     setmenuitem[1].name=menuitemwithbios;
     bioswith=0;
   }
   else
   {
     setmenuitem[1].name=menuitemwithoutbios;
     bioswith=1;
   }
   return 0;
}

int BiosOnlySet()
{
   submenuflag = 0;
   menuselect=0;
   isofilename[0]=0;
   setmenuitem[1].name=menuitemwithoutbios;
   bioswith=1;
   return 0;
}

int FrameskipOff()
{
   submenuflag = 0;
   menuselect=0;
   if(frameskipoff)
   {
     setmenuitem[7].name=menuitemoffframeskip;
     frameskipoff=0;
   }
   else
   {
     setmenuitem[7].name=menuitemonframeskip;
     frameskipoff=1;
   }
   return 0;
}

int SpecialColorOn()
{
   submenuflag = 0;
   menuselect=0;
   if(specialcoloron)
   {
     setmenuitem[9].name=menuitemonspecialcolor;
     specialcoloron=0;
   }
   else
   {
     setmenuitem[9].name=menuitemoffspecialcolor;
     specialcoloron=1;
   }
   return 0;
}

int SetTimingMenu()
{
   int i;
   submenuflag = 29;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numsettimings=0;
   for (i = 0; settimingitem[i].name != NULL; i++)
      numsettimings++;

   return 0;
}

int SetTiming()
{
   int i = settimingselect;

   switch(i)
   {
     case 0:
       smpcperipheraltiming += 100;
       if (smpcperipheraltiming > 3200) smpcperipheraltiming = 500;
       sprintf(menuitemsmpcperipheraltiming, "Smpc Peripheral Timing: %5d", smpcperipheraltiming);
       settimingitem[i].name = menuitemsmpcperipheraltiming;
       break;
     case 1:
       smpcothertiming += 10;
       if (smpcothertiming > 1200) smpcothertiming = 1000;
       sprintf(menuitemsmpcothertiming, "Smpc Other Timing: %4d", smpcothertiming);
       settimingitem[i].name = menuitemsmpcothertiming;
       break;
     case 2:
       declinenum += 1;
       if (declinenum > 17) declinenum = 2;
       sprintf(menuitemdeclinenumber, "Decline Number: %2d", declinenum);
       settimingitem[i].name = menuitemdeclinenumber;
       break;
     case 3:
       dividenumclock += 1;
       if (dividenumclock > 9) dividenumclock = 1;
       sprintf(menuitemdividenumberclock, "Divide Number for Clock: %1d", dividenumclock);
       settimingitem[i].name = menuitemdividenumberclock;
       break;
     case 4:
       submenuflag = 0;
       menuselect=0;
       break;
   }

   return 0;
}

int EachBackupramOn()
{
   submenuflag = 0;
   menuselect=0;
   if(eachbackupramon)
   {
     setmenuitem[11].name=menuitemoneachbackupram;
     eachbackupramon=0;
   }
   else
   {
     setmenuitem[11].name=menuitemoffeachbackupram;
     eachbackupramon=1;
   }
   return 0;
}

int ThreadingSCSP2On()
{
   submenuflag = 0;
   menuselect=0;
   if(threadingscsp2on)
   {
     setmenuitem[12].name=menuitemonthreadingscsp2;
     threadingscsp2on=0;
   }
   else
   {
     setmenuitem[12].name=menuitemoffthreadingscsp2;
     threadingscsp2on=1;
   }
   return 0;
}

int EachSettingOn()
{
   submenuflag = 0;
   menuselect=0;
   if(eachsettingon)
   {
     setmenuitem[13].name=menuitemoneachsetting;
     menuitem[3].name=menuitemloadsettingone;
     menuitem[4].name=menuitemsavesettingone;
     eachsettingon=0;
   }
   else
   {
     setmenuitem[13].name=menuitemoffeachsetting;
     if(filelist[selected].filename != NULL)
     {
       menuitem[3].name=menuitemloadsettingeach;
       menuitem[4].name=menuitemsavesettingeach;
     }
     else
     {
       menuitem[3].name=menuitemloadsettingone;
       menuitem[4].name=menuitemsavesettingone;
     }
     eachsettingon=1;
   }
   return 0;
}

int SoundDriver()
{
   int i;
   submenuflag = 23;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numsounddriver=0;
   for (i = 0; sounddriveritem[i].name != NULL; i++)
      numsounddriver++;

   return 0;
}

int VideoDriver()
{
   int i;
   submenuflag = 24;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numvideodriver=0;
   for (i = 0; videodriveritem[i].name != NULL; i++)
      numvideodriver++;

   return 0;
}

int M68KDriver()
{
   int i;
   submenuflag = 25;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numm68kdriver=0;
   for (i = 0; m68kdriveritem[i].name != NULL; i++)
      numm68kdriver++;

   return 0;
}

int SCSPDriver()
{
   int i;
   submenuflag = 26;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numscspdriver=0;
   for (i = 0; scspdriveritem[i].name != NULL; i++)
      numscspdriver++;

   return 0;
}

int ConfigureButtons()
{
   int i;
   submenuflag = 27;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numconfigurebuttons=0;
   for (i = 0; configurebuttonsitem[i].name != NULL; i++)
      numconfigurebuttons++;

   return 0;
}

int WIIConfigureButtons()
{
   int i;
   submenuflag = 270;
   printf("\033[2;0H");
   gotoxy(0, 0);
   for(i = 0 ; i < 9; i++)
   {
      strcpy(menuitemWIIbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemWIIbuttons[i], WIIbuttonsname[num_button_WII[i]]);
      WIIbuttonsitem[i].name = menuitemWIIbuttons[i];
   }

   numWIIconfigurebuttons=0;
   for (i = 0; WIIbuttonsitem[i].name != NULL; i++)
      numWIIconfigurebuttons++;

   return 0;
}

int CLAConfigureButtons()
{
   int i;
   submenuflag = 271;
   printf("\033[2;0H");
   gotoxy(0, 0);
   for(i = 0 ; i < 9; i++)
   {
      strcpy(menuitemCLAbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemCLAbuttons[i], CLAbuttonsname[num_button_CLA[i]]);
      CLAbuttonsitem[i].name = menuitemCLAbuttons[i];
   }

   numCLAconfigurebuttons=0;
   for (i = 0; CLAbuttonsitem[i].name != NULL; i++)
      numCLAconfigurebuttons++;

   return 0;
}
int GCCConfigureButtons()
{
   int i;
   submenuflag = 272;
   printf("\033[2;0H");
   gotoxy(0, 0);
   for(i = 0 ; i < 9; i++)
   {
      strcpy(menuitemGCCbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemGCCbuttons[i], GCCbuttonsname[num_button_GCC[i]]);
      GCCbuttonsitem[i].name = menuitemGCCbuttons[i];
   }

   numGCCconfigurebuttons=0;
   for (i = 0; GCCbuttonsitem[i].name != NULL; i++)
      numGCCconfigurebuttons++;

   return 0;
}

int SoundDriverSetExec()
{
   submenuflag = 0;
   menuselect=0;
#ifndef HAVE_LIBSDL
   if(sounddriverselect==1) sounddriverselect=0;
#endif
   return 0;
}

int VideoDriverSetExec()
{
   submenuflag = 0;
   menuselect=0;
   return 0;
}

int M68KDriverSetExec()
{
   submenuflag = 0;
   menuselect=0;
   return 0;
}

int SCSPDriverSetExec()
{
   submenuflag = 0;
   menuselect=0;
   return 0;
}

int WIISetButtons()
{
   int i = WIIconfigurebuttonsselect;

   if(i==9)
   {
     submenuflag = 0;
     menuselect=0;
     return 0;
   }
   else
   {
      ++num_button_WII[i];
      if (num_button_WII[i] >= 7) num_button_WII[i] = 0;
      strcpy(menuitemWIIbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemWIIbuttons[i], WIIbuttonsname[num_button_WII[i]]);
      WIIbuttonsitem[i].name = menuitemWIIbuttons[i];
   }

   return 0;
}
int CLASetButtons()
{
   int i = CLAconfigurebuttonsselect;

   if(i==9)
   {
     submenuflag = 0;
     menuselect=0;
     return 0;
   }
   else
   {
      ++num_button_CLA[i];
      if (num_button_CLA[i] >= 11) num_button_CLA[i] = 0;
      strcpy(menuitemCLAbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemCLAbuttons[i], CLAbuttonsname[num_button_CLA[i]]);
      CLAbuttonsitem[i].name = menuitemCLAbuttons[i];
   }

   return 0;
}
int GCCSetButtons()
{
   int i = GCCconfigurebuttonsselect;

   if(i==9)
   {
     submenuflag = 0;
     menuselect=0;
     return 0;
   }
   else
   {
      ++num_button_GCC[i];
      if (num_button_GCC[i] >= 10) num_button_GCC[i] = 0;
      strcpy(menuitemGCCbuttons[i], saturnbuttonsname[i]);
      strcat(menuitemGCCbuttons[i], GCCbuttonsname[num_button_GCC[i]]);
      GCCbuttonsitem[i].name = menuitemGCCbuttons[i];
   }

   return 0;
}

int About()
{
   int i;
   submenuflag = 3;
   printf("\033[2;0H");
   gotoxy(0, 0);
   numaboutmenu=0;
   for (i = 0; aboutmenuitem[i].name != NULL; i++)
      numaboutmenu++;

   return 0;
}


static void createXMLSection(const char * name, const char * description)
{
        section = mxmlNewElement(data, "section");
        mxmlElementSetAttr(section, "name", name);
        mxmlElementSetAttr(section, "description", description);
}

static void createXMLSetting(const char * name, const char * description, const char * value)
{
        item = mxmlNewElement(section, "setting");
        mxmlElementSetAttr(item, "name", name);
        mxmlElementSetAttr(item, "value", value);
        mxmlElementSetAttr(item, "description", description);
}


int saveSettings() {
        FILE *fp;
        char temp[256];
        char tempvar[256];
        int i;
        char eachxmlfilename[512];
        char *p_c;

        xml = mxmlNewXML("1.0");
        mxmlSetWrapMargin(0);

        data = mxmlNewElement(xml, "file");

        createXMLSection("Yabause", "Yabause Settings");
        sprintf(temp, "%d", bioswith);
        createXMLSetting("bioswith", "Bios Use", temp);
        sprintf(temp, "%d", selectedcart);
        createXMLSetting("cartridge", "Cartridge Type", temp);
        sprintf(temp, "%d", videodriverselect);
        createXMLSetting("videodriver", "Video Driver", temp);
        sprintf(temp, "%d", sounddriverselect);
        createXMLSetting("sounddriver", "Sound Driver", temp);
        sprintf(temp, "%d", m68kdriverselect);
        createXMLSetting("m68kdriver", "M68K Driver", temp);
        sprintf(temp, "%d", scspdriverselect);
        createXMLSetting("scspdriver", "SCSP Driver", temp);
     if(!eachsettingon || first_load)
     {
        sprintf(temp, "%d", selected);
        createXMLSetting("fileselected", "Selected Game", temp);
     }
        sprintf(temp, "%d", frameskipoff);
        createXMLSetting("frameskipoff", "Autoframeskip Off", temp);
        sprintf(temp, "%d", specialcoloron);
        createXMLSetting("specialcoloron", "Special Color On", temp);
        sprintf(temp, "%d", smpcperipheraltiming);
        createXMLSetting("smpcperipheraltiming", "Smpc Peripheral Timing", temp);
        sprintf(temp, "%d", smpcothertiming);
        createXMLSetting("smpcothertiming", "Smpc Other Timing", temp);
        sprintf(temp, "%d", eachbackupramon);
        createXMLSetting("eachbackupramon", "Each Backup RAM", temp);
        sprintf(temp, "%d", declinenum);
        createXMLSetting("declinenum", "Decline Number", temp);
        sprintf(temp, "%d", dividenumclock);
        createXMLSetting("dividenumclock", "Divide Number for Clock", temp);
        sprintf(temp, "%d", threadingscsp2on);
        createXMLSetting("threadingscsp2on", "Threading SCSP2 On", temp);
// buttons
        for (i = 0; i < 9; i++)
        {
           sprintf(tempvar, "wiibutton_no%d", i);
           sprintf(temp, "%d", num_button_WII[i]);
           createXMLSetting(tempvar, tempvar, temp);
        }
        for (i = 0; i < 9; i++)
        {
           sprintf(tempvar, "classicbutton_no%d", i);
           sprintf(temp, "%d", num_button_CLA[i]);
           createXMLSetting(tempvar, tempvar, temp);
        }
        for (i = 0; i < 9; i++)
        {
           sprintf(tempvar, "gccbutton_no%d", i);
           sprintf(temp, "%d", num_button_GCC[i]);
           createXMLSetting(tempvar, tempvar, temp);
        }

    if(eachsettingon && !first_load)
    {
        strcpy(eachxmlfilename, filelist[selected].filename);
        p_c = strrchr(eachxmlfilename, '.');
        *p_c = '\0';
        strcat(eachxmlfilename, ".xml");
        strcpy(settingpath, saves_dir);
        strcat(settingpath, "/");
        strcat(settingpath, eachxmlfilename);
    }
    else
    {
        strcpy(settingpath, yabause_dir);
        strcat(settingpath, settingfilename);
    }
        fp = fopen(settingpath, "wb");

        if (fp == NULL) {

                printf("File = NULL error\n");
                fclose(fp);
                sleep(5);
                return 1;
        }
        else
        {
                mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
                fclose(fp);
                mxmlDelete(data);
                mxmlDelete(xml);
                menuselect=1;
                Check_Read_Dir();
                return 0;
        }
        return 1;
}

int load_settings()
{
        FILE *fp;
        char tempvar[256];
        int i;
        char eachxmlfilename[512];
        char *p_c;

    if(eachsettingon && !first_load)
    {
#ifdef AUTOLOADPLUGIN
        if(autoload)
          strcpy(eachxmlfilename, gamefilename);
        else
          strcpy(eachxmlfilename, filelist[selected].filename);
#else
        strcpy(eachxmlfilename, filelist[selected].filename);
#endif
        p_c = strrchr(eachxmlfilename, '.');
        *p_c = '\0';
        strcat(eachxmlfilename, ".xml");
        strcpy(settingpath, saves_dir);
        strcat(settingpath, "/");
        strcat(settingpath, eachxmlfilename);
    }
    else
    {
        strcpy(settingpath, yabause_dir);
        strcat(settingpath, settingfilename);
    }

        fp = fopen(settingpath, "rb");
        if (fp == NULL)
        {
                fclose(fp);
                if(!eachsettingon) saveSettings();
                return 0;
        }
        else
        {
                fseek (fp , 0, SEEK_END);
                long settings_size = ftell (fp);
                rewind (fp);

                if (settings_size > 0)
                {
                        xml = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
                        fclose(fp);

                        data = mxmlFindElement(xml, xml, "settings", NULL, NULL,
                        MXML_DESCEND);



                        item = mxmlFindElement(xml, xml, "setting", "name", "bioswith", MXML_DESCEND);
                        if(item) bioswith = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "cartridge", MXML_DESCEND);
                        if(item) selectedcart = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "videodriver", MXML_DESCEND);
                        if(item) videodriverselect = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "sounddriver", MXML_DESCEND);
                        if(item) sounddriverselect = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "m68kdriver", MXML_DESCEND);
                        if(item) m68kdriverselect = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "scspdriver", MXML_DESCEND);
                        if(item) scspdriverselect = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "fileselected", MXML_DESCEND);
                        if(item) selected = atoi(mxmlElementGetAttr(item,"value"));
                        if(selected >= FILES_PER_PAGE) start = selected - FILES_PER_PAGE + 1;

                        item = mxmlFindElement(xml, xml, "setting", "name", "frameskipoff", MXML_DESCEND);
                        if(item) frameskipoff = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "specialcoloron", MXML_DESCEND);
                        if(item) specialcoloron = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "smpcperipheraltiming", MXML_DESCEND);
                        if(item) smpcperipheraltiming = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "smpcothertiming", MXML_DESCEND);
                        if(item) smpcothertiming = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "eachbackupramon", MXML_DESCEND);
                        if(item) eachbackupramon = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "declinenum", MXML_DESCEND);
                        if(item) declinenum = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "dividenumclock", MXML_DESCEND);
                        if(item) dividenumclock = atoi(mxmlElementGetAttr(item,"value"));

                        item = mxmlFindElement(xml, xml, "setting", "name", "threadingscsp2on", MXML_DESCEND);
                        if(item) threadingscsp2on = atoi(mxmlElementGetAttr(item,"value"));
// buttons
                        for (i = 0; i < 9; i++)
                        {
                           sprintf(tempvar, "wiibutton_no%d", i);
                           item = mxmlFindElement(xml, xml, "setting", "name", tempvar, MXML_DESCEND);
                           if(item) num_button_WII[i] = atoi(mxmlElementGetAttr(item,"value"));
                        }
                        for (i = 0; i < 9; i++)
                        {
                           sprintf(tempvar, "classicbutton_no%d", i);
                           item = mxmlFindElement(xml, xml, "setting", "name", tempvar, MXML_DESCEND);
                           if(item) num_button_CLA[i] = atoi(mxmlElementGetAttr(item,"value"));
                        }
                        for (i = 0; i < 9; i++)
                        {
                           sprintf(tempvar, "gccbutton_no%d", i);
                           item = mxmlFindElement(xml, xml, "setting", "name", tempvar, MXML_DESCEND);
                           if(item) num_button_GCC[i] = atoi(mxmlElementGetAttr(item,"value"));
                        }

                        mxmlDelete(data);
                        mxmlDelete(xml);
                        menuselect=1;
                        return 0;
                }
                else
                {
                        printf("Error\n");
                        fclose(fp);
                        sleep(5);
                        return 1;
                }
        }
        return 1;
}
