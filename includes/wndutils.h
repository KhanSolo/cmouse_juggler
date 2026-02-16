#pragma once

#include <windows.h>
#include "appstate.h"

static inline void GetResolution(AppState *state){
    state->cxscreen = GetSystemMetrics(SM_CXSCREEN);
    state->cyscreen = GetSystemMetrics(SM_CYSCREEN);
}

static inline void ChangeWindowPosition(AppState *appState) {
    RECT rc; GetWindowRect(appState->hwnd, &rc);
    const int margin = 70;
    int xPos = appState->cxscreen - (int)(rc.right - rc.left) - margin/2;
    int yPos = appState->cyscreen - (int)(rc.bottom - rc.top) - margin;
    SetWindowPos(appState->hwnd, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static inline HWND CreateMainWindow(AppState *state, HINSTANCE hInstance, LPCWSTR lpWindowName, int width, int height, WNDPROC windowProc) {
    const wchar_t CLASS_NAME[] = L"MouseJugglerWindow";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    return CreateWindowExW(
        WS_EX_TOOLWINDOW, CLASS_NAME, lpWindowName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, state
    );
}