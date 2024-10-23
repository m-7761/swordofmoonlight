##############################################################################
##
##  Utility to registry and file access APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

LIBS=$(LIBS) advapi32.lib

all: dirs \
	$(BIND)\tracereg.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\tracereg.bsc
!ENDIF

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\tracereg.dll: tracereg.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\tracereg.pdb tracereg.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\tracereg.bsc : tracereg.obj
	bscmake /v /n /o $@ tracereg.sbr

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\tracereg.* 2>nul

##############################################################################

test: $(BIND)\tracereg.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracereg.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\tracereg.dll $(BIND)\sleepold.exe
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
