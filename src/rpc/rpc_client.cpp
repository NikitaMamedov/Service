#include "rpc_client.h"
#include <windows.h>
#include <rpc.h>
#include "tray.h"

void RPC_StopService()
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

    if (status != RPC_S_OK) return;

    status = RpcBindingFromStringBindingW(pszStringBinding, &hBinding);
    RpcStringFreeW(&pszStringBinding);
    if (status != RPC_S_OK) return;

    RpcTryExcept {
        StopService(hBinding);
    } RpcExcept(1) {
    } RpcEndExcept;

    RpcBindingFree(&hBinding);
}

boolean RPC_IsServiceRunning()
{
    return TRUE;
}