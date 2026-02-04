#define UNICODE  // add before #include
#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdlib.h>

#define WINDOWS_WIDTH 360
#define WINDOWS_HEIGHT 150
#define BTN_START_WIDTH 280
#define BTN_START_HEIGHT 40

HANDLE hThread = NULL;
volatile BOOL bRunning = FALSE;
HWND hStartButton = NULL;

typedef struct State {
    int cxscreen;
    int cyscreen;
    HWND hwnd;
} AppState;

void GetResolution(AppState *state);
void CreateMainWindow(AppState *state, HINSTANCE hInstance);
void CenterWindow(AppState *appState);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI MouseMoverThread(LPVOID lpParam);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    AppState appState = {0};

    GetResolution(&appState);
    CreateMainWindow(&appState, hInstance);
    if (NULL == appState.hwnd) return 0;
    
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

void GetResolution(AppState *state){
    state->cxscreen = GetSystemMetrics(SM_CXSCREEN);
    state->cyscreen = GetSystemMetrics(SM_CYSCREEN);
}

void CreateMainWindow(AppState *state, HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"MouseJugglerWindow";
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Жонглер",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOWS_WIDTH, WINDOWS_HEIGHT,
        NULL, NULL, hInstance, state
    );

    state->hwnd = hwnd;
}

void CenterWindow(AppState *appState) {
    RECT rc;
    GetWindowRect(appState->hwnd, &rc);
    int xPos = (appState->cxscreen - (rc.right - rc.left)) / 2;
    int yPos = (appState->cyscreen - (rc.bottom - rc.top)) / 2;
    SetWindowPos(appState->hwnd, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    AppState* appState = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {

        case WM_CREATE:
            srand(GetTickCount());
            
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            AppState* state = (AppState*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state); // save on creation

            hStartButton = CreateWindowW(
                L"BUTTON", L"Старт",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                30, 30, BTN_START_WIDTH, BTN_START_HEIGHT,
                hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
            );
            break;
            
        case WM_COMMAND:
            if ((HWND)lParam == hStartButton) {

                if (!bRunning) {
                    bRunning = TRUE;
                    SetWindowText(hStartButton, L"Стоп");
                    hThread = CreateThread(NULL, 0, MouseMoverThread, appState, 0, NULL);

                } else {
                    bRunning = FALSE;
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
            bRunning = FALSE;
            if (hThread) {
                WaitForSingleObject(hThread, 2000);
                CloseHandle(hThread);
            }

            PostQuitMessage(0);
            break;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
            
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI MouseMoverThread(LPVOID lpParam) {

    AppState* appState = (AppState*)lpParam;

    long oldx = 1, oldy = 1;
    BOOL zigzag = TRUE;

    while (TRUE) {

        if (!bRunning) {
            break;
        }
        
        // getting pos
        POINT point;
        if(FALSE == GetCursorPos(&point)) {
            bRunning = FALSE;
            break;
        }

        long curx = point.x;
        long cury = point.y;

        int sleepMs = 10000;
        if (abs(curx - oldx) < 5 && abs(cury - oldy) < 5) {

            // setting pos
            long diff = 2 * (zigzag ? 1 : -1);
            curx = (curx + diff + appState->cxscreen) % appState->cxscreen;
            cury = (cury + diff + appState->cyscreen) % appState->cyscreen;
            SetCursorPos((int)curx, (int)cury);

            zigzag = !zigzag;
            sleepMs = 2000;
        }

        oldx = curx;
        oldy = cury;
        
        wchar_t buffer[64];
        wsprintfW(buffer, L"Координаты: %dx%d", curx, cury);
        SendMessage(appState->hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  // todo ref to PostMessage?

        Sleep(sleepMs + rand() % 500);
    }
    return 0;
}