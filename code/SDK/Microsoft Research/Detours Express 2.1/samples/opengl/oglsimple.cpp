//////////////////////////////////////////////////////////////////////////////
//
//  Module:     oglsimple.dll
//
//  This DLL is based on the sample simple.dll. A detour is inserted for
//  the OpenGL glFinish function.
//
#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include "detours.h"

static void (WINAPI * trueGlFinish)(void) = glFinish;

void WINAPI hookedGlFinish(void)
{
    printf("oglsimple.dll: hookedGlFinish Starting.\n");
    fflush(stdout);

    trueGlFinish();

    printf("oglsimple.dll: hookedGlFinish done.\n");
    fflush(stdout);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        printf("oglsimple.dll: Starting.\n");
        fflush(stdout);

        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)trueGlFinish, hookedGlFinish);
        error = DetourTransactionCommit();

        if (error == NO_ERROR)
	{
            printf("oglsimple.dll: Detoured glFinish().\n");
        }
        else
	{
            printf("oglsimple.dll: Error detouring glFinish(): %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)trueGlFinish, hookedGlFinish);
        error = DetourTransactionCommit();

        printf("oglsimple.dll: Removed detour glFinish() (result=%d)\n",
               error);
        fflush(stdout);
    }

    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.
