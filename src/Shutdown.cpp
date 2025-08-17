#include "ClassicShutdownP.h"

DWORD SetPrivilegeAttribute(LPCWSTR PrivilegeName, DWORD NewPrivilegeAttribute, DWORD *OldPrivilegeAttribute)
{
	LUID             PrivilegeValue;
	TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
	DWORD            ReturnLength;
	HANDLE           TokenHandle;

	if (!LookupPrivilegeValueW(NULL, PrivilegeName, &PrivilegeValue))
	{
		return GetLastError();
	}

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
	{
		return GetLastError();
	}

	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
	TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

	ReturnLength = sizeof(TOKEN_PRIVILEGES);
	if (!AdjustTokenPrivileges(
		TokenHandle,
		FALSE,
		&TokenPrivileges,
		sizeof(TOKEN_PRIVILEGES),
		&OldTokenPrivileges,
		&ReturnLength
	))
	{
		CloseHandle(TokenHandle);
		return GetLastError();
	}
	else
	{
		if (OldPrivilegeAttribute != NULL)
		{
			*OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
		}
		CloseHandle(TokenHandle);
		return NO_ERROR;
	}
}

BOOL CommonShutdown(DWORD dwExit)
{
	DWORD dwOldState;
	DWORD dwErr = SetPrivilegeAttribute(SE_SHUTDOWN_NAME, SE_PRIVILEGE_ENABLED, &dwOldState);

	DWORD dwExtra = 0;

	if (dwExit == EWX_SHUTDOWN && IsPwrShutdownAllowed())
	{
		dwExtra |= EWX_POWEROFF;
	}

	switch (dwExit)
	{
		case EWX_SHUTDOWN:
		case EWX_REBOOT:
		case EWX_LOGOFF:
			if (GetKeyState(VK_CONTROL) < 0)
				dwExtra |= EWX_FORCE;
			break;
	}

	BOOL fOK = ExitWindowsEx(dwExit | dwExtra, 0);

	// Restore privilege to its previous state
	if (dwErr == ERROR_SUCCESS)
	{
		SetPrivilegeAttribute(SE_SHUTDOWN_NAME, dwOldState, nullptr);
	}
	else if (!fOK)
	{
		ShellMessageBoxW(g_hinstShell, NULL,
			dwExit == EWX_SHUTDOWN ?
			MAKEINTRESOURCEW(IDS_NO_PERMISSION_SHUTDOWN) :
			MAKEINTRESOURCEW(IDS_NO_PERMISSION_RESTART),
			dwExit == EWX_SHUTDOWN ?
			MAKEINTRESOURCEW(IDS_SHUTDOWN) :
			MAKEINTRESOURCEW(IDS_RESTART),
			MB_ICONERROR);
	}

	return fOK;
}

HRESULT DoShutdown(SHUTDOWNTYPE type)
{
	BOOL fOK = FALSE;
	switch (type)
	{
		case SHTDN_NONE:
			return S_OK;
		case SHTDN_SHUTDOWN:
			fOK = CommonShutdown(EWX_SHUTDOWN);
			break;
		case SHTDN_RESTART:
			fOK = CommonShutdown(EWX_REBOOT);
			break;
		case SHTDN_LOGOFF:
			fOK = CommonShutdown(EWX_LOGOFF);
			break;
		case SHTDN_SLEEP:
		case SHTDN_HIBERNATE:
			fOK = SetSuspendState(
				(type == SHTDN_HIBERNATE),
				(GetKeyState(VK_CONTROL) < 0),
				FALSE
			);
			break;
		case SHTDN_RESTART_DOS:
		{
			SHELLEXECUTEINFOW sei;
			ZeroMemory(&sei, sizeof(sei));
			sei.cbSize = sizeof(sei);
			sei.fMask = SEE_MASK_DOENVSUBST;
			sei.nShow = SW_SHOWNORMAL;
			sei.lpFile = L"%SystemRoot%\\System32\\cmd.exe";
			sei.lpDirectory = L"%SystemRoot%\\System32";
			fOK = ShellExecuteExW(&sei);
			break;
		}
		case SHTDN_DISCONNECT:
			fOK = WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
			break;
		default:
			return E_INVALIDARG;
	}

	if (!fOK)
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}