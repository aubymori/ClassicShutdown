#include "Config.h"

INT_PTR CALLBACK GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			WCHAR szString[256];

			// Shutdown style combobox
			HWND hwndShutdownStyle = GetDlgItem(hwnd, IDC_SHUTDOWN_STYLE);
			for (int i = IDS_SDS_WIN95; i <= IDS_SDS_WIN03_GINA; i++)
			{
				LoadStringW(HINST_THISCOMPONENT, i, szString, ARRAYSIZE(szString));
				ComboBox_AddString(hwndShutdownStyle, szString);
			}

			SHUTDOWNSTYLE shutdownStyle = SDS_WIN2K;
			ReadSettingDWORD(CSDS_SHUTDOWNSTYLE, (LPDWORD)&shutdownStyle);
			// Subtract 1 because SDS_USER is not represented in the combobox
			ComboBox_SetCurSel(hwndShutdownStyle, shutdownStyle - 1);

			// Log off style combobox
			HWND hwndLogoffStyle = GetDlgItem(hwnd, IDC_LOGOFF_STYLE);
			for (int i = IDS_LOS_USER; i <= IDS_LOS_WINXP_GINA; i++)
			{
				LoadStringW(HINST_THISCOMPONENT, i, szString, ARRAYSIZE(szString));
				ComboBox_AddString(hwndLogoffStyle, szString);
			}

			LOGOFFSTYLE logoffStyle = LOS_WIN2K;
			ReadSettingDWORD(CSDS_LOGOFFSTYLE, (LPDWORD)&logoffStyle);
			ComboBox_SetCurSel(hwndLogoffStyle, logoffStyle);

			// Shutdown options checkboxes
			SHUTDOWNTYPE type = SHTDN_ALL;
			ReadSettingDWORD(CSDS_SHUTDOWNTYPE, (LPDWORD)&type);

			if (type & SHTDN_LOGOFF)
				SetDlgButtonCheck(hwnd, IDC_LOGOFF, BST_CHECKED);
			if (type & SHTDN_SHUTDOWN)
				SetDlgButtonCheck(hwnd, IDC_SHUTDOWN, BST_CHECKED);
			if (type & SHTDN_RESTART)
				SetDlgButtonCheck(hwnd, IDC_RESTART, BST_CHECKED);
			if (type & SHTDN_RESTART_DOS)
				SetDlgButtonCheck(hwnd, IDC_RESTART_DOS, BST_CHECKED);
			if (type & SHTDN_SLEEP)
				SetDlgButtonCheck(hwnd, IDC_SLEEP, BST_CHECKED);
			if (type & SHTDN_HIBERNATE)
				SetDlgButtonCheck(hwnd, IDC_HIBERNATE, BST_CHECKED);
			if (type & SHTDN_DISCONNECT)
				SetDlgButtonCheck(hwnd, IDC_DISCONNECT, BST_CHECKED);

			return TRUE;
		}
		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
				case CBN_SELCHANGE:
				case BN_CLICKED:
					SendPSMChanged(hwnd);
					break;
			}
			return TRUE;
		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->code == PSN_APPLY)
			{
				SHUTDOWNTYPE type = 0;
				if (IsDlgButtonChecked(hwnd, IDC_LOGOFF) == BST_CHECKED)
					type |= SHTDN_LOGOFF;
				if (IsDlgButtonChecked(hwnd, IDC_SHUTDOWN) == BST_CHECKED)
					type |= SHTDN_SHUTDOWN;
				if (IsDlgButtonChecked(hwnd, IDC_RESTART) == BST_CHECKED)
					type |= SHTDN_RESTART;
				if (IsDlgButtonChecked(hwnd, IDC_RESTART_DOS) == BST_CHECKED)
					type |= SHTDN_RESTART_DOS;
				if (IsDlgButtonChecked(hwnd, IDC_SLEEP) == BST_CHECKED)
					type |= SHTDN_SLEEP;
				if (IsDlgButtonChecked(hwnd, IDC_HIBERNATE) == BST_CHECKED)
					type |= SHTDN_HIBERNATE;
				if (IsDlgButtonChecked(hwnd, IDC_DISCONNECT) == BST_CHECKED)
					type |= SHTDN_DISCONNECT;
				if (type == 0)
				{
					MessageBoxW(
						GetParent(hwnd),
						L"At least one shutdown option must be checked.",
						L"ClassicShutdown",
						MB_ICONERROR
					);
					return TRUE;
				}
				WriteSettingDWORD(CSDS_SHUTDOWNTYPE, type);

				// + 1 to account for SDS_USER not being represented in combobox
				WriteSettingDWORD(CSDS_SHUTDOWNSTYLE, ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SHUTDOWN_STYLE)) + 1);
				WriteSettingDWORD(CSDS_LOGOFFSTYLE, ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LOGOFF_STYLE)));
			}
			return TRUE;
		default:
			return FALSE;
	}
}