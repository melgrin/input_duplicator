#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> // HANDLE_MSG
#include <stdio.h>
#include <stdlib.h> // realloc
#include <commctrl.h> // WC_EDIT

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib") // GetStockObject
//#pragma comment(lib, "comctl32.lib")

// This loads Common Controls Version 6.0, which has more modern visual styles.
// Use linker option /manifest:embed to embed this into the exe, otherwise look for *.exe.manifest.
// The publicKeyToken is specifically for Common Controls as dictated by Microsoft, and has nothing to do with this application.
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE g_hinst;
//HWND g_hwndChild;
HWND g_hwndIncludeLabel;
HWND g_hwndIncludeEdit;
HWND g_hwndExcludeLabel;
HWND g_hwndExcludeEdit;
HWND g_hwndMatchCount;

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

static void set_match_count(size_t n) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%zu", n);
    SetWindowText(g_hwndMatchCount, buf);
}

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpcs) {

    g_hwndIncludeEdit = CreateWindow/*Ex*/(
        //WS_EX_CLIENTEDGE,
        WC_EDIT,
        TEXT(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0,0,0,0, // see WM_SIZE
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndIncludeEdit) return FALSE;

    g_hwndIncludeLabel = CreateWindow(
        WC_STATIC,
        TEXT("Some Label"),
        WS_CHILD | WS_VISIBLE | SS_CENTER /*| SS_SIMPLE*/,
        0,0,0,0, // see WM_SIZE
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndIncludeLabel) return FALSE;

    g_hwndExcludeEdit = CreateWindow/*Ex*/(
        //WS_EX_CLIENTEDGE,
        WC_EDIT,
        TEXT(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0,0,0,0, // see WM_SIZE
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndExcludeEdit) return FALSE;

    g_hwndExcludeLabel = CreateWindow(
        WC_STATIC,
        TEXT("Some Label"),
        WS_CHILD | WS_VISIBLE /*| SS_SIMPLE*/,
        0,0,0,0, // see WM_SIZE
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndExcludeLabel) return FALSE;

    g_hwndMatchCount = CreateWindow( // this is probably temporary (TODO)
        WC_STATIC,
        TEXT("0"),
        WS_CHILD | WS_VISIBLE,
        0,0,0,0, // see WM_SIZE
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndMatchCount) return FALSE;

    // Somehow, the DEFAULT_GUI_FONT is not the default font, so tell the child windows to use it.
    HGDIOBJ font = GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(g_hwndIncludeLabel, WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndIncludeEdit,  WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndExcludeLabel, WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndExcludeEdit,  WM_SETFONT, (LPARAM) font, TRUE);

    return TRUE;
}

void OnSize(HWND hwnd, UINT state, int cx, int cy) {
    int input_region_x = cx / 3;
    int input_region_y = cy / 3;
    /// MoveWindow(hwnd, x, y, w, h, repaint)
    const int h = 20;
    const int pad = 10;
    const int labelw = 60;
    const int editw = 100;
    MoveWindow(g_hwndIncludeLabel, pad,          pad, labelw, h, TRUE);
    MoveWindow(g_hwndIncludeEdit,  pad + labelw, pad, editw,  h, TRUE);

    MoveWindow(g_hwndExcludeLabel, pad,          pad + h, labelw, h, TRUE);
    MoveWindow(g_hwndExcludeEdit,  pad + labelw, pad + h, editw,  h, TRUE);

    MoveWindow(g_hwndMatchCount, pad, pad + (h * 2), labelw, h, TRUE);

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

        case WM_COMMAND: {
            if (lParam) {
                HWND edit = (HWND) lParam;
                if (edit == g_hwndIncludeEdit || edit == g_hwndExcludeEdit) {
                    WORD edit_control_notification_code = HIWORD(wParam);
                    if (edit_control_notification_code == EN_CHANGE) {
                        static char buf[512];
                        int n = GetWindowText(edit, buf, sizeof(buf));
                        if (n > 0 && (size_t) n < sizeof(buf)) {
                            //FIXME appends forever (just delete the last char of the match and retype it to see what I mean)
                            //FIXME probably inefficient to enumerate all windows every time, but what's a girl to do??
                            EnumWindows(EnumWindows_MatchWindowClass, (LPARAM) buf);
                            set_match_count(g_hwnd_targets.count);
                            //TODO actually display the matches
                        }
                        return 0;
                    }
                }

            }
            int _bp = 0;
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

int WINAPI WinMain(
    HINSTANCE hinst,
    HINSTANCE hinstPrev,
    LPSTR lpCmdLine, int nShowCmd) {

    MSG msg;
    HWND hwnd;
    g_hinst = hinst;
    if (!InitApp()) return 0;
    hwnd = CreateWindow(
        TEXT(NAME),                     // Class Name
        TEXT(NAME),                     // Title
        WS_OVERLAPPEDWINDOW,            // Style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position
        200, 200,                       // Size
        NULL,                           // Parent
        NULL,                           // No menu
        hinst,                          // Instance
        0);                             // No special parameters
    ShowWindow(hwnd, nShowCmd);

    EnumWindows(EnumWindows_MatchWindowClass, (LPARAM) "RenderTestWindowClass");
    set_match_count(g_hwnd_targets.count);

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
