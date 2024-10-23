##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!include ..\common.mak

CFLAGS = $(CFLAGS) \
		/Fd$(BIND)\cping.pdb \
		/DWIN32_LEAN_AND_MEAN /DCONST_VTABLE \
		/D_WIN32_WINNT=0x0400 /DCOBJMACROS \
		-DWIN32 -DNT -DDEBUG -DUNICODE -D_UNICODE

C__FLAGS=-DENTRY_PREFIX=iping_ -DREGISTER_PROXY_DLL 
CPPFLAGS=

##############################################################################

.SUFFIXES: .c .cpp .h .idl .obj .res .rc
.c.obj:
	@$(CC) /nologo $(CFLAGS) $(C__FLAGS) /Fo$*.obj /W3 /c $*.c 
.cpp.obj:
	@$(CC) /nologo $(CFLAGS) $(CPPFLAGS) /Fo$*.obj /c $*.cpp
.rc.res:
	@rc /Fo$*.res $*.rc

##############################################################################
##
C__FLAGS=-DENTRY_PREFIX=iping_ -DREGISTER_PROXY_DLL 
CPPFLAGS=


LINKFLAGS=/nologo /nod /debug /machine:IX86 /subsystem:windows /incremental:no
MIDLFLAGS=/nologo /Oif /no_format_opt /no_robust /win32

OBJS = \
	cping.obj	\
	\
	iping_i.obj	\
	iping_p.obj	\
	iping_d.obj	\

LIBS=\
	$(LIBS) \
	uuid.lib		\
	ole32.lib		\
	rpcrt4.lib		\
	msvcrt.lib		\
	advapi32.lib	\
	wsock32.lib		\

##############################################################################

all: dirs \
	$(BIND)\cping.exe	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\cping.bsc
!ENDIF

##############################################################################

clean:
	-del iping.h *.c *.obj *.sbr *~ 2>nul
	-del $(BIND)\cping.* 2>nul

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\cping.bsc : $(OBJS)
	bscmake /v /n /o $@ $(OBJS:.obj=.sbr)

$(BIND)\cping.exe : $(OBJS) $(DEPS)
	cl $(CFLAGS) /Fe$@ $(OBJS) /link $(LINKFLAGS) \
		/subsystem:console $(LIBS)

cping.obj: cping.cpp iping.h

##############################################################################
##
iping.h iping_d.c iping_i.c iping_p.c : iping.idl
	midl $(MIDLFLAGS) /prefix all iping_ /dlldata iping_d.c iping.idl

iping_i.obj: iping_i.c
iping_p.obj: iping_p.c iping.h
iping_d.obj: iping_d.c

##############################################################################

test: $(BIND)\cping.exe
	start $(BIND)\cping.exe /s
	$(BIND)\cping.exe /p localhost

################################################################# End of File.
