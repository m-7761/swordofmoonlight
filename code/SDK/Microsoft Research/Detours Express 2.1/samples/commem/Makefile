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
	$(BIND)\commem.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\commem.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr 2> nul
	-del $(BIND)\commem.* 2> nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\commem.exe : commem.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\commem.pdb commem.cpp \
		/link $(LINKFLAGS) $(LIBS) ole32.lib /subsystem:console /incremental:no

$(BIND)\commem.bsc : commem.obj
	bscmake /v /n /o $@ commem.sbr

##############################################################################

test: $(BIND)\commem.exe
	@echo.
	$(BIND)\commem.exe
	@echo.

################################################################# End of File.
