#pragma once

#include <windows.h>
#include <shellapi.h>

typedef struct State {
    int cxscreen, cyscreen; // размер экрана    
    HWND hwnd;          // дескриптор окна
    HWND hStartButton;  // дескриптор кнопки    
    HANDLE hThread;     // дескриптор потока    
    HANDLE hStopEvent;   // событие остановки потока, если установлено, то поток завершает работу
    NOTIFYICONDATAW nid;   // иконка в tray
} AppState;

#define WM_THREAD_DONE   (WM_APP + 1) // фоновый поток завершился
#define WM_THREAD_POS    (WM_APP + 2) // рассчитанные координаты отправлены гл. окну
#define WM_TRAYICON      (WM_APP + 3)