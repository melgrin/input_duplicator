#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> // HANDLE_MSG
#include <stdio.h>
#include <stdlib.h> // realloc

HINSTANCE g_hinst;
//HWND g_hwndChild;

typedef struct {
    size_t count;
    size_t capacity;
    HWND* data;
} HWNDs;

static HWNDs g_hwnd_targets;

#define da_append(a, it) do { \
    char ok = 1; \
    if ((a).count >= (a).capacity) { \
        size_t desired_capacity = (a).capacity == 0 ? 64 : (a).capacity * 2; \
        void* new_mem = realloc((a).data, desired_capacity * sizeof(*(a).data)); \
        ok = (new_mem != NULL); \
        if (ok) (a).data = new_mem; \
    } \
    if (ok) (a).data[(a).count++] = it; \
} while (0);

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
            for (size_t i = 0; i < g_hwnd_targets.count; ++i) {
                HWND target = g_hwnd_targets.data[i];
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

BOOL CALLBACK EnumWindows_MatchWindowClass(HWND hwnd, LPARAM lParam) {
    static char class_name[512];
    int numChars = GetClassName(hwnd, class_name, (int) sizeof(class_name));
    if (numChars > 0) {
        char* desired_class_name = (char*) lParam;
        if (0 == strcmp(desired_class_name, class_name)) {
            da_append(g_hwnd_targets, hwnd);
        }
    //} else {
    //    DWORD err = GetLastError();
    }
    return TRUE; // keep enumerating
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
        200, 100,                       // Size // there's nothing in it right now, so it can be small
        NULL,                           // Parent
        NULL,                           // No menu
        hinst,                          // Instance
        0);                             // No special parameters
    ShowWindow(hwnd, nShowCmd);

    EnumWindows(EnumWindows_MatchWindowClass, (LPARAM) "RenderTestWindowClass");

    //TODO show this info in the application's window
    //char buf[128];
    //snprintf(buf, sizeof(buf), "RenderTest hwnd = %p\n", (void*) target);
    //MessageBox(hwnd, buf, NULL, MB_OK);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
