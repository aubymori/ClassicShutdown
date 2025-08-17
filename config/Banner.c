#include "Config.h"

COLORREF crBanner = RGB(0, 0, 0);

INT_PTR CALLBACK BannerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			WCHAR szBrandPath[MAX_PATH];
			WCHAR szBarPath[MAX_PATH];

			if (FAILED(ReadSettingString(CSSS_BRANDBITMAP, szBrandPath, ARRAYSIZE(szBrandPath)))
			|| FAILED(ReadSettingString(CSSS_BARBITMAP, szBarPath, ARRAYSIZE(szBrandPath)))
			|| FAILED(ReadSettingDWORD(CSDS_BANNERCOLOR, &crBanner)))
			{
				EndDialog(hwnd, 0);
				return TRUE;
			}

			// Ensure user input does not exceed max path length
			Edit_LimitText(GetDlgItem(hwnd, IDC_BRAND_PATH), MAX_PATH - 1);
			Edit_LimitText(GetDlgItem(hwnd, IDC_BAR_PATH),   MAX_PATH - 1);

			SetDlgItemTextW(hwnd, IDC_BRAND_PATH, szBrandPath);
			SetDlgItemTextW(hwnd, IDC_BAR_PATH,     szBarPath);

			BOOL fSolidBanner = FALSE;
			ReadSettingDWORD(CSDS_SOLIDBANNER, (LPDWORD)&fSolidBanner);

			if (fSolidBanner)
				SetDlgButtonCheck(hwnd, IDC_SOLID_BANNER, BST_CHECKED);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_BANNER_COLOR), FALSE);

			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SOLID_BANNER:
					if (HIWORD(wParam) == BN_CLICKED)
					{
						EnableWindow(
							GetDlgItem(hwnd, IDC_BANNER_COLOR),
							BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SOLID_BANNER)
						);
						SendPSMChanged(hwnd);
					}
					break;
				case IDC_BRAND_BROWSE:
				case IDC_BAR_BROWSE:
				{
					WCHAR szPath[MAX_PATH];
					szPath[0] = L'\0';

					OPENFILENAMEW ofn;
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner   = GetParent(hwnd);
					ofn.lpstrFilter = L"Bitmaps (*.bmp, *.dib)\0*.bmp;*.dib\0\0";
					ofn.lpstrFile   = szPath;
					ofn.nMaxFile    = ARRAYSIZE(szPath);
					ofn.Flags       = OFN_EXPLORER | OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
					
					if (GetOpenFileNameW(&ofn))
					{
						HWND hwndPath = GetDlgItem(
							hwnd,
							LOWORD(wParam) == IDC_BRAND_BROWSE
								? IDC_BRAND_PATH
								: IDC_BAR_PATH
						);
						SetWindowTextW(hwndPath, szPath);
						SendPSMChanged(hwnd);

						// Give focus to the edit box after filling it with the file path
						SetFocus(hwndPath);
						Edit_SetSel(hwndPath, 0, -1);
					}
					break;
				}
				case IDC_BANNER_COLOR:
				{
					// This has to be provided or else it causes an
					// access violation
					COLORREF rgCustomColors[16];
					ZeroMemory(rgCustomColors, sizeof(rgCustomColors));

					CHOOSECOLORW cc;
					ZeroMemory(&cc, sizeof(cc));
					cc.lStructSize  = sizeof(cc);
					cc.hwndOwner    = GetParent(hwnd);
					cc.rgbResult    = crBanner;
					cc.lpCustColors = rgCustomColors;
					cc.Flags        = CC_ANYCOLOR | CC_RGBINIT | CC_FULLOPEN;
					if (ChooseColorW(&cc))
					{
						crBanner = cc.rgbResult;
						SendPSMChanged(hwnd);
					}
					break;
				}
				case IDC_BRAND_PATH:
				case IDC_BAR_PATH:
					if (HIWORD(wParam) == EN_CHANGE)
						SendPSMChanged(hwnd);
					break;
			}
			return TRUE;
		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->code == PSN_APPLY)
			{
				WCHAR szPath[MAX_PATH];
				GetDlgItemTextW(hwnd, IDC_BRAND_PATH, szPath, ARRAYSIZE(szPath));
				WriteSettingString(CSSS_BRANDBITMAP, szPath);
				GetDlgItemTextW(hwnd, IDC_BAR_PATH, szPath, ARRAYSIZE(szPath));
				WriteSettingString(CSSS_BARBITMAP, szPath);

				WriteSettingDWORD(CSDS_SOLIDBANNER, BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SOLID_BANNER));
				WriteSettingDWORD(CSDS_BANNERCOLOR, crBanner);
			}
			return TRUE;
		default:
			return FALSE;
	}
}