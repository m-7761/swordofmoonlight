##############################################################################
##
##  Program to test DetourFindFunction.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

##############################################################################

all: dirs \
    $(BIND)\target.dll \
    $(BIND)\extend.dll \
    $(BIND)\findfunc.exe \
    $(BIND)\symtest.exe \
	$(BIND)\dbghelp.dll \
    \
!IF $(DETOURS_SOURCE_BROWSING)==1
    $(BIND)\target.bsc \
    $(BIND)\extend.bsc \
    $(BIND)\findfunc.bsc \
    $(BIND)\symtest.bsc \
!ENDIF

##############################################################################

dirs:
    @if not exist $(BIND) mkdir $(BIND)

$(BIND)\target.dll $(BIND)\target.lib: target.cpp $(DEPS)
    cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\target.pdb target.cpp \
        /link $(LINKFLAGS) /incremental:no /subsystem:console \
        /entry:$(DLLENTRY) \
        /export:Target \
        /base:0x1900000 \
        $(LIBS)

$(BIND)\target.bsc : target.obj
    bscmake /v /n /o $@ target.sbr

$(BIND)\extend.dll $(BIND)\extend.lib: extend.cpp $(DEPS)
    cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\extend.pdb extend.cpp \
        /link $(LINKFLAGS) /incremental:no /subsystem:console \
        /entry:$(DLLENTRY) \
        /export:NullExport \
        /base:0x1a00000 \
        $(LIBS)

$(BIND)\extend.bsc : extend.obj
    bscmake /v /n /o $@ extend.sbr

$(BIND)\findfunc.exe : findfunc.cpp $(BIND)\target.lib $(DEPS)
    cl $(CFLAGS) /Fe$@ /Fd$(BIND)\findfunc.pdb findfunc.cpp \
        /link $(LINKFLAGS) $(LIBS) \
        /subsystem:console /incremental:no /fixed:no $(BIND)\target.lib

$(BIND)\findfunc.bsc : findfunc.obj
    bscmake /v /n /o $@ findfunc.sbr

$(BIND)\symtest.exe : symtest.cpp $(BIND)\target.lib $(DEPS)
    cl $(CFLAGS) /Fe$@ /Fd$(BIND)\symtest.pdb symtest.cpp \
        /link $(LINKFLAGS) $(LIBS) \
        /subsystem:console /incremental:no /fixed:no $(BIND)\target.lib

$(BIND)\symtest.bsc : symtest.obj
    bscmake /v /n /o $@ symtest.sbr

# We try to get the 64-bit dbghelp first because it is a lot more useful.
$(BIND)\dbghelp.dll : {"$(PROGRAMFILES)\Debugging Tools for Windows 64-bit";$(PATH)}dbghelp.dll
	-copy $** $(BIND)\dbghelp.dll 

##############################################################################

clean:
    -del *~ *.obj *.sbr 2>nul
    -del $(BIND)\target.* $(BIND)\extend.* $(BIND)\findfunc.* $(BIND)\symtest.* $(BIND)\dbghelp.dll 2>nul

##############################################################################

verbose: all
    cls
    $(BIND)\symtest.exe

test: all
    @echo -------- Reseting test binaries to initial state. -----------------------
    $(BIND)\setdll.exe -r $(BIND)\findfunc.exe
    @echo.
    @echo -------- Should not load extend.dll--------------------------------------
    $(BIND)\findfunc.exe
    @echo.
    @echo -------- Adding extend.dll to findfunc.exe ------------------------------
    $(BIND)\setdll.exe -d:$(BIND)\extend.dll $(BIND)\findfunc.exe
    @echo.
    @echo -------- Should load extend.dll statically ------------------------------
    $(BIND)\findfunc.exe
    @echo.
    @echo -------- Removing extend.dll from findfunc.exe --------------------------
    $(BIND)\setdll.exe -r $(BIND)\findfunc.exe
    @echo.
    @echo -------- Should not load extend.dll -------------------------------------
    $(BIND)\findfunc.exe
    @echo.
    @echo -------- Should load extend.dll dynamically using withdll.exe -----------
    $(BIND)\withdll.exe -d:$(BIND)\extend.dll $(BIND)\findfunc.exe
    @echo.
    @echo -------- Test completed. ------------------------------------------------

################################################################# End of File.
