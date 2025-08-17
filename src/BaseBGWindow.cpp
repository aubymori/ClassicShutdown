#include "BaseBGWindow.h"

LRESULT CALLBACK CBaseBGWindow::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE)
	{
		CBaseBGWindow *pThis = (CBaseBGWindow *)((LPCREATESTRUCTW)lParam)->lpCreateParams;
		if (pThis)
		{
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			pThis->_hwnd = hwnd;
			return pThis->v_WndProc(hwnd, uMsg, wParam, lParam);
		}
	}

	CBaseBGWindow *pThis = (CBaseBGWindow *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (pThis)
		return pThis->v_WndProc(hwnd, uMsg, wParam, lParam);
	else
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

HWND CBaseBGWindow::CreateAndShow()
{
	WNDCLASSW wc;
	ZeroMemory(&wc, sizeof(wc));
	if (!GetClassInfoW(g_hinst, L"ClassicShutdown_Background", &wc))
	{
		wc.lpfnWndProc = s_WndProc;
		wc.hInstance = g_hinst;
		wc.lpszClassName = L"ClassicShutdown_Background";
		RegisterClassW(&wc);
	}

	HWND hwnd = CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		L"ClassicShutdown_Background",
		nullptr, WS_POPUP,
		GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
		GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
		NULL, NULL, g_hinst, this
	);

	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);

	v_AfterShown();

	return hwnd;
}

HWND CBaseBGWindow::GetHWND()
{
	return _hwnd;
}

CBaseBGWindow::~CBaseBGWindow()
{
	if (_hwnd && IsWindow(_hwnd))
		DestroyWindow(_hwnd);
}