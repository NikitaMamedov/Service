#include "rpc_server.h"
#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include "tray.h"

// Обязательные функции для RPC runtime
void* __RPC_USER MIDL_user_allocate(size_t size)
{
    return malloc(size);
}

void __RPC_USER MIDL_user_free(void* p)
{
    free(p);
}

void StartRPCServer()
{
    RpcServerUseProtseqEpW(
        (RPC_WSTR)L"ncalrpc",
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        (RPC_WSTR)L"TrayServiceEndpoint",
        NULL);

    RpcServerRegisterIf(TrayServiceRPC_v1_0_s_ifspec, NULL, NULL);
    RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
}

// Серверные реализации — сигнатура должна совпадать с tray.h
void StopService(handle_t IDL_handle)
{
    printf("StopService called via RPC!\n");
    ExitProcess(0);
}

boolean IsServiceRunning(handle_t IDL_handle)
{
    return TRUE;
}