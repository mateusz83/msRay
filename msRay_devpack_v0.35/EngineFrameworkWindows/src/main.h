#ifndef __MAIN_H__
#define __MAIN_H__

#define WIN32_LEAN_AND_MEAN

// Windows Header Files
#include <windows.h>

#include "targetver.h"
#include "resource.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// Raycaster Engine - main include
extern "C"
{
	#include "../../EngineCommonLibs/EN_Engine_Main.h"
}

// ---- APP global variables ----
HINSTANCE hInst;                                
WCHAR szTitle[] = L"Raycaster (PC)";                  
WCHAR szWindowClass[] = L"Raycaster";
HWND hWnd;

// This is the overall size of the application (window) -
// It can be requested by a launcher before start or changed in game options
int APP_requested_width = 640;
int APP_requested_height = 400;

RECT APP_rect_win = { 0, 0,	APP_requested_width, APP_requested_height };

// windows bitmap info
BITMAPINFOHEADER bitmapheader;
BITMAPINFO bitmapinfo;
HDC APP_hdc_mem;
HBITMAP APP_hbm_mem;

// for painting system text with fps
int APP_fps;
RECT APP_fps_rect = { 8, 8, 80, 20 };

// brushes and pens
HPEN hp_grey;

// ---- APP foreward declarations ----
BOOL				APP_Init(HINSTANCE, int);
ATOM                APP_MyRegisterClass(HINSTANCE);
BOOL                APP_InitInstance(HINSTANCE, int);
int					APP_MainLoop();
LRESULT CALLBACK    APP_WndProc(HWND, UINT, WPARAM, LPARAM);

#endif