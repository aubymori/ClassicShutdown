#include "FriendlyDialog.h"

void CFriendlyDialog::s_PaintBitmap(HDC hdc, LPCRECT prcDest, HBITMAP hbm, LPCRECT prcSource)
{
	RECT rcSource;

	HDC hdcBitmap = CreateCompatibleDC(NULL);
	if (hdcBitmap)
	{
		if (!prcSource)
		{
			BITMAP bm;
			if (!GetObjectW(hbm, sizeof(BITMAP), &bm))
			{
				bm.bmWidth = RECTWIDTH(*prcDest);
				bm.bmHeight = RECTHEIGHT(*prcDest);
			}
			rcSource.left   = 0;
			rcSource.top    = 0;
			rcSource.right  = bm.bmWidth;
			rcSource.bottom = bm.bmHeight;
			prcSource = &rcSource;
		}

		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBitmap, hbm);

		int iWidthSource  = RECTWIDTH(*prcSource);
		int iHeightSource = RECTHEIGHT(*prcSource);
		int iWidthDest    = RECTWIDTH(*prcDest);
		int iHeightDest   = RECTHEIGHT(*prcDest);

		bool fEqualWidthAndHeight = (iWidthSource == iWidthDest) && (iHeightSource == iHeightDest);
		int iStretchBltMode = 0;
		if (!fEqualWidthAndHeight)
		{
			iStretchBltMode = SetStretchBltMode(hdc, HALFTONE);
		}
		DWORD dwLayout = SetLayout(hdc, LAYOUT_BITMAPORIENTATIONPRESERVED);
		TransparentBlt(
			hdc,
			prcDest->left,
			prcDest->top,
			iWidthDest,
			iHeightDest,
			hdcBitmap,
			prcSource->left,
			prcSource->top,
			iWidthSource,
			iHeightSource,
			RGB(255, 0, 255)
		);
		SetLayout(hdc, dwLayout);
		if (!fEqualWidthAndHeight)
		{
			SetStretchBltMode(hdc, iStretchBltMode);
		}
		SelectObject(hdcBitmap, hbmOld);
		DeleteDC(hdcBitmap);
	}
}

void CFriendlyDialog::_DrawItem(const DRAWITEMSTRUCT *pdis)
{
	RECT  rc;
	SIZE  size;
	WCHAR szText[256];

	switch (pdis->CtlID)
	{
		case IDC_TITLE_TURNOFF:
		case IDC_TITLE_SWITCHUSER:
		{
			HFONT    hfOld = (HFONT)SelectObject(pdis->hDC, _hfTitle);
			COLORREF crOld = SetTextColor(pdis->hDC, RGB(255, 255, 255));
			int      iBkOld = SetBkMode(pdis->hDC, TRANSPARENT);

			GetWindowTextW(GetDlgItem(_hwnd, pdis->CtlID), szText, ARRAYSIZE(szText));
			GetTextExtentPointW(pdis->hDC, szText, lstrlenW(szText), &size);
			CopyRect(&rc, &pdis->rcItem);
			InflateRect(&rc, 0, -((rc.bottom - rc.top - size.cy) / 2));

			DrawTextW(pdis->hDC, szText, -1, &rc, 0);

			SetBkMode(pdis->hDC, iBkOld);
			SetTextColor(pdis->hDC, crOld);
			SelectObject(pdis->hDC, hfOld);
			break;
		}
		case IDC_TITLE_FLAG:
		{
			GetClientRect(pdis->hwndItem, &rc);
			int iFlagWidth  = RECTWIDTH(_rcFlag);
			int iFlagHeight = RECTHEIGHT(_rcFlag);

			rc.left += (RECTWIDTH(rc) - iFlagWidth) / 2;
			rc.right = rc.left + iFlagWidth;
			rc.top += (RECTHEIGHT(rc) - iFlagHeight) / 2;
			rc.bottom = rc.top + iFlagHeight;

			s_PaintBitmap(pdis->hDC, &rc, _hbmFlag, &_rcFlag);
			break;
		}
		case IDC_TEXT_TURNOFF:
		case IDC_TEXT_STANDBY:
		case IDC_TEXT_RESTART:
		case IDC_TEXT_SWITCHUSER:
		case IDC_TEXT_LOGOFF:
		{
			int iButtonID;
			switch (pdis->CtlID)
			{
				case IDC_TEXT_TURNOFF:
					iButtonID = IDC_BUTTON_TURNOFF;
					break;
				case IDC_TEXT_STANDBY:
					iButtonID = (_iStandByButtonResult == SHTDN_HIBERNATE)
						? IDC_BUTTON_HIBERNATE
						: IDC_BUTTON_STANDBY;
					break;
				case IDC_TEXT_RESTART:
					iButtonID = IDC_BUTTON_RESTART;
					break;
				case IDC_TEXT_SWITCHUSER:
					iButtonID = IDC_BUTTON_SWITCHUSER;
					break;
				case IDC_TEXT_LOGOFF:
					iButtonID = IDC_BUTTON_LOGOFF;
					break;
			}

			HFONT hfOld = (HFONT)SelectObject(pdis->hDC, _hfButton);
			COLORREF crText = RGB(255, 255, 255);
			if (pdis->CtlID == IDC_TEXT_STANDBY && !g_fStandByAvailable && !g_fHibernationAvailable)
			{
				crText = RGB(160, 160, 160);
			}

			RECT     rcText;
			COLORREF crOld = SetTextColor(pdis->hDC, crText);
			int      iBkOld = SetBkMode(pdis->hDC, TRANSPARENT);
			GetWindowTextW(GetDlgItem(_hwnd, iButtonID), szText, ARRAYSIZE(szText));
			CopyRect(&rcText, &pdis->rcItem);
			int iPixelHeight = DrawTextW(pdis->hDC, szText, -1, &rcText, DT_CALCRECT);
			CopyRect(&rc, &pdis->rcItem);
			InflateRect(&rc, -((rc.right - rc.left - (rcText.right - rcText.left)) / 2), -((rc.bottom - rc.top - iPixelHeight) / 2));

			DrawTextW(pdis->hDC, szText, -1, &rc, ((pdis->itemState & ODS_NOACCEL) != 0) ? DT_HIDEPREFIX : 0);

			SetBkMode(pdis->hDC, iBkOld);
			SetTextColor(pdis->hDC, crOld);
			SelectObject(pdis->hDC, hfOld);

			break;
		}
		case IDC_BUTTON_TURNOFF:
		case IDC_BUTTON_STANDBY:
		case IDC_BUTTON_RESTART:
		case IDC_BUTTON_SWITCHUSER:
		case IDC_BUTTON_LOGOFF:
		{
			int iState, iGroup;

			if (pdis->itemState & ODS_SELECTED)
			{
				iState = BUTTON_STATE_DOWN;
			}
			else if (_uiHoverID == pdis->CtlID || (pdis->itemState & ODS_FOCUS))
			{
				iState = BUTTON_STATE_HOVER;
			}
			else
			{
				iState = BUTTON_STATE_REST;
			}

			switch (pdis->CtlID)
			{
				case IDC_BUTTON_TURNOFF:
					iGroup = TO_BUTTON_GROUP_TURNOFF;
					break;
				case IDC_BUTTON_STANDBY:
					if (_iStandByButtonResult == SHTDN_NONE)
					{
						iGroup = TO_BUTTON_GROUP_MAX;
						iState = 0;
					}
					else
					{
						iGroup = TO_BUTTON_GROUP_STANDBY;
					}
					break;
				case IDC_BUTTON_RESTART:
					iGroup = TO_BUTTON_GROUP_RESTART;
					break;
				case IDC_BUTTON_SWITCHUSER:
					iGroup = SU_BUTTON_GROUP_SWITCHUSER;
					break;
				case IDC_BUTTON_LOGOFF:
					iGroup = SU_BUTTON_GROUP_LOGOFF;
					break;
			}

			if (iGroup >= 0)
			{
				// First paint background:
				RECT rc;
				CopyRect(&rc, &_rcBackground);
				MapWindowPoints(pdis->hwndItem, _hwnd, (LPPOINT)&rc, 2);
				rc.right = rc.left + RECTWIDTH(_rcButtons);
				rc.bottom = rc.top + _lButtonHeight;
				s_PaintBitmap(pdis->hDC, &pdis->rcItem, _hbmBackground, &rc);

				// Then paint button:
				CopyRect(&rc, &_rcButtons);
				rc.top = ((iGroup * BUTTON_STATE_MAX) + iState) * _lButtonHeight;
				rc.bottom = rc.top + _lButtonHeight;
				s_PaintBitmap(pdis->hDC, &pdis->rcItem, _hbmButtons, &rc);
			}

			break;
		}
	}
}

void CFriendlyDialog::_HandleTimer()
{
	bool fShiftKeyDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	if (fShiftKeyDown != _fShiftKeyDown)
	{
		_fShiftKeyDown = fShiftKeyDown;

		_iStandByButtonResult = (_iStandByButtonResult == SHTDN_HIBERNATE)
			? SHTDN_SLEEP
			: SHTDN_HIBERNATE;

		HWND hwndText = GetDlgItem(_hwnd, IDC_TEXT_STANDBY);
		RECT rc;
		GetClientRect(hwndText, &rc);
		MapWindowPoints(hwndText, _hwnd, (LPPOINT)&rc, 2);
		InvalidateRect(_hwnd, &rc, TRUE);

		if (_uiHoverID == IDC_BUTTON_STANDBY && _hwndTooltip)
		{
			_RemoveTooltip();
			_uiHoverID = 0;
		}
	}
}

void CFriendlyDialog::_RemoveTooltip()
{
	if (_hwndTooltip)
	{
		DestroyWindow(_hwndTooltip);
		_hwndTooltip = NULL;
	}
}

void CFriendlyDialog::s_FilterMetaCharacters(LPWSTR pszText)
{
	WCHAR *pTC;

	pTC = pszText;
	while (*pTC != L'\0')
	{
		if (*pTC == L'&')
		{
			(WCHAR *)lstrcpyW(pTC, pTC + 1);
		}
		else
		{
			++pTC;
		}
	}
}

void CFriendlyDialog::_OnButtonMouseMove(HWND hwnd, UINT uiID)
{
	SetCursor(LoadCursorW(NULL, IDC_HAND));
	if (uiID != _uiHoverID)
	{
		_uiHoverID = uiID;
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.hwndTrack = hwnd;
		tme.dwHoverTime = HOVER_DEFAULT;
		TrackMouseEvent(&tme);
		InvalidateRect(hwnd, nullptr, FALSE);
	}
}

void CFriendlyDialog::_OnButtonMouseHover(HWND hwnd, UINT uiID)
{
	int iTextID;
	HWND hwndCaption = hwnd;
	switch (uiID)
	{
		case IDC_BUTTON_TURNOFF:
			iTextID = IDS_TURNOFF_TOOLTIP_TEXT_TURNOFF;
			break;
		case IDC_BUTTON_STANDBY:
			switch (_iStandByButtonResult)
			{
				case SHTDN_SLEEP:
					iTextID = g_fHibernationAvailable
						? IDS_TURNOFF_TOOLTIP_TEXT_STANDBY_HIBERNATE
						: IDS_TURNOFF_TOOLTIP_TEXT_STANDBY;
					break;
				case SHTDN_HIBERNATE:
					hwndCaption = GetDlgItem(_hwnd, IDC_BUTTON_HIBERNATE);
					iTextID = IDS_TURNOFF_TOOLTIP_TEXT_HIBERNATE;
					break;
			}
			break;
		case IDC_BUTTON_RESTART:
			iTextID = IDS_TURNOFF_TOOLTIP_TEXT_RESTART;
			break;
		case IDC_BUTTON_SWITCHUSER:
			iTextID = IDS_SWITCHUSER_TOOLTIP_TEXT_SWITCHUSER;
			break;
		case IDC_BUTTON_LOGOFF:
			iTextID = IDS_SWITCHUSER_TOOLTIP_TEXT_LOGOFF;
			break;
	}

	_hwndTooltip = CreateWindowExW(
		0,
		TOOLTIPS_CLASSW,
		NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd, NULL,
		g_hinst, NULL
	);

	if (_hwndTooltip)
	{
		SetWindowPos(_hwndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		SendMessageW(_hwndTooltip, CCM_SETVERSION, COMCTL32_VERSION, 0);

		WCHAR szText[512];
		LoadStringW(g_hinst, iTextID, szText + 2, ARRAYSIZE(szText) - 2);
		szText[0] = L'\r';
		szText[1] = L'\n';

		TOOLINFOW ti = { 0 };
		ti.cbSize = sizeof(ti);
		ti.uFlags = TTF_TRANSPARENT | TTF_TRACK;
		ti.uId = PtrToUint(_hwndTooltip);
		ti.lpszText = szText;
		SendMessageW(_hwndTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);

		HDC hdc = GetDC(_hwndTooltip);
		int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(_hwndTooltip, hdc);
		SendMessageW(_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, MulDiv(300, dpi, 96));

		RECT rc;
		GetWindowRect(hwnd, &rc);
		LONG lPosX = (rc.left + rc.right) / 2;
		LONG lPosY = rc.bottom;
		SendMessageW(_hwndTooltip, TTM_TRACKPOSITION, 0, MAKELONG(lPosX, lPosY));

		WCHAR szTitle[256];
		GetWindowTextW(hwndCaption, szTitle, ARRAYSIZE(szText));
		s_FilterMetaCharacters(szTitle);
		SendMessageW(_hwndTooltip, TTM_SETTITLEW, 0, (LPARAM)szTitle);
		SendMessageW(_hwndTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
	}
}

void CFriendlyDialog::_OnButtonMouseLeave(HWND hwnd, UINT uiID)
{
	_RemoveTooltip();
	_uiHoverID = 0;
	InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK CFriendlyDialog::s_ButtonSubclassProc(
	HWND      hwnd,
	UINT      uMsg,
	WPARAM    wParam,
	LPARAM    lParam,
	UINT_PTR  uIDSubclass,
	DWORD_PTR dwRefData)
{
	CFriendlyDialog *pThis = (CFriendlyDialog *)dwRefData;

	LRESULT lResult = 0;
	switch (uMsg)
	{
		case BM_SETSTYLE:
			if (wParam == BS_DEFPUSHBUTTON)
			{
				pThis->_uiFocusID = uIDSubclass;
			}
			lResult = 0;
			break;
		default:
			lResult = DefSubclassProc(hwnd, uMsg, wParam, lParam);
			switch (uMsg)
			{
				case DM_GETDEFID:
					lResult = (DC_HASDEFID << 16) | (WORD)pThis->_uiFocusID;
					break;
				case WM_GETDLGCODE:
					if (uIDSubclass == pThis->_uiFocusID)
						lResult |= DLGC_DEFPUSHBUTTON;
					else
						lResult |= DLGC_UNDEFPUSHBUTTON;
					break;
				case WM_MOUSEMOVE:
					pThis->_OnButtonMouseMove(hwnd, uIDSubclass);
					break;
				case WM_MOUSEHOVER:
					pThis->_OnButtonMouseHover(hwnd, uIDSubclass);
					break;
				case WM_MOUSELEAVE:
					pThis->_OnButtonMouseLeave(hwnd, uIDSubclass);
					break;
			}
	}
	return lResult;
}

INT_PTR CFriendlyDialog::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			_Center(2, 3);

			_hbmBackground = (HBITMAP)LoadImageW(g_hinst, MAKEINTRESOURCEW(IDB_BACKGROUND), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			_hbmFlag       = (HBITMAP)LoadImageW(g_hinst, MAKEINTRESOURCEW(IDB_FLAG), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			_hbmButtons    = (HBITMAP)LoadImageW(g_hinst, MAKEINTRESOURCEW(_fLogoff ? IDB_SU_BUTTONS : IDB_TO_BUTTONS),  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

			BITMAP bm;
			if (GetObjectW(_hbmBackground, sizeof(BITMAP), &bm))
			{
				_rcBackground.left   = 0;
				_rcBackground.top    = 0;
				_rcBackground.right  = bm.bmWidth;
				_rcBackground.bottom = bm.bmHeight;
			}
			if (GetObjectW(_hbmFlag, sizeof(BITMAP), &bm))
			{
				_rcFlag.left = 0;
				_rcFlag.top = 0;
				_rcFlag.right = bm.bmWidth;
				_rcFlag.bottom = bm.bmHeight;
			}
			if (GetObjectW(_hbmButtons, sizeof(BITMAP), &bm))
			{
				_rcButtons.left = 0;
				_rcButtons.top = 0;
				_rcButtons.right = bm.bmWidth;
				_rcButtons.bottom = bm.bmHeight;

				int iDivisor;
				if (_fLogoff)
					iDivisor = SU_BUTTON_GROUP_MAX * BUTTON_STATE_MAX;
				else
					iDivisor = TO_BUTTON_GROUP_MAX * BUTTON_STATE_MAX + 1;
				_lButtonHeight = bm.bmHeight / iDivisor;
			}

			HDC hdc = GetDC(hwnd);
			int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(hwnd, hdc);

			char szPixelSize[10];
			LOGFONTW lf;

			if (LoadStringA(
				g_hinst, IDS_TURNOFF_TITLE_FACESIZE,
				szPixelSize, ARRAYSIZE(szPixelSize)
			))
			{
				ZeroMemory(&lf, sizeof(lf));
				lf.lfHeight = -MulDiv(StrToIntA(szPixelSize), dpi, 72);
				if (LoadStringW(g_hinst, IDS_TURNOFF_TITLE_FACENAME, lf.lfFaceName, LF_FACESIZE))
				{
					lf.lfWeight = FW_MEDIUM;
					_hfTitle = CreateFontIndirectW(&lf);
				}
			}

			if (LoadStringA(
				g_hinst, IDS_TURNOFF_BUTTON_FACESIZE,
				szPixelSize, ARRAYSIZE(szPixelSize)
			))
			{
				ZeroMemory(&lf, sizeof(lf));
				lf.lfHeight = -MulDiv(StrToIntA(szPixelSize), dpi, 72);
				if (LoadStringW(g_hinst, IDS_TURNOFF_BUTTON_FACENAME, lf.lfFaceName, LF_FACESIZE))
				{
					lf.lfWeight = FW_BOLD;
					_hfButton = CreateFontIndirectW(&lf);
				}
			}

			if (!_fLogoff)
			{
				SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_TURNOFF), s_ButtonSubclassProc, IDC_BUTTON_TURNOFF, (DWORD_PTR)this);
				SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_STANDBY), s_ButtonSubclassProc, IDC_BUTTON_STANDBY, (DWORD_PTR)this);
				SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_RESTART), s_ButtonSubclassProc, IDC_BUTTON_RESTART, (DWORD_PTR)this);

				HWND hwndStandBy   = GetDlgItem(hwnd, IDC_BUTTON_STANDBY);
				HWND hwndHibernate = GetDlgItem(hwnd, IDC_BUTTON_HIBERNATE);
				if (g_fStandByAvailable)
				{
					_iStandByButtonResult = SHTDN_SLEEP;
					if (g_fHibernationAvailable)
					{
						// Stand by + hibernate
						_fShiftKeyDown = false;
						SetTimer(hwnd, 1, 50, nullptr);
					}
					else
					{
						// Stand by only
						EnableWindow(hwndHibernate, FALSE);
					}
				}
				else if (g_fHibernationAvailable)
				{
					// Hibernate only.
					_iStandByButtonResult = SHTDN_HIBERNATE;

					// Set stand-by button text to hibernate text so user can't
					// trigger Hibernate through stand-by's acclerator
					WCHAR szText[256];
					GetWindowTextW(hwndHibernate, szText, ARRAYSIZE(szText));
					SetWindowTextW(hwndStandBy, szText);
				}
				else
				{
					// No stand by or hibernate
					EnableWindow(hwndStandBy, FALSE);
					EnableWindow(hwndHibernate, FALSE);
					_iStandByButtonResult = SHTDN_NONE;
				}

				_uiFocusID = IDC_BUTTON_TURNOFF;
				if (g_fStandByAvailable || g_fHibernationAvailable)
				{
					_uiFocusID = IDC_BUTTON_STANDBY;
				}
				SetFocus(GetDlgItem(_hwnd, _uiFocusID));
				SendMessageW(_hwnd, DM_SETDEFID, _uiFocusID, 0);
			}
			else
			{
				SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_SWITCHUSER), s_ButtonSubclassProc, IDC_BUTTON_SWITCHUSER, (DWORD_PTR)this);
				SetWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_LOGOFF),     s_ButtonSubclassProc, IDC_BUTTON_LOGOFF,     (DWORD_PTR)this);

				_uiFocusID = IDC_BUTTON_SWITCHUSER;
				SetFocus(GetDlgItem(_hwnd, _uiFocusID));
				SendMessageW(_hwnd, DM_SETDEFID, _uiFocusID, 0);
			}

			return FALSE;
		}
		case WM_DESTROY:
		{
			if (_fLogoff)
			{
				RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_SWITCHUSER), s_ButtonSubclassProc, IDC_BUTTON_SWITCHUSER);
				RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_LOGOFF),     s_ButtonSubclassProc, IDC_BUTTON_LOGOFF);
			}
			else
			{
				RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_TURNOFF), s_ButtonSubclassProc, IDC_BUTTON_TURNOFF);
				RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_STANDBY), s_ButtonSubclassProc, IDC_BUTTON_STANDBY);
				RemoveWindowSubclass(GetDlgItem(hwnd, IDC_BUTTON_RESTART), s_ButtonSubclassProc, IDC_BUTTON_RESTART);
			}
			return 0;
		}
		case WM_PRINTCLIENT:
			if (!(lParam & PRF_ERASEBKGND) || !(lParam & PRF_CLIENT))
				return 1;
			// fall-thru
		case WM_ERASEBKGND:
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			s_PaintBitmap((HDC)wParam, &rc, _hbmBackground, &_rcBackground);
			return 1;
		}
		case WM_DRAWITEM:
			_DrawItem((const DRAWITEMSTRUCT *)lParam);
			return TRUE;
		case WM_TIMER:
			_HandleTimer();
			return 0;
		case WM_COMMAND:
		{
			if (HIWORD(wParam) != BN_CLICKED)
				return TRUE;

			int idExit = SHTDN_NONE;
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_STANDBY:
					idExit = _iStandByButtonResult;
					break;
				// For accelerator:
				case IDC_BUTTON_HIBERNATE:
					idExit = SHTDN_HIBERNATE;
					break;
				case IDC_BUTTON_TURNOFF:
					idExit = SHTDN_SHUTDOWN;
					break;
				case IDC_BUTTON_RESTART:
					idExit = SHTDN_RESTART;
					break;
				case IDC_BUTTON_SWITCHUSER:
					idExit = SHTDN_DISCONNECT;
					break;
				case IDC_BUTTON_LOGOFF:
					idExit = SHTDN_LOGOFF;
					break;
			}

			if (idExit != SHTDN_NONE)
			{
				_RemoveTooltip();
				EndDialog(hwnd, idExit);
			}
			return TRUE;
		}
		default:
			return FALSE;
	}
}

CFriendlyDialog::CFriendlyDialog(bool fLogoff)
	: _fLogoff(fLogoff)
{
	_uDlgID = fLogoff ? DLG_SWITCHUSER : IDD_TURNOFFCOMPUTER;
}

CFriendlyDialog::~CFriendlyDialog()
{
	if (_hbmBackground)
		DeleteObject(_hbmBackground);
	if (_hbmFlag)
		DeleteObject(_hbmFlag);
	if (_hbmButtons)
		DeleteObject(_hbmButtons);
	if (_hfTitle)
		DeleteObject(_hfTitle);
	if (_hfButton)
		DeleteObject(_hfButton);
}