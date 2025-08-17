#pragma once
#define CLASSICSHUTDOWN_IMPLEMENTATION
#include <ClassicShutdown.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <powrprof.h>
#include <wtsapi32.h>
#include "Resource.h"
#include "Util.h"
#include "MinCRT.h"

// Shell32 resource IDs
#define IDS_SHUTDOWN                0x8200
#define IDS_RESTART                 (IDS_SHUTDOWN+1)
#define IDS_NO_PERMISSION_SHUTDOWN  (IDS_SHUTDOWN+2)
#define IDS_NO_PERMISSION_RESTART   (IDS_SHUTDOWN+3)

#define IDI_SHUTDOWN            0x2030
#define IDI_STLOGOFF            45
#define IDI_MU_DISCONN          49

#ifdef NDEBUG
#define DEBUG 0
#else
#define DEBUG 1
#endif

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define REGSTR_PATH_CLASSICSHUTDOWN L"SOFTWARE\\ClassicShutdown"

#if DEBUG
#define DebuggerAttached() IsDebuggerPresent()
#else
#define DebuggerAttached() 0
#endif

// Clone of WIL
#define RETURN_IF_FAILED(expr) \
do                             \
{                              \
	HRESULT __hr = (expr);     \
	if (FAILED(__hr))          \
		return __hr;           \
} while (0)

extern HINSTANCE g_hinst;
extern HINSTANCE g_hinstShell;
extern DWORD     g_dwStyle;
extern HKEY      g_hkey;
extern bool      g_fStandByAvailable;
extern bool      g_fHibernationAvailable;
extern bool      g_fRemoteSession;

HRESULT DoShutdown(SHUTDOWNTYPE type);

/* Undocumented function from winbrand.dll */
EXTERN_C WINUSERAPI HANDLE WINAPI BrandingLoadImage(LPCWSTR pszBrand, UINT uID, UINT type, int cx, int cy, UINT fuLoad);