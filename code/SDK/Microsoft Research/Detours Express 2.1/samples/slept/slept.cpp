//////////////////////////////////////////////////////////////////////////////
//
//  Detour Test Program (slept.cpp of slept.dll)
//
//  Microsoft Research Detours Package, Version 2.1.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#include <stdio.h>
#include <windows.h>
#include "detours.h"
#include "slept.h"

#include "verify.cpp"

static BOOL fBroke = FALSE;
static LONG dwSlept = 0;
static VOID (WINAPI * TrueSleep)(DWORD dwMilliseconds) = Sleep;

VOID WINAPI UntimedSleep(DWORD dwMilliseconds)
{
    TrueSleep(dwMilliseconds);
}

VOID WINAPI TimedSleep(DWORD dwMilliseconds)
{
    DWORD dwBeg = GetTickCount();
    TrueSleep(dwMilliseconds);
    DWORD dwEnd = GetTickCount();

    if (!fBroke) {
        fBroke = TRUE;
        // DebugBreak();
    }

    InterlockedExchangeAdd(&dwSlept, dwEnd - dwBeg);
}

DWORD WINAPI GetSleptTicks(VOID)
{
    return dwSlept;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        printf("slept.dll: Starting.\n");
        PVOID pbExeEntry = DetourGetEntryPoint(NULL);
        PVOID pbDllEntry = DetourGetEntryPoint(hinst);
        printf("slept.dll: ExeEntry=%p, DllEntry=%p\n", pbExeEntry, pbDllEntry);

        Verify("Sleep", (PVOID)Sleep);
        printf("\n");
        fflush(stdout);

        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueSleep, TimedSleep);
        error = DetourTransactionCommit();

        if (error == NO_ERROR) {
            printf("slept.dll: Detoured Sleep().\n");
        }
        else {
            printf("slept.dll: Error detouring Sleep(): %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueSleep, TimedSleep);
        error = DetourTransactionCommit();
        printf("slept.dll: Removed Sleep() detour (%d), slept %d ticks.\n",
               error, dwSlept);
        fflush(stdout);
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.
