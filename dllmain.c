#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "splash.h"

static BOOL CALLBACK DllMainOnProcessAttach(HINSTANCE const hDll, BOOL const bStaticLoad)
{
    SplashScreenStart(hDll);
    return TRUE;  /* do not prevent process start, even if something fails */
}

static VOID CALLBACK DllMainOnProcessDetach(HINSTANCE const hDll, BOOL const bStaticFree)
{
}

BOOL CALLBACK DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        return DllMainOnProcessAttach(hDll, lpReserved!=NULL);
    case DLL_PROCESS_DETACH:
        DllMainOnProcessDetach(hDll, lpReserved!=NULL);
        break;
    }

    return TRUE;
}
