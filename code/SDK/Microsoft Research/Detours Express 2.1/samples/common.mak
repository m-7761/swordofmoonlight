##############################################################################
##
##  Common makefile for Detours test programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!IF "$(DETOURS_SOURCE_BROWSING)" == ""
DETOURS_SOURCE_BROWSING = 0
!ENDIF

############################################## Determine Processor Build Type.
##
!IF "$(DETOURS_TARGET_PROCESSOR)" == ""
DETOURS_TARGET_PROCESSOR = $(PROCESSOR_ARCHITEW6432)
!ENDIF
!IF "$(DETOURS_TARGET_PROCESSOR)" == ""
DETOURS_TARGET_PROCESSOR = $(PROCESSOR_ARCHITECTURE)
!ENDIF

!IF "$(DETOURS_TARGET_PROCESSOR)" == "x86"
DETOURS_TARGET_PROCESSOR=X86
!ENDIF

!IF "$(DETOURS_TARGET_PROCESSOR)" == "AMD64"
#!MESSAGE Building for 64-bit AMD64.
DETOURS_TARGET_PROCESSOR=X64
!ENDIF

!IF "$(DETOURS_TARGET_PROCESSOR)" == "X64"
#!MESSAGE Building for 64-bit X64.
DETOURS_SOURCE_BROWSING = 0
!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "IA64"
#!MESSAGE Building for 64-bit IA64.
!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
#!MESSAGE Building for 32-bit X86.
!ELSE
!MESSAGE Note: To select the target processor architecture set either
!MESSAGE       PROCESSOR_ARCHITECTURE or DETOURS_TARGET_PROCESSOR.
!MESSAGE
!ERROR Unknown target processor: $(DETOURS_TARGET_ARCHITECTURE)
!ENDIF

##############################################################################

!IFNDEF CLIB
CLIB=/MD
!ENDIF

DTRLIBD = ..\..\lib
DTRINCD = ..\..\include

LIBD = ..\lib
BIND = ..\..\bin
INCD = ..\include

AFLAGS=/nologo /Zi /c /Fl
CFLAGS=/nologo /Zi $(CLIB) /Gm- /W4 /WX /O1
!IF $(DETOURS_SOURCE_BROWSING)==1
CFLAGS=$(CFLAGS) /FR
!ELSE
CFLAGS=$(CFLAGS) "/I$(DTRINCD)" "/I$(INCD)"
!ENDIF
#/D_CRT_INSECURE_DEPRECATE

LIBFLAGS=/nologo
LINKFLAGS=/release

!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"

ASM=ml
AFLAGS=$(AFLAGS) /DDETOURS_X86=1
CFLAGS=$(CFLAGS) /Gs /DDETOURS_X86=1 /D_X86_
LIBFLAGS=$(LIBFLAGS) /debug
DLLENTRY=_DllMainCRTStartup@12
LINKFLAGS=$(LINKFLAGS) /machine:x86

!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "X64"

ASM=ml64
AFLAGS=$(AFLAGS) /DDETOURS_X64=1
CFLAGS=$(CFLAGS) /D_WIN64 /DDETOURS_X64=1 /D_AMD64_ /Wp64
LIBFLAGS=$(LIBFLAGS) /machine:amd64 /debug
DLLENTRY=_DllMainCRTStartup
LINKFLAGS=$(LINKFLAGS) /machine:amd64

!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "IA64"

ASM=ias
AFLAGS=-F COFF32_PLUS
CFLAGS=$(CFLAGS) /D_WIN64 /DDETOURS_IA64=1 /D_IA64_ /wd4163
LIBFLAGS=$(LIBFLAGS) /debug
DLLENTRY=_DllMainCRTStartup
LINKFLAGS=$(LINKFLAGS) /machine:ia64

!ENDIF

LIBS = "$(LIBD)\syelog.lib" "$(DTRLIBD)\detours.lib" "$(DTRLIBD)\detoured.lib" \
        kernel32.lib gdi32.lib user32.lib shell32.lib
DEPS = "$(LIBD)\syelog.lib" "$(DTRLIBD)\detours.lib" "$(DTRLIBD)\detoured.lib"

##
################################################################# End of File.
