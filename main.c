#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "includes/appstate.h"
#include "includes/wndutils.h"
#include "includes/mouse_mover_thread.h"
#include "includes/time_calculator_timer.h"

#define WINDOWS_WIDTH           360
#define WINDOWS_HEIGHT          170
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

    AppState appState = {
        .timers = { 
            {   .timerId = TIMER_CLOCK_ID, .interval = 0, .enabled = FALSE },
            {   .timerId = TIMER_MOUSE_ID, .interval = 0, .enabled = FALSE }
        },
        .hClockFont = CreateNewFont(36),
        .hCalendarFont = CreateNewFont(20)
    };

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

static AppState* SaveAppStateForWindow(HWND hwnd, UINT uMsg, LPARAM lParam) {
    if (uMsg == WM_NCCREATE) { // Создание неклиентской области, это событие произойдёт до WM_CREATE
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            AppState* appState = (AppState*)cs->lpCreateParams;
            appState->hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)appState);
            return appState;
    } else {
        return (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AppState* appState = SaveAppStateForWindow(hwnd, uMsg, lParam);
    
    switch (uMsg) {
        case WM_CREATE: {
            SYSTEMTIME *pst = &appState->st;
            GetLocalTime(pst);
            WORD interval = TIMER_CLOCK_INTERVAL_MS - pst->wMilliseconds;
            SetTimer(hwnd, TIMER_CLOCK_ID, interval, NULL);

            OutputDebug(L"WM_CREATE interval: %ld", interval);

            appState->hMouseMoverStopEvent = CreateEventW(
                NULL,   // default security
                TRUE,   // manual-reset
                TRUE,   // initially signaled (не запущен)
                NULL    // lpName
            );
            CreateClockText(appState, 30, 10, BTN_START_WIDTH, BTN_START_HEIGHT, L"Часы");
            CreateCalendarText(appState, 30, 50, BTN_START_WIDTH, BTN_START_HEIGHT, L"Календарь");
            CreateStartButton(appState, 30, 90, BTN_START_WIDTH, BTN_START_HEIGHT, BTN_START_TEXT);
            InitTrayIcon(appState, WINDOWS_HEADER);
        } break;

        case WM_CTLCOLORSTATIC: {
            if ((HWND)lParam == GetDlgItem(hwnd, IDC_LABEL_CLOCK)) {
                SetBkMode((HDC)wParam, TRANSPARENT);
                SetTextColor((HDC)wParam, RGB(50, 150, 255));
                return (LRESULT)GetStockObject(NULL_BRUSH);
            } else
            if ((HWND)lParam == GetDlgItem(hwnd, IDC_LABEL_CALENDAR)) {
                SetBkMode((HDC)wParam, TRANSPARENT);
                SetTextColor((HDC)wParam, RGB(50, 150, 255));
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
        }break;
        
        case WM_COMMAND: {
            if ((HWND)lParam == appState->hStartButton) {
                if (!appState->hMouseMoverThread) {                    
                    ResetEvent(appState->hMouseMoverStopEvent); // разрешаем работу
                    HANDLE hMouseMoverThread = CreateThread(NULL, 0, MouseMoverThread, appState, 0, NULL);
                    if (hMouseMoverThread){                        
                        appState->hMouseMoverThread = hMouseMoverThread;
                        SetWindowText(appState->hStartButton, BTN_STOP_TEXT);
                    }
                } else {
                    SetEvent(appState->hMouseMoverStopEvent); // сигналим остановку
                }
            }
        } break;

        case WM_THREAD_DONE: { // Обработка завершения потока (параметр wParam: 0=ошибка, 1=нормальное завершение)        
            SetEvent(appState->hMouseMoverStopEvent);
            SetWindowText(appState->hStartButton, BTN_START_TEXT);
            
            static const wchar_t err[] = L"Жонглёр (ошибка доступа)";
            SetWindowTextW(hwnd, (wParam == 0) ? err : WINDOWS_HEADER);

            if (appState->hMouseMoverThread) {
                CloseHandle(appState->hMouseMoverThread);
                appState->hMouseMoverThread = NULL;
            }
        } break;

        case WM_THREAD_POS: { // событие вывода координат в заголовок
            wchar_t buffer[32] = {0,};
            swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"Координаты: %ldx%ld", wParam, lParam);
            SetWindowText(hwnd, buffer);
        } break;

        case WM_TIMER: {
            switch (wParam) {
                case TIMER_CLOCK_ID: { ProcessTimerClock(appState); } break;
                case TIMER_MOUSE_ID: { ProcessTimerMouseMover(appState); } break;
            }
        } break;

        /*
        case WM_PAINT: {
            
            SYSTEMTIME st = appState->st;
            PAINTSTRUCT ps;  HDC hdc = BeginPaint(hwnd, &ps);

            wchar_t timeBuf[10] = {0,}, dateBuf[20] = {0,};

            // Формат времени: "14:35:22"
            swprintf(timeBuf, sizeof(timeBuf) / sizeof(timeBuf[0]), L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

            // Формат даты: "18 февраля 2026"
            const wchar_t* month = (st.wMonth - 1) < 0 ? L"" : months[st.wMonth - 1];
            swprintf(dateBuf, sizeof(dateBuf) / sizeof(dateBuf[0]), L"%02d %ls %d", st.wDay, month, st.wYear);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            HFONT hClockFont = appState->hClockFont;
            HFONT hOldFont = (HFONT)SelectObject(hdc, hClockFont);
            TextOutW(hdc, 20, 15, timeBuf, wcslen(timeBuf));  // Часы

            HFONT hCalendarFont = appState->hCalendarFont;
            SelectObject(hdc, hCalendarFont);            
            TextOutW(hdc, 20, 55, dateBuf, wcslen(dateBuf)); // Календарь

            SelectObject(hdc, hOldFont);

            EndPaint(hwnd, &ps);
            
        } break;    */    

        case WM_TRAYICON: {
            if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);                
            } else
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
       
                #define MENU_SHOW 1
                #define MENU_EXIT 2

                AppendMenuW(hMenu, MF_STRING, MENU_SHOW, L"Показать");
                AppendMenuW(hMenu, MF_STRING, MENU_EXIT, L"Выход");

                POINT pt; GetCursorPos(&pt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);

                switch (cmd) {
                    case MENU_SHOW: {
                        ShowWindow(hwnd, SW_SHOW);
                        SetForegroundWindow(hwnd);
                        PostMessage(hwnd, WM_NULL, 0, 0);  // Фиксит Z-order
                        break;
                    }
                    case MENU_EXIT: DestroyWindow(hwnd); break;
                }
                
                DestroyMenu(hMenu);
            }
            break;
        }

        case WM_DESTROY:
            SetEvent(appState->hMouseMoverStopEvent);
            CloseHandle(appState->hMouseMoverStopEvent);
            KillTimer(hwnd, TIMER_CLOCK_ID);
            appState->hMouseMoverStopEvent = NULL;

            DeleteObject(appState->hCalendarFont);
            DeleteObject(appState->hClockFont);

            Shell_NotifyIconW(NIM_DELETE, &appState->nid);
            PostQuitMessage(0);
        break;

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
        break;
     
        default: return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
