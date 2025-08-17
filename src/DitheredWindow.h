#pragma once
#include "BaseBGWindow.h"

/* Implements the dithered window from 9x/2000. */
class CDitheredWindow : public CBaseBGWindow
{
private:
	void v_AfterShown() override;

public:
	static LPCWSTR s_szClassName;

	LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};