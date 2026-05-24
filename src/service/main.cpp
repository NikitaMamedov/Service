#include "service.h"
#include <windows.h>

int main()
{
    SERVICE_TABLE_ENTRY DispatchTable[] = 
    {
        { (LPTSTR)L"TrayService", ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcher(DispatchTable);
    return 0;
}