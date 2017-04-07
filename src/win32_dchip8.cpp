#define UNICODE
#define _UNICODE

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Commdlg.h>
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

#define MIN_WIDTH  256;
#define MIN_HEIGHT 128;
typedef struct Win32State
{
	bool useCorrectAspectRatio = true;
} Win32State;

FILE_SCOPE bool              globalRunning = false;
FILE_SCOPE Win32RenderBitmap globalRenderBitmap;
FILE_SCOPE LARGE_INTEGER     globalQueryPerformanceFrequency;
FILE_SCOPE Win32State        globalState;
#define win32_error_box(text, title) MessageBox(nullptr, text, title, MB_OK);

enum Win32Menu
{
	win32menu_file_exit,
	win32menu_file_open,

	win32menu_video_correct_aspect_ratio,
	win32menu_video_4x,
	win32menu_video_8x,
	win32menu_video_12x,
	win32menu_video_16x,
};

FILE_SCOPE v2 constrain_to_aspect_ratio(u32 width, u32 height, f32 ratioX,
                                        f32 ratioY)
{
	v2 result = {};
	f32 numRatioIncrementsToWidth  = (f32)(width / ratioX);
	f32 numRatioIncrementsToHeight = (f32)(height / ratioY);

	f32 leastIncrementsToSide =
	    DQNT_MATH_MIN(numRatioIncrementsToHeight, numRatioIncrementsToWidth);

	result.w = (f32)(ratioX * leastIncrementsToSide);
	result.h = (f32)(ratioY * leastIncrementsToSide);
	return result;
}

FILE_SCOPE void win32_create_menu(HWND window)
{
	HMENU menuBar  = CreateMenu();
	{ // File Menu
		HMENU menu = CreatePopupMenu();
		AppendMenu(menuBar, MF_STRING | MF_POPUP, (UINT)menu, L"File");
		AppendMenu(menu, MF_STRING, win32menu_file_open, L"Open ROM");
		AppendMenu(menu, MF_STRING, win32menu_file_exit, L"Exit");
	}

	{ // Video Menu
		HMENU menu = CreatePopupMenu();
		AppendMenu(menuBar, MF_STRING | MF_POPUP, (UINT)menu, L"Video");
		AppendMenu(menu, MF_STRING | MF_CHECKED,
		           win32menu_video_correct_aspect_ratio,
		           L"Use Correct Aspect Ratio");
		AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
		AppendMenu(menu, MF_STRING, win32menu_video_4x, L"4x");
		AppendMenu(menu, MF_STRING, win32menu_video_8x, L"8x");
		AppendMenu(menu, MF_STRING, win32menu_video_12x, L"12x");
		AppendMenu(menu, MF_STRING, win32menu_video_16x, L"16x");
	}

	SetMenu(window, menuBar);

	/*
	MENUITEMINFO testMenuInfo = {};
	testMenuInfo.cbSize       = sizeof(MENUITEMINFO);
	testMenuInfo.fMask        = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
	testMenuInfo.fType        = MFT_STRING;
	testMenuInfo.fState       = 0;
	testMenuInfo.wID          = win32menu_file_test;
	testMenuInfo.hSubMenu     = fileMenu;
	testMenuInfo.dwTypeData   = L"Test";
	testMenuInfo.cch          = dqnt_wstrlen(testMenuInfo.dwTypeData);
	InsertMenuItem(menu, 0, true, &testMenuInfo);
	*/
}

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
		case WM_CREATE:
		{
			win32_create_menu(window);
		}
		break;

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

			LONG renderWidth, renderHeight;
			win32_get_client_dim(window, &renderWidth, &renderHeight);
			if (globalState.useCorrectAspectRatio)
			{
				f32 ratioX = (f32)globalRenderBitmap.width;
				f32 ratioY = (f32)globalRenderBitmap.height;
				v2 newDim = constrain_to_aspect_ratio(renderWidth, renderHeight,
				                                      ratioX, ratioY);

				renderWidth  = (LONG)newDim.w;
				renderHeight = (LONG)newDim.h;
			}

			win32_display_render_bitmap(globalRenderBitmap, deviceContext,
			                            renderWidth, renderHeight);
			EndPaint(window, &paint);
			break;
		}

		case WM_GETMINMAXINFO:
		{
			RECT rect   = {};
			rect.right  = MIN_WIDTH;
			rect.bottom = MIN_HEIGHT;
			DWORD windowStyle = (DWORD)GetWindowLong(window, GWL_STYLE);
			AdjustWindowRect(&rect, windowStyle, true);

			MINMAXINFO *mmi  = (MINMAXINFO *)lParam;
			mmi->ptMinTrackSize.x = rect.left - rect.right;
			mmi->ptMinTrackSize.y = rect.bottom - rect.top;
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

FILE_SCOPE inline void win32_update_key(KeyState *key, bool isDown)
{
	if (key->endedDown != isDown)
	{
		key->endedDown = isDown;
		key->halfTransitionCount++;
	}
}

FILE_SCOPE void win32_handle_menu_messages(HWND window, MSG msg,
                                           PlatformInput *input)
{
	switch (LOWORD(msg.wParam))
	{
		case win32menu_file_exit:
		{
			globalRunning = false;
		}
		break;

		case win32menu_file_open:
		{
			wchar_t fileBuffer[MAX_PATH] = {};
			OPENFILENAME openDialogInfo  = {};
			openDialogInfo.lStructSize   = sizeof(OPENFILENAME);
			openDialogInfo.hwndOwner     = window;
			openDialogInfo.lpstrFile     = fileBuffer;
			openDialogInfo.nMaxFile      = DQNT_ARRAY_COUNT(fileBuffer);
			openDialogInfo.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

			if (GetOpenFileName(&openDialogInfo) != 0)
			{
				wchar_t candidateRom[MAX_PATH] = {};
				wchar_t currentRom[MAX_PATH]   = {};
				GetFullPathName(fileBuffer, DQNT_ARRAY_COUNT(candidateRom),
				                candidateRom, nullptr);
				GetFullPathName(input->rom, DQNT_ARRAY_COUNT(currentRom),
				                currentRom, nullptr);

				if (dqnt_wstrcmp(candidateRom, currentRom) != 0)
				{
					input->loadNewRom = true;

					for (i32 i = 0; i < dqnt_wstrlen(candidateRom); i++)
						input->rom[i] = candidateRom[i];
				}
			}
		}
		break;

		case win32menu_video_4x:
		case win32menu_video_8x:
		case win32menu_video_12x:
		case win32menu_video_16x:
		{
			u32 startingWidth  = 64;
			u32 startingHeight = 32;

			u32 modifier;
			switch (LOWORD(msg.wParam))
			{
				default:
				case win32menu_video_4x:  modifier = 4; break;
				case win32menu_video_8x:  modifier = 8; break;
				case win32menu_video_12x: modifier = 12; break;
				case win32menu_video_16x: modifier = 16; break;
			}

			RECT rect   = {};
			rect.right  = startingWidth * modifier;
			rect.bottom = startingHeight * modifier;

			DWORD windowStyle = (DWORD)GetWindowLong(window, GWL_STYLE);
			AdjustWindowRect(&rect, windowStyle, true);

			SetWindowPos(window, NULL, 0, 0, rect.right - rect.left,
			             rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
		}
		break;

		case win32menu_video_correct_aspect_ratio:
		{
			globalState.useCorrectAspectRatio =
			    (globalState.useCorrectAspectRatio) ? false : true;
		}
		break;

		default:
		{
			DQNT_ASSERT(DQNT_INVALID_CODE_PATH);
		}
		break;
	}
}

FILE_SCOPE void win32_process_messages(HWND window, PlatformInput *input)
{
	MSG msg;
	while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_COMMAND:
			{
				win32_handle_menu_messages(window, msg, input);
			}
			break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				bool isDown = (msg.message == WM_KEYDOWN);
				switch (msg.wParam)
				{
					case VK_UP: win32_update_key(&input->up, isDown); break;
					case VK_DOWN: win32_update_key(&input->down, isDown); break;
					case VK_LEFT: win32_update_key(&input->left, isDown); break;
					case VK_RIGHT: win32_update_key(&input->right, isDown); break;

					case '1': win32_update_key(&input->key_1, isDown); break;
					case '2': win32_update_key(&input->key_2, isDown); break;
					case '3': win32_update_key(&input->key_3, isDown); break;
					case '4': win32_update_key(&input->key_4, isDown); break;

					case 'Q': win32_update_key(&input->key_q, isDown); break;
					case 'W': win32_update_key(&input->key_w, isDown); break;
					case 'E': win32_update_key(&input->key_e, isDown); break;
					case 'R': win32_update_key(&input->key_r, isDown); break;

					case 'A': win32_update_key(&input->key_a, isDown); break;
					case 'S': win32_update_key(&input->key_s, isDown); break;
					case 'D': win32_update_key(&input->key_d, isDown); break;
					case 'F': win32_update_key(&input->key_f, isDown); break;

					case 'Z': win32_update_key(&input->key_z, isDown); break;
					case 'X': win32_update_key(&input->key_x, isDown); break;
					case 'C': win32_update_key(&input->key_c, isDown); break;
					case 'V': win32_update_key(&input->key_v, isDown); break;

					case VK_ESCAPE:
					{
						win32_update_key(&input->escape, isDown);
						if (input->escape.endedDown) globalRunning = false;
					}
					break;

					default: break;
				}
			}
			break;

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
		GetSysColorBrush(COLOR_3DFACE),
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
	rect.right  = MIN_WIDTH;
	rect.bottom = MIN_HEIGHT;
	DWORD windowStyle =
	    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	AdjustWindowRect(&rect, windowStyle, true);

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
	const f32 TARGET_FRAMES_PER_S = 30.0f;
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
			dchip8_update(platformBuffer, platformInput, platformMemory, 15);
		}

		////////////////////////////////////////////////////////////////////////
		// Rendering
		////////////////////////////////////////////////////////////////////////
		{
			LONG renderWidth, renderHeight;
			win32_get_client_dim(mainWindow, &renderWidth, &renderHeight);
			if (globalState.useCorrectAspectRatio)
			{
				f32 ratioX = (f32)globalRenderBitmap.width;
				f32 ratioY = (f32)globalRenderBitmap.height;
				v2 newDim = constrain_to_aspect_ratio(renderWidth, renderHeight,
				                                      ratioX, ratioY);

				renderWidth  = (LONG)newDim.w;
				renderHeight = (LONG)newDim.h;
			}

			HDC deviceContext = GetDC(mainWindow);
			win32_display_render_bitmap(globalRenderBitmap, deviceContext,
			                            renderWidth, renderHeight);
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
