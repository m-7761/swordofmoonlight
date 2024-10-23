//////////////////////////////////////////////////////////////////////////////
//
//  Detour Test Program (extend.cpp of extend.dll)
//
//  Microsoft Research Detours Package, Version 2.1.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  An example dynamically detouring a function.
//
#include <stdio.h>
#include <windows.h>
#include "detours.h"

static LONG nExtends = 0;
static LONG nInterns = 0;

static DWORD (WINAPI * TrueTarget)(DWORD dwCount) = NULL;
static DWORD (WINAPI * TrueHidden)(DWORD dwCount) = NULL;
static int (WINAPI * TrueWinMain)(HINSTANCE hInstance,
                                  HINSTANCE hPrevInstance,
                                  LPSTR lpCmdLine,
                                  int nCmdShow) = NULL;

// Every detour DLL needs at least one export for use by setdll.exe or
// withdll.exe.  The export is never called, just referenced to force load.
VOID NullExport()
{
}

// Extend is a detour for Target.
static DWORD WINAPI Extend(DWORD dwCount)
{
    InterlockedIncrement(&nExtends);

    printf("extend.dll: Extend    (%d) -> %d.\n", dwCount, dwCount + 1000);
    dwCount = TrueTarget(dwCount + 1000);
    printf("extend.dll: Extend    (.....) -> %d.\n", dwCount);
    return dwCount;
}

// Intern is a detour for Hidden.
static DWORD WINAPI Intern(DWORD dwCount)
{
    InterlockedIncrement(&nInterns);

    printf("extend.dll:    Intern (%d) -> %d.\n", dwCount, dwCount + 10);
    dwCount = TrueHidden(dwCount + 10);
    printf("extend.dll:    Intern (.....) -> %d.\n", dwCount);
    return dwCount;
}

static int WINAPI ExtendWinMain(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               LPSTR lpCmdLine,
                               int nCmdShow)
{
    // We couldn't call LoadLibrary in DllMain, so our functions here.
    LONG error;

    // We separate out the functions in the export table (Target)
    // from the ones that require debug symbols (Hidden).
    TrueTarget =
        (DWORD (WINAPI *)(DWORD))
        DetourFindFunction("target.dll", "Target");
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueTarget, Extend);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("extend.dll: Detoured Target().\n");
    }
    else {
        printf("extend.dll: Error detouring Target(): %d\n", error);
    }

    // Now try to detour the functions requiring debug symbols.
    TrueHidden =
        (DWORD (WINAPI *)(DWORD))
        DetourFindFunction("target.dll", "Hidden");
    if (TrueHidden == NULL) {
        error = GetLastError();
        printf("extend.dll: TrueHidden = %p (error = %d)\n", TrueHidden, error);
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueHidden, Intern);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("extend.dll: Detoured Hidden().\n");
    }
    else {
        printf("extend.dll: Error detouring Hidden(): %d\n", error);
    }

    // Now let the application start executing.
    printf("extend.dll: Calling WinMain\n");
    fflush(stdout);

    return TrueWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        printf("extend.dll: Starting.\n");
        fflush(stdout);

        DetourRestoreAfterWith();

        // NB: DllMain can't call LoadLibrary, so we hook the app entry point.

        TrueWinMain = (int (WINAPI *)(HINSTANCE, HINSTANCE, LPSTR, int))
            DetourGetEntryPoint(NULL);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueWinMain, ExtendWinMain);
        error = DetourTransactionCommit();

        if (error == NO_ERROR) {
            printf("extend.dll: Detoured WinMain().\n");
        }
        else {
            printf("extend.dll: Error detouring WinMain(): %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        // Detach functions found from the export table.
        if (TrueTarget != NULL) {
            DetourDetach(&(PVOID&)TrueTarget, (PVOID)Extend);
        }

        // Detach functions found from debug symbols.
        if (TrueHidden != NULL) {
            DetourDetach(&(PVOID&)TrueHidden, (PVOID)Intern);
        }

        // Detach the entry point.
        DetourDetach(&(PVOID&)TrueWinMain, ExtendWinMain);
        error = DetourTransactionCommit();

        printf("extend.dll: Removed Target() detours (%d), %d/%d calls.\n",
               error, nExtends, nInterns);

        fflush(stdout);
    }
    return TRUE;
}
//
///////////////////////////////////////////////////////////////// End of File.
