#include "DimmedWindow.h"

#define CHUNK_SIZE 20

LPCWSTR CDimmedWindow::s_szClassName = L"DimmedWindowClass";

void CDimmedWindow::_DimPixels()
{
	ULONG *pulSrc = (ULONG *)_pvPixels;
	int cLen = _cxVirtualScreen * _cyVirtualScreen;
	constexpr int Amount = 0xD5;
	
	for (int i = cLen - 1; i >= 0; i--)
	{
		ULONG ulR = GetRValue(*pulSrc);
		ULONG ulG = GetGValue(*pulSrc);
		ULONG ulB = GetBValue(*pulSrc);
		ULONG ulGray = (54 * ulR + 183 * ulG + 19 * ulB) >> 8;
		ULONG ulTemp = ulGray * (0xFF - Amount);
		ulR = (ulR * Amount + ulTemp) >> 8;
		ulG = (ulG * Amount + ulTemp) >> 8;
		ulB = (ulB * Amount + ulTemp) >> 8;
		*pulSrc = (*pulSrc & 0xFF000000) | RGB(ulR, ulG, ulB);
		pulSrc++;
	}
}

bool CDimmedWindow::_StepDim()
{
	HDC hdc = GetDC(_hwnd);
	if (_idxChunk >= 0)
	{
		int y = _idxChunk * CHUNK_SIZE;
		BitBlt(_hdcDimmed, 0, y, _cxVirtualScreen, CHUNK_SIZE, hdc, 0, y, SRCCOPY);

		_idxChunk--;
		if (_idxChunk < 0)
		{
			SetTimer(_hwnd, 1, 250, nullptr);
			_idxSaturation = 16;
		}

		return true;
	}
	else
	{
		_DimPixels();
		BitBlt(hdc, 0, 0, _cxVirtualScreen, _cyVirtualScreen, _hdcDimmed, 0, 0, SRCCOPY);

		_idxSaturation--;
		return (_idxSaturation > 0);
	}
}

void CDimmedWindow::_SetupDim()
{
	HDC hdc = GetDC(_hwnd);
	if (hdc)
	{
		_hdcDimmed = CreateCompatibleDC(hdc);
		if (_hdcDimmed)
		{
			BITMAPINFO bmi;
			ZeroMemory(&bmi, sizeof(bmi));
			bmi.bmiHeader.biSize        = sizeof(bmi);
			bmi.bmiHeader.biWidth       = _cxVirtualScreen;
			bmi.bmiHeader.biHeight      = _cyVirtualScreen;
			bmi.bmiHeader.biPlanes      = 1;
			bmi.bmiHeader.biBitCount    = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage   = 0;

			_hbmDimmed = CreateDIBSection(_hdcDimmed, &bmi, DIB_RGB_COLORS, &_pvPixels, NULL, 0);
			if (_hbmDimmed)
			{
				_hbmOldDimmed = (HBITMAP)SelectObject(_hdcDimmed, _hbmDimmed);
				_idxChunk = _cyVirtualScreen / CHUNK_SIZE;
			}
			else
			{
				DeleteDC(_hdcDimmed);
				_hdcDimmed = NULL;
			}
		}
		ReleaseDC(_hwnd, hdc);
	}
}

LRESULT CDimmedWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			_xVirtualScreen  = GetSystemMetrics(SM_XVIRTUALSCREEN);
			_yVirtualScreen  = GetSystemMetrics(SM_YVIRTUALSCREEN);
			_cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			_cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);

			_SetupDim();
			if (_hdcDimmed)
				SetTimer(hwnd, 1, 30, nullptr);
			return 0;
		case WM_TIMER:
			if (!_StepDim())
				KillTimer(hwnd, 1);
			return 0;
		default:
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}