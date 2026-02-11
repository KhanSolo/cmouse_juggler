#pragma once

#include <windows.h>

typedef struct State {
    int cxscreen, cyscreen; // размер экрана    
    HWND hwnd;          // дескриптор окна
    HWND hStartButton;  // дескриптор кнопки    
    HANDLE hThread;     // дескриптор потока    
    HANDLE hStopEvent;   // событие остановки потока, если установлено, то поток завершает работу
} AppState;
