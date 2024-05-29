###########################################################################################
##		Created using Monkey Studio v1.8.1.0
##
##	Author    : Filipe AZEVEDO aka Nox P@sNox <pasnox@yabause.org>
##	Project   : yabause
##	FileName  : yabause.pro
##	Date      : 2008-01-28T23:22:47
##	License   : GPL
##	Comment   : Creating using Monkey Studio RAD
##	Home Page   : http://www.monkeystudio.org
##
##	This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
##	WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
###########################################################################################


TEMPLATE	= app
LANGUAGE	= Qt4/C++
TARGET	= yabause
mac:TARGET	= Yabause
CONFIG	+= debug_and_release x86 ppc
QT	+= opengl
INCLUDEPATH	+= . ./ui
LIBS	+= -L../ -lyabause -lSDL -lfat -lwiiuse -lwiikeyboard -lbte -logc -lm -lmxml   -L/c/devkitPro/libogc/lib/wii 
AC_DEFS = -DPACKAGE_NAME=\"yabause\" -DPACKAGE_TARNAME=\"yabause\" -DPACKAGE_VERSION=\"0.9.10\" -DPACKAGE_STRING=\"yabause\ 0.9.10\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DPACKAGE=\"yabause\" -DVERSION=\"0.9.10\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DWORDS_BIGENDIAN=1 -DHAVE_SYS_TIME_H=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_FLOORF=1 -DUNKNOWN_ARCH=1 -DHAVE_C68K=1 -DHAVE_Q68=1 -DUSE_DYNAREC=1 -DGEKKO=1 -DWIIVERSION=\"ver.\ Unofficial\ r2925\ beta26\" -DUSE_WIIGX=1 -DWIICONVERTASM=1 -Dmain=SDL_main -DHAVE_LIBSDL=1 -DEXEC_FROM_CACHE=1 -DOPTIMIZED_DMA=1 -DHAVE_Q68=1 -DQ68_DISABLE_ADDRESS_ERROR=1 -DAUTOLOADPLUGIN=1 -DUSE_SCSP2=1 -DSCSP_PLUGIN=1
CPPFLAGS = -I/c/devkitPro/libogc/include 
QMAKE_CXXFLAGS	+= $$replace( AC_DEFS, "\", "\\\"" ) $$replace( CPPFLAGS, "\", "\\\"" ) $(YTSDEF)
QMAKE_CFLAGS	+= $$replace( AC_DEFS, "\", "\\\"" ) $$replace( CPPFLAGS, "\", "\\\"" ) $(YTSDEF)
mac:QMAKE_CXXFLAGS	+= -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_MAX_ALLOWED
mac:QMAKE_CFLAGS	+= -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_MAX_ALLOWED
RESOURCES	+= resources/resources.qrc
mac:ICON	+= resources/icons/yabause.icns
win32:RC_FILE	+= $$PWD/resources/icons/yabause.rc

BUILD_PATH	= ./build
BUILDER	= GNUMake
COMPILER	= G++
EXECUTE_RELEASE	= yabause
EXECUTE_DEBUG	= yabause_debug

CONFIG(debug, debug|release) {
	#Debug
	CONFIG	+= console
	unix:TARGET	= $$join(TARGET,,,_debug)
	else:TARGET	= $$join(TARGET,,,d)
	unix:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/unix
	win32:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/win32
	mac:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/mac
	UI_DIR	= $${BUILD_PATH}/debug/.ui
	MOC_DIR	= $${BUILD_PATH}/debug/.moc
	RCC_DIR	= $${BUILD_PATH}/debug/.rcc
} else {
	#Release
	unix:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/unix
	win32:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/win32
	mac:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/mac
	UI_DIR	= $${BUILD_PATH}/release/.ui
	MOC_DIR	= $${BUILD_PATH}/release/.moc
	RCC_DIR	= $${BUILD_PATH}/release/.rcc
}

prefix = /usr/local
exec_prefix = $${prefix}
target.path = $${exec_prefix}/bin
INSTALLS += target

FORMS	+= ui/UIYabause.ui \
	ui/UISettings.ui \
	ui/UIAbout.ui \
	ui/UICheats.ui \
	ui/UICheatAR.ui \
	ui/UICheatRaw.ui \
	ui/UIWaitInput.ui \
	ui/UIBackupRam.ui \
	ui/UIPortManager.ui \
	ui/UIPadSetting.ui

HEADERS	+= ui/UIYabause.h \
	YabauseGL.h \
	ui/UISettings.h \
	Settings.h \
	YabauseThread.h \
	QtYabause.h \
	ui/UIAbout.h \
	ui/UICheats.h \
	ui/UICheatAR.h \
	ui/UICheatRaw.h \
	CommonDialogs.h \
	PerQt.h \
	ui/UIWaitInput.h \
	ui/UIBackupRam.h \
	ui/UIPortManager.h \
	ui/UIPadSetting.h \
	VolatileSettings.h \
	Arguments.h

SOURCES	+= main.cpp \
	ui/UIYabause.cpp \
	YabauseGL.cpp \
	ui/UISettings.cpp \
	Settings.cpp \
	YabauseThread.cpp \
	QtYabause.cpp \
	ui/UIAbout.cpp \
	ui/UICheats.cpp \
	ui/UICheatAR.cpp \
	ui/UICheatRaw.cpp \
	CommonDialogs.cpp \
	PerQt.c \
	ui/UIWaitInput.cpp \
	ui/UIBackupRam.cpp \
	ui/UIPortManager.cpp \
	ui/UIPadSetting.cpp \
	VolatileSettings.cpp \
	Arguments.cpp
