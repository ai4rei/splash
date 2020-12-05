/* -----------------------------------------------------------------
// Splash Screen
// (c) 2020 Ai4rei/AN
//
// This work is licensed under a
// Creative Commons Attribution-ShareAlike 4.0 International
// https://creativecommons.org/licenses/by-sa/4.0/
// -------------------------------------------------------------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "splash.h"

/* not always in windows.h */
#ifndef _ARRAYSIZE
    #define _ARRAYSIZE(X) (sizeof(X)/sizeof((X)[0]))
#endif  /* _ARRAYSIZE */

/* not in windowx.h */
#define HANDLE_WM_PRINTCLIENT(hWnd, wParam, lParam, fn) ((fn)((hWnd),(HDC)(wParam),(lParam)), 0L)

/* timers */
#define IDT_DELAYSHOW 1
#define IDT_DELAYHIDE 2

/* window class */
#define WC_SPLASHSCREEN _T("SplashScreen")

/* window data for the splash screen */
typedef struct SPLASHWINDOWDATA
{
    BITMAP bmBgInfo;
    HBITMAP hbmBackground;
    HWINEVENTHOOK hWinEventHook;
    LPBOOL lpbStop;
}
SPLASHWINDOWDATA;

static BOOL IsAnimateWindowBlendSupported(void)
{
    OSVERSIONINFO Osvi = { sizeof(Osvi) };

    if(GetVersionEx(&Osvi))
    {
        /* Windows 98 is stated to support it, too, but could not be
           confirmed in the field. */
        return Osvi.dwPlatformId!=VER_PLATFORM_WIN32s && Osvi.dwPlatformId!=VER_PLATFORM_WIN32_WINDOWS && !(Osvi.dwPlatformId==VER_PLATFORM_WIN32_NT && Osvi.dwMajorVersion<5);  /* Windows 2000+ */
    }

    return FALSE;  /* assume no */
}

static BOOL CALLBACK SplashScreenGetWindowEach(HWND hWnd, LPARAM lParam)
{
    TCHAR szClassName[_ARRAYSIZE(WC_SPLASHSCREEN)+1U];  /* +1 so that truncated names are always longer */
    HWND* lphWnd = (HWND*)lParam;

    /* Normally this would be just a grab and go, but in the world
       of shims, there may be other windows that were created behind
       our back. */
   if(GetClassName(hWnd, szClassName, _ARRAYSIZE(szClassName)))
   {
        if(lstrcmp(szClassName, WC_SPLASHSCREEN)==0)
        {/* found */
            lphWnd[0] = hWnd;
            return FALSE;  /* stop */
        }
   }

   return TRUE;  /* continue */
}

static HWND SplashScreenGetWindow(void)
{
    HWND hWnd = NULL;

    /* We could have used a global hWnd, but globals are evil. */
    EnumThreadWindows(GetCurrentThreadId(), &SplashScreenGetWindowEach, (LPARAM)&hWnd);

    return hWnd;
}

static void SplashScreenPaint(HWND const hWnd, HDC const hDC)
{
    SPLASHWINDOWDATA* lpSwd = (SPLASHWINDOWDATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(lpSwd!=NULL)
    {
        HDC hdcBitmap = CreateCompatibleDC(hDC);

        if(hdcBitmap)
        {
            if(SaveDC(hdcBitmap))
            {
                SelectObject(hdcBitmap, lpSwd->hbmBackground);
                BitBlt(hDC, 0, 0, lpSwd->bmBgInfo.bmWidth, lpSwd->bmBgInfo.bmHeight, hdcBitmap, 0, 0, SRCCOPY);
                RestoreDC(hdcBitmap, -1);
            }

            DeleteDC(hdcBitmap);
            hdcBitmap = NULL;
        }
    }
}

static VOID CALLBACK SplashScreenOnEventShow(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwEventTime)
{
    /* First window to show up in this process will cancel the
       splash screen. */
    if(dwEvent==EVENT_OBJECT_SHOW && idObject==OBJID_WINDOW && idChild==CHILDID_SELF)
    {
        HWND hWndSplash = SplashScreenGetWindow();

        /* Move the point of interest to the front, otherwise it
           will stay in the background. */
        SetForegroundWindow(hWnd);

        if(hWndSplash!=NULL)
        {
            /* Delay removal so that it does not look like the main
               window covers the splash screen. */
            SetTimer(hWndSplash, IDT_DELAYHIDE, 200, NULL);
        }
    }
}

static BOOL CALLBACK SplashScreenWndProcOnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    SPLASHWINDOWDATA* lpSwd = (SPLASHWINDOWDATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(lpSwd!=NULL)
    {/* rogue message after initialization */
        return FALSE;
    }

    lpSwd = HeapAlloc(GetProcessHeap(), 0, sizeof(lpSwd[0]));

    if(lpSwd!=NULL)
    {
        /* Detect when windows from this process are showing up. We
           intentionally do not listen for window creating, becase
           the window may be created long before it is ready to be
           shown. */
        lpSwd->hWinEventHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, NULL, &SplashScreenOnEventShow, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNTHREAD);

        if(lpSwd->hWinEventHook!=NULL)
        {
            HINSTANCE const hInstance = GetWindowInstance(hWnd);

            lpSwd->hbmBackground = LoadImage(hInstance, MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, 0, 0, 0);

            if(lpSwd->hbmBackground!=NULL)
            {
                if(GetObject(lpSwd->hbmBackground, sizeof(lpSwd->bmBgInfo), &lpSwd->bmBgInfo))
                {
                    RECT rcWA;

                    /* We are not getting dimensions of the primary
                       window, because that does not exist yet. */
                    if(SystemParametersInfo(SPI_GETWORKAREA, sizeof(rcWA), &rcWA, 0))
                    {
                        RECT rcWnd;

                        rcWnd.left   = rcWA.left+(rcWA.right-rcWA.left-lpSwd->bmBgInfo.bmWidth)/2;
                        rcWnd.top    = rcWA.top+(rcWA.bottom-rcWA.top-lpSwd->bmBgInfo.bmHeight)/2;
                        rcWnd.right  = rcWnd.left+lpSwd->bmBgInfo.bmWidth;
                        rcWnd.bottom = rcWnd.top+lpSwd->bmBgInfo.bmHeight;

                        lpSwd->lpbStop = lpCreateStruct->lpCreateParams;

                        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpSwd);
                        SetWindowPos(hWnd, HWND_TOPMOST, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, 0);

                        /* Show it later in case the main window
                           shows up immediately. */
                        SetTimer(hWnd, IDT_DELAYSHOW, 200, NULL);
                        return TRUE;
                    }
                }

                DeleteBitmap(lpSwd->hbmBackground);
                lpSwd->hbmBackground = NULL;
            }

            UnhookWinEvent(lpSwd->hWinEventHook);
            lpSwd->hWinEventHook = NULL;
        }

        HeapFree(GetProcessHeap(), 0, lpSwd);
        lpSwd = NULL;
    }

    return FALSE;
}

static VOID CALLBACK SplashScreenWndProcOnDestroy(HWND hWnd)
{
    SPLASHWINDOWDATA* lpSwd = (SPLASHWINDOWDATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(lpSwd!=NULL)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);

        if(lpSwd->lpbStop)
        {
            lpSwd->lpbStop[0] = TRUE;  /* signal that the message loop should break */
            lpSwd->lpbStop = NULL;
        }

        if(lpSwd->hbmBackground!=NULL)
        {
            DeleteBitmap(lpSwd->hbmBackground);
            lpSwd->hbmBackground = NULL;
        }

        if(lpSwd->hWinEventHook!=NULL)
        {
            UnhookWinEvent(lpSwd->hWinEventHook);
            lpSwd->hWinEventHook = NULL;
        }

        HeapFree(GetProcessHeap(), 0, lpSwd);
        lpSwd = NULL;
    }
}

static VOID CALLBACK SplashScreenWndProcOnPaint(HWND hWnd)
{
    if(GetUpdateRect(hWnd, NULL, FALSE))
    {
        PAINTSTRUCT Ps;

        if(BeginPaint(hWnd, &Ps))
        {
            SplashScreenPaint(hWnd, Ps.hdc);
            EndPaint(hWnd, &Ps);
        }
    }
}

static VOID CALLBACK SplashScreenWndProcOnPrintClient(HWND hWnd, HDC hDC, LPARAM uFlags)
{
    SplashScreenPaint(hWnd, hDC);
}

static VOID CALLBACK SplashScreenWndProcOnTimer(HWND hWnd, UINT uId)
{
    switch(uId)
    {
    case IDT_DELAYSHOW:
        if(!IsAnimateWindowBlendSupported() || !AnimateWindow(hWnd, 200, AW_BLEND))
        {
            ShowWindow(hWnd, SW_SHOWNORMAL);
        }
        break;
    case IDT_DELAYHIDE:
        if(!IsAnimateWindowBlendSupported() || !AnimateWindow(hWnd, 200, AW_BLEND|AW_HIDE))
        {
            ShowWindow(hWnd, SW_HIDE);
        }

        DestroyWindow(hWnd);
        break;
    }
}

static LRESULT CALLBACK SplashScreenWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    HANDLE_MSG(hWnd, WM_CREATE, SplashScreenWndProcOnCreate);
    HANDLE_MSG(hWnd, WM_DESTROY, SplashScreenWndProcOnDestroy);
    HANDLE_MSG(hWnd, WM_PAINT, SplashScreenWndProcOnPaint);
    HANDLE_MSG(hWnd, WM_PRINTCLIENT, SplashScreenWndProcOnPrintClient);
    HANDLE_MSG(hWnd, WM_TIMER, SplashScreenWndProcOnTimer);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static BOOL SplashScreenRegister(HINSTANCE const hInstance)
{
    WNDCLASSEX Wc = { sizeof(Wc) };

    Wc.lpfnWndProc = &SplashScreenWndProc;
    Wc.hInstance = hInstance;
    Wc.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_APPSTARTING));
    Wc.lpszClassName = WC_SPLASHSCREEN;

    /* load the process' main icon */
    Wc.hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    Wc.hIconSm = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    return RegisterClassEx(&Wc)!=0;
}

static HWND SplashScreenCreate(HINSTANCE const hInstance, LPVOID const lpContext)
{
    return CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW, WC_SPLASHSCREEN, _T(""), WS_POPUP, 0, 0, GetSystemMetrics(SM_CXMIN), GetSystemMetrics(SM_CYMIN), NULL, NULL, hInstance, lpContext);
}

static DWORD CALLBACK SplashScreenThreader(LPVOID lpParam)
{
    HINSTANCE const hInstance = (HINSTANCE)lpParam;

    if(SplashScreenRegister(hInstance))
    {
        BOOL bEnd = FALSE;

        HWND const hWnd = SplashScreenCreate(hInstance, &bEnd);

        if(hWnd!=NULL)
        {
            MSG Msg = { 0 };

            for(;;)
            {
                if(bEnd)
                {
                    break;
                }

                if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
                {
                    if(Msg.message==WM_QUIT)
                    {
                        break;
                    }

                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                    continue;
                }

                if(bEnd)
                {
                    break;
                }

                WaitMessage();
            }

            if(Msg.message==WM_QUIT)
            {/* re-post and exit */
                PostQuitMessage((int)Msg.wParam);
            }
        }
    }

    return 0;
}

void SplashScreenStart(HINSTANCE const hInstance)
{
    DWORD dwThreadId;
    HANDLE hThread;

    /* Set priority above normal to prevent the initialization code
       delaying the splash screen. */
    if((hThread = CreateThread(NULL, 0, &SplashScreenThreader, hInstance, THREAD_PRIORITY_ABOVE_NORMAL, &dwThreadId))!=NULL)
    {
        CloseHandle(hThread);
    }
}
