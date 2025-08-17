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

CLASSICSHUTDOWN_API DisplayShutdownDialog(HWND hwndParent, SHUTDOWNSTYLE style, SHUTDOWNTYPE type, PSHUTDOWNOPTIONS pOptions)
{
	CBaseBGWindow         *pBGWnd   = nullptr;
	CBaseDialog           *pDialog  = nullptr;
	ULONG_PTR              ulCookie = 0;
	DPI_AWARENESS_CONTEXT  dpiOld   = 0;
	INT_PTR                nResult  = -1;
	HRESULT                hr       = S_OK;

	if (style < SDS_USER || style >= SDS_COUNT)
		return E_INVALIDARG;

	if (style == SDS_USER)
		RETURN_IF_FAILED(ReadSettingDWORD(CSDS_SHUTDOWNSTYLE, &style));

	g_dwStyle = style;
	RETURN_IF_FAILED(LoadShellModule());
	UpdatePowerCapabilities();
	g_fRemoteSession = IsRemoteSession();
		
	dpiOld = ActivateDPIAwarenessAndActCtx(&ulCookie);

	pBGWnd = CreateBGWindow(hwndParent, false);
	if (pBGWnd)
	{
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	pDialog = nullptr;
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
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	dpiOld = ActivateDPIAwarenessAndActCtx(&ulCookie);

	nResult = pDialog->Show(hwndParent);
	IFC(DoShutdown((SHUTDOWNTYPE)nResult));

cleanup:
	if (dpiOld && ulCookie)
		DeactivateDPIAwarenessAndActCtx(ulCookie, dpiOld);
	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}

CLASSICSHUTDOWN_API DisplayLogoffDialog(HWND hwndParent, LOGOFFSTYLE style)
{
	CBaseBGWindow         *pBGWnd   = nullptr;
	CBaseDialog           *pDialog  = nullptr;
	INT_PTR                nResult  = -1;
	ULONG_PTR              ulCookie = 0;
	DPI_AWARENESS_CONTEXT  dpiOld   = 0;
	HRESULT                hr       = S_OK;

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
	IFC(LoadShellModule());
	g_fRemoteSession = IsRemoteSession();

	// Log off dialog always uses the classic style on remote sessions
	if (g_fRemoteSession && style == LOS_WINXP)
	{
		style = LOS_WINXP_GINA;
		g_dwStyle = LOS_WINXP_GINA;
	}

	dpiOld = ActivateDPIAwarenessAndActCtx(&ulCookie);

	pBGWnd = CreateBGWindow(hwndParent, true);
	if (pBGWnd)
	{
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	pDialog = nullptr;
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
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	nResult = pDialog->Show(hwndParent);
	IFC(DoShutdown((SHUTDOWNTYPE)nResult));

cleanup:
	if (dpiOld && ulCookie)
		DeactivateDPIAwarenessAndActCtx(ulCookie, dpiOld);
	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}

CLASSICSHUTDOWN_API DisplayDisconnectDialog(HWND hwndParent)
{
	CDimmedWindow         *pBGWnd   = nullptr;
	CDisconnectDialog     *pDialog  = nullptr;
	ULONG_PTR              ulCookie = 0;
	DPI_AWARENESS_CONTEXT  dpiOld   = 0;
	INT_PTR                nResult  = -1;
	HRESULT                hr       = S_OK;

	g_fRemoteSession = IsRemoteSession();
	RETURN_IF_FAILED(LoadShellModule());

	dpiOld = ActivateDPIAwarenessAndActCtx(&ulCookie);

	if (!hwndParent && !DebuggerAttached() && !g_fRemoteSession)
	{
		pBGWnd = new CDimmedWindow;
		pBGWnd->CreateAndShow();
		hwndParent = pBGWnd->GetHWND();
	}

	pDialog = new CDisconnectDialog;
	if (!pDialog)
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	nResult = pDialog->Show(hwndParent);
	IFC(DoShutdown((SHUTDOWNTYPE)nResult));

cleanup:
	if (dpiOld && ulCookie)
		DeactivateDPIAwarenessAndActCtx(ulCookie, dpiOld);
	if (pBGWnd)
		delete pBGWnd;
	delete pDialog;
	return S_OK;
}