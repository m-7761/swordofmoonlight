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
	$(BIND)\setdll.exe	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\setdll.bsc
!ENDIF

##############################################################################

clean:
	-del *.obj *.sbr *~ 2>nul
	-del $(BIND)\setdll.* 2>nul

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\setdll.exe : setdll.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\setdll.pdb setdll.cpp \
		/link $(LINKFLAGS) $(LIBS) /subsystem:console /incremental:no

$(BIND)\setdll.bsc : setdll.obj
	bscmake /v /n /o $@ setdll.sbr

##############################################################################

test: $(BIND)\setdll.exe 
	@echo -------- Reseting test binaries to initial state. -----------------------
	$(BIND)\setdll.exe -d:$(BIND)\slept.dll $(BIND)\sleepold.exe
	@echo -------- Should load slept.dll statically -------------------------------
	$(BIND)\sleepold.exe
	@echo -------- Reseting test binaries to initial state. -----------------------
	$(BIND)\setdll.exe -r $(BIND)\sleepold.exe
	@echo -------- Should not load slept.dll --------------------------------------
	$(BIND)\sleepold.exe

################################################################# End of File.
