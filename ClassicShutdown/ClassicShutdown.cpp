#include <windows.h>
#include <wtsapi32.h>
#include <powrprof.h>
#include <winnls.h>

#include "ClassicShutdown.h"
#include "ExitWindowsDlg.h"
#include "LogoffDlg.h"
#include "FriendlyDlg.h"
#include "DimmedWindow.h"
#include "mui.h"

const WCHAR DITHER_CLSNAME[] = L"ClassicShutdown_Dither";

HWND          g_hDesktopWnd, g_hDlg;
HBITMAP       g_hbDesktop;
HINSTANCE     g_hAppInstance, g_hMuiInstance, g_hShell32;
BOOL          g_bLogoff;
BOOL          g_bHibernationAvailable;
SHUTDOWNSTYLE g_ssStyle;

BrandingLoadImage_t BrandingLoadImage = nullptr;

/* Virtual screen metrics */
int x, y, cx, cy;

/* Center dialog */
void PositionDlg(HWND hDlg)
{
    RECT rc;
    GetWindowRect(hDlg, &rc);

    SetWindowPos(
        hDlg,
        HWND_TOP,
        (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3,
        0, 0,
        SWP_NOSIZE
    );
}

void HandleShutdown(HWND hWnd, DWORD dwCode)
{
    switch (dwCode)
    {
        case SHTDN_NONE:
            break;
        case SHTDN_LOGOFF:
            ExitWindowsEx(EWX_LOGOFF, 0);
            break;
        case SHTDN_SHUTDOWN:
            InitiateSystemShutdownW(
                NULL,
                NULL,
                0,
                FALSE,
                FALSE
            );
            break;
        case SHTDN_RESTART:
            ExitWindowsEx(EWX_REBOOT, 0);
            break;
        case SHTDN_STANDBY:
            SetSuspendState(FALSE, TRUE, FALSE);
            break;
        case SHTDN_HIBER:
            SetSuspendState(TRUE, FALSE, FALSE);
            break;
        case SHTDN_DISCONNECT:
            WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
            break;
        default:
        {
            WCHAR msg[256];
            wsprintfW(msg, L"Invalid result %d received", dwCode);
            MessageBoxW(NULL, msg, L"ClassicShutdown", MB_ICONERROR);
        }
    }

    EndDialog(hWnd, 0);
}

const WORD GRAY_BITS[] = { 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA };

#define  ROP_DPna  0x000A0329

HBRUSH CreateDitheredBrush(void)
{
    HBITMAP hBmp = CreateBitmap(8, 8, 1, 1, GRAY_BITS);
    HBRUSH hbr = CreatePatternBrush(hBmp);
    DeleteObject(hBmp);
    return hbr;
}

void ScreenshotDesktop(void)
{
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    g_hbDesktop = CreateCompatibleBitmap(hScreenDC, cx, cy);
    HBITMAP hbOld = (HBITMAP)SelectObject(hMemDC, g_hbDesktop);
    BitBlt(
        hMemDC,
        0, 0,
        cx, cy,
        hScreenDC,
        x, y,
        SRCCOPY
    );

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
}

LRESULT CALLBACK DitherWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        /* Haha, no thanks. No clearing for me! */
        case WM_ERASEBKGND:
            return 0;
        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow
)
{
    HMODULE hWinBrand = LoadLibraryW(L"winbrand.dll");
    if (hWinBrand)
    {
        BrandingLoadImage = (BrandingLoadImage_t)GetProcAddress(hWinBrand, "BrandingLoadImage");
    }
    if (!BrandingLoadImage)
    {
        ERRORANDQUIT(
            L"Failed to load BrandingLoadImage from winbrand.dll"
        );
    }

    /* Nuke Open-Shell fader if it exists,
       and simulate start menu delay */
    HWND hFader = FindWindowW(L"OpenShell.CMenuFader", NULL);
    if (hFader)
    {
        Sleep(100);
        ShowWindow(hFader, SW_HIDE);
    }

    /* Check if hibernation is available */
    SYSTEM_POWER_CAPABILITIES spc = { 0 };
    CallNtPowerInformation(
        SystemPowerCapabilities,
        NULL, NULL,
        &spc, sizeof(spc)
    );
    g_bHibernationAvailable = spc.HiberFilePresent;

    CDimmedWindow *pDimmedWindow = NULL;

    /* Set virtual screen metrics */
    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    /* Apply the needed privilege for shutting down */
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &luid);

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);

    
    g_hShell32 = LoadLibraryW(L"shell32.dll");

    /* Parse command line */
    int argc;
    LPWSTR *argv = CommandLineToArgvW(
        lpCmdLine, &argc
    );
    WCHAR szLocale[LOCALE_NAME_MAX_LENGTH] = { 0 };
    for (int i = 0; i < argc; i++)
    {
        if (!_wcsicmp(argv[i], L"/logoff"))
        {
            g_bLogoff = TRUE;
        }
        else if (!_wcsicmp(argv[i], L"/style"))
        {
            if (i + 1 != argc)
            {
                if (!_wcsicmp(argv[i + 1], L"classic"))
                {
                    g_ssStyle = SS_CLASSIC;
                }
                else if (!_wcsicmp(argv[i + 1], L"xpclassic"))
                {
                    g_ssStyle = SS_XPCLASSIC;
                }
                else if (!_wcsicmp(argv[i + 1], L"xp"))
                {
                    g_ssStyle = SS_XPFRIENDLY;
                }
                else
                {
                    MessageBoxW(
                        NULL,
                        L"Invalid value defined for /style parameter, defaulting to classic style.",
                        L"ClassicShutdown",
                        MB_ICONWARNING
                    );
                    g_ssStyle = SS_CLASSIC;
                }

                i++;
            }
            else
            {
                MessageBoxW(
                    NULL,
                    L"No value defined for /style parameter, defaulting to classic style.",
                    L"ClassicShutdown",
                    MB_ICONWARNING
                );
                g_ssStyle = SS_CLASSIC;
            }
        }
        else if (!_wcsicmp(argv[i], L"/lang"))
        {
            if (i + 1 != argc)
            {
                wcscpy_s(szLocale, LOCALE_NAME_MAX_LENGTH, argv[i + 1]);
            }
            else
            {
                MessageBoxW(
                    NULL,
                    L"No value defined for /lang parameter, defaulting to system locale.",
                    L"ClassicShutdown",
                    MB_ICONWARNING
                );
            }
        }
    }

    /* Set up HINSTANCEs */
    g_hAppInstance = hInstance;
    g_hMuiInstance = GetMUIModule(g_hAppInstance, szLocale);
    if (!g_hMuiInstance)
    {
        ERRORANDQUIT(
            L"Failed to load language resources.\n\nMost likely, you did not copy over files properly."
        );
    }

    if (IsXP(g_ssStyle))
    {
        pDimmedWindow = new CDimmedWindow(hInstance);
        if (pDimmedWindow != NULL)
        {
            g_hDesktopWnd = pDimmedWindow->Create();
        }
        else
        {
            ERRORANDQUIT(L"Failed to create dim window!");
        }
    }
    else
    {
        WNDCLASS wcDitherCls;
        wcDitherCls.style = 0;
        wcDitherCls.lpfnWndProc = DitherWndProc;
        wcDitherCls.cbClsExtra = 0;
        wcDitherCls.cbWndExtra = 0;
        wcDitherCls.hInstance = hInstance;
        wcDitherCls.hIcon = NULL;
        wcDitherCls.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wcDitherCls.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wcDitherCls.lpszMenuName = NULL;
        wcDitherCls.lpszClassName = DITHER_CLSNAME;
        RegisterClassW(&wcDitherCls);

        g_hDesktopWnd = CreateWindowExW(
            /* To not show in taskbar: */
            WS_EX_TOOLWINDOW,              /* dwExStyle */
            DITHER_CLSNAME,                /* lpClassName */
            L"",                           /* lpWindowName */
            WS_POPUP,                      /* dwStyle */
            x,                             /* X */
            y,                             /* Y */
            cx,                            /* nWidth */
            cy,                            /* nHeight */
            NULL,                          /* hWndParent */
            NULL,                          /* hMenu */
            hInstance,                     /* hInstance */
            NULL                           /* lpParam */
        );

        ScreenshotDesktop();

        ShowWindow(g_hDesktopWnd, SW_SHOW);
        SetForegroundWindow(g_hDesktopWnd);

        HDC hDC = GetDC(g_hDesktopWnd);
        HDC hMemDC = CreateCompatibleDC(hDC);

        HBITMAP hbOld = (HBITMAP)SelectObject(hMemDC, g_hbDesktop);

        BitBlt(
            hDC,
            0, 0,
            cx, cy,
            hMemDC,
            0, 0,
            SRCCOPY
        );

        HBRUSH hbr = CreateDitheredBrush();
        HBRUSH hbrOld = (HBRUSH)SelectObject(hDC, hbr);

        PatBlt(
            hDC,
            0, 0,
            cx, cy,
            ROP_DPna
        );

        SelectObject(hDC, hbrOld);
        DeleteObject(hbr);

        SelectObject(hMemDC, hbOld);
        DeleteDC(hMemDC);
        ReleaseDC(g_hDesktopWnd, hDC);
    }

    UINT    uDlgId;
    DLGPROC pDlgProc;

    if (g_ssStyle == SS_XPFRIENDLY)
    {
        uDlgId = g_bLogoff ? IDD_LOGOFFWINDOWS_FRIENDLY : IDD_EXITWINDOWS_FRIENDLY;
        pDlgProc = FriendlyDlgProc;
    }
    else
    {
        uDlgId = g_bLogoff ? IDD_LOGOFFWINDOWS : IDD_EXITWINDOWS;
        pDlgProc = g_bLogoff ? LogoffDlgProc : ExitWindowsDlgProc;
    }


    DialogBoxParamW(
        g_hMuiInstance,
        MAKEINTRESOURCEW(uDlgId),
        g_hDesktopWnd,
        pDlgProc,
        NULL
    );

    if (pDimmedWindow != NULL)
    {
        pDimmedWindow->Release();
    }

    if (g_hMuiInstance)
    {
        FreeLibrary(g_hMuiInstance);
    }

    return 0;
}