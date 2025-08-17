#include "ClassicShutdownP.h"

HKEY g_hkey = NULL;

HRESULT EnsureRegKey()
{
	if (!g_hkey)
		return HRESULT_FROM_WIN32(RegCreateKeyExW(
			HKEY_CURRENT_USER, REGSTR_PATH_CLASSICSHUTDOWN,
			0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &g_hkey,
			nullptr
		));
	return S_OK;
}

static const struct DWORDSETTINGDEF
{
	LPCWSTR pszName;
	DWORD   dwDefault;
	DWORD   dwMin;
	DWORD   dwMax;
} c_rgDwordSettings[CSDS_COUNT] = {
	{ L"ShutdownStyle",   SDS_WIN2K,          SDS_WIN95,    SDS_COUNT - 1},
	{ L"LogoffStyle",     LOS_USER,           LOS_USER,     LOS_COUNT - 1 },
	{ L"SolidBanner",     FALSE,              FALSE,        TRUE },
	{ L"BannerColor",     RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 255, 255) },
	{ L"ShutdownSetting", SHTDN_SHUTDOWN,     SHTDN_LOGOFF, SHTDN_DISCONNECT },
	{ L"ShutdownType",    SHTDN_ALL,          SHTDN_LOGOFF, SHTDN_ALL },
};

CLASSICSHUTDOWN_API ReadSettingDWORD(DWORDSETTING setting, LPDWORD lpdwValue)
{
	if (setting < CSDS_MIN || setting >= CSDS_COUNT || !lpdwValue)
		return E_INVALIDARG;

	RETURN_IF_FAILED(EnsureRegKey());

	const DWORDSETTINGDEF *pSetting = &c_rgDwordSettings[setting];

	DWORD cb = sizeof(DWORD);
	LSTATUS lStatus = RegQueryValueExW(
		g_hkey, pSetting->pszName, nullptr, nullptr, (LPBYTE)lpdwValue, &cb
	);
	if (lStatus != ERROR_SUCCESS
	|| (*lpdwValue < pSetting->dwMin || *lpdwValue > pSetting->dwMax))
	{
		*lpdwValue = pSetting->dwDefault;
	}
	return S_OK;
}

CLASSICSHUTDOWN_API WriteSettingDWORD(DWORDSETTING setting, DWORD dwValue)
{
	if (setting < CSDS_MIN || setting >= CSDS_COUNT)
		return E_INVALIDARG;

	RETURN_IF_FAILED(EnsureRegKey());

	const DWORDSETTINGDEF *pSetting = &c_rgDwordSettings[setting];

	if (dwValue < pSetting->dwMin || dwValue > pSetting->dwMax)
		return E_INVALIDARG;

	return HRESULT_FROM_WIN32(RegSetValueExW(
		g_hkey, pSetting->pszName, 0, REG_DWORD,
		(LPBYTE)&dwValue, sizeof(DWORD)
	));
}

static const struct STRINGSETTINGDEF
{
	LPCWSTR pszName;
	LPCWSTR pszDefault;
} c_rgStringSettings[CSSS_COUNT] = {
	{ L"BrandBitmap", L"" },
	{ L"BarBitmap",   L"" },
};

CLASSICSHUTDOWN_API ReadSettingString(STRINGSETTING setting, LPWSTR szBuffer, DWORD cchBuffer)
{
	if (setting < CSSS_MIN || setting >= CSSS_COUNT || !szBuffer || !cchBuffer)
		return E_INVALIDARG;

	RETURN_IF_FAILED(EnsureRegKey());

	const STRINGSETTINGDEF *pSetting = &c_rgStringSettings[setting];

	DWORD cb = cchBuffer * sizeof(WCHAR);
	LSTATUS lStatus = RegQueryValueExW(
		g_hkey, pSetting->pszName, nullptr, nullptr,
		(LPBYTE)szBuffer, &cb
	);
	if (lStatus != ERROR_SUCCESS)
		SafeStrCpyW(szBuffer, pSetting->pszDefault, cchBuffer);
	return S_OK;
}

CLASSICSHUTDOWN_API WriteSettingString(STRINGSETTING setting, LPCWSTR szBuffer)
{
	if (setting < CSSS_MIN || setting >= CSSS_COUNT || !szBuffer)
		return E_INVALIDARG;

	RETURN_IF_FAILED(EnsureRegKey());

	const STRINGSETTINGDEF *pSetting = &c_rgStringSettings[setting];

	DWORD cb = (StrLenW(szBuffer) + 1) * sizeof(WCHAR);
	return HRESULT_FROM_WIN32(RegSetValueExW(
		g_hkey, pSetting->pszName, 0, REG_SZ,
		(LPBYTE)szBuffer, cb
	));
}