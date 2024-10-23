##############################################################################
##
##  Makefile for Detours.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##
##############################################################################

TARGETOS=WINNT
!include ..\common.mak

##############################################################################
##
.SUFFIXES: .cpp .h .obj
.cpp.obj:
	@$(CC) /nologo $(CFLAGS) /Fo$*.obj /c $*.cpp

##############################################################################
##
LFLAGS=/link $(LINKFLAGS) /incremental:no

##############################################################################

all: dirs \
	$(INCD)\syelog.h		\
	$(LIBD)\syelog.lib	\
	$(BIND)\syelogd.exe	\
	\
	$(BIND)\sltest.exe 	\
	$(BIND)\sltestp.exe 	\
	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\syelogd.bsc	\
	$(BIND)\sltest.bsc 	\
	$(BIND)\sltestp.bsc 	\
!ENDIF

##############################################################################
##
clean:
	-del *.obj *.sbr *~ test.txt 2> nul
	-del $(INCD)\syelog.* 2>nul
	-del $(LIBD)\syelog.* 2>nul
	-del $(BIND)\syelogd.* 2>nul
	-del $(BIND)\sltest.* 2>nul
	-del $(BIND)\sltestp.* 2>nul

##############################################################################

dirs:
	if not exist $(INCD) mkdir $(INCD)
	if not exist $(LIBD) mkdir $(LIBD)
	if not exist $(BIND) mkdir $(BIND)

syelog.obj : syelog.cpp syelog.h
    $(CC) $(CFLAGS) /Zl /Fd$(LIBD)\syelog.pdb /c syelog.cpp

$(INCD)\syelog.h : syelog.h
	copy syelog.h $@

$(LIBD)\syelog.lib : syelog.obj
	lib $(LIBFLAGS) /out:$@ syelog.obj

$(BIND)\sltest.exe: sltest.cpp syelog.h syelog.cpp $(DEPS)
    $(CC) $(CFLAGS) /Fe$@ /Fd$(BIND)\sltest.pdb sltest.cpp \
		/link /incremental:no $(LIBS)

$(BIND)\sltest.bsc : sltest.obj
	bscmake /v /n /o $@ sltest.sbr

$(BIND)\sltestp.exe: sltestp.cpp syelog.h $(DEPS)
    $(CC) $(CFLAGS) /Fe$@ /Fd$(BIND)\sltestp.pdb sltestp.cpp \
		/link /incremental:no

$(BIND)\sltestp.bsc : sltestp.obj
	bscmake /v /n /o $@ sltestp.sbr

$(BIND)\syelogd.exe: syelogd.cpp syelog.h $(DEPS)
    $(CC) $(CFLAGS) /Fe$@ /Fd$(BIND)\syelogd.pdb syelogd.cpp \
		/link /incremental:no ws2_32.lib mswsock.lib advapi32.lib

$(BIND)\syelogd.bsc : syelogd.obj
	bscmake /v /n /o $@ syelogd.sbr

##############################################################################

test: $(BIND)\syelogd.exe $(BIND)\sltest.exe $(BIND)\sltestp.exe
	@echo -------- Logging output to test.txt ------------
	start $(BIND)\syelogd.exe test.txt
	$(BIND)\sleep5.exe 1
	$(BIND)\sltestp.exe 
	$(BIND)\sltest.exe /x
	type test.txt

################################################################# End of File.
