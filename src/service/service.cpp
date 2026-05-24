#include "service.h"
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <vector>
#include "../rpc/rpc_server.h"

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
std::vector<DWORD>    g_TrayAppPIDs;

DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType,
                                   LPVOID lpEventData, LPVOID lpContext)
{
    if (dwControl == SERVICE_CONTROL_SESSIONCHANGE)
    {
        WTSSESSION_NOTIFICATION* pNotif = (WTSSESSION_NOTIFICATION*)lpEventData;
        if (dwEventType == WTS_SESSION_LOGON && pNotif->dwSessionId != 0)
        {
            Sleep(2000);
            LaunchTrayAppInSession(pNotif->dwSessionId);
        }
    }
    return NO_ERROR;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    g_StatusHandle = RegisterServiceCtrlHandlerExW(
        L"TrayService",
        ServiceCtrlHandlerEx,
        NULL);
    if (!g_StatusHandle) return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;
    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    StartTrayAppInAllSessions();

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    printf("TrayService started successfully.\n");

    StartRPCServer();
}


void WINAPI ServiceCtrlHandler(DWORD dwControl)
{
    if (dwControl == SERVICE_CONTROL_SESSIONCHANGE)
    {
        // Новая сессия — запускаем TrayApp
        StartTrayAppInAllSessions();
    }
    // Все остальные команды (Stop, Shutdown) игнорируем
}

bool StartTrayAppInAllSessions()
{
    PWTS_SESSION_INFO pSessionInfo = NULL;
    DWORD count = 0;

    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count))
    {
        for (DWORD i = 0; i < count; i++)
        {
            if (pSessionInfo[i].State == WTSActive && pSessionInfo[i].SessionId != 0)
            {
                LaunchTrayAppInSession(pSessionInfo[i].SessionId);
            }
        }
        WTSFreeMemory(pSessionInfo);
    }
    return true;
}

bool LaunchTrayAppInSession(DWORD sessionId)
{
    HANDLE hToken = NULL;
    if (!WTSQueryUserToken(sessionId, &hToken))
        return false;

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    si.lpDesktop = L"winsta0\\default";

    // Получаем путь к папке службы и формируем путь к TrayApp.exe
    WCHAR appPath[MAX_PATH];
    GetModuleFileNameW(NULL, appPath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(appPath, L'\\');
    if (lastSlash) *lastSlash = L'\0';

    wcscat_s(appPath, MAX_PATH, L"\\TrayApp.exe");

    WCHAR cmdLine[MAX_PATH + 20];
    swprintf_s(cmdLine, MAX_PATH + 20, L"\"%s\" --service", appPath);

    BOOL success = CreateProcessAsUserW(hToken, NULL, cmdLine, NULL, NULL, FALSE,
        CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);

    if (success)
    {
        g_TrayAppPIDs.push_back(pi.dwProcessId);
        printf("TrayApp launched in session %u\n", sessionId);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        printf("Failed to launch TrayApp in session %u. Error: %d\n", sessionId, GetLastError());
    }

    CloseHandle(hToken);
    return success;
}

void KillAllTrayApps()
{
    for (DWORD pid : g_TrayAppPIDs)
    {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
    g_TrayAppPIDs.clear();
}