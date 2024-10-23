##############################################################################
##
##  Utility to trace Win32 APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak
LIBS=$(LIBS) advapi32.lib ole32.lib ws2_32.lib

#LIBS = kernel32.lib uuid.lib advapi32.lib user32.lib ole32.lib detours.lib \
#	gdi32.lib glu32.lib shell32.lib comdlg32.lib mpr.lib  \
#	winspool.lib version.lib urlmon.lib winmm.lib opengl32.lib msvcrt.lib \
#	dbghelp.lib

##############################################################################

all: dirs \
	$(BIND)\traceapi.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\traceapi.bsc
!ENDIF

##############################################################################

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\traceapi.* 2>nul

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\traceapi.dll: traceapi.cpp _win32.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\traceapi.pdb traceapi.cpp \
		/link $(LINKFLAGS) /release /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\traceapi.bsc : traceapi.obj
	bscmake /v /n /o $@ traceapi.sbr

##############################################################################

test: $(BIND)\traceapi.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load traceapi.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\traceapi.dll $(BIND)\sleepold.exe
	@echo -------- Log from syelog -------------
	type test.txt

calc: $(BIND)\traceapi.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /q /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load traceapi.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\traceapi.dll $(SYSTEMROOT)\System32\calc.exe
	@echo -------- Log from syelog -------------
	wc test.txt

explorer: $(BIND)\traceapi.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /q /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load traceapi.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\traceapi.dll $(SYSTEMROOT)\explorer.exe
	@echo -------- Log from syelog -------------
	wc test.txt

wordpad: $(BIND)\traceapi.dll
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /q /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load traceapi.dll dynamically using withdll.exe ------------
	$(BIND)\withdll -d:$(BIND)\traceapi.dll $(PROGRAMFILES)\Windows NT\Accessories\wordpad.exe
	@echo -------- Log from syelog -------------
	wc test.txt

ws: $(BIND)\traceapi.dll
	$(BIND)\withdll -d:$(BIND)\traceapi.dll $(BIND)\WebServiceTester.exe

################################################################# End of File.
