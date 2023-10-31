//  --------------------------------------------------------------------------
//  Module Name: DimmedWindow.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements the dimmed window when displaying logoff / shut down
//  dialog.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

#include "DimmedWindow.h"

//  --------------------------------------------------------------------------
//  CDimmedWindow::s_szWindowClassName
//
//  Purpose:    static member variables.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

const WCHAR     CDimmedWindow::s_szWindowClassName[]        =   L"DimmedWindowClass";
const WCHAR     CDimmedWindow::s_szExplorerKeyName[]        =   L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";
const WCHAR     CDimmedWindow::s_szExplorerPolicyKeyName[]  =   L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
const WCHAR     CDimmedWindow::s_szForceDimValueName[]      =   L"ForceDimScreen";
#define RCW(rc) ((rc).right - (rc).left)
#define RCH(r) ((r).bottom - (r).top)
#define CHUNK_SIZE 20

void DimPixels(void* pvBitmapBits, int cLen, int Amount)
{
    ULONG* pulSrc = (ULONG*)pvBitmapBits;

    for (int i = cLen - 1; i >= 0; i--)
    {
        ULONG ulR = GetRValue(*pulSrc);
        ULONG ulG = GetGValue(*pulSrc);
        ULONG ulB = GetBValue(*pulSrc);
        ULONG ulGray = (54 * ulR + 183 * ulG + 19 * ulB) >> 8;
        ULONG ulTemp = ulGray * (0xff - Amount);
        ulR = (ulR * Amount + ulTemp) >> 8;
        ulG = (ulG * Amount + ulTemp) >> 8;
        ulB = (ulB * Amount + ulTemp) >> 8;
        *pulSrc = (*pulSrc & 0xff000000) | RGB(ulR, ulG, ulB);

        pulSrc++;
    }
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::CDimmedWindow
//
//  Arguments:  hInstance   =   HINSTANCE of the hosting process/DLL.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CDimmedWindow. Registers the window class
//              DimmedWindowClass.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

CDimmedWindow::CDimmedWindow (HINSTANCE hInstance) :
    _lReferenceCount(1),
    _hInstance(hInstance),
    _atom(0),
    _hwnd(NULL),
    _pvPixels(NULL),
    _idxChunk(0),
    _idxSaturation(0),
    _hdcDimmed(NULL),
    _hbmOldDimmed(NULL),
    _hbmDimmed(NULL),
    _xVirtualScreen(0),
    _yVirtualScreen(0),
    _cxVirtualScreen(0),
    _cyVirtualScreen(0)
{
    WNDCLASSEXW wndClassEx;

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(wndClassEx);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = s_szWindowClassName;
    _atom = RegisterClassExW(&wndClassEx);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::~CDimmedWindow
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDimmedWindow. Destroys the dimmed window and
//              unregisters the window class.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

CDimmedWindow::~CDimmedWindow (void)

{
    if (_hdcDimmed)
    {
        SelectObject(_hdcDimmed, _hbmOldDimmed);
        DeleteDC(_hdcDimmed);
    }

    if (_hbmDimmed)
    {
        DeleteObject(_hbmDimmed);
    }

    if (_hwnd != NULL)
    {
        (BOOL)DestroyWindow(_hwnd);
    }

    if (_atom != 0)
    {
        UnregisterClassW(MAKEINTRESOURCE(_atom), _hInstance);
    }
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::QueryInterface
//
//  Arguments:  riid        =   Interface to query support of.
//              ppvObject   =   Returned interface if successful.
//
//  Returns:    HRESULT
//
//  Purpose:    Returns the specified interface implemented by this object.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CDimmedWindow::QueryInterface (REFIID riid, void **ppvObject)

{
    HRESULT     hr;

    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = static_cast<IUnknown*>(this);
        (LONG)InterlockedIncrement(&_lReferenceCount);
        hr = S_OK;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::AddRef
//
//  Arguments:  <none>
//
//  Returns:    ULONG
//
//  Purpose:    Increments the reference count and returns that value.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

ULONG   CDimmedWindow::AddRef (void)

{
    return(static_cast<ULONG>(InterlockedIncrement(&_lReferenceCount)));
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::Release
//
//  Arguments:  <none>
//
//  Returns:    ULONG
//
//  Purpose:    Decrements the reference count and if it reaches zero deletes
//              the object.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

ULONG   CDimmedWindow::Release (void)

{
    ULONG cRef = InterlockedDecrement(&_lReferenceCount);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::Create
//
//  Arguments:  <none>
//
//  Returns:    HWND
//
//  Purpose:    Creates the dimmed window. Creates the window so that it
//              covers the whole screen area.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

HWND    CDimmedWindow::Create (void)

{
    _xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
    _yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
    _cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    _cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    _hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                            s_szWindowClassName,
                            NULL,
                            WS_POPUP,
                            _xVirtualScreen, _yVirtualScreen,
                            _cxVirtualScreen, _cyVirtualScreen,
                            NULL, NULL, _hInstance, this);
    if (_hwnd != NULL)
    {
        ShowWindow(_hwnd, SW_SHOW);
        SetForegroundWindow(_hwnd);

        EnableWindow(_hwnd, FALSE);
    }
    return(_hwnd);
}


BOOL CDimmedWindow::StepDim()
{
    HDC hdcWindow = GetDC(_hwnd);

    if (_idxChunk >= 0 )
    {
        //
        //  In the first couple of passes, we slowly collect the screen 
        //  into our bitmap. We do this because Blt-ing the whole thing
        //  causes the system to hang. By doing it this way, we continue
        //  to pump messages, the UI stays responsive and it keeps the 
        //  mouse alive.
        //

        int y  = _idxChunk * CHUNK_SIZE;
        BitBlt(_hdcDimmed, 0, y, _cxVirtualScreen, CHUNK_SIZE, hdcWindow, 0, y, SRCCOPY);

        _idxChunk--;
        if (_idxChunk < 0)
        {
            //
            //  We're done getting the bitmap, now reset the timer
            //  so we slowly fade to grey.
            //

            SetTimer(_hwnd, 1, 250, NULL);
            _idxSaturation = 16;
        }

        return TRUE;    // don't kill the timer.
    }
    else
    {
        //
        //  In these passes, we are making the image more and more grey and
        //  then Blt-ing the result to the screen.
        //

        DimPixels(_pvPixels, _cxVirtualScreen * _cyVirtualScreen, 0xd5);
        BitBlt(hdcWindow, 0, 0, _cxVirtualScreen, _cyVirtualScreen, _hdcDimmed, 0, 0, SRCCOPY);

        _idxSaturation--;

        return (_idxSaturation > 0);    // when we hit zero, kill the timer.
    }
}

void CDimmedWindow::SetupDim()
{
    HDC     hdcWindow = GetDC(_hwnd);
    if (hdcWindow != NULL)
    {
        _hdcDimmed = CreateCompatibleDC(hdcWindow);
        if (_hdcDimmed != NULL)
        {
            BITMAPINFO  bmi;

            ZeroMemory(&bmi, sizeof(bmi));
            bmi.bmiHeader.biSize = sizeof(bmi);
            bmi.bmiHeader.biWidth =  _cxVirtualScreen;
            bmi.bmiHeader.biHeight = _cyVirtualScreen; 
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;

            _hbmDimmed = CreateDIBSection(_hdcDimmed, &bmi, DIB_RGB_COLORS, &_pvPixels, NULL, 0);
            if (_hbmDimmed != NULL)
            {
                _hbmOldDimmed = (HBITMAP) SelectObject(_hdcDimmed, _hbmDimmed);
                _idxChunk = _cyVirtualScreen / CHUNK_SIZE;
            }
            else
            {
                DeleteDC(_hdcDimmed);
                _hdcDimmed = NULL;
            }
        }
        ReleaseDC(_hwnd, hdcWindow);
    }
}


//  --------------------------------------------------------------------------
//  CDimmedWindow::WndProc
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    See the platform SDK under WindowProc.
//
//  Purpose:    WindowProc for the dimmed window. This just passes the
//              messages thru to DefWindowProc.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    CDimmedWindow::WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    LRESULT         lResult = 0;
    CDimmedWindow   *pThis;

    pThis = reinterpret_cast<CDimmedWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT    *pCreateStruct;

            pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<CDimmedWindow*>(pCreateStruct->lpCreateParams);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            lResult = 0;
            pThis->SetupDim();
            if (pThis->_hdcDimmed)
            {
                SetTimer(hwnd, 1, 30, NULL);
            }
            break;
        }

        case WM_TIMER:
            if (!pThis->StepDim())
                KillTimer(hwnd, 1);
            break;

        case WM_PAINT:
        {
            HDC             hdcPaint;
            PAINTSTRUCT     ps;

            hdcPaint = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            lResult = 0;
            break;
        }
        default:
            lResult = DefWindowProcW(hwnd, uMsg, wParam, lParam);
            break;
    }

    return(lResult);
}


