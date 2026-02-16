#pragma once

#include <windows.h>
#include "appstate.h"

#define DEFAULT_MS              10000
#define SHORT_MS                2000
#define THRESHOLD               5L

/*==============================
 бизнес-логика движения мыши
==============================*/

static inline BOOL IsCursorIdle(const POINT *cur,
                                const POINT *old)
{
    return labs(cur->x - old->x) < THRESHOLD &&
           labs(cur->y - old->y) < THRESHOLD;
}

DWORD WINAPI MouseMoverThread(LPVOID lpParam) {
    AppState* appState = (AppState*)lpParam;
    POINT oldPos = { 0 };
    BOOL zigzag = TRUE;

    int threadDoneResult = 1; // нормальное завершение (wParam = 1); 0-ошибка

    while (TRUE) {
        POINT curPos;
        if (!GetCursorPos(&curPos)) {
            threadDoneResult = 0; // ошибка (wParam = 0)
            break;
        }

        int sleepMs;
        if (IsCursorIdle(&curPos, &oldPos)) {
            if (appState->cxscreen == 0 || appState->cyscreen == 0) {
                threadDoneResult = 0; // ошибка (wParam = 0)
                break;
            }
            long diff = 2 * (zigzag ? 1 : -1);
            curPos.x = (curPos.x + diff + appState->cxscreen) % (appState->cxscreen);
            curPos.y = (curPos.y + diff + appState->cyscreen) % (appState->cyscreen);
            if(!SetCursorPos((int)curPos.x, (int)curPos.y)){
                threadDoneResult = 0; // ошибка (wParam = 0)
                break;
            }
            zigzag = !zigzag;
            sleepMs = SHORT_MS;
        } else { 
            sleepMs = DEFAULT_MS;
        }

        oldPos.x = curPos.x;
        oldPos.y = curPos.y;
        PostMessage(appState->hwnd, WM_THREAD_POS, (WPARAM)curPos.x, (LPARAM)curPos.y);

        // Sleep, но с возможностью прерывания
        if (WaitForSingleObject(appState->hStopEvent, sleepMs) == WAIT_OBJECT_0)
            break;
    }
    PostMessage(appState->hwnd, WM_THREAD_DONE, threadDoneResult, 0);

    return 0;
}