/////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (x86.asm of disas.exe)
//
//  Microsoft Research Detours Package, Version 2.1.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

static int value = 0;

extern "C" void __declspec(naked) TestCodes()
{
    __asm {
// Each instruction is proceeded by an "int 3".
        int 3;
        mov     ecx, eax;
        int 3;
        mov     ebx, 0ffff000eh;
        int 3;
        call    ebx;
        int 3;
        call    dword ptr [eax];
        int 3;
        call    dword ptr [ebx];
        int 3;
        call    dword ptr [ecx];
        int 3;
        call    dword ptr [edx];
        int 3;
        jmp     dword ptr [eax];
        int 3;
        jmp     dword ptr [ebx];
        int 3;
        jmp     dword ptr [ecx];
        int 3;
        jmp     dword ptr [edx];
        int 3;
        call    ecx;
        int 3;
        call    eax;
        int 3;
        mov     ebx, 0ffff000eh;
        int 3;
        push    eax;
        int 3;
        call    ebx;
        int 3;
        cmp     ebx, 0;
        int 3;
        cmp     byte ptr [value], 77h;
        int 3;
        cmp     dword ptr [value], 77h;
        int 3;
        cmp     dword ptr [value], 77777777h;

// The list is terminated by an "int 3".
        int 3;
        int 3;
        ret;
    }
}
