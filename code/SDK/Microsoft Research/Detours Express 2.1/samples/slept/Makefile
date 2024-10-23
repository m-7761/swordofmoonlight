##############################################################################
##
##  API Extension to Measure time slept.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

##############################################################################

all: dirs \
	$(BIND)\slept.dll	\
	$(BIND)\dslept.dll	\
	$(BIND)\sleepold.exe	\
	$(BIND)\sleepnew.exe	\
	$(BIND)\sleepbed.exe	\
	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\slept.bsc	\
	$(BIND)\dslept.bsc	\
	$(BIND)\sleepold.bsc	\
	$(BIND)\sleepnew.bsc	\
	$(BIND)\sleepbed.bsc	\
!ENDIF

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\slept.dll $(BIND)\slept.lib: slept.cpp verify.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\slept.pdb slept.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) \
		/export:TimedSleep \
		/export:UntimedSleep \
		/export:GetSleptTicks \
		$(LIBS)

$(BIND)\slept.bsc : slept.obj
	bscmake /v /n /o $@ slept.sbr

$(BIND)\dslept.dll $(BIND)\dslept.lib: dslept.cpp verify.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\dslept.pdb dslept.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) \
		/export:TimedSleep \
		/export:UntimedSleep \
		/export:GetSleptTicks \
		$(LIBS)

$(BIND)\dslept.bsc : dslept.obj
	bscmake /v /n /o $@ dslept.sbr

$(BIND)\sleepold.exe : sleepold.cpp verify.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\sleepold.pdb sleepold.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no /fixed:no

$(BIND)\sleepold.bsc : sleepold.obj
	bscmake /v /n /o $@ sleepold.sbr

$(BIND)\sleepnew.exe : sleepnew.cpp $(BIND)\slept.lib verify.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\sleepnew.pdb sleepnew.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no /fixed:no $(BIND)\slept.lib

$(BIND)\sleepnew.bsc : sleepnew.obj
	bscmake /v /n /o $@ sleepnew.sbr

$(BIND)\sleepbed.exe : sleepbed.cpp verify.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\sleepbed.pdb sleepbed.cpp \
		/link $(LINKFLAGS) $(LIBS) \
		/subsystem:console /incremental:no /fixed:no

$(BIND)\sleepbed.bsc : sleepbed.obj
	bscmake /v /n /o $@ sleepbed.sbr

##############################################################################

clean:
	-del *~ *.obj *.sbr 2>nul
	-del $(BIND)\slept.* 2>nul
	-del $(BIND)\dslept.* 2>nul
	-del $(BIND)\sleepold.* 2>nul
	-del $(BIND)\sleepnew.* 2>nul
	-del $(BIND)\sleepbed.* 2>nul

##############################################################################

test: all
	@echo -------- Reseting test binaries to initial state. -----------------------
	$(BIND)\setdll.exe -r $(BIND)\sleepold.exe
	@echo.
	@echo -------- Should load detour self ----------------------------------------
	$(BIND)\sleepbed.exe
	@echo.
	@echo -------- Should load slept.dll statically -------------------------------
	$(BIND)\sleepnew.exe
	@echo.
	@echo -------- Should not load slept.dll --------------------------------------
	$(BIND)\sleepold.exe
	@echo.
	@echo -------- Adding slept.dll to sleepold.exe -------------------------------
	$(BIND)\setdll.exe -d:$(BIND)\slept.dll $(BIND)\sleepold.exe
	@echo.
	@echo -------- Should load slept.dll statically -------------------------------
	$(BIND)\sleepold.exe
	@echo.
	@echo -------- Replacing slept.dll with dslept.dll in sleepold.exe ------------
	$(BIND)\setdll.exe -r $(BIND)\sleepold.exe
	$(BIND)\setdll.exe -d:$(BIND)\dslept.dll $(BIND)\sleepold.exe
	@echo.
	@echo -------- Should load dslept.dll instead of slept.dll --------------------
	$(BIND)\sleepold.exe
	@echo.
	@echo -------- Removing dslept.dll from sleepold.exe --------------------------
	$(BIND)\setdll.exe -r $(BIND)\sleepold.exe
	@echo.
	@echo -------- Should not load dslept.dll or slept.dll ------------------------
	$(BIND)\sleepold.exe
	@echo.
	@echo -------- Should load slept.dll dynamically using withdll.exe ------------
	$(BIND)\withdll.exe -d:$(BIND)\slept.dll $(BIND)\sleepold.exe
	@echo.
	@echo -------- Test completed. ------------------------------------------------

################################################################# End of File.
