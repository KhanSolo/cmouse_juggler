#pragma once

#include <windows.h>
#include <shellapi.h>

typedef struct TimerState {
    int timerId;
    int interval;
    BOOL enabled;
} TimerState;

typedef struct State {
    int cxscreen, cyscreen; // размер экрана    
    HWND hwnd;              // дескриптор окна
    HWND hStartButton;      // дескриптор кнопки Старт/Стоп
    HWND hClockLabel;        // часы (STATIC)
    HWND hCalendarLabel;        // календарь (STATIC)

    HANDLE hMouseMoverThread;     // дескриптор потока
    HANDLE hMouseMoverStopEvent;   // событие остановки потока, если установлено, то поток завершает работу

    NOTIFYICONDATAW nid;   // иконка в tray
    TimerState timers[2]; // 1й таймер для часов и календаря, 2й таймер для mouse mover
    SYSTEMTIME st;

    HFONT hClockFont;
    HFONT hCalendarFont;
} AppState;

#define WM_THREAD_DONE   (WM_APP + 1) // фоновый поток завершился
#define WM_THREAD_POS    (WM_APP + 2) // рассчитанные координаты отправлены гл. окну
#define WM_TRAYICON      (WM_APP + 3)