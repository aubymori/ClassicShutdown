#include <windows.h>
#include "resource.h"

#ifndef _ClassicShutdown_
#define _ClassicShutdown_

extern HWND      g_hDesktopWnd, g_hDlg;
extern HBITMAP   g_hbDesktop;
extern HINSTANCE g_hAppInstance, g_hShell32;
extern BOOL      g_bLogoff;
extern LPDWORD   g_dwRes;

typedef enum tagSHUTDOWNSTYLE
{
    SS_CLASSIC,
    SS_XPCLASSIC,
    SS_XPFRIENDLY
} SHUTDOWNSTYLE;

extern SHUTDOWNSTYLE g_ssStyle;

void PositionDlg(HWND hDlg);
void HandleShutdown(HWND hWnd, DWORD dwCode);

#define IsXP(ss)  (ss == SS_XPCLASSIC | ss == SS_XPFRIENDLY)

#define SHTDN_NONE        0
#define SHTDN_LOGOFF      1
#define SHTDN_SHUTDOWN    2
#define SHTDN_RESTART     3
#define SHTDN_STANDBY     4
#define SHTDN_LOCK        5

#endif