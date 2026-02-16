#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "includes/appstate.h"
#include "includes/wndutils.h"
#include "includes/mouse_mover.h"

#define WINDOWS_WIDTH           360
#define WINDOWS_HEIGHT          150
const wchar_t WINDOWS_HEADER[]  = L"Жонглёр";

#define BTN_START_WIDTH         280
#define BTN_START_HEIGHT        40
const wchar_t BTN_START_TEXT[]  = L"Старт";
const wchar_t BTN_STOP_TEXT[]   = L"Стоп";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*==============================
        entry point
==============================*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    AppState appState = {0};

    GetResolution(&appState);
    CreateMainWindow(&appState, hInstance, WINDOWS_HEADER, WINDOWS_WIDTH, WINDOWS_HEIGHT, WindowProc);
    if (!appState.hwnd) return -1;
    
    ChangeWindowPosition(&appState);
    ShowWindow(appState.hwnd, nCmdShow);
    UpdateWindow(appState.hwnd);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }    
    return (int)msg.wParam;
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

            CreateStartButton(appState, 30, 50, BTN_START_WIDTH, BTN_START_HEIGHT, BTN_START_TEXT);
            InitTrayIcon(appState, WINDOWS_HEADER);
        break;
        
        case WM_COMMAND:
            if ((HWND)lParam == appState->hStartButton) {
                if (!appState->hThread) {                    
                    ResetEvent(appState->hStopEvent); // разрешаем работу
                    HANDLE hThread = CreateThread(NULL, 0, MouseMoverThread, appState, 0, NULL);
                    if (hThread){                        
                        appState->hThread = hThread;
                        SetWindowText(appState->hStartButton, BTN_STOP_TEXT);
                    }
                } else {
                    SetEvent(appState->hStopEvent); // сигналим остановку
                }
            }
        break;

        case WM_THREAD_DONE: { // Обработка завершения потока (параметр wParam: 0=ошибка, 1=нормальное завершение)        
            SetEvent(appState->hStopEvent);
            SetWindowText(appState->hStartButton, BTN_START_TEXT);
            
            static const wchar_t err[] = L"Жонглёр (ошибка доступа)";
            SetWindowTextW(hwnd, (wParam == 0) ? err : WINDOWS_HEADER);

            if (appState->hThread) {
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

        case WM_TRAYICON: {
            if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);                
            } else
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, 1, L"Показать");
                AppendMenuW(hMenu, MF_STRING, 2, L"Выход");

                POINT pt; GetCursorPos(&pt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);

                switch (cmd) {
                    case 1: {
                        ShowWindow(hwnd, SW_SHOW);
                        SetForegroundWindow(hwnd);   
                        break;
                    }
                    case 2: DestroyWindow(hwnd); break;
                }

                DestroyMenu(hMenu);
            }
            break;
        }        
    
        case WM_DESTROY:
            SetEvent(appState->hStopEvent);
            CloseHandle(appState->hStopEvent);
            appState->hStopEvent = NULL;
            Shell_NotifyIconW(NIM_DELETE, &appState->nid);
            PostQuitMessage(0);
        break;

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
        return 0;
     
        default: return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
