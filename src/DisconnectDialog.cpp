#include "DisconnectDialog.h"

INT_PTR CDisconnectDialog::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hiconDisconnect = LoadIconW(g_hinstShell, MAKEINTRESOURCEW(IDI_MU_DISCONN));
			SendDlgItemMessageW(hwnd, IDD_DISCONNECTICON, STM_SETICON, (WPARAM)hiconDisconnect, 0);
			return TRUE;
		}
		case WM_COMMAND:
			EndDialog(hwnd, SHTDN_DISCONNECT);
			return TRUE;
		default:
			return FALSE;
	}
}

CDisconnectDialog::CDisconnectDialog()
{
	_uDlgID = DLG_DISCONNECTWINDOWS;
}