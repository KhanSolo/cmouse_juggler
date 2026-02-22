#pragma once

#include <windows.h>
#include "appstate.h"

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

static inline HWND CreateMainWindow(AppState *state, HINSTANCE hInstance, LPCWSTR lpWindowName, 
    const int width, const int height, WNDPROC windowProc
) {
    const wchar_t CLASS_NAME[] = L"MouseJugglerWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);    
    
    ATOM atom = RegisterClassW(&wc);
    if(!atom) return NULL;

    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    return CreateWindowExW(WS_EX_TOOLWINDOW,
        CLASS_NAME, lpWindowName, dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, state
    );
}

static inline void CreateClockText(AppState *state, const int xPos, const int yPos, const int width, const int height, const wchar_t * text){
    DWORD dwStyle = 0;
    state->hClockText = CreateWindowW(
        L"STATIC", text, dwStyle,
        xPos, yPos, width, height,
        state->hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
    );
}

static inline void CreateCalendarText(AppState *state, const int xPos, const int yPos, const int width, const int height, const wchar_t * text){
    DWORD dwStyle = 0;
    state->hClockText = CreateWindowW(
        L"STATIC", text, dwStyle,
        xPos, yPos, width, height,
        state->hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
    );
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