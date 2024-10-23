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
	$(BIND)\excep.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\excep.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr 2>nul
	-del $(BIND)\excep.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\excep.exe : excep.cpp firstexc.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\excep.pdb excep.cpp firstexc.cpp \
		/link $(LINKFLAGS) $(LIBS) /subsystem:console /incremental:no /entry:WinMainCRTStartup

$(BIND)\excep.bsc : excep.obj
	bscmake /v /n /o $@ excep.sbr

##############################################################################

test: $(BIND)\excep.exe
	$(BIND)\excep.exe

################################################################# End of File.
