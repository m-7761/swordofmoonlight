##############################################################################
##
##  Utility to trace Dynamic Linking.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak
LIBS=$(LIBS)

##############################################################################

all: dirs \
	$(BIND)\tracelnk.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\tracelnk.bsc
!ENDIF

##############################################################################

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\tracelnk.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\tracelnk.dll: tracelnk.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\tracelnk.pdb tracelnk.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\tracelnk.bsc : tracelnk.obj
	bscmake /v /n /o $@ tracelnk.sbr

##############################################################################

test: $(BIND)\tracelnk.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracelnk.dll dynamically using withdll.exe ------------
	@echo.
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo **
	@echo ** Close the NotePad window to continue test.
	@echo **
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo.
	$(BIND)\withdll -d:$(BIND)\tracelnk.dll $(SYSTEMROOT)\system32\notepad.exe
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
