######################################################################
##
##  Hook test for glFinish
##

CLIB=/MT
!include ..\common.mak

##############################################################################

all: dirs \
	$(BIND)\oglsimple.dll	\
	$(BIND)\testogl.exe	\
	\
!IF $(DETOURS_SOURCE_BROWSING)==1
	$(BIND)\oglsimple.bsc	\
	$(BIND)\testogl.bsc	\
!ENDIF

##############################################################################

dirs:
	if not exist $(BIND) mkdir $(BIND)

$(BIND)\oglsimple.dll $(BIND)\oglsimple.lib: oglsimple.cpp $(DEPS)
	cl /LD $(CFLAGS) /Fe$@ /Fd$(BIND)\oglsimple.pdb oglsimple.cpp \
		/link $(LINKFLAGS) /incremental:no /subsystem:console \
		/entry:$(DLLENTRY) \
		/export:hookedGlFinish \
		$(LIBS) opengl32.lib

$(BIND)\oglsimple.bsc : oglsimple.obj
	bscmake /v /n /o $@ oglsimple.sbr

$(BIND)\testogl.exe : testogl.cpp $(DEPS)
	cl $(CFLAGS) /Fe$@ /Fd$(BIND)\testogl.pdb testogl.cpp \
		/link $(LINKFLAGS) $(LIBS) opengl32.lib \
		/subsystem:console /incremental:no

$(BIND)\testogl.bsc : testogl.obj
	bscmake /v /n /o $@ testogl.sbr

##############################################################################

clean:
	-del *~ *.obj *.sbr 2>nul
	-del $(BIND)\oglsimple.* 2>nul
	-del $(BIND)\testogl.* 2>nul

##############################################################################

test: $(BIND)\testogl.exe $(BIND)\oglsimple.dll
	@echo -------- Reseting test binaries to initial state. ---------------------
	$(BIND)\setdll.exe -r $(BIND)\testogl.exe
	@echo.
	@echo -------- Should not load oglsimple.dll -----------------------------------
	$(BIND)\testogl.exe
	@echo.
	@echo -------- Adding oglsimple.dll to testogl.exe ------------------------------
	$(BIND)\setdll.exe -d:$(BIND)\oglsimple.dll $(BIND)\testogl.exe
	@echo.
	@echo -------- Should load oglsimple.dll statically ----------------------------
	$(BIND)\testogl.exe
	@echo.
	@echo -------- Removing oglsimple.dll from testogl.exe --------------------------
	$(BIND)\setdll.exe -r $(BIND)\testogl.exe
	@echo.
	@echo -------- Should not load oglsimple.dll -----------------------------------
	$(BIND)\testogl.exe
	@echo.
	@echo -------- Should load oglsimple.dll dynamically using withdll.exe----------
	$(BIND)\withdll.exe -d:$(BIND)\oglsimple.dll $(BIND)\testogl.exe
	@echo.

################################################################# End of File.
