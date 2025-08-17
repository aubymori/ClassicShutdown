#include "DitheredWindow.h"

LPCWSTR CDitheredWindow::s_szClassName = L"DitheredWindowClass";

#define  ROP_DPna  0x000A0329

LRESULT CDitheredWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_ERASEBKGND:
			return 0;
		default:
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}

void CDitheredWindow::v_AfterShown()
{
	HDC hdc = GetDC(_hwnd);
	int cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	const WORD c_GrayBits[] = { 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA };
	HBITMAP hbmp = CreateBitmap(8, 8, 1, 1, c_GrayBits);
	HBRUSH hbr = CreatePatternBrush(hbmp);
	DeleteObject(hbmp);

	HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
	PatBlt(
		hdc,
		0, 0,
		cxVirtualScreen, cyVirtualScreen,
		ROP_DPna
	);
	SelectObject(hdc, hbrOld);
	DeleteObject(hbr);

	ReleaseDC(_hwnd, hdc);
}