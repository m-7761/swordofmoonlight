##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!include ..\common.mak

all: dirs \
	$(BIND)\disas.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\disas.bsc
!ENDIF

clean:
	-del *~ *.obj *.sbr *.lst 2>nul
	-del $(BIND)\disas.* 2> nul 

dirs:
	if not exist $(BIND) mkdir $(BIND)

!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
disasm.obj : x86.cpp
	cl $(CFLAGS) /Fe$@ /FAcs /Fax86.lst /Fd$(BIND)\disas.pdb /Fodisasm.obj /c x86.cpp
!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "X64"
disasm.obj : x64.asm
	$(ASM) $(AFLAGS) /Fodisasm.obj x64.asm
!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "IA64"
disasm.obj : ia64.asm
	$(ASM) $(AFLAGS) -o disasm.obj ia64.asm
!ENDIF

$(BIND)\disas.exe : disas.cpp disasm.obj $(DEPS)
	cl $(CFLAGS) /Fe$@ /FAcs /Fadisas.lst /Fd$(BIND)\disas.pdb disas.cpp disasm.obj \
		/link $(LINKFLAGS) $(LIBS) /subsystem:console /incremental:no /entry:WinMainCRTStartup

$(BIND)\disas.bsc : disas.obj
	bscmake /v /n /o $@ disas.sbr

##############################################################################

test: $(BIND)\disas.exe
	$(BIND)\disas.exe

################################################################# End of File.
