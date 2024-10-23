##############################################################################
##
##  Utility to trace serial (COM1, COM2, etc.) APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

all: dirs \
	$(BIND)\traceser.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\traceser.bsc
!ENDIF

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\traceser.dll: traceser.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\traceser.pdb traceser.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\traceser.bsc : traceser.obj
	bscmake /v /n /o $@ traceser.sbr

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\traceser.* 2>nul

##############################################################################

test: $(BIND)\traceser.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracetcp.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\tracetcp.dll $(BIND)\sleepold.exe
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
