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

#define DEFAULT_MS              10000
#define SHORT_MS                2000
#define THRESHOLD               5L

#define WM_THREAD_DONE   (WM_APP + 1) // фоновый поток завершился
#define WM_THREAD_POS    (WM_APP + 2) // рассчитанные координаты отправлены гл. окну
#define WM_TRAYICON      (WM_APP + 3)

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
    if (!appState.hwnd) return -1;
    
    CenterWindow(&appState);
    ShowWindow(appState.hwnd, nCmdShow);
    UpdateWindow(appState.hwnd);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }    
    return (int)msg.wParam;
}

void InitTrayIcon(AppState* state) {
    ZeroMemory(&state->nid, sizeof(NOTIFYICONDATAW));

    state->nid.cbSize = sizeof(NOTIFYICONDATAW);
    state->nid.hWnd = state->hwnd;
    state->nid.uID = 1;
    state->nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    state->nid.uCallbackMessage = WM_TRAYICON;

    state->nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(state->nid.szTip, 32, WINDOWS_HEADER);

    Shell_NotifyIconW(NIM_ADD, &state->nid);
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

            InitTrayIcon(appState);
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
                // if(IsIconic(hwnd)){
                //     ShowWindow(hwnd, SW_RESTORE);
                // }
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);                
            } else
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, 1, L"Показать");
                AppendMenuW(hMenu, MF_STRING, 2, L"Выход");

                POINT pt; GetCursorPos(&pt);
                //SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);

                switch (cmd) {
                    case 1: {
                        // if(IsIconic(hwnd)){
                        //     ShowWindow(hwnd, SW_RESTORE);
                        // }
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