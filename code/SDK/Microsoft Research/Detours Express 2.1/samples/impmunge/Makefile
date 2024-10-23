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
	$(BIND)\impmunge.exe	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\impmunge.bsc
!ENDIF

##############################################################################

clean:
	-del *.obj *.sbr *.exe* *~ 2>nul
	-del $(BIND)\impmunge.* 2>nul

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\impmunge.exe : impmunge.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\impmunge.pdb impmunge.cpp \
		/link $(LINKFLAGS) $(LIBS) imagehlp.lib /subsystem:console /incremental:no

$(BIND)\impmunge.bsc : impmunge.obj
	bscmake /v /n /o $@ impmunge.sbr

##############################################################################

test: $(BIND)\impmunge.exe
	$(BIND)\impmunge.exe /m /o:test.exe.1 $(BIND)\impmunge.exe
	$(BIND)\impmunge.exe /m /l- /o:test.exe.2 test.exe.1
	$(BIND)\impmunge.exe /m /l- /o:test.exe.3 test.exe.2
	$(BIND)\impmunge.exe /m /l- /o:test.exe.4 test.exe.3
	$(BIND)\impmunge.exe /l test.exe.4
	$(BIND)\impmunge.exe /r /l- /o:test.exe.0 test.exe.4
	$(BIND)\impmunge.exe /l test.exe.0

################################################################# End of File.
