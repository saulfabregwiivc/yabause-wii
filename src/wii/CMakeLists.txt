project(yabause-wii)

if(NOT WII)
    return()
endif()

#set(yabause_wii_SOURCES keys.h menu.h perwii.c perwii.h sndwii.c sndwii.h yui.c)
set(yabause_wii_SOURCES keys.h menu.h iowii.c iowii.h perwii.c perwii.h sndwii.c sndwii.h yui.c)

add_executable(yabause-wii ${yabause_wii_SOURCES})
#set_target_properties(yabause-wii PROPERTIES LINK_FLAGS -L/opt/devkitpro/libogc/lib/wii/)
#set_target_properties(yabause-wii PROPERTIES LINK_FLAGS -L$ENV{DEVKITPRO}/libogc/lib/wii/)
set_target_properties(yabause-wii PROPERTIES LINK_FLAGS "-DGEKKO -mcpu=750 -meabi -mhard-float -fsigned-char -fomit-frame-pointer -ffast-math -funroll-loops -fauto-inc-dec -finline-functions -L$ENV{DEVKITPRO}/libogc/lib/wii/")
#set_target_properties(yabause-wii PROPERTIES LINK_FLAGS "-DGEKKO -flto -mcpu=750 -meabi -mhard-float -fsigned-char -fomit-frame-pointer -ffast-math -funroll-loops -fauto-inc-dec -finline-functions -L$ENV{DEVKITPRO}/libogc/lib/wii/")
set_target_properties(yabause-wii PROPERTIES OUTPUT_NAME yabause.elf)
#target_link_libraries(yabause-wii yabause -lfat -lwiiuse -lbte -logc -lm)
target_link_libraries(yabause-wii yabause -lSDL -lfat -lwiiuse -lwiikeyboard -lbte -logc -lm -lmxml)
#
add_definitions(-DWIIVERSION=\"ver.\ Unofficial\ r2926\ beta26\")
#add_definitions(-DWORDS_BIGENDIAN=1)
add_definitions(-DREENTRANT_SYSCALLS_PROVIDED=1)
#
add_definitions(-DUSE_WIIGX=1)
add_definitions(-DWIIAUDIO_THREAD=0)
add_definitions(-DHIGHRESO=0)
add_definitions(-DWIICONVERTASM=1)
if(YAB_WANT_SDL)
    add_definitions(-Dmain=SDL_main)
endif(YAB_WANT_SDL)
#
add_definitions(-DHAVE_Q68=1)
add_definitions(-DQ68_DISABLE_ADDRESS_ERROR=1)
#
add_definitions(-DUSE_SCSP2=1)
#
add_definitions(-DAUTOLOADPLUGIN=1)
#
