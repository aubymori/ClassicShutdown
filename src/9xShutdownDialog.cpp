#include "9xShutdownDialog.h"

static const struct
{
	int          idItem;
	SHUTDOWNTYPE type;
	bool         *pfAdditionalCheck;
} c_rgShtdnDlgItems[] = {
	{ IDD_SLEEP,      SHTDN_SLEEP,         &g_fStandByAvailable },
	{ IDD_HIBERNATE,  SHTDN_HIBERNATE, &g_fHibernationAvailable },
	{ IDD_SHUTDOWN,   SHTDN_SHUTDOWN,                   nullptr },
	{ IDD_RESTART,    SHTDN_RESTART,                    nullptr },
	{ IDD_RESTARTDOS, SHTDN_RESTART_DOS,                nullptr },
	{ IDD_LOGOFF,     SHTDN_LOGOFF,                     nullptr },
};

void C9xShutdownDialog::_MoveWindow(HWND hwnd, int dx, int dy)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);
	MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rc, 2);
	OffsetRect(&rc, dx, dy);
	SetWindowPos(
		hwnd, NULL,
		rc.left, rc.top,
		0, 0,
		SWP_NOSIZE | SWP_NOZORDER
	);
}

INT_PTR C9xShutdownDialog::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hiconShutDown = LoadIconW(g_hinstShell, MAKEINTRESOURCEW(IDI_SHUTDOWN));
			SendDlgItemMessageW(hwnd, IDD_SHUTDOWNICON, STM_SETICON, (WPARAM)hiconShutDown, 0);

			RECT rcFirstItem, rcSecondItem;
			GetWindowRect(GetDlgItem(hwnd, c_rgShtdnDlgItems[0].idItem), &rcFirstItem);
			GetWindowRect(GetDlgItem(hwnd, c_rgShtdnDlgItems[1].idItem), &rcSecondItem);
			int cItemSpace = rcSecondItem.top - rcFirstItem.top;

			// Remove unsupported options and modify dialog layout as necessary
			for (int i = 0; i < ARRAYSIZE(c_rgShtdnDlgItems); i++)
			{
				if (!(_type & c_rgShtdnDlgItems[i].type)
				|| (c_rgShtdnDlgItems[i].pfAdditionalCheck && !*c_rgShtdnDlgItems[i].pfAdditionalCheck))
				{
					DestroyWindow(GetDlgItem(hwnd, c_rgShtdnDlgItems[i].idItem));

					for (int j = i + 1; j < ARRAYSIZE(c_rgShtdnDlgItems); j++)
					{
						_MoveWindow(GetDlgItem(hwnd, c_rgShtdnDlgItems[j].idItem), 0, -cItemSpace);
					}

					static const int c_rgBottomButtons[] = { IDOK, IDCANCEL, IDHELP };
					for (int j = 0; j < ARRAYSIZE(c_rgBottomButtons); j++)
					{
						_MoveWindow(GetDlgItem(hwnd, c_rgBottomButtons[j]), 0, -cItemSpace);
					}

					RECT rc;
					GetWindowRect(hwnd, &rc);
					rc.bottom -= cItemSpace;
					SetWindowPos(
						hwnd, NULL,
						0, 0,
						RECTWIDTH(rc),
						RECTHEIGHT(rc),
						SWP_NOMOVE | SWP_NOZORDER
					);
				}
			}

			_Center(2, 2);

			int idSelect = 0;
			SHUTDOWNTYPE lastType = SHTDN_NONE;
			ReadSettingDWORD(CSDS_SHUTDOWNSETTING, &lastType);
			bool fHasLastType = (_type & lastType) != 0;
			for (int i = 0; i < ARRAYSIZE(c_rgShtdnDlgItems); i++)
			{
				if ((fHasLastType && c_rgShtdnDlgItems[i].type == lastType)
				|| (!fHasLastType && GetDlgItem(hwnd, c_rgShtdnDlgItems[i].idItem)))
				{
					idSelect = c_rgShtdnDlgItems[i].idItem;
					break;
				}
			}

			CheckRadioButton(
				hwnd,
				c_rgShtdnDlgItems[0].idItem,
				c_rgShtdnDlgItems[ARRAYSIZE(c_rgShtdnDlgItems) - 1].idItem,
				idSelect
			);
			return TRUE;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
			{
				int idExit = SHTDN_NONE;
				for (int i = 0; i < ARRAYSIZE(c_rgShtdnDlgItems); i++)
				{
					if (BST_CHECKED == IsDlgButtonChecked(hwnd, c_rgShtdnDlgItems[i].idItem))
					{
						idExit = c_rgShtdnDlgItems[i].type;
						break;
					}
				}
				WriteSettingDWORD(CSDS_SHUTDOWNSETTING, idExit);
				EndDialog(hwnd, idExit);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

C9xShutdownDialog::C9xShutdownDialog(SHUTDOWNTYPE type)
	: _type(type)
{
	_uDlgID = (g_dwStyle == SDS_WIN98)
		? DLG_EXITWINDOWS_WIN98
		: DLG_EXITWINDOWS_WIN95;
}