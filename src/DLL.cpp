#include "ClassicShutdownP.h"

void *operator new(size_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void operator delete(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

void operator delete(void *ptr, size_t size)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

HANDLE g_hActCtx = NULL;

typedef DPI_AWARENESS_CONTEXT (WINAPI *SetThreadDpiAwarenessContext_t)(DPI_AWARENESS_CONTEXT dpiContext);
SetThreadDpiAwarenessContext_t g_pfnSetThreadDpiAwarenessContext = nullptr;

EXTERN_C STDAPI_(BOOL) DllMain(
	HINSTANCE hinstDLL,
	DWORD     fdwReason,
	LPVOID    lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_hinst = hinstDLL;

			ACTCTXW actCtx;
			ZeroMemory(&actCtx, sizeof(actCtx));
			actCtx.cbSize         = sizeof(actCtx);
			actCtx.dwFlags        = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
			actCtx.hModule        = g_hinst;
			actCtx.lpResourceName = MAKEINTRESOURCEW(2);
			g_hActCtx = CreateActCtxW(&actCtx);
			if (g_hActCtx == INVALID_HANDLE_VALUE)
				return FALSE;

			g_pfnSetThreadDpiAwarenessContext
				= (SetThreadDpiAwarenessContext_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext");

			break;
		}
		case DLL_PROCESS_DETACH:
			if (g_hkey)
				RegCloseKey(g_hkey);
			if (g_hinstShell)
				FreeLibrary(g_hinstShell);
			break;
	}
	return TRUE;
}

HRESULT LoadShellModule(void)
{
	g_hinstShell = GetModuleHandleW(L"shell32.dll");
	if (!g_hinstShell)
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

DPI_AWARENESS_CONTEXT ActivateDPIAwarenessAndActCtx(ULONG_PTR *pulCookie)
{
	ActivateActCtx(g_hActCtx, pulCookie);

	if (g_pfnSetThreadDpiAwarenessContext)
	{
		DPI_AWARENESS_CONTEXT dpiOld = g_pfnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		if (!dpiOld)
		{
			dpiOld = g_pfnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
			if (!dpiOld)
				dpiOld = g_pfnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
		}
		return dpiOld;
	}
	else
	{
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

void DeactivateDPIAwarenessAndActCtx(ULONG_PTR ulCookie, DPI_AWARENESS_CONTEXT dpiOld)
{
	DeactivateActCtx(0, ulCookie);
	if (g_pfnSetThreadDpiAwarenessContext)
	{
		g_pfnSetThreadDpiAwarenessContext(dpiOld);
	}
}