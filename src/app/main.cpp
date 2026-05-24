#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <tlhelp32.h>
#include <rpc.h>
#include "../rpc/tray.h"
#include <stdlib.h>
#include <winsvc.h>
extern "C" void StopService(handle_t IDL_handle);

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1001
#define IDM_OPEN    1002
#define IDM_EXIT    1003


HINSTANCE hInst;
HWND hMainWnd = NULL;
NOTIFYICONDATA nid = {};

// ===================== Forward declarations =====================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddTrayIcon(HWND hWnd);
void ShowContextMenu(HWND hWnd);
void ShowMainWindow();
void CallStopService();

// ===================== Parent check =====================
bool IsParentService()
{
    DWORD ppid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(pe) };

    if (Process32First(snap, &pe))
    {
        do
        {
            if (pe.th32ProcessID == GetCurrentProcessId())
            {
                ppid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);

    TCHAR path[MAX_PATH] = {};
    HANDLE hParent = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ppid);
    if (hParent)
    {
        DWORD size = MAX_PATH;
        QueryFullProcessImageName(hParent, 0, path, &size);
        CloseHandle(hParent);
    }
    return _tcsstr(path, TEXT("TrayService.exe")) != NULL;
}

void EnsureServiceRunning()
{
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCM) return;

    SC_HANDLE hService = OpenServiceW(hSCM, L"TrayService", 
                                      SERVICE_QUERY_STATUS | SERVICE_START);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return;
    }

    SERVICE_STATUS_PROCESS ssp = {};
    DWORD dwBytesNeeded = 0;

    QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
                         (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded);

    if (ssp.dwCurrentState == SERVICE_STOPPED)
    {
        // Запускаем службу
        StartServiceW(hService, 0, NULL);

        // Ждём пока запустится
        for (int i = 0; i < 30; i++)
        {
            Sleep(500);
            QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
                                 (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded);
            if (ssp.dwCurrentState == SERVICE_RUNNING) break;
        }

        // Служба запущена — выходим, она сама запустит нас заново
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        ExitProcess(0);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
}
// ===================== WinMain =====================
// ===================== WinMain =====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    EnsureServiceRunning();  // ← добавлено

    hInst = hInstance;
    if (!IsParentService())
    {
        MessageBox(NULL,
            TEXT("Приложение должно запускаться только службой TrayService"),
            TEXT("Ошибка"),
            MB_OK | MB_ICONERROR);
        return 1;
    }
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("TrayAppClass");
    RegisterClassEx(&wc);
    hMainWnd = CreateWindow(
        TEXT("TrayAppClass"),
        TEXT("Tray Application"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 500,
        NULL, NULL, hInstance, NULL
    );
    HMENU menu = CreateMenu();
    HMENU file = CreateMenu();
    AppendMenu(file, MF_STRING, IDM_EXIT, TEXT("Выход"));
    AppendMenu(menu, MF_POPUP, (UINT_PTR)file, TEXT("Файл"));
    SetMenu(hMainWnd, menu);
    AddTrayIcon(hMainWnd);
    ShowWindow(hMainWnd, SW_HIDE);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}
// ===================== Window proc =====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
            ShowMainWindow();
        else if (lParam == WM_RBUTTONUP)
            ShowContextMenu(hWnd);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_EXIT)
            CallStopService();
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ===================== Tray =====================
void AddTrayIcon(HWND hWnd)
{
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

void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, IDM_OPEN, TEXT("Открыть"));
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, IDM_EXIT, TEXT("Выход"));
    SetForegroundWindow(hWnd);
    TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(menu);
}

void ShowMainWindow()
{
    ShowWindow(hMainWnd, SW_SHOW);
    SetForegroundWindow(hMainWnd);
}

// ===================== RPC Stop =====================
void CallStopService()
{
    RPC_STATUS status;
    RPC_WSTR pszStringBinding = NULL;
    RPC_BINDING_HANDLE hBinding = NULL;

    status = RpcStringBindingComposeW(
        NULL,
        (RPC_WSTR)L"ncalrpc",
        NULL,
        (RPC_WSTR)L"TrayServiceEndpoint",
        NULL,
        &pszStringBinding);

    if (status != RPC_S_OK) {
        MessageBox(NULL, TEXT("Ошибка создания binding"), TEXT("RPC"), MB_OK);
        return;
    }

    status = RpcBindingFromStringBindingW(pszStringBinding, &hBinding);
    RpcStringFreeW(&pszStringBinding);

    if (status != RPC_S_OK) {
        MessageBox(NULL, TEXT("Ошибка подключения к службе"), TEXT("RPC"), MB_OK);
        return;
    }

    RpcTryExcept {
        StopService(hBinding);
    } RpcExcept(1) {
        MessageBox(NULL, TEXT("Не удалось связаться со службой"), TEXT("Ошибка"), MB_OK);
    } RpcEndExcept;

    RpcBindingFree(&hBinding);
}
