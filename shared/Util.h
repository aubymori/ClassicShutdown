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