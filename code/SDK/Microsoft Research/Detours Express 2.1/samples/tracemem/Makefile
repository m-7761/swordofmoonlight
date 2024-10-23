##############################################################################
##
##  Utility to trace HeapAlloc APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

all: dirs \
	$(BIND)\tracemem.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\tracemem.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\tracemem.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\tracemem.dll: tracemem.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\tracemem.pdb tracemem.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\tracemem.bsc : tracemem.obj
	bscmake /v /n /o $@ tracemem.sbr

##############################################################################

test: $(BIND)\tracemem.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracemem.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\tracemem.dll $(BIND)\sleepold.exe
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
