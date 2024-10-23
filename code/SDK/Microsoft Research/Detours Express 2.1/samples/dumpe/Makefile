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
	$(BIND)\dumpe.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\dumpe.bsc
!ENDIF

clean:
	-del *.obj *.sbr *~ 2>nul
	-del $(BIND)\dumpe.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\dumpe.exe : dumpe.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\dumpe.pdb dumpe.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no

$(BIND)\dumpe.bsc : dumpe.obj
	bscmake /v /n /o $@ dumpe.sbr

##############################################################################

test: $(BIND)\dumpe.exe
	$(BIND)\dumpe.exe ..\bin\slept.dll

testx: $(BIND)\dumpe.exe
	cd $(MAKEDIR)\..\..\src
	nmake
	cd $(MAKEDIR)
	if exist $(SYSTEMROOT)\system32\browseui.dll $(BIND)\dumpe.exe browseui.dll

################################################################# End of File.
