##############################################################################
##
##  Utility to trace WinSock SSL APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

LIBS=$(LIBS) ws2_32.lib secur32.lib

##############################################################################

all: dirs \
	$(BIND)\tracessl.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\tracessl.bsc
!ENDIF

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\tracessl.dll: tracessl.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\tracessl.pdb tracessl.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\tracessl.bsc : tracessl.obj
	bscmake /v /n /o $@ tracessl.sbr

##############################################################################

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\tracessl.* 2>nul

##############################################################################

test: $(BIND)\tracessl.dll "c:\program files\Internet Explorer\iexplore.exe"
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracessl.dll dynamically using withdll.exe ------------
	@echo.
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo **
	@echo ** Close the Internet Explorer window to continue test.
	@echo **
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo.
	$(BIND)\withdll -d:$(BIND)\tracessl.dll \
		"c:\program files\Internet Explorer\iexplore.exe" "https://www.microsoft.com"
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
