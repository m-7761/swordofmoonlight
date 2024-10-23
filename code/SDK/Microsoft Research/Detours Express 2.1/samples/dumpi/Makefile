##############################################################################
##
##  Makefile for Detours Test Programs - Dump Imports
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!include ..\common.mak

all: dirs \
	$(BIND)\dumpi.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\dumpi.bsc \
!ENDIF

clean:
	-del *.obj *.sbr *~ 2>nul
	-del $(BIND)\dumpi.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\dumpi.exe : dumpi.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\dumpi.pdb dumpi.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no

$(BIND)\dumpi.bsc : dumpi.obj
	bscmake /v /n /o $@ dumpi.sbr

##############################################################################

test: $(BIND)\dumpi.exe
	$(BIND)\dumpi.exe $(BIND)\slept.dll $(BIND)\sleepold.exe

################################################################# End of File.
