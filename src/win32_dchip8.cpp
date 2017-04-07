#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "dchip8.h"
#include "dchip8_platform.h"
#include "dqnt.h"

typedef struct Win32RenderBitmap
{
	BITMAPINFO  info;
	HBITMAP     handle;
	i32         width;
	i32         height;
	i32         bytesPerPixel;
	void       *memory;
} Win32RenderBitmap;

FILE_SCOPE bool              globalRunning = false;
FILE_SCOPE Win32RenderBitmap globalRenderBitmap;
FILE_SCOPE LARGE_INTEGER     globalQueryPerformanceFrequency;
#define win32_error_box(text, title) MessageBox(nullptr, text, title, MB_OK);

inline FILE_SCOPE void win32_get_client_dim(HWND window, LONG *width,
                                            LONG *height)
{
	RECT clientRect = {};
	GetClientRect(window, &clientRect);
	*width  = clientRect.right - clientRect.left;
	*height = clientRect.bottom - clientRect.top;
}

FILE_SCOPE void win32_display_render_bitmap(Win32RenderBitmap renderBitmap,
                                            HDC deviceContext, LONG width,
                                            LONG height)
{
	HDC stretchDC = CreateCompatibleDC(deviceContext);
	SelectObject(stretchDC, renderBitmap.handle);
	StretchBlt(deviceContext, 0, 0, width, height, stretchDC, 0, 0,
	           renderBitmap.width, renderBitmap.height, SRCCOPY);

    // NOTE: Win32 AlphaBlend requires the RGB components to be premultiplied
	// with alpha.
#if 0
	BLENDFUNCTION blend       = {};
	blend.BlendOp             = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat         = AC_SRC_ALPHA;

	if (!AlphaBlend(deviceContext, 0, 0, width, height, deviceContext, 0, 0,
	                width, height, blend))
	{
		OutputDebugString(L"AlphaBlend() failed.");
	}
#endif
	DeleteDC(stretchDC);
}

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

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			LONG clientWidth, clientHeight;
			win32_get_client_dim(window, &clientWidth, &clientHeight);
			win32_display_render_bitmap(globalRenderBitmap, deviceContext,
			                            clientWidth, clientHeight);
			EndPaint(window, &paint);
			break;
		}

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

FILE_SCOPE inline void win32_parse_key_msg(KeyState *key, MSG msg)
{
	WPARAM lParam        = msg.lParam;
	bool keyIsDown       = ((lParam >> 30) & 1);
	bool keyTransitioned = ((lParam >> 31) & 1);

	key->isDown = keyIsDown;
	if (keyTransitioned) key->transitionCount++;
}

FILE_SCOPE void win32_process_messages(HWND window, PlatformInput *input)
{
	MSG msg;
	while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				switch (msg.wParam)
				{
					case VK_UP: win32_parse_key_msg(&input->up, msg); break;
					case VK_DOWN: win32_parse_key_msg(&input->down, msg); break;
					case VK_LEFT: win32_parse_key_msg(&input->left, msg); break;
					case VK_RIGHT: win32_parse_key_msg(&input->right, msg); break;

					case '7': win32_parse_key_msg(&input->key_7, msg); break;
					case '8': win32_parse_key_msg(&input->key_8, msg); break;
					case '9': win32_parse_key_msg(&input->key_9, msg); break;
					case '0': win32_parse_key_msg(&input->key_0, msg); break;

					case 'U': win32_parse_key_msg(&input->key_U, msg); break;
					case 'I': win32_parse_key_msg(&input->key_I, msg); break;
					case 'O': win32_parse_key_msg(&input->key_O, msg); break;
					case 'P': win32_parse_key_msg(&input->key_P, msg); break;

					case 'J': win32_parse_key_msg(&input->key_J, msg); break;
					case 'K': win32_parse_key_msg(&input->key_K, msg); break;
					case 'L': win32_parse_key_msg(&input->key_L, msg); break;
					case ';': win32_parse_key_msg(&input->key_colon, msg); break;

					case 'M': win32_parse_key_msg(&input->key_M, msg); break;
					case ',': win32_parse_key_msg(&input->key_comma, msg); break;
					case '.': win32_parse_key_msg(&input->key_dot, msg); break;
					case '/': win32_parse_key_msg(&input->key_forward_slash, msg); break;

					case VK_ESCAPE:
					{
						win32_parse_key_msg(&input->escape, msg);
						if (input->escape.isDown) globalRunning = false;
					}
					break;

					default: break;
				}
			};

			default:
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			};
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
	////////////////////////////////////////////////////////////////////////////
	// Initialise Win32 Window
	////////////////////////////////////////////////////////////////////////////
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

	if (!RegisterClassEx(&wc))
	{
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
	rect.right  = 256;
	rect.bottom = 128;

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

		globalRenderBitmap.info.bmiHeader = header;
		globalRenderBitmap.width          = header.biWidth;
		globalRenderBitmap.height         = header.biHeight;
		globalRenderBitmap.bytesPerPixel  = header.biBitCount / 8;
		DQNT_ASSERT(globalRenderBitmap.bytesPerPixel >= 1);

		HDC deviceContext = GetDC(mainWindow);
		globalRenderBitmap.handle = CreateDIBSection(
		    deviceContext, &globalRenderBitmap.info, DIB_RGB_COLORS,
		    &globalRenderBitmap.memory, NULL, NULL);
		ReleaseDC(mainWindow, deviceContext);
	}

	if (!globalRenderBitmap.memory)
	{
		win32_error_box(L"CreateDIBSection() failed.", nullptr);
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	// Update Loop
	////////////////////////////////////////////////////////////////////////////
	u8 stackMemory[4096]            = {};
	PlatformMemory platformMemory   = {};
	platformMemory.permanentMem     = &stackMemory;
	platformMemory.permanentMemSize = DQNT_ARRAY_COUNT(stackMemory);

	QueryPerformanceFrequency(&globalQueryPerformanceFrequency);
	const f32 TARGET_FRAMES_PER_S = 180.0f;
	f32 targetSecondsPerFrame     = 1 / TARGET_FRAMES_PER_S;
	f32 frameTimeInS              = 0.0f;
	globalRunning                 = true;

	while (globalRunning)
	{
		////////////////////////////////////////////////////////////////////////
		// Update State
		////////////////////////////////////////////////////////////////////////
		LARGE_INTEGER startFrameTime = win32_query_perf_counter_time();
		{
			PlatformInput platformInput = {};
			platformInput.deltaForFrame = frameTimeInS;
			win32_process_messages(mainWindow, &platformInput);

			PlatformRenderBuffer platformBuffer = {};
			platformBuffer.memory               = globalRenderBitmap.memory;
			platformBuffer.height               = globalRenderBitmap.height;
			platformBuffer.width                = globalRenderBitmap.width;
			platformBuffer.bytesPerPixel = globalRenderBitmap.bytesPerPixel;
			dchip8_update(platformBuffer, platformInput, platformMemory);

			for (i32 i = 0; i < DQNT_ARRAY_COUNT(platformInput.key); i++)
			{
				if (platformInput.key[i].isDown)
					OutputDebugString(L"Key is down\n");
			}
		}

		////////////////////////////////////////////////////////////////////////
		// Rendering
		////////////////////////////////////////////////////////////////////////
		{
			RECT clientRect = {};
			GetClientRect(mainWindow, &clientRect);
			LONG clientWidth  = clientRect.right - clientRect.left;
			LONG clientHeight = clientRect.bottom - clientRect.top;

			HDC deviceContext = GetDC(mainWindow);
			win32_display_render_bitmap(globalRenderBitmap, deviceContext,
			                            clientWidth, clientHeight);
			ReleaseDC(mainWindow, deviceContext);
		}

		////////////////////////////////////////////////////////////////////////
		// Frame Limiting
		////////////////////////////////////////////////////////////////////////
		{
			LARGE_INTEGER endWorkTime = win32_query_perf_counter_time();
			f32 workTimeInS =
			    win32_query_perf_counter_get_time(startFrameTime, endWorkTime);
			if (workTimeInS < targetSecondsPerFrame)
			{
				DWORD remainingTimeInMs =
				    (DWORD)((targetSecondsPerFrame - workTimeInS) * 1000);
				Sleep(remainingTimeInMs);
			}
		}

		LARGE_INTEGER endFrameTime = win32_query_perf_counter_time();
		frameTimeInS =
		    win32_query_perf_counter_get_time(startFrameTime, endFrameTime);
		f32 msPerFrame = 1000.0f * frameTimeInS;

		wchar_t windowTitleBuffer[128] = {};
		_snwprintf_s(windowTitleBuffer, DQNT_ARRAY_COUNT(windowTitleBuffer),
		             DQNT_ARRAY_COUNT(windowTitleBuffer),
		             L"dchip-8 | %5.2f ms/f", msPerFrame);
		SetWindowText(mainWindow, windowTitleBuffer);
	}

	return 0;
}

void platform_close_file(PlatformFile *file)
{
	CloseHandle(file->handle);
	file->handle = NULL;
	file->size   = 0;
}

u32 platform_read_file(PlatformFile file, void *buffer, u32 numBytesToRead)
{
	DWORD numBytesRead = 0;
	if (file.handle && buffer)
	{
		HANDLE win32Handle = file.handle;
		BOOL result =
		    ReadFile(win32Handle, buffer, numBytesToRead, &numBytesRead, NULL);

		// TODO(doyle): 0 also means it is completing async, but still valid
		if (result == 0)
		{
			win32_error_box(L"ReadFile() failed.", nullptr);
		}

	}

	return numBytesRead;
}

bool platform_open_file(const wchar_t *const file, PlatformFile *platformFile)
{
	HANDLE handle = CreateFile(file, GENERIC_READ | GENERIC_WRITE, 0, NULL,
	                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		win32_error_box(L"CreateFile() failed.", nullptr);
		return false;
	}

	LARGE_INTEGER size;
	if (GetFileSizeEx(handle, &size) == 0)
	{
		win32_error_box(L"GetFileSizeEx() failed.", nullptr);
		return false;
	}

	platformFile->handle = handle;
	platformFile->size   = size.QuadPart;

	return true;
}
