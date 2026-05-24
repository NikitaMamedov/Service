#pragma once
#include <windows.h>
#include <vector>

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI ServiceCtrlHandlerEx(
    DWORD dwControl,
    DWORD dwEventType,
    LPVOID lpEventData,
    LPVOID lpContext);

bool StartTrayAppInAllSessions();
bool LaunchTrayAppInSession(DWORD sessionId);
void KillAllTrayApps();

extern SERVICE_STATUS        g_ServiceStatus;
extern SERVICE_STATUS_HANDLE g_StatusHandle;
extern std::vector<DWORD>    g_TrayAppPIDs;