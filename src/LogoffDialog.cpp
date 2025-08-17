#include "LogoffDialog.h"

INT_PTR CLogoffDialog::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hiconLogoff = LoadIconW(g_hinstShell, MAKEINTRESOURCEW(IDI_STLOGOFF));
			SendDlgItemMessageW(hwnd, IDD_LOGOFFICON, STM_SETICON, (WPARAM)hiconLogoff, 0);

			if (g_dwStyle == LOS_WINXP_GINA)
			{
				WCHAR szString[MAX_PATH];
				LoadStringW(g_hinst, IDS_LOGOFF_YES_XP, szString, ARRAYSIZE(szString));
				SetDlgItemTextW(hwnd, IDOK, szString);
				LoadStringW(g_hinst, IDS_LOGOFF_NO_XP, szString, ARRAYSIZE(szString));
				SetDlgItemTextW(hwnd, IDCANCEL, szString);
			}
			
			return TRUE;
		}
		case WM_COMMAND:
			EndDialog(hwnd, SHTDN_LOGOFF);
			return TRUE;
		default:
			return FALSE;
	}
}

CLogoffDialog::CLogoffDialog()
{
	_uDlgID = (g_dwStyle == LOS_WIN98) ? DLG_LOGOFFWINDOWS_WIN98 : DLG_LOGOFFWINDOWS;
}