#pragma once
#include <windows.h>
#include <shlwapi.h>

/* Wrapper for wnsprintfW that ensures null termination */
inline int SafePrintfW(PWSTR pszDest, int cchDest, PCWSTR pszFmt, ...)
{
	if (!pszDest || !pszFmt)
		return -1;

	va_list args;
	va_start(args, pszFmt);

	// Ensure null terminated
	pszDest[cchDest - 1] = L'\0';

	return wvnsprintfW(pszDest, cchDest - 1, pszFmt, args);
}

/* Wrapper for StrCpyNW that ensures null termination */
inline PWSTR SafeStrCpyW(PWSTR pszDst, PCWSTR pszSrc, int cchMax)
{
	if (!pszDst || !pszSrc || !cchMax)
		return pszDst;

	pszDst[cchMax - 1] = L'\0';
	return StrCpyNW(pszDst, pszSrc, cchMax - 1);
}

inline size_t StrLenW(LPCWSTR pszString)
{
	size_t len = 0;
	while (pszString[len])
		len++;
	return len;
}

inline UINT GetWindowDPI(HWND hwnd)
{
	typedef UINT (WINAPI *GetDpiForWindow_t)(HWND);
	static GetDpiForWindow_t pfnGetDpiForWindow;
	pfnGetDpiForWindow = (GetDpiForWindow_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");

	if (pfnGetDpiForWindow)
	{
		return pfnGetDpiForWindow(hwnd);
	}
	else
	{
		HDC hdc = GetDC(hwnd);
		UINT dpi = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(hwnd, hdc);
		return dpi;
	}
}

/* Wrapper for AdjustWindowRectEx(ForDpi) */
inline BOOL AdjustWindowRectDPI(
	LPRECT lpRect,
	DWORD  dwStyle,
	BOOL   fMenu,
	DWORD  dwExStyle,
	UINT   dpi
)
{
	typedef BOOL (WINAPI *AdjustWindowRectExForDpi_t)(LPRECT, DWORD, BOOL, DWORD, UINT);
	static AdjustWindowRectExForDpi_t pfnAdjustWindowRectExForDpi;
	pfnAdjustWindowRectExForDpi = (AdjustWindowRectExForDpi_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi");

	if (pfnAdjustWindowRectExForDpi)
		return pfnAdjustWindowRectExForDpi(lpRect, dwStyle, fMenu, dwExStyle, dpi);
	else
		return AdjustWindowRectEx(lpRect, dwStyle, fMenu, dwExStyle);
}