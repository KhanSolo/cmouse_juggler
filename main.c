#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "includes/appstate.h"
#include "includes/wndutils.h"

#define WINDOWS_WIDTH           360
#define WINDOWS_HEIGHT          150
const wchar_t WINDOWS_HEADER[]  = L"Жонглёр";

#define BTN_START_WIDTH         280
#define BTN_START_HEIGHT        40
const wchar_t BTN_START_TEXT[]  = L"Старт";
const wchar_t BTN_STOP_TEXT[]   = L"Стоп";

#define THREAD_WAIT_TIMEOUT     30000
#define DEFAULT_MS              10000
#define SHORT_MS                2000
#define THRESHOLD               5L

#define WM_THREAD_DONE   (WM_APP + 1) // фоновый поток завершился
#define WM_THREAD_POS    (WM_APP + 2) // рассчитанные координаты отправлены гл. окну

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI MouseMoverThread(LPVOID lpParam);

/*==============================
        entry point
==============================*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    AppState appState = {0};

    GetResolution(&appState);
    CreateMainWindow(&appState, hInstance, WINDOWS_HEADER, WINDOWS_WIDTH, WINDOWS_HEIGHT, WindowProc);
    if (!appState.hwnd) return 0;
    
    CenterWindow(&appState);
    ShowWindow(appState.hwnd, nCmdShow);
    UpdateWindow(appState.hwnd);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }    
    return 0;
}

/*==============================
 обработчик событий главного окна
==============================*/

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AppState* appState;
    if (uMsg == WM_NCCREATE){ // Создание неклиентской области, это событие произойдёт до WM_CREATE
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            AppState* state = (AppState*)cs->lpCreateParams;
            state->hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
    } else {
        appState = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    switch (uMsg) {
        case WM_CREATE:
            appState->hStopEvent = CreateEventW(
                NULL,   // default security
                TRUE,   // manual-reset
                TRUE,   // initially signaled (не запущен)
                NULL    // lpName
            );            
            DWORD dwStyle= WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
            if(NULL == appState->hStopEvent) {
                dwStyle |= WS_DISABLED;
            } 
            appState->hStartButton = CreateWindowW(
                L"BUTTON", BTN_START_TEXT, dwStyle,
                30, 30, BTN_START_WIDTH, BTN_START_HEIGHT,
                hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
            );
        break;
        
        case WM_COMMAND:
            if ((HWND)lParam == appState->hStartButton) {
                if (!appState->hThread) {                    
                    HANDLE hThread = CreateThread(NULL, 0, MouseMoverThread, appState, 0, NULL);
                    if (hThread){
                        ResetEvent(appState->hStopEvent); // разрешаем работу
                        appState->hThread = hThread;
                        SetWindowText(appState->hStartButton, BTN_STOP_TEXT);
                    }
                } else {
                    SetEvent(appState->hStopEvent); // сигналим остановку
                    WaitForSingleObject(appState->hThread, THREAD_WAIT_TIMEOUT);
                    CloseHandle(appState->hThread);
                    appState->hThread = NULL;
                    SetWindowText(appState->hStartButton, BTN_START_TEXT);
                }
            }
        break;

        case WM_THREAD_DONE: { // Обработка завершения потока (параметр wParam: 0=ошибка, 1=нормальное завершение)        
            SetEvent(appState->hStopEvent);
            SetWindowText(appState->hStartButton, BTN_START_TEXT);
            
            static const wchar_t err[] = L"Жонглёр (ошибка доступа)";
            SetWindowTextW(hwnd, (wParam == 0) ? err : WINDOWS_HEADER);

            if (appState->hThread) {
                WaitForSingleObject(appState->hThread, THREAD_WAIT_TIMEOUT);
                CloseHandle(appState->hThread);
                appState->hThread = NULL;
            }
        }
        break;

        case WM_THREAD_POS: { // событие вывода координат в заголовок
                wchar_t buffer[32];
                swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"Координаты: %ldx%ld", (long)wParam, (long)lParam);
                SetWindowText(hwnd, buffer);
        } break;
    
        case WM_DESTROY:
            SetEvent(appState->hStopEvent);
            if (appState->hThread) {
                WaitForSingleObject(appState->hThread, THREAD_WAIT_TIMEOUT);
                CloseHandle(appState->hThread);
                appState->hThread = NULL;
            }
            CloseHandle(appState->hStopEvent);
            appState->hStopEvent = NULL;
            PostQuitMessage(0);
        break;
     
        default: return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

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

    while (TRUE) {
        // проверяем сигнал остановки 
        // если событие установлено (SetEvent(hStopEvent) в другом потоке)
        // то выходим из цикла
        if (WaitForSingleObject(appState->hStopEvent, 0) == WAIT_OBJECT_0)
            break;

        POINT curPos;
        if (!GetCursorPos(&curPos)) {
            PostMessage(appState->hwnd, WM_THREAD_DONE, 0, 0); // ошибка (wParam = 0)
            break;
        }

        int sleepMs;
        if (IsCursorIdle(&curPos, &oldPos)) {
            long diff = 2 * (zigzag ? 1 : -1);
            int cx = appState->cxscreen + 1, cy = appState->cyscreen + 1;
            curPos.x = (curPos.x + diff + cx) % (cx);
            curPos.y = (curPos.y + diff + cy) % (cy);
            if(!SetCursorPos((int)curPos.x, (int)curPos.y)){
                PostMessage(appState->hwnd, WM_THREAD_DONE, 0, 0); // ошибка (wParam = 0)
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
    PostMessage(appState->hwnd, WM_THREAD_DONE, 1, 0); // нормальное завершение (wParam = 1)

    return 0;
}