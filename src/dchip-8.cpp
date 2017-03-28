#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include "Common.h"

#include <Windows.h>
#include <stdio.h>

FILE_SCOPE bool globalRunning = false;

FILE_SCOPE LRESULT CALLBACK mainWindowProcCallback(HWND window, UINT msg,
                                                   WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (msg)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		{
			globalRunning = false;
		}
		break;

		default:
		{
			result = DefWindowProc(window, msg, wParam, lParam);
		}
		break;
	}

	return result;
}

GLOBAL_VAR LARGE_INTEGER globalQueryPerformanceFrequency;
inline FILE_SCOPE f32 getTimeFromQueryPerfCounter(LARGE_INTEGER start,
                                                  LARGE_INTEGER end)
{
	f32 result = (f32)(end.QuadPart - start.QuadPart) /
	             globalQueryPerformanceFrequency.QuadPart;
	return result;
}

inline FILE_SCOPE LARGE_INTEGER getWallClock()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);

	return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		mainWindowProcCallback,
		0, // int cbClsExtra
		0, // int cbWndExtra
		hInstance,
		LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW),
		GetSysColorBrush(COLOR_3DFACE),
		L"", // LPCTSTR lpszMenuName
		L"dchip-8Class",
		NULL, // HICON hIconSm
	};

	if (!RegisterClassEx(&wc)) {
		// TODO(doyle): Logging, couldn't register class
		ASSERT(INVALID_CODE_PATH);
		return -1;
	}

	// NOTE: Regarding Window Sizes
	// If you specify a window size, e.g. 800x600, Windows regards the 800x600
	// region to be inclusive of the toolbars and side borders. So in actuality,
	// when you blit to the screen blackness, the area that is being blitted to
	// is slightly smaller than 800x600. Windows provides a function to help
	// calculate the size you'd need by accounting for the window style.
	RECT r   = {};
	r.right  = 450;
	r.bottom = 200;

	DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	AdjustWindowRect(&r, windowStyle, FALSE);

	globalRunning = true;
	HWND mainWindow = CreateWindowEx(
	    WS_EX_COMPOSITED, wc.lpszClassName, L"dchip-8", windowStyle,
	    CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
	    nullptr, nullptr, hInstance, nullptr);

	if (!mainWindow)
	{
		// TODO(doyle): Logging, couldn't create root window
		ASSERT(INVALID_CODE_PATH);
		return -1;
	}

	QueryPerformanceFrequency(&globalQueryPerformanceFrequency);
	LARGE_INTEGER startFrameTime;
	const f32 targetFramesPerSecond = 16.0f;
	f32 targetSecondsPerFrame       = 1 / targetFramesPerSecond;
	f32 frameTimeInS                = 0.0f;

	MSG msg;
	while (globalRunning)
	{
		startFrameTime = getWallClock();
		while (PeekMessage(&msg, mainWindow, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		LARGE_INTEGER endWorkTime = getWallClock();
		f32 workTimeInS =
		    getTimeFromQueryPerfCounter(startFrameTime, endWorkTime);

		if (workTimeInS < targetSecondsPerFrame)
		{
			DWORD remainingTimeInMs =
			    (DWORD)((targetSecondsPerFrame - workTimeInS) * 1000);
			Sleep(remainingTimeInMs);
		}

		LARGE_INTEGER endFrameTime = getWallClock();
		frameTimeInS =
		    getTimeFromQueryPerfCounter(startFrameTime, endFrameTime);
		f32 msPerFrame = 1000.0f * frameTimeInS;

		wchar_t windowTitleBuffer[128] = {};
		_snwprintf_s(windowTitleBuffer, ARRAY_COUNT(windowTitleBuffer),
		             ARRAY_COUNT(windowTitleBuffer), L"dchip-8 | %5.2f ms/f",
		             msPerFrame);
		SetWindowText(mainWindow, windowTitleBuffer);
	}

	return 0;
}
