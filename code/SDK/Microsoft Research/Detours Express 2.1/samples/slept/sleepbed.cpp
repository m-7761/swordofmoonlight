//////////////////////////////////////////////////////////////////////////////
//
//  Detour Test Program (sleepbed.cpp of sleepbed.exe)
//
//  Microsoft Research Detours Package, Version 2.1.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

#include <windows.h>
#include <stdio.h>

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

//
///////////////////////////////////////////////////////////////// End of File.

int __cdecl main(void)
{
    int error = 0;

    printf("sleepbed.exe: Starting.\n");
    PVOID pbExeEntry = DetourGetEntryPoint(NULL);
    printf("sleepbed.exe: ExeEntry=%p, DllEntry=%p\n", pbExeEntry);

    Verify("Sleep", (PVOID)Sleep);
    printf("\n");
    fflush(stdout);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueSleep, TimedSleep);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("sleepbed.exe: Detoured Sleep().\n");
    }
    else {
        printf("sleepbed.exe: Error detouring Sleep(): %d\n", error);
        return error;
    }
    fflush(stdout);

    printf("sleepbed.exe: After detour.\n");
    Verify("Sleep", (PBYTE)Sleep);
    printf("\n");
    fflush(stdout);

    printf("sleepbed.exe: Calling Sleep for 1 second.\n");
    Sleep(1000);
    printf("sleepbed.exe: Calling SleepEx for 1 second.\n");
    SleepEx(1000, true);
    printf("sleepbed.exe: Calling Sleep again for 1 second.\n");
    Sleep(1000);
    printf("sleepbed.exe: Calling TimedSleep for 1 second.\n");
    TimedSleep(1000);
    printf("sleepbed.exe: Calling UntimedSleep for 1 second.\n");
    UntimedSleep(1000);
    printf("sleepbed.exe: Done sleeping.\n\n");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueSleep, TimedSleep);
    error = DetourTransactionCommit();
    printf("sleepbed.exe: Removed Sleep() detour (%d), slept %d ticks.\n",
           error, dwSlept);
    fflush(stdout);

    printf("sleepbed.exe: GetSleptTicks() = %d\n\n", GetSleptTicks());
    return error;
}
//
///////////////////////////////////////////////////////////////// End of File.
