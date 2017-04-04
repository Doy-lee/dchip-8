#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include "Common.h"

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

FILE_SCOPE HBITMAP bitmap;
FILE_SCOPE BITMAPINFO bitmapInfo;

const i32 WIDTH               = 64;
const i32 HEIGHT              = 32;
const i32 RESOLUTION          = WIDTH * HEIGHT;
FILE_SCOPE void *bitmapMemory;

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

#define ErrorBox(text, title) MessageBox(nullptr, text, title, MB_OK);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
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
		ErrorBox(L"RegisterClassEx() failed.", nullptr);
		return -1;
	}

	globalRunning = true;

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
		ErrorBox(L"CreateWindowEx() failed.", nullptr);
		return -1;
	}

	QueryPerformanceFrequency(&globalQueryPerformanceFrequency);
	LARGE_INTEGER startFrameTime;
	const f32 targetFramesPerSecond = 16.0f;
	f32 targetSecondsPerFrame       = 1 / targetFramesPerSecond;
	f32 frameTimeInS                = 0.0f;

	{
		BITMAPINFOHEADER header = {};
		header.biSize           = sizeof(BITMAPINFOHEADER);
		header.biWidth          = WIDTH;
		header.biHeight         = HEIGHT;
		header.biPlanes         = 1;
		header.biBitCount       = 32;
		header.biCompression    = BI_RGB; // uncompressed bitmap
		header.biSizeImage      = 0;
		header.biXPelsPerMeter  = 0;
		header.biYPelsPerMeter  = 0;
		header.biClrUsed        = 0;
		header.biClrImportant   = 0;

		bitmapInfo.bmiHeader = header;

		const i32 numPixels     = header.biWidth * header.biHeight;
		const i32 bytesPerPixel = header.biBitCount / 8;
#if 0
		bitmapMemory            = calloc(1, numPixels * bytesPerPixel);

		if (!bitmapMemory)
		{
			ErrorBox(L"malloc() failed.", nullptr);
			return -1;
		}

#else
		HDC deviceContext = GetDC(mainWindow);
		bitmap = CreateDIBSection(deviceContext, &bitmapInfo, DIB_RGB_COLORS,
		                          &bitmapMemory, NULL, NULL);
		ReleaseDC(mainWindow, deviceContext);

		if (!bitmapMemory)
		{
			ErrorBox(L"CreateDIBSection() failed.", nullptr);
			return -1;
		}
#endif

		u32 *bitmapBuffer = (u32 *)bitmapMemory;
		for (i32 i = 0; i < numPixels; i++)
		{
			// NOTE: Win32 AlphaBlend requires the RGB components to be
			// premultiplied with alpha.
			f32 normA = 1.0f;
			f32 normR = (normA * 0.0f);
			f32 normG = (normA * 0.0f);
			f32 normB = (normA * 1.0f);

			u8 r = (u8)(normR * 255.0f);
			u8 g = (u8)(normG * 255.0f);
			u8 b = (u8)(normB * 255.0f);
			u8 a = (u8)(normA * 255.0f);

			u32 color = (a << 24) | (r << 16) | (g << 8) | (b << 0);
			bitmapBuffer[i] = color;
		}
	}

	MSG msg;
	while (globalRunning)
	{
		startFrameTime = getWallClock();
		while (PeekMessage(&msg, mainWindow, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		RECT clientRect = {};
		GetClientRect(mainWindow, &clientRect);

		LONG clientWidth  = clientRect.right - clientRect.left;
		LONG clientHeight = clientRect.bottom - clientRect.top;

		BLENDFUNCTION blend       = {};
		blend.BlendOp             = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat         = AC_SRC_ALPHA;

		HDC deviceContext = GetDC(mainWindow);
		HDC alphaBlendDC  = CreateCompatibleDC(deviceContext);
		SelectObject(alphaBlendDC, bitmap);

		AlphaBlend(deviceContext, 0, 0, WIDTH, HEIGHT, alphaBlendDC, 0, 0,
		           WIDTH, HEIGHT, blend);
		StretchBlt(deviceContext, 0, 0, clientWidth, clientHeight,
		           deviceContext, 0, 0, WIDTH, HEIGHT, SRCCOPY);

		DeleteDC(alphaBlendDC);
		ReleaseDC(mainWindow, deviceContext);

		////////////////////////////////////////////////////////////////////////
		// Frame Limiting
		////////////////////////////////////////////////////////////////////////
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
