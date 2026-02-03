#define UNICODE  // add before #include
#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdlib.h>

HANDLE hThread = NULL;
HANDLE hMutex = NULL;
volatile BOOL bRunning = FALSE;
HWND hwnd = NULL;
HWND hStartButton = NULL;

int cxscreen = 100;
int cyscreen = 100;

DWORD WINAPI MouseMoverThread(LPVOID lpParam) {

    int oldx = 1, oldy = 1;
    BOOL zigzag = TRUE;

    while (TRUE) {

        WaitForSingleObject(hMutex, INFINITE);
        if (!bRunning) {
            ReleaseMutex(hMutex);
            break;
        }
        ReleaseMutex(hMutex);
        
        // getting pos
        POINT point;
        BOOL is = GetCursorPos(&point);
        int curx = (int)point.x;
        int cury = (int)point.y;

        int sleepMs = 10000;
        if (abs(curx - oldx) < 5 || abs(cury - oldy) < 5) {

            // setting pos
            int diff = 2 * (zigzag ? 1 : -1);
            curx = (curx+diff) % cxscreen;
            cury = (cury+diff) % cyscreen;
            SetCursorPos(curx, cury);

            zigzag = !zigzag;
            sleepMs = 1000;
        }

        oldx = curx;
        oldy = cury;
        
        wchar_t buffer[64];
        wsprintfW(buffer, L"Координаты: %dx%d", curx, cury);
        SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

        Sleep(sleepMs + rand() % 500);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

        case WM_CREATE:
            hMutex = CreateMutex(NULL, FALSE, NULL);
            srand(GetTickCount());
            
            hStartButton = CreateWindowW(
                L"BUTTON", L"Старт",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                30, 30, 280, 40,
                hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
            );
            break;
            
        case WM_COMMAND:
            if ((HWND)lParam == hStartButton) {
                WaitForSingleObject(hMutex, INFINITE);

                if (!bRunning) {
                    bRunning = TRUE;
                    ReleaseMutex(hMutex);
                    SetWindowText(hStartButton, L"Стоп");
                    hThread = CreateThread(NULL, 0, MouseMoverThread, NULL, 0, NULL);

                } else {

                    bRunning = FALSE;
                    ReleaseMutex(hMutex);
                    if (hThread) {
                        WaitForSingleObject(hThread, 2000);
                        CloseHandle(hThread);
                        hThread = NULL;
                    }
                    SetWindowText(hStartButton, L"Старт");
                }
            }
            break;
            
        case WM_DESTROY:
            WaitForSingleObject(hMutex, INFINITE);
            bRunning = FALSE;
            ReleaseMutex(hMutex);
            if (hThread) {
                WaitForSingleObject(hThread, 2000);
                CloseHandle(hThread);
            }
            if (hMutex) CloseHandle(hMutex);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    cxscreen = GetSystemMetrics(SM_CXSCREEN);
    cyscreen = GetSystemMetrics(SM_CYSCREEN);

    const wchar_t CLASS_NAME[] = L"MouseJugglerWindow";
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Жонглер",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 150,
        NULL, NULL, hInstance, NULL
    );
    
    if (hwnd == NULL) return 0;
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
