##############################################################################
##
##  API Extention to Measure time slept.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

##############################################################################

all: dirs \
	$(BIND)\simple.dll	\
	$(BIND)\sleep5.exe	\
	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\simple.bsc	\
	$(BIND)\sleep5.bsc	\
!ENDIF

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\simple.dll $(BIND)\simple.lib: simple.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\simple.pdb simple.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) \
		/export:TimedSleep \
		$(LIBS)

$(BIND)\simple.bsc : simple.obj
	bscmake /v /n /o $@ simple.sbr

$(BIND)\sleep5.exe : sleep5.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\sleep5.pdb sleep5.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no

$(BIND)\sleep5.bsc : sleep5.obj
	bscmake /v /n /o $@ sleep5.sbr

##############################################################################

clean:
	-del *~ *.obj *.sbr 2>nul
	-del $(BIND)\simple.* 2>nul
	-del $(BIND)\sleep5.* 2>nul

##############################################################################

test: $(BIND)\sleep5.exe $(BIND)\simple.dll
	@echo -------- Reseting test binaries to initial state. ---------------------
	$(BIND)\setdll.exe -r $(BIND)\sleep5.exe
	@echo.
	@echo -------- Should not load simple.dll -----------------------------------
	$(BIND)\sleep5.exe
	@echo.
	@echo -------- Adding simple.dll to sleep5.exe ------------------------------
	$(BIND)\setdll.exe -d:$(BIND)\simple.dll $(BIND)\sleep5.exe
	@echo.
	@echo -------- Should load simple.dll statically ----------------------------
	$(BIND)\sleep5.exe
	@echo.
	@echo -------- Removing simple.dll from sleep5.exe --------------------------
	$(BIND)\setdll.exe -r $(BIND)\sleep5.exe
	@echo.
	@echo -------- Should not load simple.dll -----------------------------------
	$(BIND)\sleep5.exe
    @echo.
	@echo -------- Should load simple.dll dynamically using withdll.exe----------
	$(BIND)\withdll.exe -d:$(BIND)\simple.dll $(BIND)\sleep5.exe
	@echo.

################################################################# End of File.
