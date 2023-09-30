#include <windows.h>
#include <lmcons.h>

#include "resource.h"

const WCHAR CLASS_NAME[] = L"ClassicShutdown_Dither";

HWND      g_hDitherWnd, g_hDlg;
HBITMAP   g_hbDesktop, g_hbBrand;
HINSTANCE g_hAppInstance, g_hShell32;
BOOL      bLogoff;

/**
  * From Windows 2000 source code:
  * private\shell\shell32\restart.c
  */
const WORD GRAY_BITS[] = { 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA };

#define  ROP_DPna  0x000A0329

HBRUSH CreateDitheredBrush(void)
{
	HBITMAP hBmp = CreateBitmap(8, 8, 1, 1, GRAY_BITS);
	HBRUSH hbr = CreatePatternBrush(hBmp);
	DeleteObject(hBmp);
	return hbr;
}
/**
  * End stolen code from Windows 2000
  */

/**
  * From... get this... Windows Server 2003 source code:
  * ds\security\gina\msgina\brand.c
  */
VOID MoveChildren(HWND hWnd, INT dx, INT dy)
{
	HWND hWndSibling;
	RECT rc;

	//
	// walk all the children in the dialog adjusting their positions
	// by the delta.
	//

	for (hWndSibling = GetWindow(hWnd, GW_CHILD); hWndSibling; hWndSibling = GetWindow(hWndSibling, GW_HWNDNEXT))
	{
		GetWindowRect(hWndSibling, &rc);
		MapWindowPoints(NULL, GetParent(hWndSibling), (LPPOINT)&rc, 2);
		OffsetRect(&rc, dx, dy);

		SetWindowPos(hWndSibling, NULL,
			rc.left, rc.top, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
	}

	//
	// having done that then lets adjust the parent size accordingl.
	//

	GetWindowRect(hWnd, &rc);
	MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)&rc, 2);

	SetWindowPos(hWnd, NULL,
		0, 0, (rc.right - rc.left) + dx, (rc.bottom - rc.top) + dy,
		SWP_NOZORDER | SWP_NOMOVE);
}

void ExecuteShutdown(LPCWSTR pczOptions)
{
	ShellExecuteW(
		NULL,
		L"open",
		L"C:\\Windows\\System32\\shutdown.exe",
		pczOptions,
		NULL,
		SW_HIDE
	);
}

void ScreenshotDesktop(void)
{
	HDC hScreenDC = GetDC(NULL);
	HDC hMemDC = CreateCompatibleDC(hScreenDC);

	int nWidth = GetDeviceCaps(hScreenDC, HORZRES);
	int nHeight = GetDeviceCaps(hScreenDC, VERTRES);

	g_hbDesktop = CreateCompatibleBitmap(hScreenDC, nWidth, nHeight);
	HBITMAP hbOld = (HBITMAP)SelectObject(hMemDC, g_hbDesktop);
	BitBlt(
		hMemDC,
		0, 0,
		nWidth, nHeight,
		hScreenDC,
		0, 0,
		SRCCOPY
	);

	DeleteDC(hMemDC);
	ReleaseDC(NULL, hScreenDC);
}

BOOL CALLBACK ExitWindowsDlgProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hShutDown = LoadIconW(
				g_hShell32,
				MAKEINTRESOURCEW(8240)
			);

			PostMessageW(
				GetDlgItem(hWnd, IDD_EXITWINDOWS_ICON),
				STM_SETICON,
				(WPARAM)hShutDown,
				0
			);

			/* Resize window to account for branding banner */
			BITMAP bmBrand;
			GetObjectW(g_hbBrand, sizeof(BITMAP), &bmBrand);

			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			int nWidth = rcClient.right - rcClient.left;
			int dx = 0;

			if (bmBrand.bmWidth > nWidth)
			{
				dx = (bmBrand.bmWidth - nWidth) / 2;
			}

			MoveChildren(hWnd, dx, bmBrand.bmHeight);

			/* Calculate new size (for right margin) */
			if (dx > 0)
			{
				RECT rcClient;
				GetClientRect(hWnd, &rcClient);

				RECT rcDesired;
				rcDesired.top = 0;
				rcDesired.left = 0;
				rcDesired.right = bmBrand.bmWidth;
				rcDesired.bottom = rcClient.bottom - rcClient.top;

				AdjustWindowRectEx(
					&rcDesired,
					GetWindowLongW(hWnd, GWL_STYLE),
					FALSE,
					GetWindowLongW(hWnd, GWL_EXSTYLE)
				);

				int nWidth = GetSystemMetrics(SM_CXSCREEN);
				int nHeight = GetSystemMetrics(SM_CYSCREEN);

				SetWindowPos(
					hWnd,
					NULL,
					(nWidth / 2) - ((rcDesired.right - rcDesired.left) / 2),
					(nHeight / 2) - ((rcDesired.bottom - rcDesired.top) / 2),
					rcDesired.right - rcDesired.left,
					rcDesired.bottom - rcDesired.top,
					SWP_NOZORDER | SWP_NOACTIVATE
				);
			}

			HWND hComboBox = GetDlgItem(hWnd, IDD_EXITWINDOWS_COMBOBOX);
			WCHAR szLogoffFormat[64], szLogoff[300], szShutdown[64], szRestart[64], szUsername[UNLEN + 1];

			LoadStringW(g_hAppInstance, IDS_LOGOFF, szLogoffFormat, 64);
			
			DWORD dwSize = UNLEN + 1;
			GetUserNameW(szUsername, &dwSize);

			wsprintfW(szLogoff, szLogoffFormat, szUsername);

			LoadStringW(g_hAppInstance, IDS_SHUTDOWN, szShutdown, 64);
			LoadStringW(g_hAppInstance, IDS_RESTART, szRestart, 64);

			SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szLogoff);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szShutdown);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)szRestart);
			SendMessageW(hComboBox, CB_SETCURSEL, 1, 0);

			WCHAR szShutdownDesc[256];
			LoadStringW(g_hAppInstance, IDS_SHUTDOWN_DESC, szShutdownDesc, 256);

			SendMessageW(
				GetDlgItem(hWnd, IDD_EXITWINDOWS_LABEL),
				WM_SETTEXT,
				0, (LPARAM)szShutdownDesc
			);

			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
			HDC hDCMem = CreateCompatibleDC(hDC);

			HBITMAP hbmOld = (HBITMAP)SelectObject(hDCMem, g_hbBrand);

			BITMAP bmBrand;
			GetObjectW(g_hbBrand, sizeof(BITMAP), &bmBrand);

			BitBlt(
				hDC,
				0, 0,
				bmBrand.bmWidth,
				bmBrand.bmHeight,
				hDCMem,
				0, 0,
				SRCCOPY
			);

			SelectObject(hDC, hbmOld);
			DeleteDC(hDCMem);
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int i = SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
				WCHAR szMessage[256];
				UINT uStringId = NULL;
				switch (i)
				{
					case 0:
						uStringId = IDS_LOGOFF_DESC;
						break;
					case 1:
						uStringId = IDS_SHUTDOWN_DESC;
						break;
					case 2:
						uStringId = IDS_RESTART_DESC;
						break;
				}

				if (uStringId != NULL)
				{
					LoadStringW(g_hAppInstance, uStringId, szMessage, 256);
					SendMessageW(
						GetDlgItem(hWnd, IDD_EXITWINDOWS_LABEL),
						WM_SETTEXT,
						0, (LPARAM)szMessage
					);
				}
			}
			else
			{
				switch (wParam)
				{
					case IDOK:
					{
						int i = SendMessageW(
							GetDlgItem(hWnd, IDD_EXITWINDOWS_COMBOBOX),
							CB_GETCURSEL, 0, 0
						);
						switch (i)
						{
							case 0:
								ShellExecuteW(
									NULL,
									L"open",
									L"C:\\Windows\\System32\\logoff.exe",
									NULL,
									NULL,
									SW_HIDE
								);
								break;
							case 1:
								ExecuteShutdown(L"-s -t 0");
								break;
							case 2:
								ExecuteShutdown(L"-r -t 0");
								break;
						}
						EndDialog(hWnd, 0);
					}
					case IDCANCEL:
						EndDialog(hWnd, 0);
						break;
					case IDHELP:

						break;
				}
			}
			break;
		/* Disable moving */
		case WM_SYSCOMMAND:
			if ((wParam & ~0x0F) == SC_MOVE)
			{
				return TRUE;
			}
			return FALSE;
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				EndDialog(hWnd, 0);
			}
			break;
		default:
			return FALSE;
			break;
	}

	return TRUE;
}

BOOL CALLBACK LogoffDlgProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hLogoff = LoadIconW(
				g_hShell32,
				MAKEINTRESOURCEW(45)
			);

			PostMessageW(
				GetDlgItem(hWnd, IDD_LOGOFFWINDOWS_ICON),
				STM_SETICON,
				(WPARAM)hLogoff,
				0
			);
			break;
		}
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			break;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDOK:
					ShellExecuteW(
						NULL,
						L"open",
						L"C:\\Windows\\System32\\logoff.exe",
						NULL,
						NULL,
						SW_HIDE
					);
				case IDCANCEL:
					EndDialog(hWnd, 0);
			}
			break;
		/* Disable moving */
		case WM_SYSCOMMAND:
			if ((wParam & ~0x0F) == SC_MOVE)
			{
				return TRUE;
			}
			return FALSE;
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				EndDialog(hWnd, 0);
			}
			break;
		default:
			return FALSE;
			break;
	}

	return TRUE;
}

LRESULT CALLBACK DitherWndProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg)
	{
		/* Haha, no thanks. No clearing for me! */
		case WM_ERASEBKGND:
			return 0;
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

int WINAPI wWinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nCmdShow
)
{
	g_hAppInstance = hInstance;
	g_hShell32 = LoadLibraryW(L"shell32.dll");

	if (0 == wcscmp(lpCmdLine, L"logoff"))
	{
		bLogoff = TRUE;
	}
	else
	{
		bLogoff = FALSE;
		HINSTANCE hBasebrd = LoadLibraryW(L"C:\\Windows\\Branding\\Basebrd\\basebrd.dll");
		if (!hBasebrd)
		{
			MessageBoxW(NULL, L"Failed to load basebrd.dll!", L"ClassicShutdown", MB_ICONERROR);
			return 1;
		}

		/* The layout of basebrd.dll changed in 1607 */
		g_hbBrand = LoadBitmapW(
			hBasebrd,
			MAKEINTRESOURCEW(123)
		);

		if (!g_hbBrand)
		{
			g_hbBrand = LoadBitmapW(
				hBasebrd,
				MAKEINTRESOURCEW(121)
			);
		}
	}

	WNDCLASS wc;
	wc.style         = 0;
	wc.lpfnWndProc   = DitherWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = CLASS_NAME;
	RegisterClassW(&wc);

	g_hDitherWnd = CreateWindowExW(
		/* To not show in taskbar: */
		WS_EX_TOOLWINDOW,              /* dwExStyle */
		CLASS_NAME,                    /* lpClassName */
		L"",                           /* lpWindowName */
		WS_POPUP,                      /* dwStyle */
		0,                             /* X */
		0,                             /* Y */
		GetSystemMetrics(SM_CXSCREEN), /* nWidth */
		GetSystemMetrics(SM_CYSCREEN), /* nHeight */
		NULL,                          /* hWndParent */
		NULL,                          /* hMenu */
		hInstance,                     /* hInstance */
		NULL                           /* lpParam */
	);

	ScreenshotDesktop();

	ShowWindow(g_hDitherWnd, SW_SHOW);
	SetForegroundWindow(g_hDitherWnd);
	
	HDC hDC = GetDC(g_hDitherWnd);
	HDC hMemDC = CreateCompatibleDC(hDC);

	int nWidth = GetDeviceCaps(hDC, HORZRES);
	int nHeight = GetDeviceCaps(hDC, VERTRES);

	HBITMAP hbOld = (HBITMAP)SelectObject(hMemDC, g_hbDesktop);

	BitBlt(
		hDC,
		0, 0,
		nWidth, nHeight,
		hMemDC,
		0, 0,
		SRCCOPY
	);

	HBRUSH hbr = CreateDitheredBrush();
	HBRUSH hbrOld = SelectObject(hDC, hbr);

	PatBlt(
		hDC,
		0, 0,
		nWidth, nHeight,
		ROP_DPna
	);

	SelectObject(hDC, hbrOld);
	DeleteObject(hbr);

	SelectObject(hMemDC, hbOld);
	DeleteDC(hMemDC);
	ReleaseDC(g_hDitherWnd, hDC);

	if (bLogoff)
	{
		DialogBoxParamW(
			g_hAppInstance,
			MAKEINTRESOURCEW(IDD_LOGOFFWINDOWS),
			g_hDitherWnd,
			LogoffDlgProc,
			NULL
		);
	}
	else
	{
		DialogBoxParamW(
			g_hAppInstance,
			MAKEINTRESOURCEW(IDD_EXITWINDOWS),
			g_hDitherWnd,
			ExitWindowsDlgProc,
			NULL
		);
	}

	return 0;
}