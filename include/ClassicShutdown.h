#pragma once
#include <windows.h>

#ifdef CLASSICSHUTDOWN_IMPLEMENTATION
#define EXPORTED_API  __declspec(dllexport)
#else
#define EXPORTED_API  __declspec(dllimport)
#endif

#define CLASSICSHUTDOWN_API_(type)  EXTERN_C EXPORTED_API type WINAPI
#define CLASSICSHUTDOWN_API         CLASSICSHUTDOWN_API_(HRESULT)

/**
  * SDS_USER is passed to DisplayShutdownDialog to use the style from
  * registry. It can *not* be set in the registry.
  */
typedef enum _SHUTDOWNSTYLE
{
	SDS_USER = 0,
	SDS_WIN95,
	SDS_WIN98,
	SDS_WINME,
	SDS_WIN2K,
	SDS_WINXP,
	SDS_WINXP_GINA,
	SDS_WIN03_GINA,
	SDS_COUNT
} SHUTDOWNSTYLE;

/**
  * LOS_USER is passed to DisplayLogoffDialog to use the style from
  * registry. It *can* be set in registry, and if it is, then the shutdown
  * style will be mapped to one of these.
  */
typedef enum _LOGOFFSTYLE
{
	LOS_USER = 0,
	LOS_WIN98,
	LOS_WIN2K,
	LOS_WINXP,
	LOS_WINXP_GINA,
	LOS_COUNT
} LOGOFFSTYLE;

typedef enum _SHUTDOWNTYPE
{
	SHTDN_NONE        = 0,
	SHTDN_LOGOFF      = (1 << 0),
	SHTDN_SHUTDOWN    = (1 << 1),
	SHTDN_RESTART     = (1 << 2),
	SHTDN_RESTART_DOS = (1 << 3),
	SHTDN_SLEEP       = (1 << 4),
	SHTDN_HIBERNATE   = (1 << 5),
	SHTDN_DISCONNECT  = (1 << 6),
	SHTDN_ALL         = SHTDN_LOGOFF | SHTDN_SHUTDOWN | SHTDN_RESTART |
						SHTDN_RESTART_DOS |SHTDN_SLEEP | SHTDN_HIBERNATE | 
						SHTDN_DISCONNECT
} SHUTDOWNTYPE;

/* Options for Windows 2000, XP GINA, and Server 2003 GINA styled shutdown dialogs. */
typedef struct _SHUTDOWNOPTIONS
{
	HBITMAP  hbmBrand;
	HBITMAP  hbmBar;
	BOOL     fSolidBanner;
	COLORREF crBanner;
} SHUTDOWNOPTIONS, *PSHUTDOWNOPTIONS;

CLASSICSHUTDOWN_API   DisplayShutdownDialog(HWND hwndParent, SHUTDOWNSTYLE style, SHUTDOWNTYPE type, PSHUTDOWNOPTIONS pOptions);
CLASSICSHUTDOWN_API     DisplayLogoffDialog(HWND hwndParent, LOGOFFSTYLE style);
CLASSICSHUTDOWN_API DisplayDisconnectDialog(HWND hwndParent);

typedef enum _DWORDSETTING
{
	CSDS_MIN = 0,
	CSDS_SHUTDOWNSTYLE = 0,
	CSDS_LOGOFFSTYLE,
	CSDS_SOLIDBANNER,
	CSDS_BANNERCOLOR,
	CSDS_SHUTDOWNSETTING,
	CSDS_SHUTDOWNTYPE,
	CSDS_COUNT
} DWORDSETTING;

CLASSICSHUTDOWN_API  ReadSettingDWORD(DWORDSETTING setting, LPDWORD lpdwValue);
CLASSICSHUTDOWN_API WriteSettingDWORD(DWORDSETTING setting, DWORD dwValue);

typedef enum _STRINGSETTING
{
	CSSS_MIN = 0,
	CSSS_BRANDBITMAP = 0,
	CSSS_BARBITMAP,
	CSSS_COUNT
} STRINGSETTING;

CLASSICSHUTDOWN_API  ReadSettingString(STRINGSETTING setting, LPWSTR szBuffer, DWORD cchBuffer);
CLASSICSHUTDOWN_API WriteSettingString(STRINGSETTING setting, LPCWSTR szBuffer);

#ifdef __cplusplus

/* Templated helpers for (Read|Write)SettingDWORD */

template <typename T>
inline HRESULT ReadSettingDWORD(DWORDSETTING setting, T *lpValue)
{
	static_assert(sizeof(T) == sizeof(DWORD), "Size of templated argument to ReadSettingDWORD must be the same as DWORD");
	return ReadSettingDWORD(setting, (LPDWORD)lpValue);
}

template <typename T>
inline HRESULT WriteSettingDWORD(DWORDSETTING setting, T value)
{
	static_assert(sizeof(T) == sizeof(DWORD), "Size of templated argument to WriteSettingDWORD must be the same as DWORD");
	return WriteSettingDWORD(setting, (DWORD)value);
}

#endif