#pragma once
#include "BaseDialog.h"

/* Implements the shutdown/logoff used for the Windows XP style.
   These are grouped into one class due to extremely similar drawing code. */
class CFriendlyDialog : public CBaseDialog
{
private:
	enum
	{
		BUTTON_STATE_REST = 0,
		BUTTON_STATE_DOWN,
		BUTTON_STATE_HOVER,
		BUTTON_STATE_MAX,

		TO_BUTTON_GROUP_TURNOFF = 0,
		TO_BUTTON_GROUP_STANDBY,
		TO_BUTTON_GROUP_RESTART,
		TO_BUTTON_GROUP_MAX,

		SU_BUTTON_GROUP_SWITCHUSER = 0,
		SU_BUTTON_GROUP_LOGOFF,
		SU_BUTTON_GROUP_MAX
	};

	bool    _fLogoff;
	int     _iStandByButtonResult;
	bool    _fShiftKeyDown;
	HWND    _hwndTooltip;
	UINT    _uiFocusID;
	UINT    _uiHoverID;
	HBITMAP _hbmBackground;
	HBITMAP _hbmFlag;
	HBITMAP _hbmButtons;
	RECT    _rcBackground;
	RECT    _rcFlag;
	RECT    _rcButtons;
	HFONT   _hfTitle;
	HFONT   _hfButton;
	LONG    _lButtonHeight;

	static void s_PaintBitmap(HDC hdc, LPCRECT prcDest, HBITMAP hbm, LPCRECT prcSource);
	void _DrawItem(const DRAWITEMSTRUCT *pdis);
	void _HandleTimer();
	void _RemoveTooltip();

	static void s_FilterMetaCharacters(LPWSTR pszText);
	void _OnButtonMouseMove(HWND hwnd, UINT uiID);
	void _OnButtonMouseHover(HWND hwnd, UINT uiID);
	void _OnButtonMouseLeave(HWND hwnd, UINT uiID);
	static LRESULT CALLBACK s_ButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIDSubclass, DWORD_PTR dwRefData);

public:
	INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	CFriendlyDialog(bool fLogoff);
	~CFriendlyDialog();
};