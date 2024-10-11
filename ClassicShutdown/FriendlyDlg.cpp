#include "ClassicShutdown.h"
#include "FriendlyDlg.h"

#include <commctrl.h>

#define TIMER_HIBERNATE 0x1

static RECT    s_rcBkgnd, s_rcFlag, s_rcBtns;
static HBITMAP s_hbmBkgnd, s_hbmFlag, s_hbmBtns;
static HFONT   s_hfntTitle, s_hfntBtn;
static UINT    s_uFocusId, s_uHoverId;
static HWND    s_hTooltip = NULL;
static BOOL    s_bShiftDown = FALSE;

const UINT c_uExitIds[] = {
    IDC_BUTTON_STANDBY,
    IDC_BUTTON_TURNOFF,
    IDC_BUTTON_RESTART
};

const UINT c_uLogoffIds[] = {
    IDC_BUTTON_SWITCHUSER,
    IDC_BUTTON_LOGOFF
};

void PaintBitmap(
          HDC hdcDestination,
    const RECT *prcDestination,
          HBITMAP hbmSource,
    const RECT *prcSource
)
{
    HDC     hdcBitmap;

    hdcBitmap = CreateCompatibleDC(NULL);
    if (hdcBitmap != NULL)
    {
        bool        fEqualWidthAndHeight;
        int         iWidthSource, iHeightSource, iWidthDestination, iHeightDestination;
        int         iStretchBltMode;
        DWORD       dwLayout;
        HBITMAP     hbmSelected;
        RECT        rcSource;
        BITMAP      bitmap;

        if (prcSource == NULL)
        {
            if (GetObject(hbmSource, sizeof(bitmap), &bitmap) == 0)
            {
                bitmap.bmWidth = prcDestination->right - prcDestination->left;
                bitmap.bmHeight = prcDestination->bottom - prcDestination->top;
            }
            SetRect(&rcSource, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
            prcSource = &rcSource;
        }
        hbmSelected = (HBITMAP)SelectObject(hdcBitmap, hbmSource);
        iWidthSource = prcSource->right - prcSource->left;
        iHeightSource = prcSource->bottom - prcSource->top;
        iWidthDestination = prcDestination->right - prcDestination->left;
        iHeightDestination = prcDestination->bottom - prcDestination->top;
        fEqualWidthAndHeight = (iWidthSource == iWidthDestination) && (iHeightSource == iHeightDestination);
        if (!fEqualWidthAndHeight)
        {
            iStretchBltMode = SetStretchBltMode(hdcDestination, HALFTONE);
        }
        else
        {
            iStretchBltMode = 0;
        }
        dwLayout = SetLayout(hdcDestination, LAYOUT_BITMAPORIENTATIONPRESERVED);
        TransparentBlt(hdcDestination,
            prcDestination->left,
            prcDestination->top,
            iWidthDestination,
            iHeightDestination,
            hdcBitmap,
            prcSource->left,
            prcSource->top,
            iWidthSource,
            iHeightSource,
            RGB(255, 0, 255));
        (DWORD)SetLayout(hdcDestination, dwLayout);
        if (!fEqualWidthAndHeight)
        {
            (int)SetStretchBltMode(hdcDestination, iStretchBltMode);
        }
        (HGDIOBJ)SelectObject(hdcBitmap, hbmSelected);
        DeleteDC(hdcBitmap);
    }
}

void FilterMetaCharacters(WCHAR *pszText)
{
    WCHAR *pTC;

    pTC = pszText;
    while (*pTC != L'\0')
    {
        if (*pTC == L'&')
        {
            (WCHAR *)lstrcpyW(pTC, pTC + 1);
        }
        else
        {
            ++pTC;
        }
    }
}

LRESULT CALLBACK BtnSubclassProc(
    HWND      hWnd,
    UINT      uMsg,
    WPARAM    wParam,
    LPARAM    lParam,
    UINT_PTR  uIdCtl,
    DWORD_PTR dwRefData
)
{
    LRESULT lRes;

    switch (uMsg)
    {
        case BM_SETSTYLE:
            if (wParam == BS_DEFPUSHBUTTON)
            {
                s_uFocusId = uIdCtl;
            }
            
            if (uIdCtl != IDCANCEL)
            {
                lRes = 0;
                break;
            }
        default:
            lRes = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            switch (uMsg)
            {
                case DM_GETDEFID:
                    lRes = (DC_HASDEFID << 16) | (WORD)s_uFocusId;
                    break;
                case WM_GETDLGCODE:
                    if (uIdCtl == s_uFocusId)
                    {
                        lRes = DLGC_DEFPUSHBUTTON;
                    }
                    else
                    {
                        lRes |= DLGC_UNDEFPUSHBUTTON;
                    }
                    break;
                case WM_MOUSEMOVE:
                    SetCursor(LoadCursorW(NULL, IDC_HAND));
                    if (uIdCtl != s_uHoverId)
                    {
                        TRACKMOUSEEVENT tme;
                        s_uHoverId = uIdCtl;
                        tme.cbSize = sizeof(TRACKMOUSEEVENT);
                        tme.dwFlags = TME_HOVER | TME_LEAVE;
                        tme.hwndTrack = hWnd;
                        tme.dwHoverTime = HOVER_DEFAULT;
                        TrackMouseEvent(&tme);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;
                case WM_MOUSEHOVER:
                {
                    int iTxtId;
                    switch (uIdCtl)
                    {
                        case IDC_BUTTON_TURNOFF:
                            iTxtId = IDS_TURNOFF_TOOLTIP_TEXT_TURNOFF;
                            break;
                        case IDC_BUTTON_STANDBY:
                            if (g_bHibernationAvailable)
                                iTxtId = s_bShiftDown ? IDS_TURNOFF_TOOLTIP_TEXT_HIBERNATE : IDS_TURNOFF_TOOLTIP_TEXT_STANDBY_HIBERNATE;
                            else
                                iTxtId = IDS_TURNOFF_TOOLTIP_TEXT_STANDBY;
                            break;
                        case IDC_BUTTON_RESTART:
                            iTxtId = IDS_TURNOFF_TOOLTIP_TEXT_RESTART;
                            break;
                        case IDC_BUTTON_SWITCHUSER:
                            iTxtId = IDS_SWITCHUSER_TOOLTIP_TEXT_SWITCHUSER;
                            break;
                        case IDC_BUTTON_LOGOFF:
                            iTxtId = IDS_SWITCHUSER_TOOLTIP_TEXT_LOGOFF;
                            break;
                        default:
                            iTxtId = 0;
                            break;
                    }

                    s_hTooltip = CreateWindowExW(
                        0,
                        TOOLTIPS_CLASS,
                        NULL,
                        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        hWnd,
                        NULL,
                        g_hAppInstance,
                        NULL
                    );

                    if (s_hTooltip)
                    {
                        SetWindowPos(s_hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                        SendMessageW(s_hTooltip, CCM_SETVERSION, COMCTL32_VERSION, 0);

                        TOOLINFOW ti = { 0 };
                        WCHAR     szText[256] = { 0 };
                        LoadStringW(
                            g_hMuiInstance,
                            iTxtId,
                            szText + 2,
                            254
                        );
                        szText[0] = L'\r';
                        szText[1] = L'\n';

                        ti.cbSize = sizeof(TOOLINFOW);
                        ti.uFlags = TTF_TRANSPARENT | TTF_TRACK;
                        ti.uId = PtrToUint(s_hTooltip);
                        ti.lpszText = szText;
                        SendMessageW(s_hTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
                        SendMessageW(s_hTooltip, TTM_SETMAXTIPWIDTH, 0, 300);

                        RECT rc;
                        LONG lPosX, lPosY;

                        GetWindowRect(hWnd, &rc);
                        lPosX = (rc.left + rc.right) / 2;
                        lPosY = rc.bottom;
                        SendMessageW(s_hTooltip, TTM_TRACKPOSITION, 0, MAKELONG(lPosX, lPosY));

                        LPWSTR szCaption;
                        int    iCapLen;
                        HWND   hTextWnd = hWnd;
                        if (uIdCtl == IDC_BUTTON_STANDBY && s_bShiftDown)
                            hTextWnd = GetDlgItem(GetParent(hWnd), IDC_BUTTON_HIBERNATE);

                        iCapLen = GetWindowTextLengthW(hTextWnd);
                        szCaption = new WCHAR[iCapLen + 1];
                        if (szCaption)
                        {   
                            GetWindowTextW(hTextWnd, szCaption, iCapLen + 1);
                            FilterMetaCharacters(szCaption);
                            SendMessageW(s_hTooltip, TTM_SETTITLE, 0, (LPARAM)szCaption);
                        }

                        SendMessageW(s_hTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

                        if (szCaption)
                            delete[] szCaption;
                    }
                    break;
                }
                case WM_MOUSELEAVE:
                    DestroyWindow(s_hTooltip);
                    s_uHoverId = 0;
                case WM_KILLFOCUS:
                    if (uMsg == WM_KILLFOCUS)
                        s_uFocusId = 0;
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateWindow(hWnd);
                    break;
            }
    }

    return lRes;
}

INT_PTR CALLBACK FriendlyDlgProc(
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
            BITMAP bm;

            s_hbmBkgnd = (HBITMAP)LoadImageW(
                g_hAppInstance,
                MAKEINTRESOURCEW(IDB_BACKGROUND),
                IMAGE_BITMAP,
                0, 0,
                LR_CREATEDIBSECTION
            );
            GetObject(s_hbmBkgnd, sizeof(BITMAP), &bm);
            SetRect(&s_rcBkgnd, 0, 0, bm.bmWidth, bm.bmHeight);

            s_hbmFlag = (HBITMAP)LoadImageW(
                g_hAppInstance,
                MAKEINTRESOURCEW(IDB_FLAG),
                IMAGE_BITMAP,
                0, 0,
                LR_CREATEDIBSECTION
            );
            GetObject(s_hbmFlag, sizeof(BITMAP), &bm);
            SetRect(&s_rcFlag, 0, 0, bm.bmWidth, bm.bmHeight);

            s_hbmBtns = (HBITMAP)LoadImageW(
                g_hAppInstance,
                g_bLogoff ? MAKEINTRESOURCEW(IDB_LOGOFF_BUTTONS) : MAKEINTRESOURCEW(IDB_BUTTONS),
                IMAGE_BITMAP,
                0, 0,
                LR_CREATEDIBSECTION
            );
            GetObject(s_hbmBtns, sizeof(BITMAP), &bm);
            SetRect(&s_rcBtns, 0, 0, bm.bmWidth, bm.bmHeight);

            HDC hDCScreen = GetDC(NULL);
            LOGFONTW lf = { 0 };
            char szPixelSize[10];

            /* Title font */
            if (LoadStringA(
                g_hMuiInstance,
                IDS_TURNOFF_TITLE_FACESIZE,
                szPixelSize,
                10
            ))
            {
                lf.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hDCScreen, LOGPIXELSY), 72);
                if (LoadStringW(
                    g_hMuiInstance,
                    IDS_TURNOFF_TITLE_FACENAME,
                    lf.lfFaceName,
                    LF_FACESIZE
                ))
                {
                    lf.lfWeight = FW_MEDIUM;
                    lf.lfQuality = DEFAULT_QUALITY;
                    s_hfntTitle = CreateFontIndirectW(&lf);
                }
            }

            /* Button font */
            if (LoadStringA(
                g_hMuiInstance,
                IDS_TURNOFF_BUTTON_FACESIZE,
                szPixelSize,
                10
            ))
            {
                lf.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hDCScreen, LOGPIXELSY), 72);
                if (LoadStringW(
                    g_hMuiInstance,
                    IDS_TURNOFF_BUTTON_FACENAME,
                    lf.lfFaceName,
                    LF_FACESIZE
                ))
                {
                    lf.lfWeight = FW_BOLD;
                    lf.lfQuality = DEFAULT_QUALITY;
                    s_hfntBtn = CreateFontIndirectW(&lf);
                }
            }

            ReleaseDC(NULL, hDCScreen);

            const UINT *lpItemIds = g_bLogoff ? c_uLogoffIds : c_uExitIds;
            int len = g_bLogoff ? ARRAYSIZE(c_uLogoffIds) : ARRAYSIZE(c_uExitIds);

            for (int i = 0; i < len; i++)
            {
                SetWindowSubclass(
                    GetDlgItem(hWnd, lpItemIds[i]),
                    BtnSubclassProc,
                    lpItemIds[i],
                    NULL
                );
            }

            PositionDlg(hWnd);

            s_uFocusId = g_bLogoff ? IDC_BUTTON_SWITCHUSER : IDC_BUTTON_STANDBY;
            SetFocus(GetDlgItem(hWnd, s_uFocusId));
            SendMessageW(hWnd, DM_SETDEFID, s_uFocusId, 0);

            if (!g_bLogoff && g_bHibernationAvailable)
            {
                OutputDebugStringW(L"Setting timer\r\n");
                SetTimer(hWnd, TIMER_HIBERNATE, 50, NULL);
            }
            break;
        }
        case WM_TIMER:
        {
            BOOL bNewShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (bNewShiftDown != s_bShiftDown)
            {
                s_bShiftDown = bNewShiftDown;

                RECT rc;
                HWND hWndText = GetDlgItem(hWnd, IDC_TEXT_STANDBY);
                GetClientRect(hWndText, &rc);
                MapWindowPoints(hWndText, hWnd, (POINT *)&rc, 2);
                InvalidateRect(hWnd, &rc, TRUE);

                if (s_uHoverId == IDC_BUTTON_STANDBY)
                {
                    // Destroy current tooltip
                    if (s_hTooltip && IsWindow(s_hTooltip))
                    {
                        DestroyWindow(s_hTooltip);
                        s_hTooltip = NULL;

                        // Show tooltip for new action
                        // Windows XP relies on WM_MOUSEHOVER being automatically sent after the original
                        // For whatever reason this doesn't happen, so we just send it ourself
                        // 
                        // Separate thread because we need to wait for a time before doing it and locking up
                        // the thread here results in the text below the button disappearing
                        CreateThread(NULL, NULL, [](LPVOID lpParam) -> DWORD {
                            HWND hWnd = (HWND)lpParam;
                            Sleep(400);
                            if (s_uHoverId == IDC_BUTTON_STANDBY)
                                SendDlgItemMessageW(hWnd, IDC_BUTTON_STANDBY, WM_MOUSEHOVER, 0, 0);
                            ExitThread(0);
                            return 0;
                        }, hWnd, NULL, NULL);
                    }
                }
            }
            return 0;
        }
        case WM_CLOSE:
            HandleShutdown(hWnd, SHTDN_NONE);
            break;
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDCANCEL:
                    HandleShutdown(hWnd, SHTDN_NONE);
                    break;
                case IDC_BUTTON_TURNOFF:
                    HandleShutdown(hWnd, SHTDN_SHUTDOWN);
                    break;
                case IDC_BUTTON_STANDBY:
                    HandleShutdown(
                        hWnd,
                        (g_bHibernationAvailable && s_bShiftDown) ? SHTDN_HIBER : SHTDN_STANDBY
                    );
                    break;
                case IDC_BUTTON_RESTART:
                    HandleShutdown(hWnd, SHTDN_RESTART);
                    break;
                case IDC_BUTTON_SWITCHUSER:
                    HandleShutdown(hWnd, SHTDN_DISCONNECT);
                    break;
                case IDC_BUTTON_LOGOFF:
                    HandleShutdown(hWnd, SHTDN_LOGOFF);
                    break;
                default:
                    break;
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            PaintBitmap(hDC, &rc, s_hbmBkgnd, &s_rcBkgnd);

            EndPaint(hWnd, &ps);
        }
        case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            PaintBitmap((HDC)wParam, &rc, s_hbmBkgnd, &s_rcBkgnd);
        }
        case WM_PRINTCLIENT:
            if ((lParam & (PRF_ERASEBKGND | PRF_CLIENT)) != 0)
            {
                PostMessageW(
                    hWnd,
                    WM_ERASEBKGND,
                    wParam,
                    NULL
                );
            }
            break;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                HandleShutdown(hWnd, SHTDN_NONE);
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            switch (wParam)
            {
                case IDC_TITLE_TURNOFF:
                case IDC_TITLE_SWITCHUSER:
                {
                    RECT rc;
                    SIZE sz;
                    WCHAR szText[256];

                    HFONT hfOld = (HFONT)SelectObject(pDIS->hDC, s_hfntTitle);
                    COLORREF crOld = SetTextColor(pDIS->hDC, RGB(255, 255, 255));
                    int iBkOld = SetBkMode(pDIS->hDC, TRANSPARENT);

                    GetDlgItemTextW(hWnd, pDIS->CtlID, szText, 256);
                    GetTextExtentPointW(pDIS->hDC, szText, 256, &sz);
                    CopyRect(&rc, &pDIS->rcItem);
                    InflateRect(&rc, 0, -((rc.bottom - rc.top - sz.cy) / 2));

                    DrawTextW(
                        pDIS->hDC,
                        szText,
                        -1,
                        &rc,
                        0
                    );

                    SetBkMode(pDIS->hDC, iBkOld);
                    SetTextColor(pDIS->hDC, crOld);
                    SelectObject(pDIS->hDC, hfOld);
                    break;
                }
                case IDC_TEXT_STANDBY:
                case IDC_TEXT_TURNOFF:
                case IDC_TEXT_RESTART:
                case IDC_TEXT_SWITCHUSER:
                case IDC_TEXT_LOGOFF:
                {
                    int   iPxlHght;
                    UINT  uBtnId = NULL;
                    WCHAR szText[256];
                    RECT  rc, rcText;

                    switch (wParam)
                    {
                        case IDC_TEXT_STANDBY:
                            uBtnId = (g_bHibernationAvailable && s_bShiftDown) ? IDC_BUTTON_HIBERNATE : IDC_BUTTON_STANDBY;
                            break;
                        case IDC_TEXT_TURNOFF:
                            uBtnId = IDC_BUTTON_TURNOFF;
                            break;
                        case IDC_TEXT_RESTART:
                            uBtnId = IDC_BUTTON_RESTART;
                            break;
                        case IDC_TEXT_SWITCHUSER:
                            uBtnId = IDC_BUTTON_SWITCHUSER;
                            break;
                        case IDC_TEXT_LOGOFF:
                            uBtnId = IDC_BUTTON_LOGOFF;
                            break;
                    }

                    
                    HFONT hfOld = (HFONT)SelectObject(pDIS->hDC, s_hfntBtn);
                    COLORREF crOld = SetTextColor(pDIS->hDC, RGB(255, 255, 255));
                    int iBkOld = SetBkMode(pDIS->hDC, TRANSPARENT);

                    GetDlgItemTextW(
                        hWnd, uBtnId, szText, 256
                    );
                    CopyRect(&rcText, &pDIS->rcItem);
                    iPxlHght = DrawTextW(
                        pDIS->hDC,
                        szText,
                        -1,
                        &rcText,
                        DT_CALCRECT
                    );
                    CopyRect(&rc, &pDIS->rcItem);
                    InflateRect(
                        &rc,
                        -((rc.right - rc.left - (rcText.right - rcText.left)) / 2),
                        -((rc.bottom - rc.top - iPxlHght) / 2)
                    );

                    DrawTextW(
                        pDIS->hDC,
                        szText,
                        -1,
                        &rc,
                        ((pDIS->itemState & ODS_NOACCEL) != 0) ? DT_HIDEPREFIX : 0
                    );

                    SetBkMode(pDIS->hDC, iBkOld);
                    SetTextColor(pDIS->hDC, crOld);
                    SelectObject(pDIS->hDC, hfOld);
                    break;
                }
                case IDC_BUTTON_STANDBY:
                case IDC_BUTTON_TURNOFF:
                case IDC_BUTTON_RESTART:
                case IDC_BUTTON_SWITCHUSER:
                case IDC_BUTTON_LOGOFF:
                {
                    RECT rc;
                    int iVOffset;

                    switch (wParam)
                    {
                        case IDC_BUTTON_SWITCHUSER:
                        case IDC_BUTTON_TURNOFF:
                            iVOffset = 0;
                            break;
                        case IDC_BUTTON_LOGOFF:
                        case IDC_BUTTON_STANDBY:
                            iVOffset = 1;
                            break;
                        case IDC_BUTTON_RESTART:
                            iVOffset = 2;
                            break;
                        default:
                            iVOffset = 0;
                            break;
                    }

                    iVOffset *= 3;

                    if ((pDIS->itemState & ODS_SELECTED))
                    {
                        iVOffset++;
                    }
                    else if (s_uHoverId == wParam || s_uFocusId == wParam || (pDIS->itemState & ODS_FOCUS))
                    {
                        iVOffset += 2;
                    }

                    iVOffset *= 32;

                    SetRect(
                        &rc,
                        0,
                        iVOffset,
                        32,
                        iVOffset + 32
                    );

                    PaintBitmap(
                        pDIS->hDC, &pDIS->rcItem, s_hbmBtns, &rc
                    );
                    break;
                }
                case IDC_TITLE_FLAG:
                    PaintBitmap(pDIS->hDC, &pDIS->rcItem, s_hbmFlag, &s_rcFlag);
                    break;
            }
        }
        default:
            return FALSE;
    }

    return TRUE;
}