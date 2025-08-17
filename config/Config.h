#pragma once
#include <ClassicShutdown.h>
#include "resource.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "MinCRT.h"
#include "Util.h"

#define SetDlgButtonCheck(hDlg, nIDButton, check) \
	SendDlgItemMessageW(hDlg, nIDButton, BM_SETCHECK, check, 0)

#define SendPSMChanged(hDlg) \
	SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L)

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

INT_PTR CALLBACK GeneralDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK  BannerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK   AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);