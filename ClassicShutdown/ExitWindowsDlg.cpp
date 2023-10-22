#include <lmcons.h>

#include "ClassicShutdown.h"
#include "ExitWindowsDlg.h"

#ifndef _ExitWindowsDlg_
#define _ExitWindowsDlg_

HBITMAP hbBrand;

void MoveChildren(HWND hWnd, int dx, int dy)
{
    HWND hWndSibling;
    RECT rc;

    //
    // walk all the children in the dialog adjusting their positions
    // by the delta.
    //

    for (hWndSibling = GetWindow(hWnd, GW_CHILD); hWndSibling; hWndSibling = GetWindow(hWndSibling, GW_HWNDNEXT))
    {
        GetWindowRect(hWndSibling, &rc);
        MapWindowPoints(NULL, GetParent(hWndSibling), (LPPOINT)&rc, 2);
        OffsetRect(&rc, dx, dy);

        SetWindowPos(hWndSibling, NULL,
            rc.left, rc.top, 0, 0,
            SWP_NOZORDER | SWP_NOSIZE);
    }

    //
    // having done that then lets adjust the parent size accordingl.
    //

    GetWindowRect(hWnd, &rc);
    MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)&rc, 2);

    SetWindowPos(hWnd, NULL,
        0, 0, (rc.right - rc.left) + dx, (rc.bottom - rc.top) + dy,
        SWP_NOZORDER | SWP_NOMOVE);
}

INT_PTR CALLBACK ExitWindowsDlgProc(
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
            HINSTANCE hBasebrd = LoadLibraryW(L"C:\\Windows\\Branding\\Basebrd\\basebrd.dll");
            if (!hBasebrd)
            {
                MessageBoxW(NULL, L"Failed to load basebrd.dll!", L"ClassicShutdown", MB_ICONERROR);
                HandleShutdown(hWnd, SHTDN_NONE);
                return FALSE;
            }

            /* The layout of basebrd.dll changed in 1607 */
            hbBrand = LoadBitmapW(
                hBasebrd,
                MAKEINTRESOURCEW(123)
            );

            if (!hbBrand)
            {
                hbBrand = LoadBitmapW(
                    hBasebrd,
                    MAKEINTRESOURCEW(121)
                );
            }

            HICON hShutDown = LoadIconW(
                g_hShell32,
                MAKEINTRESOURCEW(8240)
            );

            PostMessageW(
                GetDlgItem(hWnd, IDD_EXITWINDOWS_ICON),
                STM_SETICON,
                (WPARAM)hShutDown,
                0
            );

            /* Resize window to account for branding banner */
            BITMAP bmBrand;
            GetObjectW(hbBrand, sizeof(BITMAP), &bmBrand);

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int nWidth = rcClient.right - rcClient.left;
            int dx = 0;

            if (bmBrand.bmWidth > nWidth)
            {
                dx = (bmBrand.bmWidth - nWidth) / 2;
            }

            MoveChildren(hWnd, dx, bmBrand.bmHeight);

            /* Calculate new size (for right margin) */
            if (dx > 0)
            {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);

                RECT rcDesired;
                rcDesired.top = 0;
                rcDesired.left = 0;
                rcDesired.right = bmBrand.bmWidth;
                rcDesired.bottom = rcClient.bottom - rcClient.top;

                AdjustWindowRectEx(
                    &rcDesired,
                    GetWindowLongW(hWnd, GWL_STYLE),
                    FALSE,
                    GetWindowLongW(hWnd, GWL_EXSTYLE)
                );

                int nWidth = GetSystemMetrics(SM_CXSCREEN);
                int nHeight = GetSystemMetrics(SM_CYSCREEN);

                SetWindowPos(
                    hWnd,
                    NULL,
                    0, 0,
                    rcDesired.right - rcDesired.left,
                    rcDesired.bottom - rcDesired.top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE
                );
            }

            HWND hComboBox = GetDlgItem(hWnd, IDD_EXITWINDOWS_COMBOBOX);
            WCHAR szLogoffFormat[64], szLogoff[300], szShutdown[64], szRestart[64], szStandby[64], szLock[64], szUsername[UNLEN + 1];

            LoadStringW(g_hMuiInstance, IDS_LOGOFF, szLogoffFormat, 64);

            DWORD dwSize = UNLEN + 1;
            GetUserNameW(szUsername, &dwSize);

            wsprintfW(szLogoff, szLogoffFormat, szUsername);

            LoadStringW(g_hMuiInstance, IDS_SHUTDOWN, szShutdown, 64);
            LoadStringW(g_hMuiInstance, IDS_RESTART, szRestart, 64);
            LoadStringW(g_hMuiInstance, IDS_STANDBY, szStandby, 64);
            LoadStringW(g_hMuiInstance, IDS_LOCK, szLock, 64);

            SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szLogoff);
            SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szShutdown);
            SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szRestart);
            SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szStandby);
            SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szLock);
            SendMessageW(hComboBox, CB_SETCURSEL, 1, 0);

            WCHAR szShutdownDesc[256];
            LoadStringW(g_hMuiInstance, IDS_SHUTDOWN_DESC, szShutdownDesc, 256);

            SendMessageW(
                GetDlgItem(hWnd, IDD_EXITWINDOWS_LABEL),
                WM_SETTEXT,
                0, (LPARAM)szShutdownDesc
            );

            /* Remove close button */
            if (g_ssStyle == SS_XPCLASSIC)
            {
                DWORD dwStyle = GetWindowLongPtrW(
                    hWnd, GWL_STYLE
                );
                dwStyle &= ~WS_SYSMENU;
                SetWindowLongPtrW(
                    hWnd, GWL_STYLE, dwStyle
                );
            }

            PositionDlg(hWnd);

            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            HDC hDCMem = CreateCompatibleDC(hDC);

            HBITMAP hbmOld = (HBITMAP)SelectObject(hDCMem, hbBrand);

            BITMAP bmBrand;
            GetObjectW(hbBrand, sizeof(BITMAP), &bmBrand);

            BitBlt(
                hDC,
                0, 0,
                bmBrand.bmWidth,
                bmBrand.bmHeight,
                hDCMem,
                0, 0,
                SRCCOPY
            );

            SelectObject(hDC, hbmOld);
            DeleteDC(hDCMem);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_CLOSE:
            HandleShutdown(hWnd, SHTDN_NONE);
            return FALSE;
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int i = SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
                WCHAR szMessage[256];
                UINT uStringId = NULL;

                /* Comboboxes are zero-indexed, while SHTDN_CONSTANTS are one-indexed */
                switch (i + 1)
                {
                    case SHTDN_LOGOFF:
                        uStringId = IDS_LOGOFF_DESC;
                        break;
                    case SHTDN_SHUTDOWN:
                        uStringId = IDS_SHUTDOWN_DESC;
                        break;
                    case SHTDN_RESTART:
                        uStringId = IDS_RESTART_DESC;
                        break;
                    case SHTDN_STANDBY:
                        uStringId = IDS_STANDBY_DESC;
                        break;
                    case SHTDN_LOCK:
                        uStringId = IDS_LOCK_DESC;
                        break;
                }

                if (uStringId != NULL)
                {
                    LoadStringW(g_hMuiInstance, uStringId, szMessage, 256);
                    SendMessageW(
                        GetDlgItem(hWnd, IDD_EXITWINDOWS_LABEL),
                        WM_SETTEXT,
                        0, (LPARAM)szMessage
                    );
                }
            }
            else
            {
                switch (wParam)
                {
                    case IDOK:
                    {
                        int i = SendMessageW(
                            GetDlgItem(hWnd, IDD_EXITWINDOWS_COMBOBOX),
                            CB_GETCURSEL, 0, 0
                        );
                        /* Again, combobox zero-indexed, consts one-indexed */
                        HandleShutdown(hWnd, i + 1);
                        return FALSE;
                    }
                    case IDCANCEL:
                        HandleShutdown(hWnd, SHTDN_NONE);
                        return FALSE;
                }
            }
            break;
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
                return FALSE;
            }
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

#endif