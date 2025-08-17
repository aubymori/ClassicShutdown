#include "Config.h"

void WinMainCRTStartup(void)
{
	WCHAR szPropTitle[128];
	LoadStringW(HINST_THISCOMPONENT, IDS_PROPTITLE, szPropTitle, ARRAYSIZE(szPropTitle));

	HANDLE hMutex = CreateMutexW(NULL, TRUE, L"ClassicShutdown_Config");
	if (hMutex && ERROR_ALREADY_EXISTS == GetLastError())
	{
		HWND hwnd = FindWindowW(MAKEINTATOM(32770), szPropTitle);
		if (hwnd)
		{
			SwitchToThisWindow(hwnd, TRUE);
		}
		ExitProcess(0);
	}

	PROPSHEETPAGEW psps[3];
	ZeroMemory(psps, sizeof(psps));

	// General
	psps[0].dwSize      = sizeof(PROPSHEETPAGEW);
	psps[0].hInstance   = HINST_THISCOMPONENT;
	psps[0].pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL);
	psps[0].pfnDlgProc  = GeneralDlgProc;

	// Banner
	psps[1].dwSize      = sizeof(PROPSHEETPAGEW);
	psps[1].hInstance   = HINST_THISCOMPONENT;
	psps[1].pszTemplate = MAKEINTRESOURCEW(IDD_BANNER);
	psps[1].pfnDlgProc  = BannerDlgProc;

	// About
	psps[2].dwSize      = sizeof(PROPSHEETPAGEW);
	psps[2].hInstance   = HINST_THISCOMPONENT;
	psps[2].pszTemplate = MAKEINTRESOURCEW(IDD_ABOUT);
	psps[2].pfnDlgProc  = AboutDlgProc;

	PROPSHEETHEADERW psh;
	ZeroMemory(&psh, sizeof(psh));
	psh.dwSize     = sizeof(psh);
	psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP;
	psh.hInstance  = HINST_THISCOMPONENT;
	psh.pszCaption = szPropTitle;
	psh.ppsp       = psps;
	psh.nPages     = ARRAYSIZE(psps);

	PropertySheetW(&psh);

	ExitProcess(0);
}