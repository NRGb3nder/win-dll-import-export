#include "windows.h"
#include "dllmain.h"

#define STR_TARGET "world"
#define STR_REPLACEMENT "bsuir"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    ReplaceString(GetCurrentProcessId(), STR_TARGET, STR_REPLACEMENT);
    return TRUE;
}

