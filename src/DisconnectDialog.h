#pragma once
#include "BaseDialog.h"

class CDisconnectDialog : public CBaseDialog
{
private:
	INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
	CDisconnectDialog();
};