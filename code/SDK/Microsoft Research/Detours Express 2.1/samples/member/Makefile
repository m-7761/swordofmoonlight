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
	$(BIND)\member.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\member.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr 2> nul
	-del $(BIND)\member.* 2> nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\member.exe : member.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\member.pdb member.cpp \
		/link $(LINKFLAGS) $(LIBS) /subsystem:console /incremental:no

$(BIND)\member.bsc : member.obj
	bscmake /v /n /o $@ member.sbr

##############################################################################

test: $(BIND)\member.exe
	@echo.
	$(BIND)\member.exe
	@echo.

################################################################# End of File.
