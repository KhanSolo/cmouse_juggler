#pragma once

#include <windows.h>

typedef struct ManualResetEventSlim {
    volatile LONG signalled;
    HANDLE hEvent;
} ManualResetEventSlim;

BOOL MRES_Init(ManualResetEventSlim * mres, BOOL state) {
    if (!mres) return FALSE;
    mres->signalled =  state ? 1L : 0L;
    mres->hEvent = CreateEventW(
        NULL,
        TRUE, // manual
        state,
        NULL
    );
    return mres->hEvent != NULL;
}

BOOL MRES_Set(ManualResetEventSlim * mres) {
    if (!mres) return FALSE;
    LONG prev = InterlockedCompareExchange(&mres->signalled, 1, 0);
    if (prev == 1) return TRUE;
    return SetEvent(mres->hEvent) != 0;
}

BOOL MRES_Reset(ManualResetEventSlim * mres) {
    if (!mres) return FALSE;
    InterlockedExchange(&mres->signalled, 0);
    return ResetEvent(mres->hEvent) != 0;
}

BOOL MRES_Wait(ManualResetEventSlim * mres, DWORD dwMilliseconds) {
    if (!mres) return FALSE;
    if (!mres->hEvent) return FALSE;
    if (InterlockedCompareExchange(&mres->signalled, 0, 0) == 1) return TRUE;
    DWORD result = WaitForSingleObject(mres->hEvent, dwMilliseconds);
    return (result == WAIT_OBJECT_0);
}