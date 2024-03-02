#define STRICT
#include <windows.h>
#include <windowsx.h> // HANDLE_MSG
#include <stdio.h>

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

HINSTANCE g_hinst;
//HWND g_hwndChild;

HWND g_hwnd_targets[16]; // arbitrary limit

void OnSize(HWND hwnd, UINT state, int cx, int cy) {
    //if (g_hwndChild) {
    //    MoveWindow(g_hwndChild, 0, 0, cx, cy, TRUE);
    //}
}

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpcs) {
    return TRUE;
}

void OnDestroy(HWND hwnd) {
    PostQuitMessage(0);
}

void PaintContent(HWND hwnd, PAINTSTRUCT *pps) {
}

void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    PaintContent(hwnd, &ps);
    EndPaint(hwnd, &ps);
}

void OnPrintClient(HWND hwnd, HDC hdc)
{
    PAINTSTRUCT ps;
    ps.hdc = hdc;
    GetClientRect(hwnd, &ps.rcPaint);
    PaintContent(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    switch (uiMsg) {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        case WM_PRINTCLIENT: OnPrintClient(hwnd, (HDC)wParam); return 0;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL: {
            for (size_t i = 0; i < array_count(g_hwnd_targets); ++i) {
                HWND target = g_hwnd_targets[i];
                if (target != NULL) {
                    BOOL ok = PostMessage(target, uiMsg, wParam, lParam);
                    if (!ok) {
                        DWORD err = GetLastError();
                        static char buf[256];
                        snprintf(buf, sizeof(buf), "PostMessage for target %zu failed: error %u", i, err);
                        OutputDebugString(buf);
                    }
                }
            }
        } return 0;
    }

    return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}

#define NAME "Hello"

BOOL InitApp() {
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hinst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT(NAME);
    if (!RegisterClass(&wc)) return FALSE;
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev,
    LPSTR lpCmdLine, int nShowCmd)
{
    MSG msg;
    HWND hwnd;
    g_hinst = hinst;
    if (!InitApp()) return 0;
    hwnd = CreateWindow(
        TEXT(NAME),                     // Class Name
        TEXT(NAME),                     // Title
        WS_OVERLAPPEDWINDOW,            // Style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position
        CW_USEDEFAULT, CW_USEDEFAULT,   // Size
        NULL,                           // Parent
        NULL,                           // No menu
        hinst,                          // Instance
        0);                             // No special parameters
    ShowWindow(hwnd, nShowCmd);

    HWND target = FindWindow("RenderTestWindowClass", NULL);
    g_hwnd_targets[0] = target;
    //char buf[128];
    //snprintf(buf, sizeof(buf), "RenderTest hwnd = %p\n", (void*) target);
    //MessageBox(hwnd, buf, NULL, MB_OK);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
