#include "main.h"
#include <time.h>


int APIENTRY wWinMain(_In_ HINSTANCE _hInstance, _In_opt_ HINSTANCE _hPrevInstance, _In_ LPWSTR _lpCmdLine, _In_ int _nCmdShow)
{
    UNREFERENCED_PARAMETER(_hPrevInstance);
    UNREFERENCED_PARAMETER(_lpCmdLine);

    // init Windows stuff
    if (!APP_Init(_hInstance, _nCmdShow))
        return 0;

    // Application main loop
    return APP_MainLoop();
}

BOOL APP_Init(HINSTANCE _hInstance, int _nCmdShow)
{
    // Initialize global strings
    APP_MyRegisterClass(_hInstance);

    // Perform application initialization:
    if (!APP_InitInstance(_hInstance, _nCmdShow))
    {
        return 0;
    }

    // windows bitmap info - filling up structures
    bitmapheader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapheader.biWidth = APP_requested_width;
    bitmapheader.biHeight = -1 * APP_requested_height;
    bitmapheader.biPlanes = 1;
    bitmapheader.biBitCount = 4 * 8;
    bitmapheader.biCompression = BI_RGB;
    bitmapheader.biSizeImage = 0;
    bitmapheader.biXPelsPerMeter = 0;
    bitmapheader.biYPelsPerMeter = 0;
    bitmapheader.biClrUsed = 0;
    bitmapheader.biClrImportant = 0;

    bitmapinfo.bmiHeader = bitmapheader;

    APP_hdc_mem = CreateCompatibleDC(GetDC(0));
    APP_hbm_mem = CreateDIBitmap(GetDC(0), &bitmapheader, CBM_INIT, IO_prefs.output_buffer_32, &bitmapinfo, DIB_RGB_COLORS);

    SetTextColor(APP_hdc_mem, RGB(255, 255, 255));
    SetBkColor(APP_hdc_mem, RGB(0, 0, 0));
    SetBkMode(APP_hdc_mem, OPAQUE);

    // brushes and pens init
    hp_grey = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));

    return 1;
}

ATOM APP_MyRegisterClass(HINSTANCE _hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = APP_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = _hInstance;
    wcex.hIcon          = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_RAYCASTERPC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL APP_InitInstance(HINSTANCE _hInstance, int _nCmdShow)
{
   hInst = _hInstance;

   // window centre position
   int tmp_posX = abs((GetSystemMetrics(SM_CXSCREEN) - APP_requested_width)) / 2;
   int tmp_posY = abs((GetSystemMetrics(SM_CYSCREEN) - APP_requested_height)) / 2;

   AdjustWindowRect(&APP_rect_win, WS_CAPTION | WS_OVERLAPPEDWINDOW, 0);

   hWnd = CreateWindowEx(   WS_EX_APPWINDOW, szWindowClass, szTitle, WS_CAPTION | WS_OVERLAPPEDWINDOW,
                            tmp_posX, tmp_posY, APP_rect_win.right - APP_rect_win.left, APP_rect_win.bottom - APP_rect_win.top,
                            nullptr, nullptr, _hInstance, nullptr);

   if (!hWnd)
      return 0;

   // TODO: EM init function - where to place it - for now here..
   if (!EN_Init(APP_screen_buffers_number, APP_requested_width, APP_requested_height, IO_PIXFMT_BGRA32))
   {
       MessageBox(hWnd, L"Allocation error...", L"Error...", MB_OK);
       EN_Cleanup();
       return 0;
   }

   IO_prefs.output_buffer_32 = (u_int32*)malloc(IO_prefs.screen_frame_buffer_size);

   ShowCursor(0);

   ShowWindow(hWnd, _nCmdShow);
   UpdateWindow(hWnd);

   return 1;
}

DWORD WINAPI AppThread_01(LPVOID lParam)
{
  /*  while (&lParam)
    {
        EN_Run((int8*)lParam);
    }
    */
    return 0;
}

int APP_MainLoop()
{
    // Windows messages
    MSG msg;

    // Main loop status
    int8 APP_main_loop = 1;
    
    // for FPS counter
    int frames = 0;
    long elapsed_time = 0;
    long curr_time = clock();
    long start_time = curr_time;

          // DWORD dwThreadId;
          // HANDLE hThread;
         // hThread = CreateThread(NULL, 0, AppThread_01, (LPVOID)APP_main_loop, 0, &dwThreadId);

    // Enter APP main loop
    while (APP_main_loop)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // fps counter
        frames++;
        curr_time = clock();
        long TM_time_total = (curr_time - start_time) / CLOCKS_PER_SEC;

        if (TM_time_total - elapsed_time >= 1)
        {
            APP_fps = frames;

            // reset
            frames = 0;
            elapsed_time += 1;
        }

        // make everything
       EN_Run(&APP_main_loop);

        // force Windows to repaint all frame
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
    }

           // CloseHandle(hThread);

    EN_Cleanup();

    return (int)msg.wParam;
}

LRESULT CALLBACK APP_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);

              //  switch (wmId)
              //  {           
                  //  default:
                        return DefWindowProc(hWnd, message, wParam, lParam);
               // }
            }
            break;

        case WM_MOUSEMOVE:
            {
                POINT p0, p1;
                GetCursorPos(&p0);
                SetCursorPos(GetSystemMetrics(SM_CXSCREEN) >> 1, GetSystemMetrics(SM_CYSCREEN) >> 1);
                GetCursorPos(&p1);

                IO_input.mouse_x = (int8)p0.x;
                IO_input.mouse_y = (int8)p0.y;

                IO_input.mouse_dx = (int8)(p0.x - p1.x);
                IO_input.mouse_dy = (int8)(p0.y - p1.y);
            }
            break;

        case WM_LBUTTONDOWN:
            {
                IO_input.mouse_left = 1;
            }
            break;

        case WM_LBUTTONUP:
            {
                IO_input.mouse_left = 0;
            }
            break;

        case WM_RBUTTONDOWN:
            {
                IO_input.mouse_right = 1;
            }
            break;

        case WM_RBUTTONUP:
            {
                IO_input.mouse_right = 0;
            }
            break;

        case WM_KEYDOWN:
            {
                IO_input.keys[ (u_int8)wParam ] = 1;
            }
            break;

        case WM_KEYUP:
            {
                IO_input.keys[(u_int8)wParam] = 0;
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                    SetDIBits(APP_hdc_mem, APP_hbm_mem, 0, APP_requested_height, IO_prefs.output_buffer_32, &bitmapinfo, DIB_RGB_COLORS);
                    SelectObject(APP_hdc_mem, APP_hbm_mem);

                    // draw FPS
                        TCHAR tmp_fps_buff[16];
                        ZeroMemory(&tmp_fps_buff, sizeof(tmp_fps_buff));
                        swprintf_s(tmp_fps_buff, L"%d   %.5f", APP_fps, IO_prefs.delta_time);
                        DrawText(APP_hdc_mem, tmp_fps_buff, -1, &APP_fps_rect, DT_SINGLELINE | DT_NOCLIP);
                    // ---

                    BitBlt(hdc, 0, 0, APP_requested_width, APP_requested_height, APP_hdc_mem, 0, 0, SRCCOPY);

                EndPaint(hWnd, &ps);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
