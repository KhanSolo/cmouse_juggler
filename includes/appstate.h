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
    HWND hClockText;        // часы (STATIC)
    HANDLE hMouseMoverThread;     // дескриптор потока

    HANDLE hMouseMoverStopEvent;   // событие остановки потока, если установлено, то поток завершает работу
    NOTIFYICONDATAW nid;   // иконка в tray

    TimerState timers[2];
    SYSTEMTIME st;
} AppState;

#define WM_THREAD_DONE   (WM_APP + 1) // фоновый поток завершился
#define WM_THREAD_POS    (WM_APP + 2) // рассчитанные координаты отправлены гл. окну
#define WM_TRAYICON      (WM_APP + 3)