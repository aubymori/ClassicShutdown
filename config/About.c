#include "Config.h"
#include "Version.h"

#define WSTR_INTERNAL(str) L ## str
#define WSTR(str)          WSTR_INTERNAL(str)

INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Set up version text
			WCHAR szFormat[256], szBuffer[256];
			GetDlgItemTextW(hwnd, IDC_VERSION, szFormat, ARRAYSIZE(szFormat));
			SafePrintfW(szBuffer, ARRAYSIZE(szBuffer), szFormat, WSTR(VER_PRODUCTVERSION_STR));
			SetDlgItemTextW(hwnd, IDC_VERSION, szBuffer);

			// Set up copyright text
			SetDlgItemTextW(hwnd, IDC_COPYRIGHT, WSTR(VER_LEGALCOPYRIGHT_STR));

			return TRUE;
		}
		case WM_NOTIFY:
		{
			if (wParam == IDC_GITHUB_LINK
			&& ((LPNMHDR)lParam)->code == NM_CLICK)
			{
				ShellExecuteW(
					hwnd, L"open",
					L"https://github.com/aubymori/ClassicShutdown",
					NULL, NULL,
					SW_SHOWNORMAL
				);
			}
			return TRUE;
		}
		default:
			return FALSE;
	}
}