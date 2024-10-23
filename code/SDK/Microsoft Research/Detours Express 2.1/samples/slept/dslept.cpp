//////////////////////////////////////////////////////////////////////////////
//
//  Detour Test Program (dslept.cpp of dslept.dll)
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
#include "slept.h"

#include "verify.cpp"

LONG dwSlept = 0;

static VOID (WINAPI * TrueSleep)(DWORD dwMilliseconds) = Sleep;
static VOID (WINAPI * TrueSleepEx)(DWORD dwMilliseconds, BOOL bAlertable) = NULL;
static int (WINAPI * TrueWinMain)(HINSTANCE hInstance,
                                  HINSTANCE hPrevInstance,
                                  LPSTR lpCmdLine,
                                  int nCmdShow) = NULL;
static int (WINAPI * RawWinMain)(HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR lpCmdLine,
                                 int nCmdShow) = NULL;

VOID WINAPI UntimedSleep(DWORD dwMilliseconds)
{
    TrueSleep(dwMilliseconds);
}

VOID WINAPI TimedSleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
    DWORD dwBeg = GetTickCount();
    TrueSleepEx(dwMilliseconds, bAlertable);
    DWORD dwEnd = GetTickCount();

    InterlockedExchangeAdd(&dwSlept, dwEnd - dwBeg);
}

VOID WINAPI TimedSleep(DWORD dwMilliseconds)
{
    // NB: Without this operation, Detours will think the call to UntimedSleep
    // is a DLL Import and detour UntimedSleep instead of TimedSleep.
    InterlockedExchangeAdd(&dwSlept, 0);

    UntimedSleep(dwMilliseconds);

    // NB: Sleep calls SleepEx, so all timing is done there.
}

DWORD WINAPI GetSleptTicks(VOID)
{
    return dwSlept;
}

int WINAPI TimedWinMain(HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine,
                        int nCmdShow)
{
    // We couldn't call LoadLibrary in DllMain,
    // so we detour SleepEx here...
    LONG error;

    TrueSleepEx = (VOID (WINAPI *)(DWORD, BOOL))
        DetourFindFunction("kernel32.dll", "SleepEx");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueSleepEx, TimedSleepEx);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("dslept.dll: Detoured SleepEx().\n");
    }
    else {
        printf("dslept.dll: Error detouring SleepEx(): %d\n", error);
    }

    Verify("SleepEx", (PVOID)SleepEx);
    printf("\n");
    fflush(stdout);

    printf("dslept.dll: Calling WinMain\n");
    fflush(stdout);

    return TrueWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        printf("dslept.dll: Starting.\n");
        Verify("Sleep", (PVOID)Sleep);
        Verify("SleepEx", (PVOID)SleepEx);
        printf("\n");
        fflush(stdout);

        DetourRestoreAfterWith();

        // NB: DllMain can't call LoadLibrary, so we hook the app entry point.
        TrueWinMain =
            (int (WINAPI *)(HINSTANCE, HINSTANCE, LPSTR, int))
            DetourGetEntryPoint(NULL);
        RawWinMain = TrueWinMain;

        Verify("WinMain", RawWinMain);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueSleep, TimedSleep);
        DetourAttach(&(PVOID&)TrueWinMain, TimedWinMain);
        error = DetourTransactionCommit();

        Verify("WinMain after attach", RawWinMain);
        Verify("WinMain trampoline", TrueWinMain);

        printf("dslept.dll: TimedSleep = %p\n", TimedSleep);
        printf("dslept.dll: UntimedSleep = %p\n", UntimedSleep);

        if (error == NO_ERROR) {
            printf("dslept.dll: Detoured Sleep().\n");
        }
        else {
            printf("dslept.dll: Error detouring Sleep(): %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        if (TrueSleepEx != NULL) {
            DetourDetach(&(PVOID&)TrueSleepEx, (PVOID)TimedSleepEx);
        }
        DetourDetach(&(PVOID&)TrueSleep, TimedSleep);
        DetourDetach(&(PVOID&)TrueWinMain, TimedWinMain);
        error = DetourTransactionCommit();

        printf("dslept.dll: Removed Sleep() detours (%d), slept %d ticks.\n",
               error, dwSlept);

        fflush(stdout);
    }
    return TRUE;
}
//
///////////////////////////////////////////////////////////////// End of File.
