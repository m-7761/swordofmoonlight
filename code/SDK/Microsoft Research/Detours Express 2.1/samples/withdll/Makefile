##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##
!include ..\common.mak

##############################################################################

all: dirs	\
	$(BIND)\withdll.exe	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\withdll.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr 2>nul
	-del $(BIND)\withdll.* 2>nul

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\withdll.exe : withdll.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\withdll.pdb withdll.cpp \
		/link $(LINKFLAGS) $(LIBS) /subsystem:console /incremental:no

$(BIND)\withdll.bsc : withdll.obj
	bscmake /v /n /o $@ withdll.sbr

##############################################################################

test: $(BIND)\withdll.exe
	$(BIND)\withdll.exe -d:$(BIND)\slept.dll $(BIND)\sleepold.exe

debug: $(BIND)\withdll.exe
	windbg  -c ".srcfix;l+s;l+t" -o $(BIND)\withdll.exe -d:$(BIND)\slept.dll $(BIND)\sleepold.exe

################################################################# End of File.
