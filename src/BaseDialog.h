#pragma once
#include "ClassicShutdownP.h"

class __declspec(novtable) CBaseDialog
{
private:
	INT_PTR _nResult;

protected:
	HWND _hwnd;
	UINT _uDlgID;

	static INT_PTR CALLBACK s_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	void _Center(int nDivideX, int nDivideY);

	void EndDialog(HWND hwnd, INT_PTR nResult);

public:
	INT_PTR Show(HWND hwndParent);

	virtual ~CBaseDialog() {}
};