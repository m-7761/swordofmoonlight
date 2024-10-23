##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##
##  Note:
##    syelog, setdll, and withdll must be built first because a number of the
##   other samples depend on them.
##

CLIB=/MT
!include .\common.mak

##############################################################################

all:
	cd "$(MAKEDIR)\syelog"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\simple"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\slept"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\setdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\withdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\cping"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
!ENDIF
	cd "$(MAKEDIR)\disas"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\dtest"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\dumpe"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\dumpi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\einst"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\excep"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
!ENDIF
	cd "$(MAKEDIR)\commem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\findfunc"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\member"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\traceapi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\tracemem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\tracereg"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\traceser"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\tracessl"
#	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\tracetcp"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\tracelnk"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\impmunge"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"

clean:
	cd "$(MAKEDIR)\syelog"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\simple"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\slept"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\setdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\withdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\cping"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
!ENDIF
	cd "$(MAKEDIR)\disas"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\dtest"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\dumpe"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\dumpi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\einst"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\excep"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
!ENDIF
	cd "$(MAKEDIR)\commem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\findfunc"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\member"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\traceapi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\tracemem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\tracereg"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\traceser"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\tracessl"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\tracetcp"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\tracelnk"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\impmunge"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"
	-rmdir lib 2>nul
	-rmdir include 2>nul

test:
	cd "$(MAKEDIR)\syelog"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\simple"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\slept"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\setdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\withdll"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\cping"
#	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
!ENDIF
	cd "$(MAKEDIR)\disas"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\dtest"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\dumpe"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\dumpi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\einst"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
	cd "$(MAKEDIR)\excep"
#	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
!ENDIF
	cd "$(MAKEDIR)\commem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\findfunc"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\member"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\traceapi"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\tracemem"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\tracereg"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\traceser"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\tracessl"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\tracetcp"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\tracelnk"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)\impmunge"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)"

##
################################################################# End of File.
