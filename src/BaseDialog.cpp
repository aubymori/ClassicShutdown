#include "BaseDialog.h"

#define IDH_TRAY_SHUTDOWN_HELP          6015

INT_PTR CALLBACK CBaseDialog::s_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		CBaseDialog *pThis = (CBaseDialog *)lParam;
		if (pThis)
		{
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->_hwnd = hwnd;
			pThis->_nResult = 0;
			return pThis->v_DlgProc(hwnd, uMsg, wParam, lParam);
		}
	}

	// Common between all dialogs
	switch (uMsg)
	{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					::EndDialog(hwnd, SHTDN_NONE);
					return TRUE;
				case IDHELP:
					WinHelpW(hwnd, L"windows.hlp>proc4", HELP_CONTEXT, (DWORD)IDH_TRAY_SHUTDOWN_HELP);
					return TRUE;
			}
			break;
		case WM_CLOSE:
			::EndDialog(hwnd, SHTDN_NONE);
			return TRUE;
		case WM_SYSCOMMAND:
			if ((wParam & ~0x0F) == SC_MOVE)
			{
				return TRUE;
			}
			return FALSE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				::EndDialog(hwnd, SHTDN_NONE);
				return FALSE;
			}
			break;
	}

	CBaseDialog *pThis = (CBaseDialog *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (pThis)
		return pThis->v_DlgProc(hwnd, uMsg, wParam, lParam);
	else
		return false;
}

void CBaseDialog::_Center(int nDivideX, int nDivideY)
{
	RECT rc;
	GetWindowRect(_hwnd, &rc);

	SetWindowPos(
		_hwnd,
		NULL,
		(GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / nDivideX,
		(GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / nDivideY,
		0, 0,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

// HACKHACK: Windows for some reason will return 0 instead of the result code set
// by EndDialog if the dialog has a parent. Hack around it by storing the result
// ourselves and returning that in the Show method instead.
void CBaseDialog::EndDialog(HWND hwnd, INT_PTR nResult)
{
	_nResult = nResult;
	::EndDialog(hwnd, nResult);
}

INT_PTR CBaseDialog::Show(HWND hwndParent)
{
	DialogBoxParamW(g_hinst, MAKEINTRESOURCEW(_uDlgID), hwndParent, s_DlgProc, (LPARAM)this);
	return _nResult;
}