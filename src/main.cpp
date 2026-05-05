#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <strsafe.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1001
#define IDM_OPEN    1002
#define IDM_EXIT    1003

HINSTANCE hInst;
HWND hMainWnd = NULL;
NOTIFYICONDATA nid = {};
HANDLE hMutex = NULL;
UINT uTaskbarRestart = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void AddTrayIcon(HWND hWnd);
void ShowContextMenu(HWND hWnd);
void ShowMainWindow();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    hInst = hInstance;

    hMutex = CreateMutex(NULL, TRUE, TEXT("Global\\TrayAppSingleInstance"));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, TEXT("Приложение уже запущено!"), TEXT("TrayApp"), MB_OK);
        return 1;
    }

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("TrayAppClass");

    RegisterClassEx(&wc);

    hMainWnd = CreateWindow(TEXT("TrayAppClass"), TEXT("Tray Application"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInstance, NULL);

    // Создаём меню Файл → Выход
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, TEXT("Выход"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, TEXT("Файл"));
    SetMenu(hMainWnd, hMenuBar);

    uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
    AddTrayIcon(hMainWnd);

    ShowWindow(hMainWnd, SW_HIDE);   // запускаемся скрыто

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}

void AddTrayIcon(HWND hWnd) {
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Tray Application"));
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowContextMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, IDM_OPEN, TEXT("Открыть"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, TEXT("Выход"));
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

void ShowMainWindow() {
    ShowWindow(hMainWnd, SW_SHOW);
    SetForegroundWindow(hMainWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            ShowMainWindow();
        } else if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hWnd);
        }
    }
    else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == IDM_EXIT) {
            DestroyWindow(hWnd);
        }
    }
    else if (msg == WM_CLOSE) {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    }
    else if (msg == uTaskbarRestart) {
        AddTrayIcon(hWnd);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}