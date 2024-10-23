##############################################################################
##
##  Utility to trace WinSock TCP APIs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

CLIB=/MT
!include ..\common.mak

LIBS=$(LIBS) ws2_32.lib

##############################################################################

all: dirs \
	$(BIND)\tracetcp.dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\tracetcp.bsc
!ENDIF

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\tracetcp.dll: tracetcp.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\tracetcp.pdb tracetcp.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) /export:NullExport \
		$(LIBS)

$(BIND)\tracetcp.bsc : tracetcp.obj
	bscmake /v /n /o $@ tracetcp.sbr

##############################################################################

clean:
	-del *~ *.obj *.sbr test.txt 2>nul
	-del $(BIND)\tracetcp.* 2>nul

##############################################################################

test: $(BIND)\tracetcp.dll "c:\program files\Internet Explorer\iexplore.exe"
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe /o test.txt
	$(BIND)\sleep5.exe 1
	@echo -------- Should load tracetcp.dll dynamically using withdll.exe ------------
	@echo.
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo **
	@echo ** Close the Internet Explorer window to continue test.
	@echo **
	@echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
	@echo.
	$(BIND)\withdll -d:$(BIND)\tracetcp.dll \
		"c:\program files\Internet Explorer\iexplore.exe" "http://www.microsoft.com"
	@echo -------- Log from syelog -------------
	type test.txt

################################################################# End of File.
