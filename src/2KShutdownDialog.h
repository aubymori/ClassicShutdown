#pragma once
#include "BaseDialog.h"

/* Implements the shutdown dialog used for Windows Me, 2000, XP GINA,
   and Server 2003 GINA styles. */
class C2KShutdownDialog : public CBaseDialog
{
private:
	SHUTDOWNTYPE     _type;
	PSHUTDOWNOPTIONS _pOptions;
	HBITMAP          _hbmBrand;
	HBITMAP          _hbmBar;
	SIZE             _sizeBrand;
	SIZE             _sizeBar;
	bool             _fSolidBanner;
	COLORREF         _crBanner;
	bool             _fUsingWinbrand;

	HWND             _hwndCombo;
	int              _iLogOffIndex;
	int              _iShutDownIndex;
	int              _iRestartIndex;
	int              _iRestartDOSIndex;
	int              _iStandByIndex;
	int              _iHibernateIndex;
	int              _iDisconnectIndex;

	void _MoveChildren(int dx, int dy);
	HRESULT _InitBannerFromOptions();
	HRESULT _InitBannerFromRegistry();
	HRESULT _InitBanner();

	int _AddComboString(UINT uStringID);
	void _UpdateDescription();

	INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
public:
	C2KShutdownDialog(SHUTDOWNTYPE type, PSHUTDOWNOPTIONS pOptions);
	~C2KShutdownDialog();
};