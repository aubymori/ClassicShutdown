#include <ClassicShutdown.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "Util.h"

static const LPCWSTR c_rgShutdownStyles[SDS_COUNT] = {
	L"",
	L"win95",
	L"win98",
	L"winme",
	L"win2k",
	L"winxp",
	L"winxp_gina",
	L"win03_gina"
};

static const LPCWSTR c_rgLogoffStyles[LOS_COUNT] = {
	L"",
	L"win98",
	L"win2k",
	L"winxp",
	L"winxp_gina"
};

void WinMainCRTStartup(void)
{
	enum SHUTDOWNMODE
	{
		SDM_SHUTDOWN = 0,
		SDM_LOGOFF,
		SDM_DISCONNECT
	} mode = SDM_SHUTDOWN;
	BOOL fLogoff = FALSE;
	DWORD dwStyle = SDS_USER;

	LPWSTR pszCommandLine = GetCommandLineW();
	int nArgs;
	LPWSTR *ppszArgs = CommandLineToArgvW(pszCommandLine, &nArgs);
	
	if (nArgs == 2)
	{
		if (!StrCmpIW(ppszArgs[1], L"/?")
		|| !StrCmpIW(ppszArgs[1], L"/help"))
		{
			MessageBoxW(
				NULL,

				L"Usage:\n"
				L"    ClassicShutdown.exe [/style win95|win98|winme|win2k|winxp|winxp_gina|win03_gina]\n"
				L"    ClassicShutdown.exe /logoff [/style win98|win2k|winxp|winxp_gina]\n"
				L"    ClassicShutdown.exe /disconnect",

				L"ClassicShutdown",
				MB_ICONINFORMATION
			);
			ExitProcess(0);
		}
	}

	LPWSTR pszStyle = NULL;

	for (int i = 1; i < nArgs; i++)
	{
		if (!StrCmpIW(ppszArgs[i], L"/logoff")
		|| !StrCmpIW(ppszArgs[i], L"/disconnect"))
		{
			if (mode != SDM_SHUTDOWN)
			{
				MessageBoxW(
					NULL,
					L"Only one mode (e.g. /logoff, /disconnect) is allowed.",
					L"ClassicShutdown",
					MB_ICONERROR
				);
				ExitProcess(ERROR_INVALID_PARAMETER);
			}

			if (!StrCmpIW(ppszArgs[i], L"/disconnect"))
			{
				mode = SDM_DISCONNECT;
			}
			else
			{
				mode = SDM_LOGOFF;
			}
		}
		else if (!StrCmpIW(ppszArgs[i], L"/style"))
		{
			if (i + 1 == nArgs)
			{
				MessageBoxW(
					NULL,
					L"/style argument defined with no value.",
					L"ClassicShutdown",
					MB_ICONERROR
				);
				ExitProcess(ERROR_INVALID_PARAMETER);
			}

			pszStyle = ppszArgs[i + 1];

			i++;
		}
		else
		{
			WCHAR szMessage[512];
			SafePrintfW(szMessage, ARRAYSIZE(szMessage), L"Unrecognized argument '%s'", ppszArgs[i]);
			MessageBoxW(
				NULL,
				szMessage,
				L"ClassicShutdown",
				MB_ICONERROR
			);
			ExitProcess(ERROR_INVALID_PARAMETER);
		}
	}

	if (pszStyle)
	{
		BOOL fParsed = FALSE;

		LPCWSTR *ppszValues;
		int nValues;
		switch (mode)
		{
			case SDM_SHUTDOWN:
				ppszValues = c_rgShutdownStyles;
				nValues = ARRAYSIZE(c_rgShutdownStyles);
				break;
			case SDM_LOGOFF:
				ppszValues = c_rgLogoffStyles;
				nValues = ARRAYSIZE(c_rgLogoffStyles);
				break;
			case SDM_DISCONNECT:
				MessageBoxW(
					NULL,
					L"/style argument can't be specified for disconnect mode.",
					L"ClassicShutdown",
					MB_ICONERROR
				);
				ExitProcess(ERROR_INVALID_PARAMETER);
				break;
		}

		for (int i = 0; i < nValues; i++)
		{
			if (!StrCmpIW(ppszValues[i], pszStyle))
			{
				dwStyle = i;
				fParsed = TRUE;
			}
		}

		if (!fParsed)
		{
			WCHAR szMessage[512];
			SafePrintfW(szMessage, ARRAYSIZE(szMessage), L"Invalid value '%s' for /style argument.", pszStyle);
			MessageBoxW(
				NULL,
				szMessage,
				L"ClassicShutdown",
				MB_ICONERROR
			);
			ExitProcess(ERROR_INVALID_PARAMETER);
		}
	}

	/* Nuke Open-Shell fader if it exists,
       and simulate start menu delay */
    HWND hwndFader = FindWindowW(L"OpenShell.CMenuFader", NULL);
    if (hwndFader)
    {
        Sleep(100);
        ShowWindow(hwndFader, SW_HIDE);
    }
    /* Windows XP Explorer creates one as well, but the
       delay doesn't need to be simulated */
    else if (hwndFader = FindWindowW(L"SysFader", NULL))
    {
        /* The SysFader runs a loop that locks up the message
           loop, so we just make it completely transparent instead
           of really hiding it. */
        SetLayeredWindowAttributes(hwndFader, NULL, 0, LWA_ALPHA);
    }

	HRESULT hr;
	switch (mode)
	{
		case SDM_SHUTDOWN:
		{
			SHUTDOWNTYPE type = SHTDN_ALL;
			ReadSettingDWORD(CSDS_SHUTDOWNTYPE, (LPDWORD)&type);
			hr = DisplayShutdownDialog(NULL, dwStyle, type, NULL);
			break;
		}
		case SDM_LOGOFF:
			hr = DisplayLogoffDialog(NULL, dwStyle);
			break;
		case SDM_DISCONNECT:
			hr = DisplayDisconnectDialog(NULL);
			break;
	}

	if (FAILED(hr))
	{
		WCHAR szMessage[64];
		SafePrintfW(
			szMessage,
			ARRAYSIZE(szMessage),
			L"Failed to display the %s dialog. Error code: 0x%X",
			fLogoff ? L"logoff" : L"shutdown",
			hr
		);
	}

	LocalFree(ppszArgs);
	ExitProcess(0);
}