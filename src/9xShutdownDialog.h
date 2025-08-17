#pragma once
#include "BaseDialog.h"

/* Implements the shutdown dialog used for Windows 95 and 98 styles. */
class C9xShutdownDialog : public CBaseDialog
{
private:
	SHUTDOWNTYPE _type;

	void _MoveWindow(HWND hwnd, int dx, int dy);

	INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
	C9xShutdownDialog(SHUTDOWNTYPE type);
};