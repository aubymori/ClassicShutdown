#pragma once
#include "BaseBGWindow.h"

class CDimmedWindow : public CBaseBGWindow
{
private:
	int     _xVirtualScreen,
		    _yVirtualScreen,
		    _cxVirtualScreen,
		    _cyVirtualScreen;
	HDC     _hdcDimmed;
	HBITMAP _hbmDimmed;
	HBITMAP _hbmOldDimmed;
	PVOID   _pvPixels;
	int     _idxChunk;
	int     _idxSaturation;

	void _DimPixels();
	bool _StepDim();
	void _SetupDim();

public:
	static LPCWSTR s_szClassName;

	LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};