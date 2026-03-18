#ifndef WNDUTILS_H
#define WNDUTILS_H

#include <windows.h>
#include <wchar.h>
#include "appstate.h"

#define IDC_LABEL_CLOCK     1001
#define IDC_LABEL_CALENDAR  1002

static inline void GetResolution(AppState *state){
    state->cxscreen = GetSystemMetrics(SM_CXSCREEN);
    state->cyscreen = GetSystemMetrics(SM_CYSCREEN);
}

static inline void ChangeWindowPosition(const AppState *appState) {
    RECT rc; GetWindowRect(appState->hwnd, &rc);
    const int ymargin = 70, xmargin = 35;
    int xPos = appState->cxscreen - (int)(rc.right - rc.left) - xmargin;
    int yPos = appState->cyscreen - (int)(rc.bottom - rc.top) - ymargin;
    SetWindowPos(appState->hwnd, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static inline HWND CreateMainWindow(AppState * const state, HINSTANCE hInstance, 
    LPCWSTR lpWindowClassName, LPCWSTR lpWindowName, 
    const int width, const int height, WNDPROC windowProc
) {    
    WNDCLASSW wc = {
        .hInstance = hInstance,
        .lpszClassName = lpWindowClassName,
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpfnWndProc = windowProc,
    };
    
    ATOM atom = RegisterClassW(&wc);
    if(!atom) return NULL;

    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    return CreateWindowExW(WS_EX_TOOLWINDOW,
        lpWindowClassName, lpWindowName, dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, state
    );
}

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

typedef LPCWSTR (*systime2str)(SYSTEMTIME*, wchar_t*, const size_t);

HWND CreateStatic(HWND hwnd, LPCWSTR text, const int xPos, const int yPos, const int width, const int height, long long hmenu) {
    return CreateWindowExW(
        0,  L"STATIC", text,                // dwExStyle, lpClassName, lpWindowName (текст)
        WS_CHILD | WS_VISIBLE | SS_LEFT,    // dwStyle (левый выравнивание)
        xPos, yPos, width, height,          // x, y, ширина, высота
        hwnd, (HMENU)hmenu,                 // родительское окно, hMenu (ID контрола)
        GetModuleHandle(NULL), NULL         // hInstance, lpParam
    );
}

static inline void CreateClockText(AppState *state, const int xPos, const int yPos, const int width, const int height, const systime2str func) {
    wchar_t buf[10] = {0,};
    LPCWSTR text = (*func)(&state->st, buf, sizeof(buf) / sizeof(buf[0]));
    state->hClockLabel = CreateStatic(state->hwnd, text, xPos, yPos, width, height, IDC_LABEL_CLOCK);
    SendMessageW(state->hClockLabel, WM_SETFONT, (WPARAM)state->hClockFont, TRUE);
}

static inline void CreateCalendarText(AppState *state, const int xPos, const int yPos, const int width, const int height, const systime2str func) {
    wchar_t buf[32] = {0,};
    LPCWSTR text = (*func)(&state->st, buf, sizeof(buf) / sizeof(buf[0]));
    state->hCalendarLabel = CreateStatic(state->hwnd, text, xPos, yPos, width, height, IDC_LABEL_CALENDAR);
    SendMessageW(state->hCalendarLabel, WM_SETFONT, (WPARAM)state->hCalendarFont, TRUE);
}

static inline void CreateStartButton(AppState *state, const int xPos, const int yPos, const int width, const int height, const wchar_t * text){
    DWORD dwStyle= WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
    if(NULL == state->hMouseMoverStopEvent) { dwStyle |= WS_DISABLED; } 
    state->hStartButton = CreateWindowW(
        L"BUTTON", text, dwStyle,
        xPos, yPos, width, height,
        state->hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
    );
}

static inline void InitTrayIcon(AppState* state, const wchar_t * szTip) {
    memset(&state->nid, 0, sizeof(NOTIFYICONDATAW));

    state->nid.cbSize = sizeof(NOTIFYICONDATAW);
    state->nid.hWnd = state->hwnd;
    state->nid.uID = 1;
    state->nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    state->nid.uCallbackMessage = WM_TRAYICON;

    state->nid.hIcon = LoadIcon(NULL, IDI_ASTERISK);
    wcscpy_s(state->nid.szTip, 32, szTip);

    Shell_NotifyIconW(NIM_ADD, &state->nid);
}

static inline HFONT CreateNewFont(int cHeight) {
    HFONT hFont = CreateFontW(
        cHeight, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        VARIABLE_PITCH,
        L"Segoe UI"
    );
    return hFont;
}

static inline int OutputDebug(const wchar_t *format, ...) {
    va_list args;
    va_start(args, format);     // инициализация va_list
    wchar_t buffer[64] = {0,};
    int result = vswprintf(
        buffer, sizeof(buffer) / sizeof(buffer[0]), 
        format, args);          // вызов версии с va_list
    va_end(args);
    OutputDebugStringW(buffer);
    return result;
}

#define MUTEX_NAME "Global\\{1A3C5F20-7E7F-4F1A-8F9A-123456789ABC}"
int IsSingleInstance(LPCWSTR mainWimdowClassName, HANDLE *hMutex) {
    *hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND hwnd = FindWindowW(mainWimdowClassName, NULL);
        if (hwnd) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
        return FALSE;
    }
    return TRUE;
}

static void FreeMutex (HANDLE hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }

#endif