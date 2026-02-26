#pragma once

#include <windows.h>
#include "appstate.h"

#define TIMER_CLOCK_ID  1
static int timer_clock_current_timer_interval = 1;
const int TIMER_CLOCK_INTERVAL_MS = 1000; // раз в секунду

const wchar_t* months[] = {
    L"января", L"февраля", L"марта", L"апреля",
    L"мая", L"июня", L"июля", L"августа",
    L"сентября", L"октября", L"ноября", L"декабря"
};

const wchar_t* days[] = {
    L"воскресенье", L"понедельник", L"вторник", 
    L"среда", L"четверг", L"пятница", L"суббота"
};

static void ProcessTimerClock(AppState *appState) {
    SYSTEMTIME *pst = &appState->st;
    GetLocalTime(pst);
    OutputDebug(L"WM_TIMER st: %ld:%ld:%ld.%ld", pst->wHour, pst->wMinute, pst->wSecond, pst->wMilliseconds);

    HWND hwnd = appState->hwnd;

    if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
        InvalidateRect(hwnd, NULL, TRUE);        

        // 1. Блокируем рисование
        SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
        
        // 2. Обновляем все тексты
        wchar_t timeBuf[10] = {0,}, dateBuf[32] = {0,};
        swprintf(timeBuf, sizeof(timeBuf) / sizeof(timeBuf[0]), L"%02d:%02d:%02d", pst->wHour, pst->wMinute, pst->wSecond);
        SetDlgItemTextW(hwnd, IDC_LABEL_CLOCK, timeBuf);

        WORD dow = pst->wDayOfWeek;
        const wchar_t* dayOfWeek = (dow < 0 || dow > 6) ? L"" : days[dow];

        const wchar_t* month = (pst->wMonth - 1) < 0 ? L"" : months[pst->wMonth - 1];
        swprintf(dateBuf, sizeof(dateBuf) / sizeof(dateBuf[0]), L"%ls, %02d %ls %d", dayOfWeek, pst->wDay, month, pst->wYear);
        SetDlgItemTextW(hwnd, IDC_LABEL_CALENDAR, dateBuf);
        
        // 3. Включаем рисование + мгновенная перерисовка
        SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }

    BOOL isNeedToAdjustTimer = timer_clock_current_timer_interval != TIMER_CLOCK_INTERVAL_MS 
                                ||
                                pst->wMilliseconds > TIMER_CLOCK_INTERVAL_MS / 2;
    if (isNeedToAdjustTimer) {
        KillTimer(hwnd, TIMER_CLOCK_ID);
        int adjustment = TIMER_CLOCK_INTERVAL_MS - pst->wMilliseconds;
        timer_clock_current_timer_interval = TIMER_CLOCK_INTERVAL_MS + (
            adjustment > (TIMER_CLOCK_INTERVAL_MS / 2) ? 0 : adjustment
        );
        SetTimer(hwnd, TIMER_CLOCK_ID, timer_clock_current_timer_interval, NULL);

        OutputDebug(L"WM_TIMER SetTimer %ld", timer_clock_current_timer_interval);
    }
}