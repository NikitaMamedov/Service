#include <windows.h>
#include <rpc.h>
#include <stdlib.h>

void* __RPC_USER MIDL_user_allocate(size_t size)
{
    return malloc(size);
}

void __RPC_USER MIDL_user_free(void* p)
{
    free(p);
}