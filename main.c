#define _UNICODE

#include <windows.h>
#include <process.h>
#include <stdlib.h>

#define WINDOWS_WIDTH 360
#define WINDOWS_HEIGHT 150
#define BTN_START_WIDTH 280
#define BTN_START_HEIGHT 40

#define DEFAULT_MS 10000
#define SHORT_MS 2000

typedef struct State {
    int cxscreen;
    int cyscreen;
    HWND hwnd;
    HWND hStartButton;
    HANDLE hThread;
    volatile BOOL bRunning;
} AppState;

void GetResolution(AppState *state);
void CreateMainWindow(AppState *state, HINSTANCE hInstance);
void CenterWindow(AppState *appState);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI MouseMoverThread(LPVOID lpParam);


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {

    AppState appState = {0};

    GetResolution(&appState);
    CreateMainWindow(&appState, hInstance);
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
        0, CLASS_NAME, L"Жонглёр",
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

            state->hStartButton = CreateWindowW(
                L"BUTTON", L"Старт",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                30, 30, BTN_START_WIDTH, BTN_START_HEIGHT,
                hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
            );
            break;
            
        case WM_COMMAND:
            if ((HWND)lParam == appState->hStartButton) {

                if (!appState->bRunning) {
                    appState->bRunning = TRUE;
                    SetWindowText(appState->hStartButton, L"Стоп");
                    appState->hThread = CreateThread(NULL, 0, MouseMoverThread, appState, 0, NULL);

                } else {
                    appState->bRunning = FALSE;
                    if (appState->hThread) {
                        WaitForSingleObject(appState->hThread, 1000);
                        CloseHandle(appState->hThread);
                        appState->hThread = NULL;
                    }
                    SetWindowText(appState->hStartButton, L"Старт");
                }
            }
            break;

        case WM_USER + 1:  // Обработка завершения потока (параметр wParam: 0=ошибка, 1=нормальное завершение)
            appState->bRunning = FALSE;
            SetWindowText(appState->hStartButton, L"Старт");
            
            if (wParam == 0) {
                SetWindowText(hwnd, L"Жонглёр (ошибка доступа)");
            } else {
                SetWindowText(hwnd, L"Жонглёр");
            }

            if (appState->hThread) {
                WaitForSingleObject(appState->hThread, 1000);
                CloseHandle(appState->hThread);
                appState->hThread = NULL;
            }
        break;

        case WM_USER + 2: {
                wchar_t buffer[32];
                wsprintfW(buffer, L"Координаты: %ldx%ld", wParam, lParam);
                SetWindowText(hwnd, buffer);
        } break;
            
        case WM_DESTROY:
            appState->bRunning = FALSE;
            if (appState->hThread) {
                WaitForSingleObject(appState->hThread, 1000);
                CloseHandle(appState->hThread);
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
    POINT oldPos = {
        .x = 1,
        .y = 1,
    };

    BOOL zigzag = TRUE;

    while (TRUE) {
        if (!appState->bRunning) {
            break;
        }

        POINT curPos;
        if (!GetCursorPos(&curPos)) {
            // Ошибка! Завершаем поток и обновляем UI через PostMessage
            PostMessage(appState->hwnd, WM_USER + 1, 0, 0);
            break;
        }

        int sleepMs = DEFAULT_MS;
        if (abs(curPos.x - oldPos.x) < 5 && abs(curPos.y - oldPos.y) < 5) {

            // setting pos
            long diff = 2 * (zigzag ? 1 : -1);
            curPos.x = (curPos.x + diff + appState->cxscreen) % appState->cxscreen;
            curPos.y = (curPos.y + diff + appState->cyscreen) % appState->cyscreen;
            SetCursorPos((int)curPos.x, (int)curPos.y);

            zigzag = !zigzag;
            sleepMs = SHORT_MS;
        }

        oldPos.x = curPos.x;
        oldPos.y = curPos.y;
        
        PostMessage(appState->hwnd, WM_USER + 2, (WPARAM)curPos.x, (LPARAM)curPos.y);
        Sleep(sleepMs + rand() % 500);
    }
    return 0;
}