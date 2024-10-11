#include "ClassicShutdown.h"
#include "LogoffDlg.h"

INT_PTR CALLBACK LogoffDlgProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HICON hLogoff = LoadIconW(
                g_hShell32,
                MAKEINTRESOURCEW(45)
            );

            PostMessageW(
                GetDlgItem(hWnd, IDD_LOGOFFWINDOWS_ICON),
                STM_SETICON,
                (WPARAM)hLogoff,
                0
            );

            if (g_ssStyle == SS_XPCLASSIC)
            {
                WCHAR szBuffer[256];

                LoadStringW(
                    g_hMuiInstance,
                    IDS_LOGOFF_BTN,
                    szBuffer,
                    256
                );
                SetDlgItemText(
                    hWnd,
                    IDOK,
                    szBuffer
                );

                LoadStringW(
                    g_hMuiInstance,
                    IDS_CANCEL_BTN,
                    szBuffer,
                    256
                );
                SetDlgItemText(
                    hWnd,
                    IDCANCEL,
                    szBuffer
                );
            }

            PositionDlg(hWnd);
            break;
        }
        case WM_CLOSE:
            HandleShutdown(hWnd, SHTDN_NONE);
            break;
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDOK:
                    HandleShutdown(hWnd, SHTDN_LOGOFF);
                    break;
                case IDCANCEL:
                    HandleShutdown(hWnd, SHTDN_NONE);
                    break;
            }
            break;
        }
        /* Disable moving */
        case WM_SYSCOMMAND:
            if ((wParam & ~0x0F) == SC_MOVE)
            {
                return TRUE;
            }
            return FALSE;
            break;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                HandleShutdown(hWnd, SHTDN_NONE);
            }
            break;
        default:
            return FALSE;
    }
}