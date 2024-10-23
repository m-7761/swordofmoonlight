##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!include ..\common.mak

all: dirs \
	$(BIND)\dtarget.dll \
	$(BIND)\dtest.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\dtarget.bsc
	$(BIND)\dtest.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr 2> nul
	-del $(BIND)\dtest.* $(BIND)\dtarget.* 2> nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\dtarget.dll $(BIND)\dtarget.lib: dtarget.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\dtarget.pdb dtarget.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) \
		/export:Target0 \
		/export:Target1 \
		/export:Target2 \
		/export:Target3 \
		/export:Target4 \
		/export:Target5 \
		/export:Target6 \
		/export:Target7 \
		/export:Target8 \
		/export:Target9 \
		/export:Target10 \
		/export:Target11 \
		/export:Target12 \
		/export:Target13 \
		/export:Target14 \
		/export:Target15 \
		/export:Target16 \
		/export:TargetV \
		/export:TargetR \
		$(LIBS)

$(BIND)\dtarget.bsc : dtarget.obj
	bscmake /v /n /o $@ dtarget.sbr

$(BIND)\dtest.exe : dtest.cpp $(BIND)\dtarget.lib $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\dtest.pdb dtest.cpp \
		/link $(LINKFLAGS) $(LIBS) $(BIND)\dtarget.lib \
		/subsystem:console /incremental:no /entry:WinMainCRTStartup

$(BIND)\dtest.bsc : dtest.obj
	bscmake /v /n /o $@ dtest.sbr

##############################################################################

test: $(BIND)\dtarget.dll $(BIND)\dtest.exe
	$(BIND)\dtest.exe

################################################################# End of File.
