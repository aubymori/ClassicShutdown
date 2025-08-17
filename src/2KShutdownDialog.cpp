#include "2KShutdownDialog.h"

void C2KShutdownDialog::_MoveChildren(int dx, int dy)
{
	RECT rc;

	for (HWND hwndSibling = GetWindow(_hwnd, GW_CHILD); hwndSibling; hwndSibling = GetWindow(hwndSibling, GW_HWNDNEXT))
	{
		GetWindowRect(hwndSibling, &rc);
		MapWindowPoints(NULL, _hwnd, (LPPOINT)&rc, 2);
		OffsetRect(&rc, dx, dy);
		
		SetWindowPos(hwndSibling, NULL,
			rc.left, rc.top, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
	}

	GetWindowRect(_hwnd, &rc);
	MapWindowPoints(NULL, GetParent(_hwnd), (LPPOINT)&rc, 2);
	
	SetWindowPos(_hwnd, NULL,
        0, 0, (rc.right - rc.left) + dx, (rc.bottom - rc.top) + dy,
        SWP_NOZORDER | SWP_NOMOVE);
}

HRESULT C2KShutdownDialog::_InitBannerFromOptions()
{
	// Validate HBITMAPs
	BITMAP bm;
	if (GetObjectW(_pOptions->hbmBrand, sizeof(bm), &bm))
		_hbmBrand = _pOptions->hbmBrand;
	if (GetObjectW(_pOptions->hbmBar, sizeof(bm), &bm))
		_hbmBar = _pOptions->hbmBar;

	_fSolidBanner = _pOptions->fSolidBanner;
	_crBanner = _pOptions->crBanner;
	return S_OK;
}

HRESULT C2KShutdownDialog::_InitBannerFromRegistry()
{
	WCHAR szBrandPath[MAX_PATH], szBarPath[MAX_PATH];
	RETURN_IF_FAILED(ReadSettingString(CSSS_BRANDBITMAP, szBrandPath, ARRAYSIZE(szBrandPath)));
	RETURN_IF_FAILED(ReadSettingString(CSSS_BARBITMAP, szBarPath, ARRAYSIZE(szBarPath)));

	// Reading into a Win32 BOOL here to be safe because sizeof(DWORD) != sizeof(bool)
	BOOL fSolidBanner;
	RETURN_IF_FAILED(ReadSettingDWORD(CSDS_SOLIDBANNER, &fSolidBanner));
	_fSolidBanner = fSolidBanner;

	RETURN_IF_FAILED(ReadSettingDWORD(CSDS_BANNERCOLOR, &_crBanner));

	RTL_OSVERSIONINFOW osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	RtlGetVersion(&osvi);

	if (*szBrandPath)
		_hbmBrand = (HBITMAP)LoadImageW(NULL, szBrandPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	
	if (!_hbmBrand)
	{
		// Windows 7+ behavior
		if (osvi.dwMajorVersion > 6 || osvi.dwMinorVersion > 0)
		{
			// We set _fUsingWinbrand here so we can TransparentBlt the image with white as the transparent
			// color like Windows 7+ does.
			_hbmBrand = (HBITMAP)BrandingLoadImage(L"Basebrd", 121, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
			_fUsingWinbrand = true;
		}
		// Windows Vista behavior
		else
		{
			_hbmBrand = (HBITMAP)BrandingLoadImage(L"Basebrd", 101, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		}
	}

	if (*szBarPath)
		_hbmBar = (HBITMAP)LoadImageW(NULL, szBarPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	// Vista bar
	if (!_hbmBar && osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
	{
		_hbmBar = (HBITMAP)BrandingLoadImage(L"Basebrd", 111, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	}

	return S_OK;
}

HRESULT C2KShutdownDialog::_InitBanner()
{
	if (_pOptions)
		RETURN_IF_FAILED(_InitBannerFromOptions());
	else
		RETURN_IF_FAILED(_InitBannerFromRegistry());

	if (_hbmBrand)
	{
		BITMAP bm;
		ZeroMemory(&bm, sizeof(bm));
		GetObjectW(_hbmBrand, sizeof(BITMAP), &bm);
		_sizeBrand.cx = bm.bmWidth;
		_sizeBrand.cy = bm.bmHeight;
	}

	if (_hbmBar)
	{
		BITMAP bm;
		ZeroMemory(&bm, sizeof(bm));
		GetObjectW(_hbmBar, sizeof(BITMAP), &bm);
		_sizeBar.cx = bm.bmWidth;
		_sizeBar.cy = bm.bmHeight;
	}

	// Resize the window for the banner.
	// We are free to not care about the position here; it will be fixed
	// later by our call to CBaseDialog::_Position in the WM_INITDIALOG handler.
	if (_hbmBrand || _hbmBar)
	{
		int dx = 0;
		int dy = _sizeBrand.cy + _sizeBar.cy;
		int cx = 0;

		RECT rcClient;
		GetClientRect(_hwnd, &rcClient);

		int cxBanner = max(_sizeBrand.cx, _sizeBar.cx);
		if (cxBanner > RECTWIDTH(rcClient))
		{
			cx = cxBanner;
			dx = (cxBanner - RECTWIDTH(rcClient)) / 2;
		}

		_MoveChildren(dx, dy);

		if (cx)
		{
			RECT rcWindow;
			GetWindowRect(_hwnd, &rcWindow);

			// Add border width
			cx += RECTWIDTH(rcWindow) - RECTWIDTH(rcClient);

			SetWindowPos(
				_hwnd, NULL,
				0, 0,
				cx, RECTHEIGHT(rcWindow),
				SWP_NOMOVE | SWP_NOZORDER
			);
		}
	}
}

int C2KShutdownDialog::_AddComboString(UINT uStringID)
{
	WCHAR szBuffer[256];
	LoadStringW(g_hinst, uStringID, szBuffer, ARRAYSIZE(szBuffer));
	return ComboBox_AddString(_hwndCombo, szBuffer);
}

void C2KShutdownDialog::_UpdateDescription()
{
	int iCurSel = ComboBox_GetCurSel(_hwndCombo);
	UINT uStringID = 0;
	if (iCurSel == CB_ERR)
		return;

	if (iCurSel == _iLogOffIndex)
		uStringID = IDS_LOGOFF_DESC;
	else if (iCurSel == _iShutDownIndex)
		uStringID = IDS_SHUTDOWN_DESC;
	else if (iCurSel == _iRestartIndex)
		uStringID = IDS_RESTART_DESC;
	else if (iCurSel == _iRestartDOSIndex)
		uStringID = IDS_RESTARTDOS_DESC;
	else if (iCurSel == _iStandByIndex)
		uStringID = IDS_SLEEP_DESC;
	else if (iCurSel == _iHibernateIndex)
		uStringID = IDS_HIBERNATE_DESC;
	else if (iCurSel == _iDisconnectIndex)
		uStringID = IDS_DISCONNECT_DESC;
	else
		return;

	WCHAR szBuffer[256];
	LoadStringW(g_hinst, uStringID, szBuffer, ARRAYSIZE(szBuffer));
	SetDlgItemTextW(_hwnd, IDC_EXITOPTIONS_DESCRIPTION, szBuffer);
}

INT_PTR C2KShutdownDialog::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hiconShutDown = LoadIconW(g_hinstShell, MAKEINTRESOURCEW(IDI_SHUTDOWN));
			SendDlgItemMessageW(hwnd, IDC_EXITWINDOWS_ICON, STM_SETICON, (WPARAM)hiconShutDown, 0);

			// Initialize banner and set off-center position
			if (g_dwStyle != SDS_WINME)
			{
				if (!g_fRemoteSession)
					_InitBanner();
				_Center(2, 3);
			}

			// Remove close button
			if (g_dwStyle == SDS_WIN03_GINA)
			{
				SetWindowLongW(hwnd, GWL_STYLE, GetWindowLongW(hwnd, GWL_STYLE) & ~WS_SYSMENU);
			}

			// Set up shutdown options
			_hwndCombo = GetDlgItem(hwnd, IDC_EXITOPTIONS_COMBO);

			// Shut down is special since it shows the user's username
			if (_type & SHTDN_LOGOFF)
			{
				WCHAR szUsername[128], szFormat[256], szBuffer[256];
				DWORD cchUsername = ARRAYSIZE(szUsername);
				GetUserNameW(szUsername, &cchUsername);
				LoadStringW(g_hinst, IDS_LOGOFF_NAME, szFormat, ARRAYSIZE(szFormat));
				SafePrintfW(szBuffer, ARRAYSIZE(szBuffer), szFormat, szUsername);
				_iLogOffIndex = ComboBox_AddString(_hwndCombo, szBuffer);
			}

			if (_type & SHTDN_SHUTDOWN)
				_iShutDownIndex = _AddComboString(IDS_SHUTDOWN_NAME);
			if (_type & SHTDN_RESTART)
				_iRestartIndex = _AddComboString(IDS_RESTART_NAME);
			// The "Restart to MS-DOS mode" option doesn't show up by default in
			// WinMe, but it can be restored through patches, so I elected to make
			// it available for that style.
			if (g_dwStyle == SDS_WINME && (_type & SHTDN_RESTART_DOS))
				_iRestartDOSIndex = _AddComboString(IDS_RESTARTDOS_NAME);
			if (g_fStandByAvailable && (_type & SHTDN_SLEEP))
				_iStandByIndex = _AddComboString(IDS_SLEEP_NAME);
			if (g_fHibernationAvailable && (_type & SHTDN_HIBERNATE))
				_iHibernateIndex = _AddComboString(IDS_HIBERNATE_NAME);
			if (g_fRemoteSession && (_type & SHTDN_DISCONNECT))
				_iDisconnectIndex = _AddComboString(IDS_DISCONNECT_NAME);

			// Read and select last shutdown option from registry.
			SHUTDOWNTYPE lastType = SHTDN_NONE;
			ReadSettingDWORD(CSDS_SHUTDOWNSETTING, &lastType);
			int idxSelect = 0;
			if ((g_dwStyle == SDS_WINME || lastType != SHTDN_RESTART_DOS)
			&& (_type & lastType))
			{
				switch (lastType)
				{
					case SHTDN_LOGOFF:
						idxSelect = _iLogOffIndex;
						break;
					case SHTDN_SHUTDOWN:
						idxSelect = _iShutDownIndex;
						break;
					case SHTDN_RESTART:
						idxSelect = _iRestartIndex;
						break;
					case SHTDN_RESTART_DOS:
						idxSelect = _iRestartDOSIndex;
						break;
					case SHTDN_SLEEP:
						idxSelect = _iStandByIndex;
						break;
					case SHTDN_HIBERNATE:
						idxSelect = _iHibernateIndex;
						break;
					case SHTDN_DISCONNECT:
						idxSelect = _iDisconnectIndex;
						break;
				}
			}
			ComboBox_SetCurSel(_hwndCombo, idxSelect);

			_UpdateDescription();

			return TRUE;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			HDC hdcMem = CreateCompatibleDC(hdc);

			RECT rcClient;
			GetClientRect(hwnd, &rcClient);

			if (_fSolidBanner)
			{
				RECT rcFill = { 0, 0, RECTWIDTH(rcClient), _sizeBrand.cy };
				HBRUSH hbrFill = CreateSolidBrush(_crBanner);
				FillRect(hdc, &rcFill, hbrFill);
				DeleteObject(hbrFill);
			}

			if (_hbmBrand)
			{
				int xDest = ((RECTWIDTH(rcClient) - _sizeBrand.cx)) / 2;

				HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, _hbmBrand);
				if (_fUsingWinbrand)
					TransparentBlt(
						hdc,
						xDest, 0,
						_sizeBrand.cx, _sizeBrand.cy,
						hdcMem,
						0, 0,
						_sizeBrand.cx, _sizeBrand.cy,
						RGB(255, 255, 255)
					);
				else
					BitBlt(
						hdc,
						xDest, 0,
						_sizeBrand.cx, _sizeBrand.cy,
						hdcMem,
						0, 0,
						SRCCOPY
					);
				SelectObject(hdcMem, hbmOld);
			}

			if (_hbmBrand)
			{
				HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, _hbmBar);
				StretchBlt(
					hdc,
					0, _sizeBrand.cy,
					RECTWIDTH(rcClient), _sizeBar.cy,
					hdcMem,
					0, 0,
					_sizeBar.cx, _sizeBar.cy,
					SRCCOPY
				);
				SelectObject(hdcMem, hbmOld);
			}

			DeleteDC(hdcMem);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					int iCurSel = ComboBox_GetCurSel(_hwndCombo);
					if (iCurSel == CB_ERR)
					{
						EndDialog(hwnd, SHTDN_NONE);
						break;
					}

					int idExit = SHTDN_NONE;
					if (iCurSel == _iLogOffIndex)
						idExit = SHTDN_LOGOFF;
					else if (iCurSel == _iShutDownIndex)
						idExit = SHTDN_SHUTDOWN;
					else if (iCurSel == _iRestartIndex)
						idExit = SHTDN_RESTART;
					else if (iCurSel == _iRestartDOSIndex)
						idExit = SHTDN_RESTART_DOS;
					else if (iCurSel == _iStandByIndex)
						idExit = SHTDN_SLEEP;
					else if (iCurSel == _iHibernateIndex)
						idExit = SHTDN_HIBERNATE;
					else if (iCurSel == _iDisconnectIndex)
						idExit = SHTDN_DISCONNECT;

					WriteSettingDWORD(CSDS_SHUTDOWNSETTING, idExit);
					EndDialog(hwnd, idExit);
					break;
				}
				case IDC_EXITOPTIONS_COMBO:
					if (HIWORD(wParam) == CBN_SELCHANGE)
						_UpdateDescription();
					break;
			}
			return TRUE;
		default:
			return FALSE;
	}
}

C2KShutdownDialog::C2KShutdownDialog(SHUTDOWNTYPE type, PSHUTDOWNOPTIONS pOptions)
	: CBaseDialog()
	, _type(type)	
	, _pOptions(pOptions)
	, _hbmBrand(NULL)
	, _hbmBar(NULL)
	, _sizeBrand{ 0, 0 }
	, _sizeBar{ 0, 0 }
	, _fSolidBanner(false)
	, _crBanner(RGB(0, 0, 0))
	, _fUsingWinbrand(false)
	, _hwndCombo(NULL)
	// -2 to differentiate from CB_ERR which is -1
	, _iLogOffIndex(-2)
	, _iShutDownIndex(-2)
	, _iRestartIndex(-2)
	, _iRestartDOSIndex(-2)
	, _iStandByIndex(-2)
	, _iHibernateIndex(-2)
	, _iDisconnectIndex(-2)
{
	_uDlgID = (g_dwStyle == SDS_WINME)
		? IDD_EXITWINDOWS_DIALOG_WINME
		: IDD_EXITWINDOWS_DIALOG;
}

C2KShutdownDialog::~C2KShutdownDialog()
{
	// Options-supplied bitmaps are managed by the caller, not us.
	if (!_pOptions)
	{
		if (_hbmBrand)
			DeleteObject(_hbmBrand);
		if (_hbmBar)
			DeleteObject(_hbmBar);
	}
}