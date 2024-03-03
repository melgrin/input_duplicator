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
HWND g_hwndIncludeLabel;
HWND g_hwndIncludeEdit;
HWND g_hwndExcludeLabel;
HWND g_hwndExcludeEdit;
HWND g_hwndMatchCount;
HWND g_hwndInfo;
HWND g_hwndUnderCursorInfo;
HWND g_hwndTargetTable;

WNDPROC g_hwndIncludeEditOriginalWndProc;
WNDPROC g_hwndTargetTableOriginalWndProc;

typedef struct {
    HWND hwnd;
    char* window_text;
    char* class_name;
} HWND_Info;

typedef struct {
    size_t count;
    size_t capacity;
    HWND_Info* data;
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
            HWND_Info info = {
                .hwnd = hwnd,
                .class_name = strdup(class_name),
                .window_text = NULL,
            };
            da_append(g_hwnd_targets, info);
        }
    }
    return TRUE; // keep enumerating
}

static void set_match_count(size_t n) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%zu", n);
    SetWindowText(g_hwndMatchCount, buf);
}

LRESULT CALLBACK MyEditWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    if (uiMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        static char buf[512];
        int n = GetWindowText(hwnd, buf, sizeof(buf));
        if (n > 0 && (size_t) n < sizeof(buf)) {
            g_hwnd_targets.count = 0;
            EnumWindows(EnumWindows_MatchWindowClass, (LPARAM) buf);
            set_match_count(g_hwnd_targets.count);
            static char buf2[1024];
            buf2[0] = 0;
            char* pos = buf2;
            size_t rem = sizeof(buf2);
            for (size_t i = 0; i < g_hwnd_targets.count; ++i) {
                int n2 = snprintf(pos, rem, "%zu    %p    %s\n",
                    i + 1,
                    g_hwnd_targets.data[i].hwnd,
                    g_hwnd_targets.data[i].class_name);
                if (n2 > 0) {
                    pos += n2;
                    rem -= n2;
                } else break;
            }
            SetWindowText(g_hwndInfo, buf2);
        }
    }
    return CallWindowProc(g_hwndIncludeEditOriginalWndProc, hwnd, uiMsg, wParam, lParam);
}

LRESULT CALLBACK MyListViewWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    if (uiMsg == WM_KEYDOWN && (wParam == VK_DELETE || wParam == VK_BACK /*backspace*/)) {
        // Delete all of the selected items
        while (1) {
            BOOL deleted = FALSE;
            for (int i = 0; i < ListView_GetItemCount(hwnd); ++i) {
                if (ListView_GetItemState(hwnd, i, LVIS_SELECTED)) {
                    ListView_DeleteItem(hwnd, i);
                    // ListView_DeleteItem modifies the indices of the list, so 'i' is validated
                    deleted = TRUE;
                    break;
                }
            }
            if (!deleted) break;
        }
    }
    return CallWindowProc(g_hwndTargetTableOriginalWndProc, hwnd, uiMsg, wParam, lParam);
}


BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpcs) {

    g_hwndIncludeEdit = CreateWindow/*Ex*/(
        //WS_EX_CLIENTEDGE,
        WC_EDIT,
        TEXT(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndIncludeEdit) return FALSE;
    g_hwndIncludeEditOriginalWndProc = (WNDPROC) SetWindowLongPtr(g_hwndIncludeEdit, GWLP_WNDPROC, (LONG_PTR) MyEditWndProc);
    if (!g_hwndIncludeEditOriginalWndProc) return FALSE;

    g_hwndIncludeLabel = CreateWindow(
        WC_STATIC,
        TEXT("Some Label"),
        WS_CHILD | WS_VISIBLE | SS_CENTER /*| SS_SIMPLE*/,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndIncludeLabel) return FALSE;

    g_hwndExcludeEdit = CreateWindow/*Ex*/(
        //WS_EX_CLIENTEDGE,
        WC_EDIT,
        TEXT(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndExcludeEdit) return FALSE;

    g_hwndExcludeLabel = CreateWindow(
        WC_STATIC,
        TEXT("Some Label"),
        WS_CHILD | WS_VISIBLE /*| SS_SIMPLE*/,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndExcludeLabel) return FALSE;

    g_hwndMatchCount = CreateWindow( // this is probably temporary (TODO)
        WC_STATIC,
        TEXT("0"),
        WS_CHILD | WS_VISIBLE,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndMatchCount) return FALSE;

    //XXX
    //g_hwndInfo = CreateWindow(
    //    WC_STATIC,
    //    TEXT("?"),
    //    WS_CHILD | WS_VISIBLE,
    //    0,0,0,0, // see OnSize
    //    hwnd, // parent
    //    NULL,
    //    g_hinst,
    //    NULL);
    //if (!g_hwndInfo) return FALSE;

    g_hwndUnderCursorInfo = CreateWindow(
        WC_STATIC,
        TEXT("?"),
        WS_CHILD | WS_VISIBLE,
        0,0,0,0, // see OnSize
        hwnd, // parent
        NULL,
        g_hinst,
        NULL);
    if (!g_hwndUnderCursorInfo) return FALSE;

    g_hwndTargetTable = CreateWindow(
        WC_LISTVIEW,
        TEXT("Target Table"),
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
        0,0,0,0, // see OnSize
        hwnd, NULL, g_hinst, 0);
    if (!g_hwndTargetTable) return FALSE;

    g_hwndTargetTableOriginalWndProc = (WNDPROC) SetWindowLongPtr(g_hwndTargetTable, GWLP_WNDPROC, (LONG_PTR) MyListViewWndProc);
    if (!g_hwndTargetTableOriginalWndProc) return FALSE;

    {
        HWND h = g_hwndTargetTable;
        LVCOLUMNA lvc = {0};
        lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvc.fmt = LVCFMT_LEFT; // text justification

        lvc.pszText = "Handle";
        lvc.cx = 120;
        ListView_InsertColumn(h, 0, &lvc);
        //ListView_SetColumnWidth(h, 0, LVSCW_AUTOSIZE_USEHEADER);

        lvc.pszText = "Title";
        lvc.cx = 120;
        ListView_InsertColumn(h, 1, &lvc);
        //ListView_SetColumnWidth(h, 1, LVSCW_AUTOSIZE_USEHEADER);

        lvc.pszText = "Class";
        lvc.cx = 120;
        ListView_InsertColumn(h, 2, &lvc);
        //ListView_SetColumnWidth(h, 2, LVSCW_AUTOSIZE_USEHEADER);
    }

    // TODO
    //DWORD lvstyle = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER;
    DWORD lvstyle = LVS_EX_FULLROWSELECT /*| LVS_EX_ONECLICKACTIVATE*/ | LVS_EX_DOUBLEBUFFER;
    ListView_SetExtendedListViewStyleEx(g_hwndTargetTable, lvstyle, lvstyle);


    // Somehow, the DEFAULT_GUI_FONT is not the default font, so tell the child windows to use it.
    HGDIOBJ font = GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(g_hwndIncludeLabel,    WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndIncludeEdit,     WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndExcludeLabel,    WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndExcludeEdit,     WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndMatchCount,      WM_SETFONT, (LPARAM) font, TRUE);
    //SendMessage(g_hwndInfo,            WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndUnderCursorInfo, WM_SETFONT, (LPARAM) font, TRUE);
    SendMessage(g_hwndTargetTable,     WM_SETFONT, (LPARAM) font, TRUE);


    return TRUE;
}

void OnSize(HWND hwnd, UINT state, int cx, int cy) {
    //int input_region_x = cx / 3;
    //int input_region_y = cy / 3;

    /// MoveWindow(hwnd, x, y, w, h, repaint)

    const int pad = 10;

    {
      int h = 20;
      int labelw = 60;
      int editw = 100;
      MoveWindow(g_hwndIncludeLabel, pad,          pad, labelw, h, TRUE);
      MoveWindow(g_hwndIncludeEdit,  pad + labelw, pad, editw,  h, TRUE);

      MoveWindow(g_hwndExcludeLabel, pad,          pad + h, labelw, h, TRUE);
      MoveWindow(g_hwndExcludeEdit,  pad + labelw, pad + h, editw,  h, TRUE);

      MoveWindow(g_hwndMatchCount, pad, pad + (h * 2), labelw, h, TRUE);
    }

    {
        int info_region_x = cx / 3;
        int info_region_y = cy / 3;

        //int x = input_region_x + pad;
        //int y = pad;

        //XXX MoveWindow(g_hwndInfo, pad, info_region_y + pad, cx, cy / 2, TRUE);

        //int w = cx;
        int h = cy / 3;
        //int x = pad;
        int y0 = cy / 2; // info_region_y;
        //place_window(g_hwndUnderCursorInfo, 0 + pad, y0 + pad, cx - pad, cy - pad);
        //place_window(g_hwndTargetTable,     0 + pad, cy + pad, cx - pad, cy);
        int lh = 16; // line height of single line of text
        MoveWindow(g_hwndUnderCursorInfo, 0 + pad, y0,            cx - pad * 2, lh, TRUE);
        MoveWindow(g_hwndTargetTable,     0 + pad, y0 + pad + lh, cx - pad * 2, h - pad, TRUE);
    }

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


static void update_cursor_under_window_text(POINT pt) {
    HWND found = WindowFromPoint(pt);
    static char buf[512];
    buf[0] = 0;
    if (found) {
        int n = snprintf(buf, sizeof(buf), "under cursor @ (%4u, %4u):    %p    ", pt.x, pt.y, found);
        if (n > 0 && (size_t) n < sizeof(buf)) {
            char* p = buf + n;
            GetClassName(found, p, (int) sizeof(buf) - n);
            SetWindowText(g_hwndUnderCursorInfo, buf);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    switch (uiMsg) {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        case WM_PRINTCLIENT: OnPrintClient(hwnd, (HDC)wParam); return 0;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            POINT pt;
            if (GetCursorPos(&pt)) {
                update_cursor_under_window_text(pt);

                static const char* texts[] = {
                    "one",
                    "two",
                    "buckle my shoe"
                };
                static const size_t num_texts = sizeof(texts) / sizeof(texts[0]);


                int index = ListView_GetItemCount(g_hwndTargetTable); // add to the end
                const char* text = texts[index % num_texts];
                LVITEM lvi = {0};
                lvi.iItem = index;
                //lvi.mask = LVIF_TEXT;
                //lvi.pszText = (char*) text;

                ListView_InsertItem(g_hwndTargetTable, &lvi);

                HWND found = WindowFromPoint(pt);
                if (found) {
                    static char buf[128];
                    snprintf(buf, sizeof(buf), "%p", found);
                    char* handle_string = strdup(buf); // FIXME leak
                    GetWindowText(found, buf, (int) sizeof(buf));
                    char* title_string = strdup(buf); // FIXME leak
                    GetClassName(found, buf, (int) sizeof(buf));
                    char* class_string = strdup(buf); // FIXME leak
                    ListView_SetItemText(g_hwndTargetTable, index, 0, handle_string);
                    ListView_SetItemText(g_hwndTargetTable, index, 1, title_string);
                    ListView_SetItemText(g_hwndTargetTable, index, 2, class_string);
                }
            }
        }

        /*
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            for (size_t i = 0; i < g_hwnd_targets.count; ++i) {
                HWND_Info* target = &g_hwnd_targets.data[i];
                if (target != NULL) {
                    BOOL ok = PostMessage(target->hwnd, uiMsg, wParam, lParam);
                    if (!ok) {
                        DWORD err = GetLastError();
                        static char buf[256];
                        snprintf(buf, sizeof(buf), "PostMessage for target %zu failed: error %u", i, err);
                        OutputDebugString(buf);
                    }
                }
            }
        } return 0;
        */

        /* FIXME? works much better if I just wait for spacebar.  This one is funky.
        case WM_MOUSEMOVE: {
            POINT pt = {
                .x = GET_X_LPARAM(lParam),
                .y = GET_Y_LPARAM(lParam)
            };
            if (ClientToScreen(hwnd, &pt)) {
                update_cursor_under_window_text(pt);
            }
        } break;
        */

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL: {
        } break;

        /*
        case WM_COMMAND: {
            if (lParam) {
                HWND edit = (HWND) lParam;
                if (edit == g_hwndIncludeEdit || edit == g_hwndExcludeEdit) {
                    WORD edit_control_notification_code = HIWORD(wParam);
                    if (edit_control_notification_code == EN_CHANGE) {
                        // use GetWindowText to see the edit's contents
                        return 0;
                    }
                }

            }
        } return 0;
        */
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
        400, 600,                       // Size
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

// TODO: allow user to add HWND by typing it in somewhere (extend the search to search "%p" hwnds, not just window titles/classes)

