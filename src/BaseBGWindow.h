#pragma once
#include "ClassicShutdownP.h"

class __declspec(novtable) CBaseBGWindow
{
protected:
	HWND _hwnd;

	virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void v_AfterShown() {}

public:
	HWND CreateAndShow();
	HWND GetHWND();

	virtual ~CBaseBGWindow();
};