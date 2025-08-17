#include "ClassicShutdownP.h"
#include "DitheredWindow.h"
#include "DimmedWindow.h"
#include "2KShutdownDialog.h"
#include "9xShutdownDialog.h"
#include "FriendlyDialog.h"
#include "LogoffDialog.h"
#include "DisconnectDialog.h"

HINSTANCE g_hinst;
HINSTANCE g_hinstShell;
DWORD     g_dwStyle;
bool      g_fStandByAvailable;
bool      g_fHibernationAvailable;
bool      g_fRemoteSession;

void *operator new(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void operator delete(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

void operator delete(void *ptr, size_t size)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

EXTERN_C STDAPI_(BOOL) DllMain(
	HINSTANCE hinstDLL,
	DWORD     fdwReason,
	LPVOID    lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_hinst = hinstDLL;
			break;
		}
		case DLL_PROCESS_DETACH:
			if (g_hkey)
				RegCloseKey(g_hkey);
			if (g_hinstShell)
				FreeLibrary(g_hinstShell);
			break;
	}
	return TRUE;
}

void UpdatePowerCapabilities()
{
	SYSTEM_POWER_CAPABILITIES spc;
	ZeroMemory(&spc, sizeof(spc));
	CallNtPowerInformation(
		SystemPowerCapabilities,
		nullptr, 0,
		&spc, sizeof(spc)
	);
	g_fStandByAvailable     = spc.SystemS1 || spc.SystemS2 || spc.SystemS3;
	g_fHibernationAvailable = spc.SystemS4 && spc.HiberFilePresent;
}

bool IsRemoteSession()
{
#if DEBUG
	DWORD dwValue = 0;
	DWORD cbValue = sizeof(DWORD);
	RegGetValueW(
		HKEY_CURRENT_USER,
		REGSTR_PATH_CLASSICSHUTDOWN,
		L"IsRemoteSession",
		RRF_RT_REG_DWORD,
		nullptr,
		(LPVOID)&dwValue,
		&cbValue
	);
	return dwValue != 0;
#else
	return GetSystemMetrics(SM_REMOTESESSION);
#endif
}

CBaseBGWindow *CreateBGWindow(HWND hwndParent, bool fLogoff)
{
	if (hwndParent || DebuggerAttached() || g_fRemoteSession)
		return nullptr;

	if (fLogoff)
	{
		switch (g_dwStyle)
		{
			case LOS_WIN2K:
				return new CDitheredWindow;
			case LOS_WINXP:
			case LOS_WINXP_GINA:
				return new CDimmedWindow;
		}
	}
	else
	{
		switch (g_dwStyle)
		{
			case SDS_WIN95:
			case SDS_WIN98:
			case SDS_WINME:
			case SDS_WIN2K:
				return new CDitheredWindow;
			case SDS_WINXP:
			case SDS_WINXP_GINA:
			case SDS_WIN03_GINA:
				return new CDimmedWindow;
		}
	}
}

HRESULT LoadShellModule(void)
{
	g_hinstShell = LoadLibraryExW(L"shell32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_DATAFILE);
	if (!g_hinstShell)
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

CLASSICSHUTDOWN_API DisplayShutdownDialog(HWND hwndParent, SHUTDOWNSTYLE style, SHUTDOWNTYPE type, PSHUTDOWNOPTIONS pOptions)
{
	if (style < SDS_USER || style >= SDS_COUNT)
		return E_INVALIDARG;

	if (style == SDS_USER)
		RETURN_IF_FAILED(ReadSettingDWORD(CSDS_SHUTDOWNSTYLE, &style));

	g_dwStyle = style;
	RETURN_IF_FAILED(LoadShellModule());
	UpdatePowerCapabilities();
	g_fRemoteSession = IsRemoteSession();
		
	CBaseBGWindow *pBGWnd = CreateBGWindow(hwndParent, false);
	if (pBGWnd)
	{
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	CBaseDialog *pDialog = nullptr;
	switch (style)
	{
		case SDS_WIN95:
		case SDS_WIN98:
			pDialog = new C9xShutdownDialog(type);
			break;
		case SDS_WINME:
		case SDS_WIN2K:
		case SDS_WINXP_GINA:
		case SDS_WIN03_GINA:
			pDialog = new C2KShutdownDialog(type, pOptions);
			break;
		case SDS_WINXP:
			pDialog = new CFriendlyDialog(false);
			break;
	}
	if (!pDialog)
		return E_OUTOFMEMORY;

	INT_PTR nResult = pDialog->Show(hwndParent);
	RETURN_IF_FAILED(DoShutdown((SHUTDOWNTYPE)nResult));

	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}

CLASSICSHUTDOWN_API DisplayLogoffDialog(HWND hwndParent, LOGOFFSTYLE style)
{
	if (style < LOS_USER || style >= LOS_COUNT)
		return E_INVALIDARG;

	if (style == LOS_USER)
		RETURN_IF_FAILED(ReadSettingDWORD(CSDS_LOGOFFSTYLE, &style));

	if (style == LOS_USER)
	{
		SHUTDOWNSTYLE shutdownStyle = SDS_USER;
		RETURN_IF_FAILED(ReadSettingDWORD(CSDS_SHUTDOWNSTYLE, &shutdownStyle));
		switch (shutdownStyle)
		{
			case SDS_WIN95:
			case SDS_WIN98:
				style = LOS_WIN98;
				break;
			case SDS_WINME:
			case SDS_WIN2K:
				style = LOS_WIN2K;
				break;
			case SDS_WINXP:
				style = LOS_WINXP;
				break;
			case SDS_WINXP_GINA:
			case SDS_WIN03_GINA:
				style = LOS_WINXP_GINA;
				break;
			default:
				style = LOS_WIN2K;
		}
	}

	g_dwStyle = style;
	RETURN_IF_FAILED(LoadShellModule());
	g_fRemoteSession = IsRemoteSession();

	// Log off dialog always uses the classic style on remote sessions
	if (g_fRemoteSession && style == LOS_WINXP)
	{
		style = LOS_WINXP_GINA;
		g_dwStyle = LOS_WINXP_GINA;
	}

	CBaseBGWindow *pBGWnd = CreateBGWindow(hwndParent, true);
	if (pBGWnd)
	{
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	CBaseDialog *pDialog = nullptr;
	switch (style)
	{
		case LOS_WIN2K:
		case LOS_WINXP_GINA:
			pDialog = new CLogoffDialog;
			break;
		case LOS_WINXP:
			pDialog = new CFriendlyDialog(true);
			break;
	}
	if (!pDialog)
		return E_OUTOFMEMORY;

	INT_PTR nResult = pDialog->Show(hwndParent);
	RETURN_IF_FAILED(DoShutdown((SHUTDOWNTYPE)nResult));

	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}

CLASSICSHUTDOWN_API DisplayDisconnectDialog(HWND hwndParent)
{
	g_fRemoteSession = IsRemoteSession();
	RETURN_IF_FAILED(LoadShellModule());

	CDimmedWindow *pBGWnd = nullptr;
	if (!hwndParent && !DebuggerAttached() && !g_fRemoteSession)
	{
		pBGWnd = new CDimmedWindow;
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	CDisconnectDialog *pDialog = new CDisconnectDialog;
	if (!pDialog)
		return E_OUTOFMEMORY;

	INT_PTR nResult = pDialog->Show(hwndParent);
	RETURN_IF_FAILED(DoShutdown((SHUTDOWNTYPE)nResult));

	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}