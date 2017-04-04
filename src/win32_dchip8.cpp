#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "dchip8.h"
#include "dchip8_platform.h"

FILE_SCOPE bool globalRunning = false;
FILE_SCOPE LARGE_INTEGER globalQueryPerformanceFrequency;
#define win32_error_box(text, title) MessageBox(nullptr, text, title, MB_OK);

FILE_SCOPE LRESULT CALLBACK win32_main_proc_callback(HWND window, UINT msg,
                                                     WPARAM wParam,
                                                     LPARAM lParam)
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

inline FILE_SCOPE f32 win32_query_perf_counter_get_time(LARGE_INTEGER start,
                                                        LARGE_INTEGER end)
{
	f32 result = (f32)(end.QuadPart - start.QuadPart) /
	             globalQueryPerformanceFrequency.QuadPart;
	return result;
}

inline FILE_SCOPE LARGE_INTEGER win32_query_perf_counter_time()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);

	return result;
}

typedef struct Win32RenderBitmap
{
	BITMAPINFO  info;
	HBITMAP     handle;
	i32         width;
	i32         height;
	i32         bytesPerPixel;
	void       *memory;
} Win32RenderBitmap;

FILE_SCOPE void win32_display_render_bitmap(Win32RenderBitmap renderBitmap,
                                            HDC deviceContext, LONG width,
                                            LONG height)
{
	BLENDFUNCTION blend       = {};
	blend.BlendOp             = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat         = AC_SRC_ALPHA;

	HDC alphaBlendDC  = CreateCompatibleDC(deviceContext);
	SelectObject(alphaBlendDC, renderBitmap.handle);

	AlphaBlend(deviceContext, 0, 0, width, height, alphaBlendDC, 0, 0,
	           renderBitmap.width, renderBitmap.height, blend);
	StretchBlt(deviceContext, 0, 0, width, height, deviceContext, 0, 0,
	           renderBitmap.width, renderBitmap.height, SRCCOPY);

	DeleteDC(alphaBlendDC);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		win32_main_proc_callback,
		0, // int cbClsExtra
		0, // int cbWndExtra
		hInstance,
		LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW),
		NULL, // GetSysColorBrush(COLOR_3DFACE),
		L"", // LPCTSTR lpszMenuName
		L"dchip-8Class",
		NULL, // HICON hIconSm
	};

	if (!RegisterClassEx(&wc)) {
		win32_error_box(L"RegisterClassEx() failed.", nullptr);
		return -1;
	}

	// NOTE: Regarding Window Sizes
	// If you specify a window size, e.g. 800x600, Windows regards the 800x600
	// region to be inclusive of the toolbars and side borders. So in actuality,
	// when you blit to the screen blackness, the area that is being blitted to
	// is slightly smaller than 800x600. Windows provides a function to help
	// calculate the size you'd need by accounting for the window style.
	RECT rect   = {};
	rect.right  = 128;
	rect.bottom = 64;

	DWORD windowStyle =
	    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	AdjustWindowRect(&rect, windowStyle, FALSE);

	HWND mainWindow = CreateWindowEx(
	    WS_EX_COMPOSITED, wc.lpszClassName, L"dchip-8", windowStyle,
	    CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
	    rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);

	if (!mainWindow)
	{
		win32_error_box(L"CreateWindowEx() failed.", nullptr);
		return -1;
	}

	Win32RenderBitmap renderBitmap = {};
	{ // Initialise the renderbitmap
		BITMAPINFOHEADER header = {};
		header.biSize           = sizeof(BITMAPINFOHEADER);
		header.biWidth          = 64;
		header.biHeight         = 32;
		header.biPlanes         = 1;
		header.biBitCount       = 32;
		header.biCompression    = BI_RGB; // uncompressed bitmap
		header.biSizeImage      = 0;
		header.biXPelsPerMeter  = 0;
		header.biYPelsPerMeter  = 0;
		header.biClrUsed        = 0;
		header.biClrImportant   = 0;

		renderBitmap.info.bmiHeader = header;
		renderBitmap.width          = header.biWidth;
		renderBitmap.height         = header.biHeight;
		renderBitmap.bytesPerPixel  = header.biBitCount / 8;
		ASSERT(renderBitmap.bytesPerPixel >= 1);

		HDC deviceContext = GetDC(mainWindow);
		renderBitmap.handle =
		    CreateDIBSection(deviceContext, &renderBitmap.info, DIB_RGB_COLORS,
		                     &renderBitmap.memory, NULL, NULL);
		ReleaseDC(mainWindow, deviceContext);
	}

	if (!renderBitmap.memory)
	{
		win32_error_box(L"CreateDIBSection() failed.", nullptr);
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	// Update Loop
	////////////////////////////////////////////////////////////////////////////
	QueryPerformanceFrequency(&globalQueryPerformanceFrequency);
	const f32 TARGET_FRAMES_PER_S = 60.0f;
	f32 targetSecondsPerFrame     = 1 / TARGET_FRAMES_PER_S;
	f32 frameTimeInS              = 0.0f;
	globalRunning                 = true;

	while (globalRunning)
	{
		LARGE_INTEGER startFrameTime = win32_query_perf_counter_time();

		MSG msg;
		while (PeekMessage(&msg, mainWindow, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		////////////////////////////////////////////////////////////////////////
		// Rendering
		////////////////////////////////////////////////////////////////////////
		PlatformRenderBuffer renderBuffer = {};
		renderBuffer.memory               = renderBitmap.memory;
		renderBuffer.height               = renderBitmap.height;
		renderBuffer.width                = renderBitmap.width;
		dchip8_update(&renderBuffer);

		HDC deviceContext = GetDC(mainWindow);
		win32_display_render_bitmap(renderBitmap, deviceContext, clientWidth,
		                            clientHeight);
		ReleaseDC(mainWindow, deviceContext);

		////////////////////////////////////////////////////////////////////////
		// Frame Limiting
		////////////////////////////////////////////////////////////////////////
		LARGE_INTEGER endWorkTime = win32_query_perf_counter_time();
		f32 workTimeInS =
		    win32_query_perf_counter_get_time(startFrameTime, endWorkTime);
		if (workTimeInS < targetSecondsPerFrame)
		{
			DWORD remainingTimeInMs =
			    (DWORD)((targetSecondsPerFrame - workTimeInS) * 1000);
			Sleep(remainingTimeInMs);
		}

		LARGE_INTEGER endFrameTime = win32_query_perf_counter_time();
		frameTimeInS =
		    win32_query_perf_counter_get_time(startFrameTime, endFrameTime);
		f32 msPerFrame = 1000.0f * frameTimeInS;

		wchar_t windowTitleBuffer[128] = {};
		_snwprintf_s(windowTitleBuffer, ARRAY_COUNT(windowTitleBuffer),
		             ARRAY_COUNT(windowTitleBuffer), L"dchip-8 | %5.2f ms/f",
		             msPerFrame);
		SetWindowText(mainWindow, windowTitleBuffer);
	}

	return 0;
}