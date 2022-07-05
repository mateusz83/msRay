#include "main.h"

// ---------------------------------
// --- APP FUNCTIONS DEFINITIONS ---
// ---------------------------------

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // shutting off system animation of windows and icons
    SystemParametersInfo(SPI_SETCLIENTAREAANIMATION, 0, (LPVOID)FALSE, 0);

    APP_MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!APP_InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LEVELEDITOR));

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM APP_MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = APP_WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LEVELEDITOR));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LEVELEDITOR);
    wcex.lpszClassName = APP_szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL APP_InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // Store instance handle in our global variable
    APP_hInst = hInstance;

    // window centre position
    int tmp_posX = abs((GetSystemMetrics(SM_CXSCREEN) - APP_WIN_SIZEX)) / 2;
    int tmp_posY = abs((GetSystemMetrics(SM_CYSCREEN) - APP_WIN_SIZEY)) / 2;

    APP_hWnd = CreateWindowEx(  WS_EX_APPWINDOW, APP_szWindowClass, APP_szTitle, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            tmp_posX, tmp_posY, APP_WIN_SIZEX, APP_WIN_SIZEY,
                            nullptr, nullptr, hInstance, nullptr);

    if (!APP_hWnd)
    {
        return FALSE;
    }

    InitCommonControls();

    ShowWindow(APP_hWnd, nCmdShow);
    UpdateWindow(APP_hWnd);

    return TRUE;
}


LRESULT CALLBACK APP_WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
{
    switch (_message)
    {
        case WM_CREATE:
            APP_Init_Once();
            APP_Reset();
            TEX_Reset();
            TEX_Count_Textures();
            APP_Count_or_Export_Lightmaps(1);
            GUI_Create(_hwnd);
            GUI_Reset(_hwnd);
            Map_Reset();
            Map_Update_Colors();
            break;

        case WM_COMMAND:
        {
            int wm_id = LOWORD(_wParam);
            int wm_event = HIWORD(_wParam);

            // all combos and lists
            switch (wm_event)
            {
                case CBN_SELCHANGE:
                    switch (wm_id)
                    {
                        case IDC_LAYERS_COMBO:
                            gui_current_layer = SendMessage((HWND)_lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            gui_current_list_index = 0;

                            GUI_Update_List_Box(_hwnd);
                            GUI_Update_Properties(_hwnd);
                            GUI_Update_Add_Mode_Button(_hwnd);
                            Map_Update_Colors();
                            break;

                        case IDL_LIST_LIST:
                            gui_current_list_index = SendMessage((HWND)_lParam, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                            GUI_Update_Properties(_hwnd);
                            GUI_Update_Add_Mode_Button(_hwnd);
                            break;
                    }
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;
            }

            switch (wm_id)
            {
                case IDB_MAP_FIT:
                    f_offset_x = 0.0f;
                    f_offset_y = 0.0f;
                    f_scale_x = 15.0f;
                    f_scale_y = 15.0f;
                    SetFocus(_hwnd);
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;

                case IDB_RES_REFRESH:
                    APP_Count_or_Export_Lightmaps(1);
                    TEX_Count_Textures();
                    GUI_Update_Resources(_hwnd);
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;

                case IDB_ADD_ADDTOMAP:
                    gui_add_mode = true;
                    GUI_Update_Add_Mode_Button(_hwnd);
                    break;


                // --- properties ---
 

                    // player
                    case IDB_PROP_PLAYER_RIGHT:     
                        APP_level_file.player_starting_angle = 0;
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;

                    case IDB_PROP_PLAYER_UP:    
                        APP_level_file.player_starting_angle = 30;  // for 90*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;

                    case IDB_PROP_PLAYER_LEFT: 
                        APP_level_file.player_starting_angle = 60; // for 180*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;

                    case IDB_PROP_PLAYER_DOWN:      
                        APP_level_file.player_starting_angle = 90; // for 270*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;
                        
                    // select top, right, bottom, left texture
                    case IDB_PROP_WALL_IMAGE_TOP_SELECT:
                    case IDB_PROP_WALL_IMAGE_RIGHT_SELECT:
                    case IDB_PROP_WALL_IMAGE_BOTTOM_SELECT:
                    case IDB_PROP_WALL_IMAGE_LEFT_SELECT:

                        gui_selected_image = wm_id;
                        DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_SELECT_TEXTURE), _hwnd, APP_Wnd_Select_Texture_Proc);

                        TEX_Count_Textures();
                        GUI_Update_Resources(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;

                // --- menu ---

                    case ID_FILE_NEW:
                        // -- reset --
                        APP_Reset();
                        TEX_Reset();
                        TEX_Count_Textures();
                        APP_Count_or_Export_Lightmaps(1);
                        GUI_Reset(_hwnd);
                        Map_Reset();

                        // -- update --
                        GUI_Update_List_Box(_hwnd);
                        GUI_Update_Properties(_hwnd);
                        GUI_Update_Add_Mode_Button(_hwnd);
                        Map_Update_Colors();
                        break;

                    case ID_FILE_OPEN:
                        if (APP_Open())
                        {
                            TEX_Count_Textures();
                            APP_Count_or_Export_Lightmaps(1);
                            GUI_Reset(_hwnd);

                            GUI_Update_List_Box(_hwnd);
                            GUI_Update_Properties(_hwnd);
                            GUI_Update_Add_Mode_Button(_hwnd);
                            Map_Update_Colors();
                        }
                        break;

                    case ID_FILE_SAVE:
                        APP_Save();
                        break;

                    case ID_FILE_SAVEAS:
                        APP_Save_As();
                        break;

                    case ID_FILE_EXPORT3D_LM:
                        APP_Count_or_Export_Lightmaps(0);
                        break;

                    case ID_FILE_EXIT:
                        APP_Reset();
                        TEX_Reset();
                        DestroyWindow(_hwnd);
                        break;

                    case ID_TOOLS_CONV_TEX:
                        DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_CONVERT), _hwnd, APP_Wnd_Convert_Tex_Proc);
                        break;

                   case ID_TOOLS_VIEW_TEX:
                        DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_VIEW_TEXTURE), _hwnd, APP_Wnd_View_Texture_Proc);
                        break;                

                   case ID_TOOLS_CONV_LIGHTMAP:
                       DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_CONVERT_LIGHTMAP), _hwnd, APP_Wnd_Convert_Lightmap_Proc);
                       break;

                   case ID_HELP_CONTROLS:
                        DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_CONTROLS), _hwnd, APP_Wnd_Help_Controls_Proc);
                        break;

                   case ID_HELP_LIGHTMAPS:
                       DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_HELP_LIGHTMAPS), _hwnd, APP_Wnd_Help_Lightmaps_Proc);
                       break;

                default:
                    return DefWindowProc(_hwnd, _message, _wParam, _lParam);
            }
        }
        break;

        // ----------------
        // --- keyboard ---
        // ----------------

        case WM_KEYDOWN:
            switch ((UINT)_wParam)
            {
                case VK_CONTROL:
                    APP_control_pressed = true;
                    break;
            }
            break;

         case WM_KEYUP:
         {
             switch ((UINT)_wParam)
             {
                 case VK_CONTROL:
                     APP_control_pressed = false;
                     break;

                 case VK_SPACE:
                     gui_texprew_mode = !gui_texprew_mode;
                     break;

                 case '1':
                     gui_current_layer = LAYERS_MAIN_LAYER;
                     gui_current_list_index = 0;
                     break;

                 case '2':
                     gui_current_layer = LAYERS_FLOOR_LAYER;
                     gui_current_list_index = 0;
                     break;

                 case '3':
                     gui_current_layer = LAYERS_CEIL_LAYER;
                     gui_current_list_index = 0;
                     break;

                 case '4':
                     gui_current_layer = LAYERS_LIGHTMAPS_LAYER;
                     gui_current_list_index = 0;
                     break;
             }

             GUI_Update_List_Box(_hwnd);
             GUI_Update_Properties(_hwnd);
             GUI_Update_Add_Mode_Button(_hwnd);
             Map_Update_Colors();

             RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
         }
         break;

        // -------------
        // --- MOUSE ---
        // -------------

         case WM_MOUSEMOVE:
         {
             GetCursorPos(&APP_mp);
             ScreenToClient(GetDlgItem(_hwnd, IDS_MAP), &APP_mp);

             APP_f_mouse_x = (float)APP_mp.x;
             APP_f_mouse_y = (float)APP_mp.y;

             float mx = 0, my = 0;
             Screen_To_World((int)APP_f_mouse_x, (int)APP_f_mouse_y, mx, my);

             hover_cell_x = (int)mx;
             hover_cell_y = (int)my;
             hover_cell_index = hover_cell_x + hover_cell_y * MAP_LENGTH;

             bool is_map = GUI_is_Map_Region();

             if (is_map)
             {
                 if (APP_is_left_hold)
                 {
                     selected_cell_x = hover_cell_x;
                     selected_cell_y = hover_cell_y;
                     selected_cell_index = hover_cell_index;

                     if (APP_control_pressed)
                     {
                        Map_Clear_Current_Cell();
                     }
                     else
                     {
                         if (gui_add_mode)
                         {
                             Map_Add_Current_Object();
                         }
                     }
                 }

                 if (APP_is_middle_hold)
                 {
                     f_offset_x -= (APP_f_mouse_x - f_start_pan_x) / f_scale_x;
                     f_offset_y -= (APP_f_mouse_y - f_start_pan_y) / f_scale_y;

                     f_start_pan_x = APP_f_mouse_x;
                     f_start_pan_y = APP_f_mouse_y;
                 }
             }

             GUI_Update_Map_Coords(is_map);
             RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
         }
         break;

        case WM_LBUTTONDOWN:
        {
            bool is_map = GUI_is_Map_Region();

            if (is_map)
            {
                APP_is_left_hold = true;

                selected_cell_x = hover_cell_x;
                selected_cell_y = hover_cell_y;
                selected_cell_index = hover_cell_index;

                if (APP_control_pressed)
                {
                    Map_Clear_Current_Cell();

                    TEX_Count_Textures();
                    GUI_Update_Resources(_hwnd);
                }
                else
                {
                    if (gui_add_mode)
                    {
                        // if add to map pressed - we are in ADD TO MAP MODE
                        Map_Add_Current_Object();

                        TEX_Count_Textures();
                        GUI_Update_Resources(_hwnd);
                    }
                    else
                    {
                        // if not pressed we are in SELECTION mode
                        Map_Read_Current_Cell();

                        // update all gui with that info
                        GUI_Update_Layers_Combo(_hwnd);
                        GUI_Update_List_Box(_hwnd);
                        GUI_Update_Properties(_hwnd);
                        GUI_Update_Add_Mode_Button(_hwnd);
                    }
                }

                GUI_Update_Map_Coords(is_map);
                RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            }
        }
        break;

        case WM_LBUTTONUP:
            APP_is_left_hold = false;
            break;

        case WM_MBUTTONDOWN:
            APP_is_middle_hold = true;
            f_start_pan_x = APP_f_mouse_x;
            f_start_pan_y = APP_f_mouse_y;
            break;

        case WM_MBUTTONUP:
            APP_is_middle_hold = false;
            break;

        case WM_RBUTTONUP:
            if (gui_add_mode)
            {
                gui_add_mode = false;
                GUI_Update_Add_Mode_Button(_hwnd);
            }
            break;

        case WM_MOUSEWHEEL:
        {
            float f_mouse_x_before_zoom, f_mouse_y_before_zoom;
            Screen_To_World((int)APP_f_mouse_x, (int)APP_f_mouse_y, f_mouse_x_before_zoom, f_mouse_y_before_zoom);

            if (GET_WHEEL_DELTA_WPARAM(_wParam) > 0)
            {
                f_scale_x *= 1.1f;
                f_scale_y *= 1.1f;
            }
            else
            {
                f_scale_x *= 0.9f;
                f_scale_y *= 0.9f;
            }

            float f_mouse_x_after_zoom, f_mouse_y_after_zoom;
            Screen_To_World((int)APP_f_mouse_x, (int)APP_f_mouse_y, f_mouse_x_after_zoom, f_mouse_y_after_zoom);

            f_offset_x += (f_mouse_x_before_zoom - f_mouse_x_after_zoom);
            f_offset_y += (f_mouse_y_before_zoom - f_mouse_y_after_zoom);

            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
        }
        break;

        case WM_PAINT:
        {
            // --- _hwnd
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);

            // --- h_editor
            PAINTSTRUCT ps_map;
            HDC hdc_map = BeginPaint(GetDlgItem(_hwnd, IDS_MAP), &ps_map);

                HBITMAP hmb_map = CreateCompatibleBitmap(hdc_map, MAP_WINDOW_SIZE, MAP_WINDOW_SIZE);

                HDC hdc_tmp_buffer = CreateCompatibleDC(hdc_map);
                SelectObject(hdc_tmp_buffer, hmb_map);

                Map_Draw_BG(hdc_tmp_buffer);
                Map_Draw_Objects(hdc_tmp_buffer);           
                Map_Draw_Player(hdc_tmp_buffer);
                Map_Draw_Grid(hdc_tmp_buffer);
                Map_Draw_Selected_Cell(hdc_tmp_buffer);
                Map_Draw_Layer_Info(hdc_tmp_buffer);

                BitBlt(hdc_map, map_rc.left, map_rc.top, map_rc.right, map_rc.bottom, hdc_tmp_buffer, 0, 0, SRCCOPY);

                DeleteDC(hdc_map);
                DeleteDC(hdc_tmp_buffer);
                DeleteObject(hmb_map);

            EndPaint(GetDlgItem(_hwnd, IDS_MAP), &ps_map);

            // redraw windows width textures in properites group
            GUI_Draw_Properites_Textures(_hwnd);
        }
        break;

        case WM_DESTROY:
            APP_Reset();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(_hwnd, _message, _wParam, _lParam);

    }
    return 0;
}

INT_PTR CALLBACK APP_Wnd_Help_Controls_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);
    switch (_message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(_wparam) == IDOK)
        {
            EndDialog(_hwnd, LOWORD(_wparam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK APP_Wnd_Help_Lightmaps_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);
    switch (_message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(_wparam) == IDOK)
            {
                EndDialog(_hwnd, LOWORD(_wparam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}


INT_PTR CALLBACK APP_Wnd_Convert_Tex_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:

            // Init Drag & Drop.
            DragAcceptFiles(_hwnd, TRUE);

            ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
            ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
            ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

            // Set positions of preview windows.
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE), HWND_TOP, 340, 10, 508+2, 256+2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), HWND_TOP, 24, 455, 256+2, 256+2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), HWND_TOP, 24 + 256 + 10, 450, 128+2, 128+2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), HWND_TOP, 24 + 256 + 10+128+10, 450, 128+2, 128+2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), HWND_TOP, 24 + 256 + 10 + 128 + 10+128+10, 450, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), HWND_TOP, 24 + 256 + 10 + 128 + 10 + 128+10+128+10, 450, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), HWND_TOP, 24 + 256 + 10, 450 + 128 + 10,128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), HWND_TOP, 24 + 256 + 10+128+10, 450 +128+10, 128 + 2, 128 + 2, SWP_SHOWWINDOW);

            // Init combo.
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"01. No light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"02. Solid light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"03. Dimmed light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

            // Init slider.
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, IO_TEXTURE_MAX_SHADES-1));
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETPOS, TRUE, IO_TEXTURE_MAX_SHADES-1);

            // Reset current visible filename to -none-
            TCHAR output[128];
            ZeroMemory(&output, sizeof(output));
            swprintf_s(output, L"%s", L"- none -");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

            // Init other windows.
            EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_WALL), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_FLAT), FALSE);

            EnableWindow(GetDlgItem(_hwnd, IDC_DC_INTENSITY), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_SLIDER), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LEFT), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LABEL), FALSE);

            EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO_LABEL), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), FALSE);

            EnableWindow(GetDlgItem(_hwnd, IDC_DC_MODE), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_TRESHOLD), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_TRESHOLD_LABEL), FALSE);

            EnableWindow(GetDlgItem(_hwnd, IDC_DC_OUTPUT), FALSE);

            // Set init raw info.
            SetWindowText(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), L"---");

            // Set init light threshold.
            SetWindowText(GetDlgItem(_hwnd, IDC_DC_TRESHOLD), L"240");

            TCHAR output2[4];
            ZeroMemory(&output2, sizeof(output2));
            swprintf_s(output2, L"%ld", IO_TEXTURE_MAX_SHADES - 1);
            SendMessage(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), WM_SETTEXT, 0, (LPARAM)output2);
            SendMessage(GetDlgItem(_hwnd, IDC_DC_INTENSITY), WM_SETTEXT, 0, (LPARAM)output2);

            // Init other parameters.
            ZeroMemory(&cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));

            cnv_current_light_mode = 0;
            cnv_current_light_treshold = 240;

            cnv_is_input_loaded = false;
            cnv_is_raw_loaded = false;            

            // Alloc memory for raw texture when loaded.
            cnv_raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            cnv_raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            return (INT_PTR)TRUE;


        case WM_DROPFILES:

            // Get dropped TGA filename.
            // Read in that TGA.
            // Convert to Windows bitmap to display in window.
            // Release TGA.
            // Update GUI.

            HDROP file_drop;
            file_drop = (HDROP)_wparam;

            TCHAR file_name[MAX_PATH];
            ZeroMemory(&file_name, sizeof(file_name));

            if (DragQueryFile(file_drop, 0xFFFFFFFF, NULL, MAX_PATH) != 1)
            {
                MessageBox(_hwnd, L"Drag & drop problem.", L"Error.", MB_ICONWARNING | MB_OK);
            }
            else
            {
                DragQueryFile(file_drop, 0, file_name, MAX_PATH);

                // convert TCHAR path_filename to CHAR path_filename
                ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                wcstombs_s(NULL, cnv_input_bitmap_filename, file_name, wcslen(file_name) + 1);

                // Lets read TGA textue with BGRA color order
                // - it will be visible correctly in preview window because of Windows Api format
                // and we could also use memcpy for fast color copying.
                sIO_Prefs prefs;
                prefs.ch1 = 16;
                prefs.ch2 = 8;
                prefs.ch3 = 0;

                // read in texture and keep it
                int tga_result = BM_Read_Bitmap_Indexed_TGA(cnv_input_bitmap_filename, &cnv_input_bitmap_BGRA, &prefs);

                if ((cnv_input_bitmap_BGRA.width != 508) || (cnv_input_bitmap_BGRA.height != 256))
                    tga_result = -5;

                if (cnv_input_bitmap_BGRA.num_colors > IO_TEXTURE_MAX_COLORS)
                    tga_result = -6;

                switch (tga_result)
                {
                    case 0:
                        MessageBox(_hwnd, L"File not found.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -1:
                        MessageBox(_hwnd, L"TGA is not indexed bitmap.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -2:
                        MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -3:
                        MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -4:
                        MessageBox(_hwnd, L"Can't read TGA format.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -5:
                        MessageBox(_hwnd, L"The input TGA bitmap should be 508 x 256 pixels.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -6:
                        MessageBox(_hwnd, L"The number of colors in TGA bitmap differs from IO_TEXTURES_MAX_COLORS.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;
                }

                if (tga_result > 0)
                {
                    cnv_hbm_texture = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, cnv_input_bitmap_BGRA.width, cnv_input_bitmap_BGRA.height, cnv_input_bitmap_BGRA.num_colors,
                                                                                  cnv_input_bitmap_BGRA.color_data, cnv_input_bitmap_BGRA.image_data);

                    TCHAR output[256];
                    swprintf(output, 256, L"%hs", cnv_input_bitmap_filename);

                    SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

                    EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_WALL), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_FLAT), TRUE);

                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_MODE), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_TRESHOLD), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDS_DC_TRESHOLD_LABEL), TRUE);

                    cnv_is_input_loaded = true;
                }

                // Not needed anymore.
                BM_Free_Bitmap_Indexed(&cnv_input_bitmap_BGRA);
            }

            DragFinish(file_drop);
            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            break;


        case WM_COMMAND:
        {
            int wm_id = LOWORD(_wparam);
            int wm_event = HIWORD(_wparam);

            // all combos and lists
            switch (wm_event)
            {
                case CBN_SELCHANGE:
                    switch (wm_id)
                    {
                        case IDC_DC_MODE:
                            cnv_current_light_mode = SendMessage((HWND)_lparam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            break;
                    }
                    break;
            }

            switch (wm_id)
            {
                case IDB_DC_CONVERT_WALL:
                case IDB_DC_CONVERT_FLAT:
                {
                    // read again in texture and keep it
                    // read in TGA as RGBA - its gonna be saved that way - so on Amiga with RGBA mode - no conversion will be need.
                    sIO_Prefs prefs;
                    prefs.ch1 = 24;
                    prefs.ch2 = 16;
                    prefs.ch3 = 8;

                    int rgba_read_result = BM_Read_Bitmap_Indexed_TGA(cnv_input_bitmap_filename, &cnv_input_bitmap_RGBA, &prefs);

                    switch (rgba_read_result)
                    {
                        case 0:
                            MessageBox(_hwnd, L"File not found.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -1:
                            MessageBox(_hwnd, L"TGA is not indexed bitmap.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -2:
                            MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -3:
                            MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -4:
                            MessageBox(_hwnd, L"Can't read TGA format.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;
                    }
                    
                    if (rgba_read_result > 0)
                    {
                        // Wall or Flat texture?
                        int texture_type = 0;

                        if (wm_id == IDB_DC_CONVERT_WALL)   texture_type = 0;
                        else                                texture_type = 1;


                        // Create output filename by getting filename and adding proper texture extension 
                        // according to selected option: wall or flat texture. Also with path.
                        char output_filename[256];
                        char fname[32];

                        ZeroMemory(output_filename, sizeof(output_filename));
                        ZeroMemory(fname, sizeof(fname));

                        _splitpath(cnv_input_bitmap_filename, NULL, NULL, fname, NULL);
                        
                        if (texture_type == 0)  strcat(output_filename, APP_WALL_TEXTURES_DIRECTORY);
                        else                    strcat(output_filename, APP_FLAT_TEXTURES_DIRECTORY);
                       
                        strcat(output_filename, "\\");
                        strcat(output_filename, fname);

                        if (texture_type == 0)  strcat(output_filename, IO_WALL_TEXTURE_FILE_EXTENSION);
                        else                    strcat(output_filename, IO_FLAT_TEXTURE_FILE_EXTENSION);


                        // Check if light threshold parameter is OK.
                        wchar_t output[4];
                        ZeroMemory(output, sizeof(output));

                        GetWindowText(GetDlgItem(_hwnd, IDC_DC_TRESHOLD), output, 4);
                        cnv_current_light_treshold = _wtoi(output);

                        if (cnv_current_light_treshold < 0) cnv_current_light_treshold = 0;
                        if (cnv_current_light_treshold > 255) cnv_current_light_treshold = 255;


                        // Convert and save to RAW format.
                        int raw_result = 0;

                        raw_result = BM_Convert_And_Save_Bitmap_Indexed_To_Texture_RAW(  output_filename, &cnv_input_bitmap_RGBA, texture_type,
                                                                                         cnv_current_light_mode, cnv_current_light_treshold  );

                        
                        // We can free RGBA TGA now because it is not needed anymore.
                        BM_Free_Bitmap_Indexed(&cnv_input_bitmap_RGBA);


                        // Check the result and give message.
                        switch (raw_result)
                        {
                            case 0:
                                MessageBox(_hwnd, L"Can't create output file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;

                            case -2:
                                MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;

                            case -3:
                                MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;
                        }

                        if (raw_result > 0)
                        {
                            wchar_t info[512];
                            ZeroMemory(info, sizeof(info));

                            if (texture_type == 0)  swprintf_s(info, 512, L"Bitmap converted to WALL TEXTURE and saved as: %hs", output_filename);
                            else                    swprintf_s(info, 512, L"Bitmap converted to FLAT TEXTURE and saved as: %hs", output_filename);

                            EnableWindow(GetDlgItem(_hwnd, IDC_DC_OUTPUT), TRUE);
                            SetWindowText(GetDlgItem(_hwnd, IDC_DC_OUTPUT), info);
                           
                            sIO_Prefs prefs;
                            prefs.ch1 = 8;
                            prefs.ch2 = 16;
                            prefs.ch3 = 24;

                            int read_raw_result = BM_Read_Texture_RAW(output_filename, cnv_raw_table, cnv_raw_index, &prefs);

                            if (read_raw_result <= 0)
                                MessageBox(_hwnd, L"Can't open converted file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                            else
                            {
                                LRESULT slider_pos = SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_GETPOS, 0, 0);

                                wchar_t output[4];
                                wsprintf(output, L"%ld", slider_pos);

                                SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

                                int intensity = _wtoi(output);
                                int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;
                 
                                cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 256, 256, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                                cnv_hbm_texture_128 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256);
                                cnv_hbm_texture_64 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128);
                                cnv_hbm_texture_32 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64);
                                cnv_hbm_texture_16 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 16, 16, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
                                cnv_hbm_texture_8 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 8, 8, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
                                cnv_hbm_texture_4 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 4, 4, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);
                                
                                EnableWindow(GetDlgItem(_hwnd, IDC_DC_INTENSITY), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDC_DC_SLIDER), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LEFT), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LABEL), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO_LABEL), TRUE);
                                EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), TRUE);

                                // Print the raw file size.
                                wchar_t output2[20];
                                wchar_t output3[16];

                                ZeroMemory(output2, sizeof(output2));
                                ZeroMemory(output3, sizeof(output3));

                                Add_Number_Spaces(read_raw_result, output3);
                                swprintf_s(output2, L" %s  Bytes", output3);

                                SetWindowText(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), output2);

                                // Set that raw file is loaded.
                                cnv_is_raw_loaded = true;
                            }
                        }
                    }

                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                }
                break;
            }
        }
        break;

        case WM_HSCROLL:
        {
            LRESULT slider_pos = SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_GETPOS, 0, 0);

            wchar_t output[4];
            wsprintf(output, L"%ld", slider_pos);

            SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

            int intensity = _wtoi(output);
            int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;

            if (cnv_is_raw_loaded)
            {
                cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 256, 256, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                cnv_hbm_texture_128 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256);
                cnv_hbm_texture_64 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128);
                cnv_hbm_texture_32 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64);
                cnv_hbm_texture_16 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 16, 16, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
                cnv_hbm_texture_8 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 8, 8, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
                cnv_hbm_texture_4 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 4, 4, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);
                
                RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);                    
            EndPaint(_hwnd, &ps);

            // display input texture
            if (cnv_is_input_loaded)
            {
                PAINTSTRUCT ps_texture;
                HDC hdc_texture = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE), &ps_texture);

                    HDC hdc_tmp = CreateCompatibleDC(hdc_texture);
                    SelectObject(hdc_tmp, cnv_hbm_texture);
                    BitBlt(hdc_texture, 0, 0, 508, 256, hdc_tmp, 0, 0, SRCCOPY);
                    DeleteObject(cnv_hbm_texture);
                    DeleteDC(hdc_tmp);

                EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE), &ps_texture);
            }
            
            // display 256 preview
            PAINTSTRUCT ps_texture_256;
            HDC hdc_texture_256 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

                HDC hdc_tmp_256 = CreateCompatibleDC(hdc_texture_256);
                SelectObject(hdc_tmp_256, cnv_hbm_texture_256);
                BitBlt(hdc_texture_256, 0, 0, 256, 256, hdc_tmp_256, 0, 0, SRCCOPY);
                DeleteObject(cnv_hbm_texture_256);
                DeleteDC(hdc_tmp_256);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

            // display 128 preview
            PAINTSTRUCT ps_texture_128;
            HDC hdc_texture_128 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

                HDC hdc_tmp_128 = CreateCompatibleDC(hdc_texture_128);
                SelectObject(hdc_tmp_128, cnv_hbm_texture_128);
                BitBlt(hdc_texture_128, 0, 0, 128, 128, hdc_tmp_128, 0, 0, SRCCOPY);
                DeleteObject(cnv_hbm_texture_128);
                DeleteDC(hdc_tmp_128);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

            // display 64 preview
            PAINTSTRUCT ps_texture_64;
            HDC hdc_texture_64 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

                HDC hdc_tmp_64 = CreateCompatibleDC(hdc_texture_64);
                SelectObject(hdc_tmp_64, cnv_hbm_texture_64);
                StretchBlt(hdc_texture_64, 0, 0, 128, 128, hdc_tmp_64, 0, 0, 64, 64, SRCCOPY);
                DeleteObject(cnv_hbm_texture_64);
                DeleteDC(hdc_tmp_64);                

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

            // display 32 preview
            PAINTSTRUCT ps_texture_32;
            HDC hdc_texture_32 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), &ps_texture_32);

                HDC hdc_tmp_32 = CreateCompatibleDC(hdc_texture_32);
                SelectObject(hdc_tmp_32, cnv_hbm_texture_32);
                StretchBlt(hdc_texture_32, 0, 0, 128, 128, hdc_tmp_32, 0, 0, 32, 32, SRCCOPY);
                DeleteObject(cnv_hbm_texture_32);
                DeleteDC(hdc_tmp_32);        

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), &ps_texture_32);

            // display 16 preview
            PAINTSTRUCT ps_texture_16;
            HDC hdc_texture_16 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), &ps_texture_16);

                HDC hdc_tmp_16 = CreateCompatibleDC(hdc_texture_16);
                SelectObject(hdc_tmp_16, cnv_hbm_texture_16);
                StretchBlt(hdc_texture_16, 0, 0, 128, 128, hdc_tmp_16, 0, 0, 16, 16, SRCCOPY);
                DeleteObject(cnv_hbm_texture_16);
                DeleteDC(hdc_tmp_16);                

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), &ps_texture_16);

            // display 8 preview
            PAINTSTRUCT ps_texture_8;
            HDC hdc_texture_8 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), &ps_texture_8);

                HDC hdc_tmp_8 = CreateCompatibleDC(hdc_texture_8);
                SelectObject(hdc_tmp_8, cnv_hbm_texture_8);
                StretchBlt(hdc_texture_8, 0, 0, 128, 128, hdc_tmp_8, 0, 0, 8, 8, SRCCOPY);
                DeleteObject(cnv_hbm_texture_8);
                DeleteDC(hdc_tmp_8);                

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), &ps_texture_8);

            // display 4 preview
            PAINTSTRUCT ps_texture_4;
            HDC hdc_texture_4 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), &ps_texture_4);

                HDC hdc_tmp_4 = CreateCompatibleDC(hdc_texture_4);
                SelectObject(hdc_tmp_4, cnv_hbm_texture_4);
                StretchBlt(hdc_texture_4, 0, 0, 128, 128, hdc_tmp_4, 0, 0, 4, 4, SRCCOPY);
                DeleteObject(cnv_hbm_texture_4);
                DeleteDC(hdc_tmp_4);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), &ps_texture_4);

            return (INT_PTR)TRUE;
        }

        case WM_CLOSE:
            free(cnv_raw_table);
            free(cnv_raw_index);

            EndDialog(_hwnd, LOWORD(_wparam));
            return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK APP_Wnd_View_Texture_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:

            // Init Drag & Drop.
            DragAcceptFiles(_hwnd, TRUE);

            ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
            ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
            ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

            // Set positions of preview windows.
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), HWND_TOP, 24, 115, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), HWND_TOP, 24 + 256 + 10, 110, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), HWND_TOP, 24 + 256 + 10 + 128 + 10, 110, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), HWND_TOP, 24 + 256 + 10 + 128 + 10 + 128 + 10, 110, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), HWND_TOP, 24 + 256 + 10 + 128 + 10 + 128 + 10 + 128 + 10, 110, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), HWND_TOP, 24 + 256 + 10, 110 + 128 + 10, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), HWND_TOP, 24 + 256 + 10 + 128 + 10, 110 + 128 + 10, 128 + 2, 128 + 2, SWP_SHOWWINDOW);

            // Init slider.
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, IO_TEXTURE_MAX_SHADES - 1));
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETPOS, TRUE, IO_TEXTURE_MAX_SHADES - 1);

            // Reset current visible filename to -none-
            TCHAR output[128];
            ZeroMemory(&output, sizeof(output));
            swprintf_s(output, L"%s", L"--- Drag & Drope texture file ---");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

            // Init other windows.
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_INTENSITY), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_SLIDER), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LEFT), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LABEL), FALSE);

            EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO_LABEL), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), FALSE);

            // Set init raw info.
            SetWindowText(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), L"---");

            TCHAR output2[4];
            ZeroMemory(&output2, sizeof(output2));
            swprintf_s(output2, L"%ld", IO_TEXTURE_MAX_SHADES - 1);
            SendMessage(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), WM_SETTEXT, 0, (LPARAM)output2);
            SendMessage(GetDlgItem(_hwnd, IDC_DC_INTENSITY), WM_SETTEXT, 0, (LPARAM)output2);

            // Init other parameters.
            ZeroMemory(&cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));

            cnv_is_raw_loaded = false;

            // Alloc memory for raw texture when loaded.
            cnv_raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            cnv_raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            return (INT_PTR)TRUE;


        case WM_DROPFILES:
            HDROP file_drop;
            file_drop = (HDROP)_wparam;

            TCHAR file_name[MAX_PATH];
            ZeroMemory(&file_name, sizeof(file_name));

            if (DragQueryFile(file_drop, 0xFFFFFFFF, NULL, MAX_PATH) != 1)
            {
                MessageBox(_hwnd, L"Drag & drop problem.", L"Error.", MB_ICONWARNING | MB_OK);
            }
            else
            {
                DragQueryFile(file_drop, 0, file_name, MAX_PATH);

                SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)file_name);

                // convert TCHAR path_filename to CHAR path_filename
                ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                wcstombs_s(NULL, cnv_input_bitmap_filename, file_name, wcslen(file_name) + 1);

                sIO_Prefs prefs;
                prefs.ch1 = 16;
                prefs.ch2 = 8;
                prefs.ch3 = 0;

                int read_raw_result = BM_Read_Texture_RAW(cnv_input_bitmap_filename, cnv_raw_table, cnv_raw_index, &prefs);

                if (read_raw_result <= 0)
                    MessageBox(_hwnd, L"Can't open converted file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                else
                {
                    LRESULT slider_pos = SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_GETPOS, 0, 0);

                    wchar_t output[4];
                    wsprintf(output, L"%ld", slider_pos);

                    SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

                    int intensity = _wtoi(output);
                    int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;

                    cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 256, 256, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                    cnv_hbm_texture_128 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256);
                    cnv_hbm_texture_64 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128);
                    cnv_hbm_texture_32 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64);
                    cnv_hbm_texture_16 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 16, 16, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
                    cnv_hbm_texture_8 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 8, 8, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
                    cnv_hbm_texture_4 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 4, 4, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);

                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_INTENSITY), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_SLIDER), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LEFT), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDS_DC_SLIDER_LABEL), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO_LABEL), TRUE);
                    EnableWindow(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), TRUE);

                    // Print the raw file size.
                    wchar_t output2[20];
                    wchar_t output3[16];

                    ZeroMemory(output2, sizeof(output2));
                    ZeroMemory(output3, sizeof(output3));

                    Add_Number_Spaces(read_raw_result, output3);
                    swprintf_s(output2, L" %s  Bytes", output3);

                    SetWindowText(GetDlgItem(_hwnd, IDC_DC_RAW_INFO), output2);

                    // Set that raw file is loaded.
                    cnv_is_raw_loaded = true;
                }
            }
                         
            DragFinish(file_drop);
            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            break;


        case WM_HSCROLL:
        {
            LRESULT slider_pos = SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_GETPOS, 0, 0);

            wchar_t output[4];
            wsprintf(output, L"%ld", slider_pos);

            SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

            int intensity = _wtoi(output);
            int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;

            if (cnv_is_raw_loaded)
            {
                cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 256, 256, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                cnv_hbm_texture_128 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256);
                cnv_hbm_texture_64 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128);
                cnv_hbm_texture_32 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64);
                cnv_hbm_texture_16 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 16, 16, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
                cnv_hbm_texture_8 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 8, 8, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
                cnv_hbm_texture_4 = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 4, 4, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);

                RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            }
        }
        break;


        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);

            // display 256 preview
            PAINTSTRUCT ps_texture_256;
            HDC hdc_texture_256 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

            HDC hdc_tmp_256 = CreateCompatibleDC(hdc_texture_256);
            SelectObject(hdc_tmp_256, cnv_hbm_texture_256);
            BitBlt(hdc_texture_256, 0, 0, 256, 256, hdc_tmp_256, 0, 0, SRCCOPY);
            DeleteObject(cnv_hbm_texture_256);
            DeleteDC(hdc_tmp_256);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

            // display 128 preview
            PAINTSTRUCT ps_texture_128;
            HDC hdc_texture_128 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

            HDC hdc_tmp_128 = CreateCompatibleDC(hdc_texture_128);
            SelectObject(hdc_tmp_128, cnv_hbm_texture_128);
            BitBlt(hdc_texture_128, 0, 0, 128, 128, hdc_tmp_128, 0, 0, SRCCOPY);
            DeleteObject(cnv_hbm_texture_128);
            DeleteDC(hdc_tmp_128);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

            // display 64 preview
            PAINTSTRUCT ps_texture_64;
            HDC hdc_texture_64 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

            HDC hdc_tmp_64 = CreateCompatibleDC(hdc_texture_64);
            SelectObject(hdc_tmp_64, cnv_hbm_texture_64);
            StretchBlt(hdc_texture_64, 0, 0, 128, 128, hdc_tmp_64, 0, 0, 64, 64, SRCCOPY);
            DeleteObject(cnv_hbm_texture_64);
            DeleteDC(hdc_tmp_64);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

            // display 32 preview
            PAINTSTRUCT ps_texture_32;
            HDC hdc_texture_32 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), &ps_texture_32);

            HDC hdc_tmp_32 = CreateCompatibleDC(hdc_texture_32);
            SelectObject(hdc_tmp_32, cnv_hbm_texture_32);
            StretchBlt(hdc_texture_32, 0, 0, 128, 128, hdc_tmp_32, 0, 0, 32, 32, SRCCOPY);
            DeleteObject(cnv_hbm_texture_32);
            DeleteDC(hdc_tmp_32);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_32), &ps_texture_32);

            // display 16 preview
            PAINTSTRUCT ps_texture_16;
            HDC hdc_texture_16 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), &ps_texture_16);

            HDC hdc_tmp_16 = CreateCompatibleDC(hdc_texture_16);
            SelectObject(hdc_tmp_16, cnv_hbm_texture_16);
            StretchBlt(hdc_texture_16, 0, 0, 128, 128, hdc_tmp_16, 0, 0, 16, 16, SRCCOPY);
            DeleteObject(cnv_hbm_texture_16);
            DeleteDC(hdc_tmp_16);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_16), &ps_texture_16);

            // display 8 preview
            PAINTSTRUCT ps_texture_8;
            HDC hdc_texture_8 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), &ps_texture_8);

            HDC hdc_tmp_8 = CreateCompatibleDC(hdc_texture_8);
            SelectObject(hdc_tmp_8, cnv_hbm_texture_8);
            StretchBlt(hdc_texture_8, 0, 0, 128, 128, hdc_tmp_8, 0, 0, 8, 8, SRCCOPY);
            DeleteObject(cnv_hbm_texture_8);
            DeleteDC(hdc_tmp_8);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_8), &ps_texture_8);

            // display 4 preview
            PAINTSTRUCT ps_texture_4;
            HDC hdc_texture_4 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), &ps_texture_4);

            HDC hdc_tmp_4 = CreateCompatibleDC(hdc_texture_4);
            SelectObject(hdc_tmp_4, cnv_hbm_texture_4);
            StretchBlt(hdc_texture_4, 0, 0, 128, 128, hdc_tmp_4, 0, 0, 4, 4, SRCCOPY);
            DeleteObject(cnv_hbm_texture_4);
            DeleteDC(hdc_tmp_4);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_4), &ps_texture_4);

            return (INT_PTR)TRUE;
        }


        case WM_CLOSE:
            free(cnv_raw_table);
            free(cnv_raw_index);

            EndDialog(_hwnd, LOWORD(_wparam));
            return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK APP_Wnd_Convert_Lightmap_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:

            if (APP_never_saved)
            {
                MessageBox(_hwnd, L"You should save the thevel first. The converted lightmap file will have the same name as level file.", L"Error.", MB_ICONWARNING | MB_OK);

                EndDialog(_hwnd, LOWORD(_wparam));
                return (INT_PTR)TRUE;
            }

            APP_Count_or_Export_Lightmaps(1);

            if (APP_level_file.wall_lightmaps_count == 0 || APP_level_file.flat_lightmaps_count == 0)
            {
                MessageBox(_hwnd, L"You should add some LIGHTMAPS first...", L"Error.", MB_ICONWARNING | MB_OK);

                EndDialog(_hwnd, LOWORD(_wparam));
                return (INT_PTR)TRUE;
            }

            // Init Drag & Drop.
            DragAcceptFiles(_hwnd, TRUE);

            ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
            ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
            ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

            // Set positions of preview windows.
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE), HWND_TOP, 570, 10, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), HWND_TOP, 24, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), HWND_TOP, 24 + 256 + 10, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), HWND_TOP, 24 + 256 + 10 + 256 + 10, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);

            // Reset current visible filename to -none-
            TCHAR output[128];
            ZeroMemory(&output, sizeof(output));
            swprintf_s(output, L"%s", L"- none -");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

            // Init other windows.
            EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_LIGHTMAP), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_OUTPUT), FALSE);

            // Init other parameters.
            ZeroMemory(&cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));

            cnv_is_input_loaded = false;
            cnv_is_raw_loaded = false;

            // Alloc memory for raw texture when loaded.
            cnv_raw_lm_wall = (u_int8*)malloc(IO_LIGHTMAP_BYTE_SIZE * APP_level_file.wall_lightmaps_count);
            cnv_raw_lm_floor = (u_int8*)malloc(IO_LIGHTMAP_BYTE_SIZE * (APP_level_file.flat_lightmaps_count / 2));
            cnv_raw_lm_ceil = (u_int8*)malloc(IO_LIGHTMAP_BYTE_SIZE * (APP_level_file.flat_lightmaps_count / 2));

            return (INT_PTR)TRUE;


        case WM_DROPFILES:

            // Get dropped TGA filename.
            // Read in that TGA.
            // Convert to Windows bitmap to display in window.
            // Release TGA.
            // Update GUI.

            HDROP file_drop;
            file_drop = (HDROP)_wparam;

            TCHAR file_name[MAX_PATH];
            ZeroMemory(&file_name, sizeof(file_name));

            if (DragQueryFile(file_drop, 0xFFFFFFFF, NULL, MAX_PATH) != 1)
            {
                MessageBox(_hwnd, L"Drag & drop problem.", L"Error.", MB_ICONWARNING | MB_OK);
            }
            else
            {
                DragQueryFile(file_drop, 0, file_name, MAX_PATH);

                // convert TCHAR path_filename to CHAR path_filename
                ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                wcstombs_s(NULL, cnv_input_bitmap_filename, file_name, wcslen(file_name) + 1);

                // Lets read TGA textue with BGRA color order
                // - it will be visible correctly in preview window because of Windows Api format
                // and we could also use memcpy for fast color copying.
                sIO_Prefs prefs;
                prefs.ch1 = 16;
                prefs.ch2 = 8;
                prefs.ch3 = 0;

                // read in texture and keep it
                int tga_result = BM_Read_Bitmap_Indexed_TGA(cnv_input_bitmap_filename, &cnv_input_bitmap_BGRA, &prefs);

                if ((cnv_input_bitmap_BGRA.width != 2048) || (cnv_input_bitmap_BGRA.height != 2048))
                    tga_result = -5;

                switch (tga_result)
                {
                    case 0:
                        MessageBox(_hwnd, L"File not found.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -1:
                        MessageBox(_hwnd, L"TGA is not indexed bitmap.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -2:
                        MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -3:
                        MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -4:
                        MessageBox(_hwnd, L"Can't read TGA format.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;

                    case -5:
                        MessageBox(_hwnd, L"The input TGA bitmap should be 2048 x 2048 pixels.", L"Error.", MB_ICONWARNING | MB_OK);
                        break;
                }

                if (tga_result > 0)
                {
                    cnv_hbm_texture = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, cnv_input_bitmap_BGRA.width, cnv_input_bitmap_BGRA.height, cnv_input_bitmap_BGRA.num_colors,
                                                                        cnv_input_bitmap_BGRA.color_data, cnv_input_bitmap_BGRA.image_data);

                    TCHAR output[256];
                    swprintf(output, 256, L"%hs", cnv_input_bitmap_filename);

                    SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

                    EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_LIGHTMAP), TRUE);

                    cnv_is_input_loaded = true;
                }

                // Not needed anymore.
                BM_Free_Bitmap_Indexed(&cnv_input_bitmap_BGRA);
            }

            DragFinish(file_drop);
            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            break;


        case WM_COMMAND:
        {
            int wm_id = LOWORD(_wparam);
            int wm_event = HIWORD(_wparam);

            switch (wm_id)
            {
                case IDB_DC_CONVERT_LIGHTMAP:
                {
                    // read again in texture and keep it
                    // read in TGA as RGBA - its gonna be saved that way - so on Amiga with RGBA mode - no conversion will be need.
                    sIO_Prefs prefs;
                    prefs.ch1 = 24;
                    prefs.ch2 = 16;
                    prefs.ch3 = 8;

                    int rgba_read_result = BM_Read_Bitmap_Indexed_TGA(cnv_input_bitmap_filename, &cnv_input_bitmap_RGBA, &prefs);

                    switch (rgba_read_result)
                    {
                        case 0:
                            MessageBox(_hwnd, L"File not found.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -1:
                            MessageBox(_hwnd, L"TGA is not indexed bitmap.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -2:
                            MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -3:
                            MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;

                        case -4:
                            MessageBox(_hwnd, L"Can't read TGA format.", L"Error.", MB_ICONWARNING | MB_OK);
                            break;
                    }

                    if (rgba_read_result > 0)
                    {
                        // Create output filename by getting filename and adding proper lightmap 
                        char output_filename[256];
                        ZeroMemory(output_filename, sizeof(output_filename));

                        strcat(output_filename, APP_LIGHTMAPS_DIRECTORY);

                        strcat(output_filename, "\\");

                        // convert TCHAR to CHAR and split path to get filename
                        char fname[32], app_filename_char[260];

                        ZeroMemory(fname, sizeof(fname));
                        ZeroMemory(app_filename_char, sizeof(app_filename_char));

                        wcstombs_s(NULL, app_filename_char, APP_filename, wcslen(APP_filename) + 1);
                        _splitpath(app_filename_char, NULL, NULL, fname, NULL);   

                        strcat(output_filename, fname);
                        strcat(output_filename, IO_LIGHTMAP_FILE_EXTENSION);

                        // Convert and save to RAW format.
                        int raw_result = 0;

                        raw_result = BM_Convert_And_Save_Bitmap_Indexed_To_Lightmap_RAW(output_filename, &cnv_input_bitmap_RGBA,
                                                                                        APP_level_file.wall_lightmaps_count, APP_level_file.flat_lightmaps_count);


                        // We can free RGBA TGA now because it is not needed anymore.
                        BM_Free_Bitmap_Indexed(&cnv_input_bitmap_RGBA);


                        // Check the result and give message.
                        switch (raw_result)
                        {
                            case 0:
                                MessageBox(_hwnd, L"Can't create output file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;

                            case -2:
                                MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;

                            case -3:
                                MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
                                break;
                        }

                        if (raw_result > 0)
                        {
                            wchar_t info[512];
                            ZeroMemory(info, sizeof(info));

                            swprintf_s(info, 512, L"Bitmap converted to LIGHTMAP and saved as: %hs", output_filename);

                            EnableWindow(GetDlgItem(_hwnd, IDC_DC_OUTPUT), TRUE);
                            SetWindowText(GetDlgItem(_hwnd, IDC_DC_OUTPUT), info);

                            int read_raw_result = BM_Read_Lightmaps_RAW_Separate(   output_filename, cnv_raw_lm_wall, cnv_raw_lm_floor, cnv_raw_lm_ceil,
                                                                                    APP_level_file.wall_lightmaps_count, APP_level_file.flat_lightmaps_count);

                            if (read_raw_result <= 0)
                                MessageBox(_hwnd, L"Can't open converted file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                            else
                            {
                                cnv_raw_lm_wall_lenght = (int)(ceilf(sqrtf(APP_level_file.wall_lightmaps_count))) * 32;
                                cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall);

                                cnv_raw_lm_flat_lenght = (int)(ceilf(sqrtf(APP_level_file.flat_lightmaps_count / 2))) * 32;
                                cnv_hbm_texture_128 = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, cnv_raw_lm_floor);
                                cnv_hbm_texture_64 = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, cnv_raw_lm_ceil);

                                // Set that raw file is loaded.
                                cnv_is_raw_loaded = true;
                            }
                        }
                    }

                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                }
            break;
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);

            // display input texture
            if (cnv_is_input_loaded)
            {
                PAINTSTRUCT ps_texture;
                HDC hdc_texture = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE), &ps_texture);

                HDC hdc_tmp = CreateCompatibleDC(hdc_texture);
                SelectObject(hdc_tmp, cnv_hbm_texture);
                StretchBlt(hdc_texture, 0, 0, 256, 256, hdc_tmp, 0, 0, 2048, 2048, SRCCOPY);
                DeleteObject(cnv_hbm_texture);
                DeleteDC(hdc_tmp);

                EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE), &ps_texture);
            }

            // display 256 preview
            PAINTSTRUCT ps_texture_256;
            HDC hdc_texture_256 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

            HDC hdc_tmp_256 = CreateCompatibleDC(hdc_texture_256);
            SelectObject(hdc_tmp_256, cnv_hbm_texture_256);
            StretchBlt(hdc_texture_256, 0, 0, 256, 256, hdc_tmp_256, 0, 0, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall_lenght, SRCCOPY);
            DeleteObject(cnv_hbm_texture_256);
            DeleteDC(hdc_tmp_256);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);
            
            // display 128 preview
            PAINTSTRUCT ps_texture_128;
            HDC hdc_texture_128 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

            HDC hdc_tmp_128 = CreateCompatibleDC(hdc_texture_128);
            SelectObject(hdc_tmp_128, cnv_hbm_texture_128);
            StretchBlt(hdc_texture_128, 0, 0, 256, 256, hdc_tmp_128, 0, 0, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, SRCCOPY);
            DeleteObject(cnv_hbm_texture_128);
            DeleteDC(hdc_tmp_128);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_128), &ps_texture_128);

            // display 64 preview
            PAINTSTRUCT ps_texture_64;
            HDC hdc_texture_64 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

            HDC hdc_tmp_64 = CreateCompatibleDC(hdc_texture_64);
            SelectObject(hdc_tmp_64, cnv_hbm_texture_64);
            StretchBlt(hdc_texture_64, 0, 0, 256, 256, hdc_tmp_64, 0, 0, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, SRCCOPY);
            DeleteObject(cnv_hbm_texture_64);
            DeleteDC(hdc_tmp_64);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_64), &ps_texture_64);

            return (INT_PTR)TRUE;
        }

        case WM_CLOSE:
            free(cnv_raw_lm_wall);
            free(cnv_raw_lm_floor);
            free(cnv_raw_lm_ceil);

            EndDialog(_hwnd, LOWORD(_wparam));
            return (INT_PTR)TRUE;
        }

        return (INT_PTR)FALSE;
}


INT_PTR CALLBACK APP_Wnd_Select_Texture_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:
        {
            // Set positions of preview windows.
            SetWindowPos(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), HWND_TOP, 200, 10, 256 + 2, 256 + 2, SWP_SHOWWINDOW);

            // Alloc memory for raw texture when loaded.
            cnv_raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            cnv_raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            // restore to default directory
            SetCurrentDirectory(APP_curr_dir);

            // get files from directory
            TCHAR directory[256];
            ZeroMemory(directory, sizeof(directory));

            if (gui_current_layer == LAYERS_MAIN_LAYER)
            {
                wcscpy(directory, TEXT(APP_WALL_TEXTURES_DIRECTORY));
                wcscat(directory, L"\\*.tw");
            }
            else
            {
                wcscpy(directory, TEXT(APP_FLAT_TEXTURES_DIRECTORY));
                wcscat(directory, L"\\*.tf");
            }

            WIN32_FIND_DATA data;
            HANDLE hFind = FindFirstFile(directory, &data);

            if (hFind != INVALID_HANDLE_VALUE) 
            {
                do
                {
                    SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_ADDSTRING, 0, (LPARAM)data.cFileName);
                } while (FindNextFile(hFind, &data));
                FindClose(hFind);
            }

            SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
            SendMessage(_hwnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDC_SELECT_TEXTURE_LIST, CBN_SELCHANGE), (LPARAM)0);
            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
        }
        return (INT_PTR)TRUE;

        case WM_COMMAND:
        {
            int wm_id = LOWORD(_wparam);
            int wm_event = HIWORD(_wparam);

            // all combos and lists
            switch (wm_event)
            {
                case CBN_SELCHANGE:
                    switch (wm_id)
                    {
                        case IDC_SELECT_TEXTURE_LIST:
                        {
                            ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));

                            int index = 0;
                            index = SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_GETCURSEL, 0, 0);
                            SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_GETTEXT, index, (LPARAM)TEX_selected_file_name);

                            TCHAR path_file_name[256];
                            ZeroMemory(path_file_name, sizeof(path_file_name));

                            if (gui_current_layer == LAYERS_MAIN_LAYER) wcscpy(path_file_name, TEXT(APP_WALL_TEXTURES_DIRECTORY));
                            else                                        wcscpy(path_file_name, TEXT(APP_FLAT_TEXTURES_DIRECTORY));

                            wcscat(path_file_name, L"\\");
                            wcscat(path_file_name, TEX_selected_file_name);

                            // convert TCHAR path_filename to CHAR path_filename
                            ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                            wcstombs_s(NULL, cnv_input_bitmap_filename, path_file_name, wcslen(path_file_name) + 1);

                            sIO_Prefs prefs;
                            prefs.ch1 = 8;
                            prefs.ch2 = 16;
                            prefs.ch3 = 24;

                            int read_raw_result = BM_Read_Texture_RAW(cnv_input_bitmap_filename, cnv_raw_table, cnv_raw_index, &prefs);

                            if (read_raw_result <= 0)
                                MessageBox(_hwnd, L"Can't open that file.", L"Error.", MB_ICONWARNING | MB_OK);
                            else
                            {
                                cnv_hbm_texture_256 = Make_HBITMAP_From_Bitmap_Indexed( _hwnd, 256, 256, IO_TEXTURE_MAX_COLORS, 
                                                                                            cnv_raw_table + IO_TEXTURE_MAX_COLORS * (IO_TEXTURE_MAX_COLORS-1), cnv_raw_index);
                            }
                        }
                        break;
                    }
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;
            }

            switch (wm_id)
            {
                case IDOK:

                    switch (gui_current_layer)
                    {
                        case LAYERS_MAIN_LAYER:
                        {
                            switch (gui_selected_image)
                            {
                                case IDB_PROP_WALL_IMAGE_TOP_SELECT:
                                {
                                    TEX_prev_id = APP_level_file.map[selected_cell_index].wall_texture_id[id_top];

                                    int id = TEX_Add_Texture_To_List(&TEX_wall_list, 0);
                                    APP_level_file.map[selected_cell_index].wall_texture_id[id_top] = id;
                                    MAP_working_cell.wall_texture_id[id_top] = id;
                                }
                                break;

                                case IDB_PROP_WALL_IMAGE_RIGHT_SELECT:
                                {
                                    TEX_prev_id = APP_level_file.map[selected_cell_index].wall_texture_id[id_right];

                                    int id = TEX_Add_Texture_To_List(&TEX_wall_list, 0);
                                    APP_level_file.map[selected_cell_index].wall_texture_id[id_right] = id;
                                    MAP_working_cell.wall_texture_id[id_right] = id;
                                }
                                break;

                                case IDB_PROP_WALL_IMAGE_BOTTOM_SELECT:
                                {
                                    TEX_prev_id = APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom];

                                    int id = TEX_Add_Texture_To_List(&TEX_wall_list, 0);
                                    APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom] = id;
                                    MAP_working_cell.wall_texture_id[id_bottom] = id;
                                }
                                break;

                                case IDB_PROP_WALL_IMAGE_LEFT_SELECT:
                                {
                                    TEX_prev_id = APP_level_file.map[selected_cell_index].wall_texture_id[id_left];

                                    int id = TEX_Add_Texture_To_List(&TEX_wall_list, 0);
                                    APP_level_file.map[selected_cell_index].wall_texture_id[id_left] = id;
                                    MAP_working_cell.wall_texture_id[id_left] = id;
                                }
                                break;
                            }
                        }
                        break;

                        case LAYERS_FLOOR_LAYER:
                        {
                            TEX_prev_id = APP_level_file.map[selected_cell_index].flat_texture_id[id_floor];

                            int id = TEX_Add_Texture_To_List(&TEX_flat_list, 1);
                            APP_level_file.map[selected_cell_index].flat_texture_id[id_floor] = id;
                        }
                        break;

                        case LAYERS_CEIL_LAYER:
                        {
                            TEX_prev_id = APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil];

                            int id = TEX_Add_Texture_To_List(&TEX_flat_list, 1);
                            APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil] = id;
                        }
                        break;
                    }

                    free(cnv_raw_table);
                    free(cnv_raw_index);

                    EndDialog(_hwnd, LOWORD(_wparam));

                    break;

                default:
                    return DefWindowProc(_hwnd, _message, _wparam, _lparam);
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);

            // display 256 preview
            PAINTSTRUCT ps_texture_256;
            HDC hdc_texture_256 = BeginPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);

            HDC hdc_tmp_256 = CreateCompatibleDC(hdc_texture_256);
            SelectObject(hdc_tmp_256, cnv_hbm_texture_256);
            BitBlt(hdc_texture_256, 0, 0, 256, 256, hdc_tmp_256, 0, 0, SRCCOPY);
            DeleteObject(cnv_hbm_texture_256);
            DeleteDC(hdc_tmp_256);

            EndPaint(GetDlgItem(_hwnd, IDS_DC_TEXTURE_256), &ps_texture_256);
        }
        return (INT_PTR)TRUE;

        case WM_CLOSE:
        {
            free(cnv_raw_table);
            free(cnv_raw_index);

            EndDialog(_hwnd, LOWORD(_wparam));
        }
        return (INT_PTR)TRUE; 
    }

    return (INT_PTR)FALSE;
}


void APP_Init_Once()
{
    // save path to current directory
    GetCurrentDirectory(MAX_PATH, APP_curr_dir);

    // save path to level directory
    SetCurrentDirectory(TEXT(APP_LEVELS_DIRECTORY));
    GetCurrentDirectory(MAX_PATH, APP_level_dir);
}

void APP_Reset()
{
    // --- APP settings ---
    SetCurrentDirectory(APP_curr_dir);

    ZeroMemory(APP_filename, sizeof(APP_filename));
    wcscpy(APP_filename, APP_level_dir);
    wcscat(APP_filename, L"\\new-level.m");
    APP_never_saved = 1;

    APP_is_left_hold = false;
    APP_is_middle_hold = false;

    // --- level structure settings ---
    ZeroMemory(&APP_level_file, sizeof(APP_level_file));

    APP_level_file.player_starting_angle = 30;
    APP_level_file.player_starting_cell_x = MAP_LENGTH / 2;
    APP_level_file.player_starting_cell_y = MAP_LENGTH / 2;

    strcpy(APP_level_file.title_01, "Level -01-");
    strcpy(APP_level_file.title_02, "Level Tiltle -02-");
}

int APP_Open()
{
    // remember current filename
    TCHAR old_filename[128];
    ZeroMemory(old_filename, sizeof(old_filename));
    wcscpy(old_filename, APP_filename);

    // get opened filename
    if (GUI_OpenDialog(APP_filename, APP_Open_FILE, APP_MAP_FILTER, APP_DEFEXT_MAP))
    {
        // open selected filename
        FILE* file_in;
        file_in = _wfopen(APP_filename, L"rb");

        if (file_in == NULL)
        {
            // if file not opened get the old name and return false
            wcscpy(APP_filename, old_filename);

            MessageBox(APP_hWnd, L"Couldn't read from MAP file...", L"Info...", MB_OK);
            return 0;
        }
        else
        {
            // remember opened filename because we are resetting
            wcscpy(old_filename, APP_filename);

            // if file ok - reset all and read in all info to file structure
            APP_Reset();
            Map_Reset();
            TEX_Reset();

            fread(&APP_level_file, sizeof(APP_level_file), 1, file_in);
            fclose(file_in);

            APP_never_saved = 0;

            // restore opened filename
            wcscpy(APP_filename, old_filename);

            // restore current dir to default
            SetCurrentDirectory(APP_curr_dir);

            // read in wall textures
            for (int i = 0; i < APP_level_file.wall_textures_count; i++)
            {
                ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));
                mbstowcs_s(NULL, TEX_selected_file_name, IO_MAX_STRING_TITLE, APP_level_file.wall_texture_filename[i], _TRUNCATE);

                TEX_Add_Texture_To_List(&TEX_wall_list, 0);
            }

            // read in flat textures
            for (int i = 0; i < APP_level_file.flat_textures_count; i++)
            {
                ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));
                mbstowcs_s(NULL, TEX_selected_file_name, IO_MAX_STRING_TITLE, APP_level_file.flat_texture_filename[i], _TRUNCATE);

                TEX_Add_Texture_To_List(&TEX_flat_list, 1);
            }

            // restore negative floor and ceil cells
            for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
            {
                if (APP_level_file.map[i].was_floor_negative) APP_level_file.map[i].flat_texture_id[id_floor] = -1;
                if (APP_level_file.map[i].was_ceil_negative) APP_level_file.map[i].flat_texture_id[id_ceil] = -1;
            }

            return 1;
        }
    }
    else
        return 0;
}

void APP_Save()
{
    if (APP_never_saved)
        APP_Save_As();

    // open selected filename
    FILE* file_out;
    file_out = _wfopen(APP_filename, L"wb");

    if (file_out == NULL)
    {
        MessageBox(APP_hWnd, L"Could not open file to save...", L"Info...", MB_OK);
        return;
    }
    else
    {
        // save level title 1 and 2 to level structure
            TCHAR tc_title_input[IO_MAX_STRING_TITLE];
            ZeroMemory(tc_title_input, sizeof(tc_title_input));

            char title_out[IO_MAX_STRING_TITLE];
            ZeroMemory(title_out, sizeof(title_out));

            // get first title
            GetWindowText(GetDlgItem(APP_hWnd, IDS_TITLE_EDIT_1), tc_title_input, IO_MAX_STRING_TITLE);

            wcstombs_s(NULL, title_out, sizeof(title_out), tc_title_input, sizeof(title_out));
            memcpy(APP_level_file.title_01, title_out, sizeof(APP_level_file.title_01));

            // get second title
            ZeroMemory(tc_title_input, sizeof(tc_title_input));
            ZeroMemory(title_out, sizeof(title_out));
            GetWindowText(GetDlgItem(APP_hWnd, IDS_TITLE_EDIT_2), tc_title_input, IO_MAX_STRING_TITLE);

            wcstombs_s(NULL, title_out, sizeof(title_out), tc_title_input, sizeof(title_out));
            memcpy(APP_level_file.title_02, title_out, sizeof(APP_level_file.title_02));

        // lets count all textures
            TEX_Count_Textures();

        // lets deal with WALL TEXTURES - if there is 'empty' texture (ids=-1) replace it as _default.tw texture
            if (TEX_wall_is_default)
            {
                // add default texture if not on the list
                ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));
                wcscpy(TEX_selected_file_name, L"_default.tw");

                int new_def_id = TEX_Add_Texture_To_List(&TEX_wall_list, 0);

                // replace id
                for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
                {
                    if (APP_level_file.map[i].is_cell_solid)
                    {
                        switch (APP_level_file.map[i].cell_type)
                        {
                            case LV_C_WALL_STANDARD:
                                if (APP_level_file.map[i].wall_texture_id[id_top] < 0) APP_level_file.map[i].wall_texture_id[id_top] = new_def_id;
                                break;

                            case LV_C_WALL_FOURSIDE:
                                if (APP_level_file.map[i].wall_texture_id[id_top] < 0) APP_level_file.map[i].wall_texture_id[id_top] = new_def_id;
                                if (APP_level_file.map[i].wall_texture_id[id_right] < 0) APP_level_file.map[i].wall_texture_id[id_right] = new_def_id;
                                if (APP_level_file.map[i].wall_texture_id[id_bottom] < 0) APP_level_file.map[i].wall_texture_id[id_bottom] = new_def_id;
                                if (APP_level_file.map[i].wall_texture_id[id_left] < 0) APP_level_file.map[i].wall_texture_id[id_left] = new_def_id;
                                break;
                        }
                    }
                }
            }

            // cout all textures again
            TEX_Count_Textures();

            // update WALL lists
                TEX_Prepare_List_To_Save(&TEX_wall_list, TEX_wall_table, 0);
                TEX_wall_count = (int)TEX_wall_list.size();
                APP_level_file.wall_textures_count = TEX_wall_count;

                // switch old ids to new ids in wall list
                for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
                {
                    if (APP_level_file.map[i].is_cell_solid)
                    {
                            switch (APP_level_file.map[i].cell_type)
                            {
                                case LV_C_WALL_STANDARD:
                                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                                    {
                                        if (APP_level_file.map[i].wall_texture_id[id_top] == (*tex_iterator).id)
                                        {
                                            APP_level_file.map[i].wall_texture_id[id_top] = (*tex_iterator).new_id;
                                            break;
                                        }
                                    }
                                    break;

                                case LV_C_WALL_FOURSIDE:
                                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                                    {
                                        if (APP_level_file.map[i].wall_texture_id[id_top] == (*tex_iterator).id)
                                        {
                                            APP_level_file.map[i].wall_texture_id[id_top] = (*tex_iterator).new_id;
                                            break;
                                        }
                                    }

                                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                                    {
                                        if (APP_level_file.map[i].wall_texture_id[id_right] == (*tex_iterator).id)
                                        {
                                            APP_level_file.map[i].wall_texture_id[id_right] = (*tex_iterator).new_id;
                                            break;
                                        }
                                    }

                                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                                    {
                                        if (APP_level_file.map[i].wall_texture_id[id_bottom] == (*tex_iterator).id)
                                        {
                                            APP_level_file.map[i].wall_texture_id[id_bottom] = (*tex_iterator).new_id;
                                            break;
                                        }
                                    }

                                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                                    {
                                        if (APP_level_file.map[i].wall_texture_id[id_left] == (*tex_iterator).id)
                                        {
                                            APP_level_file.map[i].wall_texture_id[id_left] = (*tex_iterator).new_id;
                                            break;
                                        }
                                    }
                                    break;
                            }
                    }
                }

                // update new ids on list
                for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                    (*tex_iterator).id = (*tex_iterator).new_id;

         // update FLAT lists
                TEX_Prepare_List_To_Save(&TEX_flat_list, TEX_flat_table, 1);
                TEX_flat_count = (int)TEX_flat_list.size();

                if (TEX_flat_count == 0)
                {
                    APP_level_file.flat_textures_count = TEX_flat_count + 1;
                    strcpy(APP_level_file.flat_texture_filename[0], "_default.tf");
                }
                else
                    APP_level_file.flat_textures_count = TEX_flat_count;

                // switch old ids to new ids in wall list
                for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
                {
                    if (APP_level_file.map[i].flat_texture_id[id_floor] < 0)
                    {
                        APP_level_file.map[i].flat_texture_id[id_floor] = 0;
                        APP_level_file.map[i].was_floor_negative = 1;
                    }
                    else
                        APP_level_file.map[i].was_floor_negative = 0;

                    if (APP_level_file.map[i].flat_texture_id[id_ceil] < 0)
                    {
                        APP_level_file.map[i].flat_texture_id[id_ceil] = 0;
                        APP_level_file.map[i].was_ceil_negative = 1;
                    }
                    else
                        APP_level_file.map[i].was_ceil_negative = 0;

                    for (auto tex_iterator = TEX_flat_list.begin(); tex_iterator != TEX_flat_list.end(); tex_iterator++)
                    {
                        if (APP_level_file.map[i].flat_texture_id[id_floor] == (*tex_iterator).id)
                        {
                            APP_level_file.map[i].flat_texture_id[id_floor] = (*tex_iterator).new_id;
                            break;
                        }
                    }

                    for (auto tex_iterator = TEX_flat_list.begin(); tex_iterator != TEX_flat_list.end(); tex_iterator++)
                    {
                        if (APP_level_file.map[i].flat_texture_id[id_ceil] == (*tex_iterator).id)
                        {
                            APP_level_file.map[i].flat_texture_id[id_ceil] = (*tex_iterator).new_id;
                            break;
                        }
                    }
                }

                // update new ids on list
                for (auto tex_iterator = TEX_flat_list.begin(); tex_iterator != TEX_flat_list.end(); tex_iterator++)
                    (*tex_iterator).id = (*tex_iterator).new_id;

        // Update lightmaps info..
            APP_Count_or_Export_Lightmaps(1);

        // Save for Amiga - lets always set to "1".
        // The value is set here because it could be overwritten when loading diferent map.
            APP_level_file.endiannes = 1;

        // Swap endiannes for 16 and 32 bit values before saving
            APP_level_file.wall_lightmaps_count = MA_swap_int16(APP_level_file.wall_lightmaps_count);
            APP_level_file.flat_lightmaps_count = MA_swap_int16(APP_level_file.flat_lightmaps_count);

            for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
            {
                APP_level_file.map[i].wall_lightmap_id[0] = MA_swap_int16(APP_level_file.map[i].wall_lightmap_id[0]);
                APP_level_file.map[i].wall_lightmap_id[1] = MA_swap_int16(APP_level_file.map[i].wall_lightmap_id[1]);
                APP_level_file.map[i].wall_lightmap_id[2] = MA_swap_int16(APP_level_file.map[i].wall_lightmap_id[2]);
                APP_level_file.map[i].wall_lightmap_id[3] = MA_swap_int16(APP_level_file.map[i].wall_lightmap_id[3]);

                APP_level_file.map[i].flat_lightmap_id[0] = MA_swap_int16(APP_level_file.map[i].flat_lightmap_id[0]);
                APP_level_file.map[i].flat_lightmap_id[1] = MA_swap_int16(APP_level_file.map[i].flat_lightmap_id[1]);
            }

        // try save structure to file
            int result = (int)fwrite(&APP_level_file, sizeof(APP_level_file), 1, file_out);
            fclose(file_out);

            // restore negative floor and ceil cells
            for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
            {
                if (APP_level_file.map[i].was_floor_negative) APP_level_file.map[i].flat_texture_id[id_floor] = -1;
                if (APP_level_file.map[i].was_ceil_negative) APP_level_file.map[i].flat_texture_id[id_ceil] = -1;
            }

            if (!result)
                MessageBox(APP_hWnd, L"Could not save to file...", L"Info...", MB_OK);                    
    }
}

void APP_Save_As()
{
    if (GUI_OpenDialog(APP_filename, APP_SAVE_FILE, APP_MAP_FILTER, APP_DEFEXT_MAP))
    {
        APP_never_saved = 0;
        APP_Save();
    }
}

void APP_Count_or_Export_Lightmaps(int _mode)
{
    // _mode == 0 is for exporting to OBJ file
    // _mode == 1 is for counting lightmaps and memory only

    TCHAR tc_filename_out[MAX_PATH];
    ZeroMemory(tc_filename_out, sizeof(tc_filename_out));

    bool open_result = 0;

    if (_mode == 0)
    {
        int is_lightmap = 0;

        for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
        {
            if (APP_level_file.map[i].is_lightmapped)
            {
                is_lightmap = 1;
                break;
            }
        }

        if (!is_lightmap)
        {
            MessageBox(APP_hWnd, L"You must first select cells that will going to have lightmaps. \nNo lightmaps found.", L"Info...", MB_OK);
            return;
        }

        open_result = GUI_OpenDialog(tc_filename_out, APP_SAVE_FILE, APP_OBJ_FILTER, APP_DEFEXT_OBJ);
    }
    else
        open_result = 1;

    if (open_result)
    {
        FILE* file_out = NULL;
        bool file_result = 0;

        if (_mode == 0)
        {
            file_out = _wfopen(tc_filename_out, L"wb");

            if (file_out)   file_result = 1;
            else            file_result = 0;
        }
        else
            file_result = 1;

        if (file_result)
        {
            // tex coords buffer
            // let one line is max 16 bytes
            int buf_size = sizeof(char) * 6 * 4 * 16 * LV_MAP_CELLS_COUNT;
            char* tex_coords_buffer = (char*)malloc(buf_size);
            memset(tex_coords_buffer, 0, sizeof(buf_size));

            char tmp_buffer[32];
            memset(tmp_buffer, 0, sizeof(tmp_buffer));

            // assuming fixed texture 2048 x 2048, so 64 in a row when 32x32
            double fraction = 1.0 / 64.0;

            int16 v_num = 0;
            int16 q_num = 0;

            double grid = 10.0;
            double mirror_grid = -1.0 * grid;

            // For exorting lightmaps UV only - starting coords.
            double curr_x = 0.0;
            double curr_y = 0.0;

            APP_level_file.wall_lightmaps_count = 0;
            APP_level_file.flat_lightmaps_count = 0;

            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;

                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    if (APP_level_file.map[index].is_cell_solid)
                    {
                        // if SOLID - check each wall (its neightbour) - and save as quad polygon.

                        // EXPORT TOP WALL? 
                        // check if cell that is higher is solid or marked as lightmap, or this cell is the topmost.
                        int top_index = x + (y - 1) * MAP_LENGTH;
                        if (!APP_level_file.map[top_index].is_cell_solid && APP_level_file.map[top_index].is_lightmapped && (y > 0))
                        {
                            if (_mode == 0)
                            {
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

                                // check and reset current uv coords
                                if (curr_x + fraction > 1.0)
                                {
                                    curr_x = 0.0;
                                    curr_y += fraction;
                                }

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                APP_level_file.map[index].wall_lightmap_id[id_top] = q_num;

                                curr_x += fraction;

                                v_num += 4;
                                q_num += 1;
                            }

                            APP_level_file.wall_lightmaps_count++;
                        }

                        // EXPORT BOTTOM WALL? 
                        // check if cell that is lower is solid or marked as lightmap, or this cell is the bottom most.
                        int bottom_index = x + (y + 1) * MAP_LENGTH;
                        if (!APP_level_file.map[bottom_index].is_cell_solid && APP_level_file.map[bottom_index].is_lightmapped && (y < LV_MAP_LENGTH - 1))
                        {
                            if (_mode == 0)
                            {
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));

                                // check and reset current uv coords
                                if (curr_x + fraction > 1.0)
                                {
                                    curr_x = 0.0;
                                    curr_y += fraction;
                                }

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                APP_level_file.map[index].wall_lightmap_id[id_bottom] = q_num;

                                curr_x += fraction;

                                v_num += 4;
                                q_num += 1;
                            }

                            APP_level_file.wall_lightmaps_count++;
                        }

                        // EXPORT LEFT WALL? 
                        // check if cell that is on th left is solid or marked as lightmap, or this cell is the leftmost
                        int left_index = x - 1 + y * MAP_LENGTH;
                        if (!APP_level_file.map[left_index].is_cell_solid && APP_level_file.map[left_index].is_lightmapped && (x > 0))
                        {
                            if (_mode == 0)
                            {
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

                                // check and reset current uv coords
                                if (curr_x + fraction > 1.0)
                                {
                                    curr_x = 0.0;
                                    curr_y += fraction;
                                }

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);

                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);

                                strcat(tex_coords_buffer, tmp_buffer);

                                APP_level_file.map[index].wall_lightmap_id[id_left] = q_num;

                                curr_x += fraction;

                                v_num += 4;
                                q_num += 1;
                            }

                            APP_level_file.wall_lightmaps_count++;
                        }

                        // EXPORT RIGHT WALL? 
                        // check if cell that is on the right is solid or marked as lightmap, or this cell is the rightmost.
                        int right_index = x + 1 + y * MAP_LENGTH;
                        if (!APP_level_file.map[right_index].is_cell_solid && APP_level_file.map[right_index].is_lightmapped && (x < LV_MAP_LENGTH - 1))
                        {
                            if (_mode == 0)
                            {
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));

                                // check and reset current uv coords
                                if (curr_x + fraction > 1.0)
                                {
                                    curr_x = 0.0;
                                    curr_y += fraction;
                                }

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                memset(tmp_buffer, 0, sizeof(tmp_buffer));
                                sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                                strcat(tex_coords_buffer, tmp_buffer);

                                APP_level_file.map[index].wall_lightmap_id[id_right] = q_num;

                                curr_x += fraction;

                                v_num += 4;
                                q_num += 1;
                            }

                            APP_level_file.wall_lightmaps_count++;
                        }
                    }
                }
            }

            // Now lets add only floor lightmaps UVs...
            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;

                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    if (!APP_level_file.map[index].is_cell_solid)
                    {
                        // If NOT SOLID - save only floor and ceil as quad polygons.

                        if (_mode == 0)
                        {
                            // add floor quad
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

                            // check and reset current uv coords
                            if (curr_x + fraction > 1.0)
                            {
                                curr_x = 0.0;
                                curr_y += fraction;
                            }

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            APP_level_file.map[index].flat_lightmap_id[id_floor] = q_num;
                            q_num++;

                            curr_x += fraction;
                            v_num += 4;
                        }

                        APP_level_file.flat_lightmaps_count++;
                    }
                }
            }

            // Now add only ceil lightmaps UVs...
            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;

                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    if (!APP_level_file.map[index].is_cell_solid)
                    {
                        // If NOT SOLID - save only floor and ceil as quad polygons.

                        if (_mode == 0)
                        {
                            // add ceil quad
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                            fprintf(file_out, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));

                            // check and reset current uv coords
                            if (curr_x + fraction > 1.0)
                            {
                                curr_x = 0.0;
                                curr_y += fraction;
                            }

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x, 1.0 - curr_y - fraction, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            memset(tmp_buffer, 0, sizeof(tmp_buffer));
                            sprintf(tmp_buffer, "vt %.5f %.5f %.1f\n", curr_x + fraction, 1.0 - curr_y - fraction, 0.0);
                            strcat(tex_coords_buffer, tmp_buffer);

                            APP_level_file.map[index].flat_lightmap_id[id_ceil] = q_num;
                            q_num++;

                            curr_x += fraction;
                            v_num += 4;
                        }

                        APP_level_file.flat_lightmaps_count++;
                    }
                }
            }

            if (_mode == 0)
            {
                // write vertices number
                fprintf(file_out, "# %d vertices\n\n", v_num);

                // copy tex coords buffor to file
                fwrite(tex_coords_buffer, sizeof(char), strlen(tex_coords_buffer), file_out);

                // write tex coords number
                fprintf(file_out, "# %d texture coords\n\n", v_num);

                fprintf(file_out, "o MAP\ng MAP\n");

                for (int v = 0; v < v_num; v += 4)
                {
                    fprintf(file_out, "f %d/%d %d/%d %d/%d %d/%d\n", v + 3, v + 3, v + 1, v + 1, v + 2, v + 2, v + 4, v + 4);
                }

                // write quads number
                fprintf(file_out, "# %d polygons\n\n", q_num);

                fclose(file_out);
            }
         
            free(tex_coords_buffer);
        }
        else
            MessageBox(APP_hWnd, L"Could not open file...", L"Info...", MB_OK);
    }
    else
        MessageBox(APP_hWnd, L"Nothing saved...", L"Info...", MB_OK);
}

// ---------------------------------
// --- GUI FUNCTIONS DEFINITIONS ---
// ---------------------------------


void GUI_Create(HWND _hwnd)
{
    // init gui stuff
    h_font_1 = CreateFont(  18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Segoe UI"));

    hp_none = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
    hp_map_not_found_texture = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

    hp_main_grid = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    hp_floor_grid = CreatePen(PS_SOLID, 1, RGB(45, 36, 28));
    hp_ceil_grid = CreatePen(PS_SOLID, 1, RGB(31, 38, 50));
    hp_lightmaps_grid = CreatePen(PS_SOLID, 1, RGB(20, 20, 20));

    hp_floor_dotted = CreatePen(PS_DOT, 0, RGB(114, 91, 71));
    hp_ceil_dotted = CreatePen(PS_DOT, 0, RGB(64, 78, 103));
    hp_lightmaps_dotted = CreatePen(PS_DOT, 0, RGB(230, 230, 230));

    hbr_map_not_found_texture = CreateSolidBrush(RGB(200, 0, 0));
    hbr_black = CreateSolidBrush(RGB(0, 0, 0));
    hbr_map_player = CreateSolidBrush(RGB(0, 200, 0));

    hbr_map_walls_standard = CreateSolidBrush(RGB(100, 100, 100));
    hbr_map_walls_fourside_01 = hbr_map_walls_standard;
    hbr_map_walls_fourside_02 = CreateSolidBrush(RGB(110, 110, 110));

    hbr_map_floor = CreateSolidBrush(RGB(110, 8, 8));
    hbr_map_ceil = CreateSolidBrush(RGB(40, 60, 100));

    hbr_map_lightmaps = CreateSolidBrush(RGB(100, 100, 100));

    hbr_main_bg = CreateSolidBrush(RGB(30, 30, 30));
    hbr_floor_bg = CreateSolidBrush(RGB(34, 27, 21));
    hbr_ceil_bg = CreateSolidBrush(RGB(23, 28, 37));
    hbr_lightmaps_bg = CreateSolidBrush(RGB(45, 45, 45));

    hbr_selected_cell = CreateSolidBrush(RGB(255, 255, 0));

    // ---- EDITOR WINDOW -----

        int window_margin = 5;

        // graphic editor window
        CreateWindowEx( WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                        window_margin, window_margin, MAP_WINDOW_SIZE + 2, MAP_WINDOW_SIZE + 2, _hwnd, (HMENU)IDS_MAP, GetModuleHandle(NULL), nullptr);

        // fit button
        CreateWindowEx( 0, L"BUTTON", L"Fit to window", WS_CHILD | WS_VISIBLE,
                        window_margin, 972, 140, 44, _hwnd, (HMENU)IDB_MAP_FIT, GetModuleHandle(NULL), nullptr);

        // info
        CreateWindowEx( 0, L"STATIC", L"selected: ", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        810, 973, 110, 40, _hwnd, (HMENU)IDS_MAP_SELECTED_COORDS_L, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        868, 973, 110, 40, _hwnd, (HMENU)IDS_MAP_SELECTED_COORDS, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"hover: ", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        825, 993, 110, 40, _hwnd, (HMENU)IDS_MAP_HOVER_COORDS_L, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        868, 993, 110, 40, _hwnd, (HMENU)IDS_MAP_HOVER_COORDS, GetModuleHandle(NULL), nullptr);

        // font assign
        SendDlgItemMessage(_hwnd, IDB_MAP_FIT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDB_MAP_CONTROLS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_MAP_INFO1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_MAP_SELECTED_COORDS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_MAP_SELECTED_COORDS_L, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_MAP_HOVER_COORDS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_MAP_HOVER_COORDS_L, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // --- MEMORY INFO ----

        int mem_groupbox_x = 980, mem_groupbox_y = 12;

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                        mem_groupbox_x, mem_groupbox_y, 210, 160, _hwnd, (HMENU)-1, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L" Level resources info:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 5, mem_groupbox_y - 10, 130, 20, _hwnd, (HMENU)IDS_RES_LABEL, GetModuleHandle(NULL), nullptr);


        CreateWindowEx( 0, L"STATIC", L"Wall textures:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 10, mem_groupbox_y + 13, 90, 20, _hwnd, (HMENU)IDS_RES_WALL_TEX_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                        mem_groupbox_x + 100, mem_groupbox_y + 13, 90, 20, _hwnd, (HMENU)IDS_RES_WALL_TEX, GetModuleHandle(NULL), nullptr);


        CreateWindowEx( 0, L"STATIC", L"Flat textures:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 10, mem_groupbox_y + 31, 90, 20, _hwnd, (HMENU)IDS_RES_FLAT_TEX_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                        mem_groupbox_x + 100, mem_groupbox_y + 31, 90, 20, _hwnd, (HMENU)IDS_RES_FLAT_TEX, GetModuleHandle(NULL), nullptr);


        CreateWindowEx( 0, L"STATIC", L"Wall lightmaps:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 10, mem_groupbox_y + 55, 90, 20, _hwnd, (HMENU)IDS_RES_W_LIGHTMAPS_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                        mem_groupbox_x + 100, mem_groupbox_y + 55, 90, 20, _hwnd, (HMENU)IDS_RES_W_LIGHTMAPS, GetModuleHandle(NULL), nullptr);


        CreateWindowEx(0, L"STATIC", L"Flat lightmaps:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 10, mem_groupbox_y + 73, 90, 20, _hwnd, (HMENU)IDS_RES_F_LIGHTMAPS_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                        mem_groupbox_x + 100, mem_groupbox_y + 73, 90, 20, _hwnd, (HMENU)IDS_RES_F_LIGHTMAPS, GetModuleHandle(NULL), nullptr);


        CreateWindowEx( 0, L"STATIC", L"Total:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        mem_groupbox_x + 10, mem_groupbox_y + 100, 90, 20, _hwnd, (HMENU)IDS_RES_TOTAL_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                        mem_groupbox_x + 100, mem_groupbox_y + 100, 90, 20, _hwnd, (HMENU)IDS_RES_TOTAL, GetModuleHandle(NULL), nullptr);

        // add refresh button

        CreateWindowEx(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE,
                        mem_groupbox_x + 60, mem_groupbox_y + 126, 80, 24, _hwnd, (HMENU)IDB_RES_REFRESH, GetModuleHandle(NULL), NULL);

        SendDlgItemMessage(_hwnd, IDB_RES_REFRESH, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_FLAT_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_RES_FLAT_TEX, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_W_LIGHTMAPS_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_RES_W_LIGHTMAPS, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_F_LIGHTMAPS_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_RES_F_LIGHTMAPS, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendDlgItemMessage(_hwnd, IDS_RES_TOTAL_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendDlgItemMessage(_hwnd, IDS_RES_TOTAL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    // --- TITLE ----

        int title_groupbox_x = 1200, title_groupbox_y = 12;

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                        title_groupbox_x, title_groupbox_y, 240, 100, _hwnd, (HMENU)-1, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L" Map title && subtile:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        title_groupbox_x + 5, title_groupbox_y - 10, 120, 20, _hwnd, (HMENU)IDS_TITLE_LABEL, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE,
                        title_groupbox_x + 10, title_groupbox_y + 26, 220, 22, _hwnd, (HMENU)IDS_TITLE_EDIT_1, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE,
                        title_groupbox_x + 10, title_groupbox_y + 56, 220, 22, _hwnd, (HMENU)IDS_TITLE_EDIT_2, GetModuleHandle(NULL), nullptr);

        SendDlgItemMessage(_hwnd, IDS_TITLE_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendMessage(GetDlgItem(_hwnd, IDS_TITLE_EDIT_1), (UINT)EM_SETLIMITTEXT, (WPARAM)32, (LPARAM)0);        
        SendDlgItemMessage(_hwnd, IDS_TITLE_EDIT_1, WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendMessage(GetDlgItem(_hwnd, IDS_TITLE_EDIT_2), (UINT)EM_SETLIMITTEXT, (WPARAM)32, (LPARAM)0);
        SendDlgItemMessage(_hwnd, IDS_TITLE_EDIT_2, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    // ---- LAYERS ----

        int layers_groupbox_x = 980, layers_groupbox_y = 184;

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                        layers_groupbox_x, layers_groupbox_y, 210, 65, _hwnd, (HMENU)IDS_LAYERS_GROUPBOX, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L" Layers:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        layers_groupbox_x + 5, layers_groupbox_y - 10, 50, 20, _hwnd, (HMENU)IDS_LAYERS_LABEL1, GetModuleHandle(NULL), nullptr);

        CreateWindow(   WC_COMBOBOX, TEXT(""), CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                        layers_groupbox_x + 15, layers_groupbox_y + 22, 180, 100, _hwnd, (HMENU)IDC_LAYERS_COMBO, GetModuleHandle(NULL), NULL);

        SendDlgItemMessage(_hwnd, IDS_LAYERS_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), WM_SETFONT, (WPARAM)h_font_1, TRUE);

        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"01. Main layer");
        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"02. Floor layer");
        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"03. Ceiling layer");
        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"04. Lightmaps layer");

        SendMessage(GetDlgItem(_hwnd, IDC_LAYERS_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    // ---- LIST BOX ----

        int list_groupbox_x = 980, list_groupbox_y = 262;

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                        list_groupbox_x, list_groupbox_y, 210, 162, _hwnd, (HMENU)IDS_LIST_GROUPBOX, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L" List of elements:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        list_groupbox_x + 5, list_groupbox_y - 10, 100, 20, _hwnd, (HMENU)IDS_LIST_LABEL1, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( WS_EX_CLIENTEDGE, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                        list_groupbox_x + 15, list_groupbox_y + 20, 180, 144, _hwnd, (HMENU)IDL_LIST_LIST, GetModuleHandle(NULL), nullptr);

        SendDlgItemMessage(_hwnd, IDS_LIST_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
        SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // --- PROPERTIES ---

       int prop_groupbox_x = 1200, prop_groupbox_y = 124;

        CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
                        prop_groupbox_x, prop_groupbox_y, 530, 520, _hwnd, (HMENU)IDS_PROPERTIES_GROUPBOX, GetModuleHandle(NULL), nullptr);

        CreateWindowEx( 0, L"STATIC", L" Properties:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        prop_groupbox_x + 5, prop_groupbox_y - 10, 80, 20, _hwnd, (HMENU)IDS_PROPERTIES_LABEL1, GetModuleHandle(NULL), nullptr);

        SendDlgItemMessage(_hwnd, IDS_PROPERTIES_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);


            // ADD MODE button

                CreateWindowEx( 0, L"BUTTON", L"ADD TO MAP MODE", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 10, prop_groupbox_y + 480, 510, 32, _hwnd, (HMENU)IDB_ADD_ADDTOMAP, GetModuleHandle(NULL), NULL);

                SendDlgItemMessage(_hwnd, IDB_ADD_ADDTOMAP, WM_SETFONT, (WPARAM)h_font_1, TRUE);


            // PROPERTIES --- player start
                CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                prop_groupbox_x + 15, prop_groupbox_y + 25, 250, 60, _hwnd, (HMENU)IDS_PROP_PLAYER_L1, GetModuleHandle(NULL), nullptr);

                CreateWindowEx( 0, L"BUTTON", L"0* - RIGHT", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 310, prop_groupbox_y + 180, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_RIGHT, GetModuleHandle(NULL), NULL);

                CreateWindowEx( 0, L"BUTTON", L"90* - UP", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 180, prop_groupbox_y + 110, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_UP, GetModuleHandle(NULL), NULL);

                CreateWindowEx( 0, L"BUTTON", L"180* - LEFT", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 50, prop_groupbox_y + 180, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_LEFT, GetModuleHandle(NULL), NULL);

                CreateWindowEx( 0, L"BUTTON", L"270* - DOWN", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 180, prop_groupbox_y + 250, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_DOWN, GetModuleHandle(NULL), NULL);

                SendDlgItemMessage(_hwnd, IDS_PROP_PLAYER_L1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_RIGHT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_UP, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_LEFT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_DOWN, WM_SETFONT, (WPARAM)h_font_1, TRUE);


            // PROPERTIES --- 4 textures window for walls etc.

                CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 190, prop_groupbox_y + 20, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_TOP, GetModuleHandle(NULL), nullptr);

                CreateWindowEx( 0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 167, prop_groupbox_y + 20, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_TOP_SELECT, GetModuleHandle(NULL), NULL);

              //  CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                    //            prop_groupbox_x + 175, prop_groupbox_y + 150, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_TOP_LABEL, GetModuleHandle(NULL), nullptr);


                CreateWindowEx( WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 370, prop_groupbox_y + 170, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_RIGHT, GetModuleHandle(NULL), nullptr);

                CreateWindowEx( 0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 348, prop_groupbox_y + 170, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_RIGHT_SELECT, GetModuleHandle(NULL), NULL);

             //   CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                          //      prop_groupbox_x + 325, prop_groupbox_y + 280, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_RIGHT_LABEL, GetModuleHandle(NULL), nullptr);


                CreateWindowEx( WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 190, prop_groupbox_y + 320, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_BOTTOM, GetModuleHandle(NULL), nullptr);

                CreateWindowEx( 0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 167, prop_groupbox_y + 320, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_BOTTOM_SELECT, GetModuleHandle(NULL), NULL);

             //   CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                           //     prop_groupbox_x + 175, prop_groupbox_y + 420, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_BOTTOM_LABEL, GetModuleHandle(NULL), nullptr);


                CreateWindowEx( WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 33, prop_groupbox_y + 170, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_LEFT, GetModuleHandle(NULL), nullptr);

                CreateWindowEx( 0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
                                prop_groupbox_x + 10, prop_groupbox_y + 170, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_LEFT_SELECT, GetModuleHandle(NULL), NULL);

               // CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                            //    prop_groupbox_x + 20, prop_groupbox_y + 280, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_LEFT_LABEL, GetModuleHandle(NULL), nullptr);

                SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

                SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_RIGHT_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

                SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_BOTTOM_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

                SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_LEFT_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
                SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
}

void GUI_Reset(HWND _hwnd)
{
    gui_current_layer = 0;
    gui_current_list_index = -1;

    gui_texprew_mode = 0;
    gui_add_mode = 0;    

    // --- update titles - get from file structure ---
        SetWindowText(_hwnd, APP_filename);

        TCHAR t_tile[32];

        ZeroMemory(t_tile, sizeof(t_tile));
        mbstowcs_s(NULL, t_tile, 32, APP_level_file.title_01, _TRUNCATE);
        SetWindowText(GetDlgItem(_hwnd, IDS_TITLE_EDIT_1), t_tile);

        ZeroMemory(t_tile, sizeof(t_tile));
        mbstowcs_s(NULL, t_tile, 32, APP_level_file.title_02, _TRUNCATE);
        SetWindowText(GetDlgItem(_hwnd, IDS_TITLE_EDIT_2), t_tile);

    GUI_Update_Resources(_hwnd);
    GUI_Update_List_Box(_hwnd);
    GUI_Hide_Properties(_hwnd);
}

BOOL GUI_OpenDialog(TCHAR* _pathfilename, bool _mode, LPCWSTR _filter, LPCWSTR _defext)
{
    // mode = 0 for open, mode = 1 for saving 

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = APP_hWnd;
    ofn.lpstrFilter = _filter;
    ofn.lpstrFile = _pathfilename;
    ofn.lpstrDefExt = _defext;
    ofn.nMaxFile = MAX_PATH;

    if (_mode)
    {
        // for saving
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        return GetSaveFileName(&ofn);
    }
    else
    {
        // for open
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        return GetOpenFileName(&ofn);
    }
}

bool GUI_is_Map_Region(void)
{
    if (hover_cell_x >= 0 && hover_cell_x < MAP_LENGTH &&
        hover_cell_y >= 0 && hover_cell_y < MAP_LENGTH &&
        APP_mp.x >= 0 && APP_mp.x < MAP_WINDOW_SIZE &&
        APP_mp.y >= 0 && APP_mp.y < MAP_WINDOW_SIZE)

        return true;
    else
        return false;
}


void GUI_Update_Resources(HWND _hwnd)
{
    TCHAR output[16];

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"%d:   %.2f MB", TEX_wall_count, TEX_wall_mem / (1024.0f*1024.0f) );
    SendMessage(GetDlgItem(_hwnd, IDS_RES_WALL_TEX), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"%d:   %.2f MB", TEX_flat_count, TEX_flat_mem / (1024.0f * 1024.0f) );
    SendMessage(GetDlgItem(_hwnd, IDS_RES_FLAT_TEX), WM_SETTEXT, 0, (LPARAM)output);

    float wall_lm_mem = (APP_level_file.wall_lightmaps_count * IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE) / (1024.0f * 1024.0f);
    float flat_lm_mem = (APP_level_file.flat_lightmaps_count * IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE) / (1024.0f * 1024.0f);

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"%d:  %.2f MB", APP_level_file.wall_lightmaps_count, wall_lm_mem);
    SendMessage(GetDlgItem(_hwnd, IDS_RES_W_LIGHTMAPS), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"%d:  %.2f MB", APP_level_file.flat_lightmaps_count, flat_lm_mem);
    SendMessage(GetDlgItem(_hwnd, IDS_RES_F_LIGHTMAPS), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"%.2f MB", (TEX_wall_mem + TEX_flat_mem + wall_lm_mem + flat_lm_mem) / (1024.0f * 1024.0f) );
    SendMessage(GetDlgItem(_hwnd, IDS_RES_TOTAL), WM_SETTEXT, 0, (LPARAM)output);
}

void GUI_Update_Map_Coords(bool _is_map)
{
    TCHAR output[32];

    ZeroMemory(&output, sizeof(output));
    swprintf_s(output, L"[ %d, %d ] [ %d ]", selected_cell_x, selected_cell_y, selected_cell_index);

    SendMessage(GetDlgItem(APP_hWnd, IDS_MAP_SELECTED_COORDS), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(&output, sizeof(output));

    if (_is_map)    swprintf_s(output, L"[ %d, %d ] [ %d ]", hover_cell_x, hover_cell_y, hover_cell_index);
    else            swprintf_s(output, L"---");

    SendMessage(GetDlgItem(APP_hWnd, IDS_MAP_HOVER_COORDS), WM_SETTEXT, 0, (LPARAM)output);
}

void GUI_Update_Layers_Combo(HWND _hwnd)
{
    SendMessage(GetDlgItem(APP_hWnd, IDC_LAYERS_COMBO), CB_SETCURSEL, (WPARAM)gui_current_layer, (LPARAM)0);
}

void GUI_Update_List_Box(HWND _hwnd)
{
    SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), (UINT)LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Player start");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"02. Standard wall");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"03. Four sided wall");
            break;

        case LAYERS_FLOOR_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Floor");
            break;

        case LAYERS_CEIL_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Ceiling");
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Lightmaps selection");
          /*  SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"02. Show walls Lightmaps");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"03. Show floor Lightmaps");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"04. Show ceil Lightmaps");*/
            break;
    }

    SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_SETCURSEL, (WPARAM)gui_current_list_index, (LPARAM)0);
}

void GUI_Update_Add_Mode_Button(HWND _hwnd)
{
    if (gui_current_list_index != -1)
    {
        if (gui_add_mode)
            SendMessage(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), WM_SETTEXT, 0, (LPARAM)L"Press RMB to exit ADD TO MAP MODE");
        else
            SendMessage(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), WM_SETTEXT, 0, (LPARAM)L"ADD TO MAP MODE");

        SendMessage(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), BM_SETSTATE, gui_add_mode, 0);
        EnableWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), FALSE);
    }

    SetFocus(_hwnd);
}


void GUI_Update_Properties(HWND _hwnd)
{
    GUI_Hide_Properties(_hwnd);

    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_MAIN_PLAYER:
                        GUI_Update_Properties_Player(_hwnd);
                    break;

                case LIST_MAIN_WALL_STANDARD:
                case LIST_MAIN_WALL_FOURSIDE:
                        GUI_Update_Properties_Walls(_hwnd);
                    break;
            }
            break;

        case LAYERS_FLOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_FLOOR:
                        GUI_Update_Properties_Floor(_hwnd);
                    break;
            }
            break;

        case LAYERS_CEIL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_FLOOR:
                    GUI_Update_Properties_Ceil(_hwnd);
                    break;
            }
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_LIGHTMAPS_SELECT:
                case LIST_LIGHTMAPS_WALLS:
                case LIST_LIGHTMAPS_FLOOR:
                case LIST_LIGHTMAPS_CEIL:
                    GUI_Update_Properties_Lightmaps(_hwnd);
                    break;
            }
            break;
    }
}

void GUI_Update_Properties_Player(HWND _hwnd)
{
    TCHAR output[512];

    ZeroMemory(output, sizeof(output));
    swprintf_s( output, L"Player start cell X: %d\nPlayer start cell Y: %d\nPlayer orientation in degrees: %d* (%d)",
                APP_level_file.player_starting_cell_x, APP_level_file.player_starting_cell_y,
                APP_level_file.player_starting_angle*3, APP_level_file.player_starting_angle);

    SendMessage(GetDlgItem(_hwnd, IDS_PROP_PLAYER_L1), WM_SETTEXT, 0, (LPARAM)output);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_PLAYER_L1), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_RIGHT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_UP), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_LEFT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_DOWN), TRUE);
}

void GUI_Update_Properties_Walls(HWND _hwnd)
{
    switch (gui_current_list_index)
    {
        // standard wall    
        case LIST_MAIN_WALL_STANDARD:
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
            break;

        // fourside wall    
        case LIST_MAIN_WALL_FOURSIDE:
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_RIGHT_SELECT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL), TRUE);

                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_BOTTOM_SELECT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL), TRUE);

                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_LEFT_SELECT), TRUE);
                ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL), TRUE);

                ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
            break;
    }

    /*
    TCHAR output[64];
    ZeroMemory(&output, sizeof(output));

    swprintf_s(output, L"%s", level.map[selected_cell_index].top_id, level.map[selected_cell_index].top_id);
    SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%s", level.map[selected_cell_index].left_id, level.map[selected_cell_index].left_id);
    SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%s", level.map[selected_cell_index].bottom_id, level.map[selected_cell_index].bottom_id);
    SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%s", level.map[selected_cell_index].right_id, level.map[selected_cell_index].right_id);
    SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL), WM_SETTEXT, 0, (LPARAM)output);*/
}

void GUI_Update_Properties_Floor(HWND _hwnd)
{
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
}

void GUI_Update_Properties_Ceil(HWND _hwnd)
{
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
}

void GUI_Update_Properties_Lightmaps(HWND _hwnd)
{
    switch (gui_current_list_index)
    {
        case LIST_LIGHTMAPS_SELECT:
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), TRUE);
            break;

        case LIST_LIGHTMAPS_WALLS:
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), FALSE);
            break;

        case LIST_LIGHTMAPS_FLOOR:
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), FALSE);
            break;

        case LIST_LIGHTMAPS_CEIL:
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), FALSE);
            ShowWindow(GetDlgItem(_hwnd, IDB_ADD_ADDTOMAP), FALSE);
            break;
    }
}

void GUI_Draw_Properites_Textures(HWND _hwnd)
{
    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_MAIN_WALL_STANDARD:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    break;

                case LIST_MAIN_WALL_FOURSIDE:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT, APP_level_file.map[selected_cell_index].wall_texture_id[id_right], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM, APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_LEFT, APP_level_file.map[selected_cell_index].wall_texture_id[id_left], &TEX_wall_list);
                    break;
            }
            break;

        case LAYERS_FLOOR_LAYER:
            GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].flat_texture_id[id_floor], &TEX_flat_list);
            break;

        case LAYERS_CEIL_LAYER:
            GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil], &TEX_flat_list);
            break;
    }
}

void GUI_Draw_Properties_Textures_Window(HWND _hwnd, int _wm_id, int _id, std::list<s_Texture>* _list)
{
    int found = 0;

    std::list<s_Texture>::iterator tex_iterator;
    s_Texture tmp_texture;

    for (tex_iterator = _list->begin(); tex_iterator != _list->end(); ++tex_iterator)
    {
        tmp_texture = *tex_iterator;

        if (_id == tmp_texture.id)
        {
            found = 1;
            break;
        }
    }

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetDlgItem(_hwnd, _wm_id), &ps);

        if (found)
        {
            HDC hdc_tmp = CreateCompatibleDC(hdc);
            SelectObject(hdc_tmp, tmp_texture.bitmap_128);
            BitBlt(hdc, 0, 0, 128, 128, hdc_tmp, 0, 0, SRCCOPY);
            DeleteDC(hdc_tmp);
        }
        else
            TEX_Draw_Not_Found_Texture(hdc, 0, 0, 128, 128);

    EndPaint(GetDlgItem(_hwnd, _wm_id), &ps);
}

void GUI_Hide_Properties(HWND _hwnd)
{
    // clear the properties window
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_PLAYER_L1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_RIGHT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_UP), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_LEFT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_PLAYER_DOWN), FALSE);
    
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_LEFT_SELECT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_BOTTOM_SELECT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_RIGHT_SELECT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL), FALSE);
}


// ---------------------------------
// --- MAP FUNCTIONS DEFINITIONS ---
// ---------------------------------


void Map_Reset(void)
{
    selected_cell_x = MAP_LENGTH / 2;
    selected_cell_y = MAP_LENGTH / 2 - 1;
    selected_cell_index = selected_cell_x + selected_cell_y * MAP_LENGTH;

    hover_cell_x = 0;
    hover_cell_y = 0;
    hover_cell_index = 0;

    f_offset_x = 0.0f, f_offset_y = 0.0f;
    f_start_pan_x = 0.0f, f_start_pan_y = 0.0f;
    f_scale_x = 15.0f, f_scale_y = 15.0f;

    // --- reset editor map cells ---
    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
        APP_level_file.map[i].is_cell_solid = 0;
        APP_level_file.map[i].cell_type = -1;
        APP_level_file.map[i].was_floor_negative = 1;
        APP_level_file.map[i].was_ceil_negative = 1;

        APP_level_file.map[i].wall_texture_id[id_top] = -1;
        APP_level_file.map[i].wall_texture_id[id_right] = -1;
        APP_level_file.map[i].wall_texture_id[id_bottom] = -1;
        APP_level_file.map[i].wall_texture_id[id_left] = -1;
        APP_level_file.map[i].flat_texture_id[id_floor] = -1;
        APP_level_file.map[i].flat_texture_id[id_ceil] = -1;

        APP_level_file.map[i].wall_lightmap_id[id_top] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_right] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_bottom] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_left] = -1;
        APP_level_file.map[i].flat_lightmap_id[id_floor] = -1;
        APP_level_file.map[i].flat_lightmap_id[id_ceil] = -1;
    }

    MAP_working_cell.is_cell_solid = 0;
    MAP_working_cell.cell_type = -1;

    MAP_working_cell.wall_texture_id[id_top] = -1;
    MAP_working_cell.wall_texture_id[id_right] = -1;
    MAP_working_cell.wall_texture_id[id_bottom] = -1;
    MAP_working_cell.wall_texture_id[id_left] = -1;
    MAP_working_cell.flat_texture_id[id_floor] = -1;
    MAP_working_cell.flat_texture_id[id_ceil] = -1;

    MAP_working_cell.wall_lightmap_id[id_top] = -1;
    MAP_working_cell.wall_lightmap_id[id_right] = -1;
    MAP_working_cell.wall_lightmap_id[id_bottom] = -1;
    MAP_working_cell.wall_lightmap_id[id_left] = -1;
    MAP_working_cell.flat_lightmap_id[id_floor] = -1;
    MAP_working_cell.flat_lightmap_id[id_ceil] = -1;
}

void Map_Update_Colors(void)
{
    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            hbr_active_bg = hbr_main_bg;
            hp_active_grid = hp_main_grid;
            break;

        case LAYERS_FLOOR_LAYER:
            hbr_active_bg = hbr_floor_bg;
            hp_active_grid = hp_floor_grid;
            hp_active_dotted = hp_floor_dotted;
            break;

        case LAYERS_CEIL_LAYER:
            hbr_active_bg = hbr_ceil_bg;
            hp_active_grid = hp_ceil_grid;
            hp_active_dotted = hp_ceil_dotted;
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            hbr_active_bg = hbr_lightmaps_bg;
            hp_active_grid = hp_lightmaps_grid;
            hp_active_dotted = hp_lightmaps_dotted;
            break;
    }
}


void Map_Draw_BG(HDC _hdc)
{
    FillRect(_hdc, &map_rc, hbr_active_bg);
}

void Map_Draw_Grid(HDC _hdc)
{
    float f_world_left, f_world_top, f_world_right, f_world_bottom;
    Screen_To_World(0, 0, f_world_left, f_world_top);
    Screen_To_World(MAP_WINDOW_SIZE, MAP_WINDOW_SIZE, f_world_right, f_world_bottom);

    SelectObject(_hdc, hp_active_grid);

    for (float i = 0.0f; i <= MAP_LENGTH; i++)
    {
        if (i >= f_world_top && i <= f_world_bottom)
        {
            int pixel_sx, pixel_ex, pixel_sy, pixel_ey;

            World_To_Screen(0.0f, i, pixel_sx, pixel_sy);
            World_To_Screen(MAP_LENGTH, i, pixel_ex, pixel_ey);

            MoveToEx(_hdc, pixel_sx, pixel_sy, NULL);
            LineTo(_hdc, pixel_ex, pixel_ey);
        }

        if (i >= f_world_left && i <= f_world_right)
        {
            int pixel_sx, pixel_ex, pixel_sy, pixel_ey;

            World_To_Screen(i, 0.0f, pixel_sx, pixel_sy);
            World_To_Screen(i, MAP_LENGTH, pixel_ex, pixel_ey);

            MoveToEx(_hdc, pixel_sx, pixel_sy, NULL);
            LineTo(_hdc, pixel_ex, pixel_ey);
        }
    }
}

void Map_Draw_Selected_Cell(HDC _hdc)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)selected_cell_x, (float)selected_cell_y, cx0, cy0);
    World_To_Screen((float)selected_cell_x + 1.0f, (float)selected_cell_y + 1.0f, cx1, cy1);

    RECT tmp_cell = { cx0, cy0, cx1, cy1 };

    FrameRect(_hdc, &tmp_cell, hbr_selected_cell);
}

void Map_Draw_Layer_Info(HDC _hdc)
{
    SetTextColor(_hdc, RGB(255, 255, 255));
    SetBkMode(_hdc, TRANSPARENT);

    DrawText(_hdc, map_text_layers[gui_current_layer], -1, &map_rc_text_layer, DT_SINGLELINE | DT_NOCLIP);
    DrawText(_hdc, map_text_texture_mode[gui_texprew_mode], -1, &map_rc_text_mode, DT_SINGLELINE | DT_NOCLIP);
}


void Map_Draw_Textured_Square(HDC _hdc, int _cx0, int _cy0, int _cx1, int _cy1, std::list<s_Texture>* _tex, int _id)
{
    int found = 0;

    std::list<s_Texture>::iterator tex_iterator;
    s_Texture tmp_texture;

    for (tex_iterator = _tex->begin(); tex_iterator != _tex->end(); ++tex_iterator)
    {
        tmp_texture = *tex_iterator;

        if (_id == tmp_texture.id)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        HDC hdc_tmp = CreateCompatibleDC(_hdc);
        SelectObject(hdc_tmp, tmp_texture.bitmap_128);

        SetStretchBltMode(_hdc, COLORONCOLOR);
        StretchBlt(_hdc, _cx0, _cy0, _cx1 - _cx0, _cy1 - _cy0, hdc_tmp, 0, 0, 128, 128, SRCCOPY);

        DeleteDC(hdc_tmp);
    }
    else
        TEX_Draw_Not_Found_Texture(_hdc, _cx0, _cy0, _cx1, _cy1);
}

void Map_Draw_Player(HDC _hdc)
{
    // draw player
    int px0, py0, px1, py1, px2, py2;

    SelectObject(_hdc, hbr_map_player);
    SelectObject(_hdc, hp_none);

    // player_starting_angle is kept as 1 signed byte value divided by 3
    switch (APP_level_file.player_starting_angle)
    {
    case 0:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 0.5f, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 30:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 1.0f, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 0.5f, (float)APP_level_file.player_starting_cell_y, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 60:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 0.5f, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 90:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 0.5f, (float)APP_level_file.player_starting_cell_y + 1.0f, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;
    }
}


void Map_Draw_Objects(HDC _hdc)
{
    float f_world_left, f_world_top, f_world_right, f_world_bottom;
    Screen_To_World(0, 0, f_world_left, f_world_top);
    Screen_To_World(MAP_WINDOW_SIZE, MAP_WINDOW_SIZE, f_world_right, f_world_bottom);

    for (int y = 0; y < MAP_LENGTH; y++)
    {
        // crop unseen
        if (y >= f_world_top - 1 && y <= f_world_bottom)
        {
            for (int x = 0; x < MAP_LENGTH; x++)
            {
                // cropp unseen
                if (x >= f_world_left - 1 && x <= f_world_right)
                {
                    int index = x + y * MAP_LENGTH;

                    // drawing depends on active layer type
                    switch (gui_current_layer)
                    {
                        case LAYERS_MAIN_LAYER:
                            if (APP_level_file.map[index].is_cell_solid)
                            {
                                switch (APP_level_file.map[index].cell_type)
                                {
                                    case LV_C_WALL_STANDARD:
                                        Map_Draw_Objects_Walls_Standard(_hdc, x, y, index);
                                        break;

                                    case LV_C_WALL_FOURSIDE:
                                        Map_Draw_Objects_Walls_Fourside(_hdc, x, y, index);
                                        break;
                                }
                            }
                        break;

                        case LAYERS_FLOOR_LAYER:
                            Map_Draw_Objects_Floor(_hdc, x, y, index);

                            if (APP_level_file.map[index].is_cell_solid)
                                switch (APP_level_file.map[index].cell_type)
                                {
                                    case LV_C_WALL_STANDARD:
                                    case LV_C_WALL_FOURSIDE:
                                        Map_Draw_Objects_Dotted_Rectangle(_hdc, (float)x + 0.04f, (float)y + 0.04f, (float)x + 0.96f, (float)y + 0.96f);
                                        break;
                                }
                            break;

                        case LAYERS_CEIL_LAYER:
                            Map_Draw_Objects_Ceil(_hdc, x, y, index);

                            if (APP_level_file.map[index].is_cell_solid)
                                switch (APP_level_file.map[index].cell_type)
                                {
                                    case LV_C_WALL_STANDARD:
                                    case LV_C_WALL_FOURSIDE:
                                        Map_Draw_Objects_Dotted_Rectangle(_hdc, (float)x + 0.04f, (float)y + 0.04f, (float)x + 0.96f, (float)y + 0.96f);
                                        break;
                                }
                            break;

                        case LAYERS_LIGHTMAPS_LAYER:
                            Map_Draw_Objects_Lightmaps(_hdc, x, y, index);

                            if (APP_level_file.map[index].is_cell_solid)
                                switch (APP_level_file.map[index].cell_type)
                                {
                                    case LV_C_WALL_STANDARD:
                                    case LV_C_WALL_FOURSIDE:
                                        Map_Draw_Objects_Dotted_Rectangle(_hdc, (float)x + 0.04f, (float)y + 0.04f, (float)x + 0.96f, (float)y + 0.96f);
                                        break;
                                }
                            break;
                    }
                }
            }
        }
    }
}

void Map_Draw_Objects_Walls_Standard(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    if (gui_texprew_mode)
    {
        int id = APP_level_file.map[_index].wall_texture_id[id_top];
        Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_wall_list, id);
    }
    else
    {
        RECT r = { cx0, cy0, cx1, cy1 };
        FillRect(_hdc, &r, hbr_map_walls_standard);
    }
}

void Map_Draw_Objects_Walls_Fourside(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    if (gui_texprew_mode)
    {
        int sx0, sy0;
        World_To_Screen((float)_mx + 0.5f, (float)_my + 0.5f, sx0, sy0);

        POINT vertices[] = { {cx0, cy0}, {sx0, sy0}, {cx1, cy0}, {cx1, cy1}, {cx0, cy1} };
        POINT vertices2[] = { {0, 0}, {128, 0}, {128 / 2, 128 / 2}, {128, 128}, {0, 128} };
        POINT vertices3[] = { {0, 0}, {128, 0}, {128, 128}, {128 / 2, 128 / 2}, {0, 128} };
        POINT vertices4[] = { {0, 0}, {128, 0}, {128, 128}, {0, 128}, {128 / 2, 128 / 2} };

        int size = sizeof(vertices) / sizeof(vertices[0]);

        int cx1_cx0 = cx1 - cx0;
        int cy1_cy0 = cy1 - cy0;

        // draw top
        Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_wall_list, APP_level_file.map[_index].wall_texture_id[id_top]);

        SelectObject(_hdc, hbr_black);
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));

        // draw rest slices - must be masked with black    
        HDC hdc_tmp_2 = CreateCompatibleDC(_hdc);
        HBITMAP btm_tmp = CreateCompatibleBitmap(_hdc, 128, 128);

        SelectObject(hdc_tmp_2, btm_tmp);

            // draw right slice
            Map_Draw_Textured_Square(hdc_tmp_2, 0, 0, 128, 128, &TEX_wall_list, APP_level_file.map[_index].wall_texture_id[id_right]);
            SelectObject(hdc_tmp_2, hbr_black);
            Polygon(hdc_tmp_2, vertices2, size);        
            StretchBlt(_hdc, cx0, cy0, cx1_cx0, cy1_cy0, hdc_tmp_2, 0, 0, 128, 128, SRCPAINT);

            // draw bottom slice
            Map_Draw_Textured_Square(hdc_tmp_2, 0, 0, 128, 128, &TEX_wall_list, APP_level_file.map[_index].wall_texture_id[id_bottom]);
            SelectObject(hdc_tmp_2, hbr_black); 
            Polygon(hdc_tmp_2, vertices3, size);
            StretchBlt(_hdc, cx0, cy0, cx1_cx0, cy1_cy0, hdc_tmp_2, 0, 0, 128, 128, SRCPAINT);

            // draw left slice
            Map_Draw_Textured_Square(hdc_tmp_2, 0, 0, 128, 128, &TEX_wall_list, APP_level_file.map[_index].wall_texture_id[id_left]);
            SelectObject(hdc_tmp_2, hbr_black);
            Polygon(hdc_tmp_2, vertices4, size);
            StretchBlt(_hdc, cx0, cy0, cx1_cx0, cy1_cy0, hdc_tmp_2, 0, 0, 128, 128, SRCPAINT);

        DeleteObject(btm_tmp);
        DeleteDC(hdc_tmp_2);
    }
    else
    {
        RECT r = { cx0, cy0, cx1, cy1 };
        FillRect(_hdc, &r, hbr_map_walls_fourside_01);

        POINT vertices[] = { {cx0, cy0}, {cx1, cy1}, {cx1, cy0}, {cx0, cy1} };
        SelectObject(_hdc, hbr_map_walls_fourside_02);
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
}

void Map_Draw_Objects_Floor(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    int id = APP_level_file.map[_index].flat_texture_id[id_floor];

    if (id >= 0)
    {
        if (gui_texprew_mode)
            Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_flat_list, id);
        else
        {
                RECT r = { cx0, cy0, cx1, cy1 };
                FillRect(_hdc, &r, hbr_map_floor);
        }
    }
}

void Map_Draw_Objects_Ceil(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    int id = APP_level_file.map[_index].flat_texture_id[id_ceil];

    if (id >= 0)
    {
        if (gui_texprew_mode)
            Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_flat_list, id);
        else
        {
            RECT r = { cx0, cy0, cx1, cy1 };
            FillRect(_hdc, &r, hbr_map_ceil);
        }
    }
}

void Map_Draw_Objects_Lightmaps(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    if (APP_level_file.map[_index].is_lightmapped != 0)
    {
        RECT r = { cx0, cy0, cx1, cy1 };
        FillRect(_hdc, &r, hbr_map_lightmaps);
    }
}

void Map_Draw_Objects_Dotted_Rectangle(HDC _hdc, float _x0, float _y0, float _x1, float _y1)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_x0, (float)_y0, cx0, cy0);
    World_To_Screen((float)_x1, (float)_y1, cx1, cy1);

    SelectObject(_hdc, hp_active_dotted);

    SetBkMode(_hdc, TRANSPARENT);

    MoveToEx(_hdc, cx0, cy0, NULL);
    LineTo(_hdc, cx1, cy0);
    LineTo(_hdc, cx1, cy1);
    LineTo(_hdc, cx0, cy1);
    LineTo(_hdc, cx0, cy0);
}


void Map_Add_Current_Object()
{
    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_MAIN_PLAYER:
                    APP_level_file.player_starting_cell_x = selected_cell_x;
                    APP_level_file.player_starting_cell_y = selected_cell_y;
                    break;

                case LIST_MAIN_WALL_STANDARD:
                case LIST_MAIN_WALL_FOURSIDE:
                    if (selected_cell_x != APP_level_file.player_starting_cell_x || selected_cell_y != APP_level_file.player_starting_cell_y)
                    {
                        if (gui_current_list_index == LIST_MAIN_WALL_STANDARD) APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_STANDARD;
                        if (gui_current_list_index == LIST_MAIN_WALL_FOURSIDE) APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_FOURSIDE;
                        
                        APP_level_file.map[selected_cell_index].is_cell_solid = true;

                        APP_level_file.map[selected_cell_index].wall_texture_id[id_top] = MAP_working_cell.wall_texture_id[id_top];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_right] = MAP_working_cell.wall_texture_id[id_right];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom] = MAP_working_cell.wall_texture_id[id_bottom];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_left] = MAP_working_cell.wall_texture_id[id_left];
                    }
                    break;
            }
            break;

        case LAYERS_FLOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_FLOOR: 
                    APP_level_file.map[selected_cell_index].flat_texture_id[id_floor] = MAP_working_cell.flat_texture_id[id_floor]; 
                    break;
            }
            break;

        case LAYERS_CEIL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_FLOOR:
                    APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil] = MAP_working_cell.flat_texture_id[id_ceil];
                    break;
            }
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_LIGHTMAPS_SELECT:
                    APP_level_file.map[selected_cell_index].is_lightmapped = 1;
                    break;
            }
            break;
    }
}

void Map_Read_Current_Cell()
{
    if (selected_cell_x == APP_level_file.player_starting_cell_x && selected_cell_y == APP_level_file.player_starting_cell_y)
    {
        gui_current_layer = LAYERS_MAIN_LAYER;
        gui_current_list_index = 0;
    }
    else
    {
        switch (gui_current_layer)
        {
            case LAYERS_MAIN_LAYER:
                if (APP_level_file.map[selected_cell_index].is_cell_solid)
                {
                    switch (APP_level_file.map[selected_cell_index].cell_type)
                    {
                        case LV_C_WALL_STANDARD:
                            gui_current_layer = LAYERS_MAIN_LAYER;
                            gui_current_list_index = LIST_MAIN_WALL_STANDARD;
                            break;

                        case LV_C_WALL_FOURSIDE:
                            gui_current_layer = LAYERS_MAIN_LAYER;
                            gui_current_list_index = LIST_MAIN_WALL_FOURSIDE;
                            break;
                    }
                }
                else
                {
                    // if cell is empty nothing in the list box should be highlighted or selected
                    gui_current_list_index = -1;
                }
                break;

            case LAYERS_FLOOR_LAYER:
                gui_current_layer = LAYERS_FLOOR_LAYER;
                gui_current_list_index = LIST_FLOOR;
                break;

            case LAYERS_CEIL_LAYER:
                gui_current_layer = LAYERS_CEIL_LAYER;
                gui_current_list_index = LIST_CEIL;
                break;
        }

        // copy selected cell from map to working cell
        MAP_working_cell.is_cell_solid = APP_level_file.map[selected_cell_index].is_cell_solid;
        MAP_working_cell.cell_type = APP_level_file.map[selected_cell_index].cell_type;        

        MAP_working_cell.wall_texture_id[id_top] = APP_level_file.map[selected_cell_index].wall_texture_id[id_top];
        MAP_working_cell.wall_texture_id[id_right] = APP_level_file.map[selected_cell_index].wall_texture_id[id_right];
        MAP_working_cell.wall_texture_id[id_bottom] = APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom];
        MAP_working_cell.wall_texture_id[id_left] = APP_level_file.map[selected_cell_index].wall_texture_id[id_left];

        MAP_working_cell.flat_texture_id[id_floor] = APP_level_file.map[selected_cell_index].flat_texture_id[id_floor];
        MAP_working_cell.flat_texture_id[id_ceil] = APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil];
    }
}

void Map_Clear_Current_Cell()
{
    switch (gui_current_layer)
    {
        case LAYERS_MAIN_LAYER:
            APP_level_file.map[hover_cell_index].is_cell_solid = false;
            APP_level_file.map[hover_cell_index].cell_type = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_top] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_right] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_bottom] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_left] = -1;
            break;

        case LAYERS_FLOOR_LAYER:
            APP_level_file.map[hover_cell_index].flat_texture_id[id_floor] = -1;
            break;

        case LAYERS_CEIL_LAYER:
            APP_level_file.map[hover_cell_index].flat_texture_id[id_ceil] = -1;
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_LIGHTMAPS_SELECT:
                    APP_level_file.map[selected_cell_index].is_lightmapped = 0;
                    break;
            }
            break;
    }
}

// ---------------------------------------------
// --- TEXTURE CONTROL FUNCTIONS DEFINITIONS ---
// ---------------------------------------------


void TEX_Reset()
{
    TEX_wall_current_id = 0;
    TEX_flat_current_id = 0;

    // There must be always at least one texture (default) for wall and flat.
    TEX_wall_count = 1;
    TEX_flat_count = 1;

    TEX_Reset_Texture_List(&TEX_wall_list);
    TEX_Reset_Texture_List(&TEX_flat_list);
}

bool TEX_List_Sort_Function(const s_Texture& a, const s_Texture& b)
{
    return (wcscmp(a.filename, b.filename) < 0);
}

void TEX_Prepare_List_To_Save(std::list<s_Texture>* _list, unsigned char* _count, int _mode)
{
    // clear new id in the list
    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); ++tex_iterator)
        (*tex_iterator).new_id = -1;

    // update list
    int counter = 0;

    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); tex_iterator++)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if (_count[counter] <= 0)
        {
            DeleteObject((*tex_iterator).bitmap_128);
            tex_iterator = _list->erase(tex_iterator);

            if (!_list->empty())
            {
                if (tex_iterator != _list->begin())
                    tex_iterator--;
            }
            else
                break;
        }

        counter++;
    }

    // first sort by name - then add new id
    _list->sort(TEX_List_Sort_Function);

    int new_id = 0;

    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); tex_iterator++)
    {
        (*tex_iterator).new_id = new_id;

        // convert TCHAR to char and add that texture filename to list in file structure
        if (_mode == 0) wcstombs_s(NULL, APP_level_file.wall_texture_filename[new_id], (*tex_iterator).filename, wcslen((*tex_iterator).filename) + 1);
        else            wcstombs_s(NULL, APP_level_file.flat_texture_filename[new_id], (*tex_iterator).filename, wcslen((*tex_iterator).filename) + 1);

        new_id++;
    }
}

int TEX_Add_Texture_To_List(std::list<s_Texture>* _list, int _mode)
{
    // check if new selected texture is already on list
    int tmp_id = -1;
    int found = 0;

    std::list<s_Texture>::iterator tex_iterator;

    for (tex_iterator = _list->begin(); tex_iterator != _list->end(); ++tex_iterator)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if (wcscmp(TEX_selected_file_name, tmp_texture.filename) == 0)
        {
            tmp_id = tmp_texture.id;
            found = 1;
            break;
        }
    }

    if (found)
    {
        if (TEX_prev_id == tmp_id)
        {
            // if prev texture and new selected have the same IDs do nothing, just return the same ID
            return tmp_id;
        }
    }
    else
    {
            // if texture not found on the list - load it and add it - return the new id
            s_Texture tmp_texture;

            if (_mode == 0) tmp_texture.id = TEX_wall_current_id;
            else            tmp_texture.id = TEX_flat_current_id;

            tmp_texture.new_id = -1;
            wcscpy(tmp_texture.filename, TEX_selected_file_name);

            // Load raw texture and convert it to windows HBITMAP
            TCHAR path_file_name[256];
            ZeroMemory(path_file_name, sizeof(path_file_name));

            if (_mode == 0) wcscpy(path_file_name, TEXT(APP_WALL_TEXTURES_DIRECTORY));
            else            wcscpy(path_file_name, TEXT(APP_FLAT_TEXTURES_DIRECTORY));

            wcscat(path_file_name, L"\\");
            wcscat(path_file_name, TEX_selected_file_name);

            // convert TCHAR path_filename to CHAR path_filename
            ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
            wcstombs_s(NULL, cnv_input_bitmap_filename, path_file_name, wcslen(path_file_name) + 1);

            sIO_Prefs prefs;
            prefs.ch1 = 8;
            prefs.ch2 = 16;
            prefs.ch3 = 24;

            // Alloc memory for raw texture when loaded.
            u_int8*  raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            u_int32* raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            // restore to default directory
            SetCurrentDirectory(APP_curr_dir);

            int read_raw_result = BM_Read_Texture_RAW(cnv_input_bitmap_filename, raw_table, raw_index, &prefs);

            if (read_raw_result <= 0)
                MessageBox(GetActiveWindow(), L"Can't open that file.", L"Error.", MB_ICONWARNING | MB_OK);
            else
            {
                tmp_texture.bitmap_128 = Make_HBITMAP_From_Bitmap_Indexed(  GetActiveWindow(), 128, 128, IO_TEXTURE_MAX_COLORS,
                                                                                raw_table + IO_TEXTURE_MAX_COLORS * (IO_TEXTURE_MAX_COLORS - 1), raw_index + 256 * 256);
            }

            free(raw_index);
            free(raw_table);

            _list->push_back(tmp_texture);

            if (_mode == 0)
            {
                tmp_id = TEX_wall_current_id;
                TEX_wall_current_id++;
            }
            else
            {
                tmp_id = TEX_flat_current_id;
                TEX_flat_current_id++;
            }
    }

    return tmp_id;
}

void TEX_Reset_Texture_List(std::list<s_Texture>* _list)
{
    std::list<s_Texture>::iterator tex_iterator;

    for (tex_iterator = _list->begin(); tex_iterator != _list->end(); ++tex_iterator)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        DeleteObject(tmp_texture.bitmap_128);
    }

    _list->clear();
}

void TEX_Count_Textures()
{
    ZeroMemory(TEX_wall_table, sizeof(TEX_wall_table));
    ZeroMemory(TEX_flat_table, sizeof(TEX_flat_table));

    TEX_wall_is_default = 0;

    TEX_wall_mem = 0; 
    TEX_flat_mem = 0;

    TEX_wall_count = 0;
    TEX_flat_count = 0;    

    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
        if (APP_level_file.map[i].is_cell_solid)
        {
            switch (APP_level_file.map[i].cell_type)
            {
                case LV_C_WALL_STANDARD:
                    if      (APP_level_file.map[i].wall_texture_id[id_top] >= 0) TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_top]]++;
                    else    TEX_wall_is_default = 1;

                    break;

                case LV_C_WALL_FOURSIDE:
                    if      (APP_level_file.map[i].wall_texture_id[id_top] >= 0) TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_top]]++;
                    else    TEX_wall_is_default = 1;

                    if      (APP_level_file.map[i].wall_texture_id[id_right] >= 0) TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_right]]++;
                    else    TEX_wall_is_default = 1;

                    if      (APP_level_file.map[i].wall_texture_id[id_bottom] >= 0) TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_bottom]]++;
                    else    TEX_wall_is_default = 1;

                    if      (APP_level_file.map[i].wall_texture_id[id_left] >= 0) TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_left]]++;
                    else    TEX_wall_is_default = 1;

                    break;
            }
        }

        if (APP_level_file.map[i].flat_texture_id[id_floor] >= 0) TEX_flat_table[APP_level_file.map[i].flat_texture_id[id_floor]]++;
        if (APP_level_file.map[i].flat_texture_id[id_ceil] >= 0) TEX_flat_table[APP_level_file.map[i].flat_texture_id[id_ceil]]++;
    }

    for (int p = 0; p < 512; p++)
    {
        if (TEX_wall_table[p] > 0)  TEX_wall_count++;
        if (TEX_flat_table[p] > 0)  TEX_flat_count++;
    }

    if (TEX_wall_is_default)    TEX_wall_count++;

    // There is always at least one texture (default) for wall and flat.
    if (TEX_wall_count == 0) TEX_wall_count = 1;
    if (TEX_flat_count == 0) TEX_flat_count = 1;

    TEX_wall_mem = TEX_wall_count * (IO_TEXTURE_IMAGE_DATA_BYTE_SIZE + IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
    TEX_flat_mem = TEX_flat_count * (IO_TEXTURE_IMAGE_DATA_BYTE_SIZE + IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
}

void TEX_Draw_Not_Found_Texture(HDC _hdc, int _cx0, int _cy0, int _cx1, int _cy1)
{
    RECT r = { _cx0, _cy0, _cx1, _cy1 };

    HPEN old = (HPEN)SelectObject(_hdc, hp_map_not_found_texture);

    FillRect(_hdc, &r, hbr_map_not_found_texture);

    MoveToEx(_hdc, _cx0, _cy0, NULL);
    LineTo(_hdc, _cx1, _cy1);
    MoveToEx(_hdc, _cx1, _cy0, NULL);
    LineTo(_hdc, _cx0, _cy1);

    SelectObject(_hdc, old);
}


// ------------------------------------
// --- HELPER FUNCTIONS DEFINITIONS ---
// ------------------------------------


void    World_To_Screen(float f_x_world, float f_y_world, int& i_x_screen, int& i_y_screen)
{
    i_x_screen = (int)((f_x_world - f_offset_x) * f_scale_x);
    i_y_screen = (int)((f_y_world - f_offset_y) * f_scale_y);
}

void    Screen_To_World(int i_x_screen, int i_y_screen, float& f_x_world, float& f_y_world)
{
    f_x_world = ((float)(i_x_screen) / f_scale_x) + f_offset_x;
    f_y_world = ((float)(i_y_screen) / f_scale_y) + f_offset_y;
}

void    Add_Number_Spaces(int _number, wchar_t* _output_string)
{
    wchar_t tmp[16];
    ZeroMemory(tmp, sizeof(tmp));

    wsprintf(tmp, L"%ld", _number);

    int length = wcslen(tmp);
    int spaces = length / 3;
    int index = length - 1 + spaces;
    int counter = 0;

    for (int i = length-1; i >= 0; i--)
    {
        _output_string[index] = tmp[i];
        index--;

        counter++;
        if (counter == 3)
        {
            counter = 0;
            _output_string[index] = L' ';
            index--;
        }
    }
}

void    Center_Window(HWND _hwnd)
{
    RECT rc;

    GetWindowRect(_hwnd, &rc);

    int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

    SetWindowPos(_hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

HBITMAP Make_HBITMAP_From_Bitmap_Indexed(HWND _hwnd, int _width, int _height, int _num_colors, u_int32* _color_data_ptr, u_int8* _image_data_ptr)
{
    // window bitamp structure - to use with window API
    struct sAPP_Windows_Bitmap
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[256];
    };

    sAPP_Windows_Bitmap tmp_w_bitmap;

    tmp_w_bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    tmp_w_bitmap.bmiHeader.biWidth = _width;
    tmp_w_bitmap.bmiHeader.biHeight = -1 * _height;
    tmp_w_bitmap.bmiHeader.biPlanes = 1;
    tmp_w_bitmap.bmiHeader.biBitCount = 8;
    tmp_w_bitmap.bmiHeader.biCompression = BI_RGB;
    tmp_w_bitmap.bmiHeader.biSizeImage = 0;
    tmp_w_bitmap.bmiHeader.biXPelsPerMeter = 0;
    tmp_w_bitmap.bmiHeader.biYPelsPerMeter = 0;
    tmp_w_bitmap.bmiHeader.biClrUsed = _num_colors;
    tmp_w_bitmap.bmiHeader.biClrImportant = _num_colors;

    memcpy(tmp_w_bitmap.bmiColors, _color_data_ptr, _num_colors * sizeof(int));

    HDC tmp_hdc = GetDC(_hwnd);

    HBITMAP result_hbm = CreateDIBitmap(tmp_hdc, &tmp_w_bitmap.bmiHeader, CBM_INIT, _image_data_ptr, (BITMAPINFO*)&tmp_w_bitmap, DIB_RGB_COLORS);

    ReleaseDC(_hwnd, tmp_hdc);

    return result_hbm;
}

HBITMAP Make_HBITMAP_From_Bitmap_Indexed_Grayscale(HWND _hwnd, int _width, int _height, u_int8* _image_data_ptr)
{
    // window bitamp structure - to use with window API
    struct sAPP_Windows_Bitmap
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[256];
    };

    sAPP_Windows_Bitmap tmp_w_bitmap;

    tmp_w_bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    tmp_w_bitmap.bmiHeader.biWidth = _width;
    tmp_w_bitmap.bmiHeader.biHeight = -1 * _height;
    tmp_w_bitmap.bmiHeader.biPlanes = 1;
    tmp_w_bitmap.bmiHeader.biBitCount = 8;
    tmp_w_bitmap.bmiHeader.biCompression = BI_RGB;
    tmp_w_bitmap.bmiHeader.biSizeImage = 0;
    tmp_w_bitmap.bmiHeader.biXPelsPerMeter = 0;
    tmp_w_bitmap.bmiHeader.biYPelsPerMeter = 0;
    tmp_w_bitmap.bmiHeader.biClrUsed = 256;
    tmp_w_bitmap.bmiHeader.biClrImportant = 256;

    for (int i = 0; i < 256; i++)
    {
        tmp_w_bitmap.bmiColors[i].rgbBlue = i;
        tmp_w_bitmap.bmiColors[i].rgbGreen = i;
        tmp_w_bitmap.bmiColors[i].rgbRed = i;
    }

    HDC tmp_hdc = GetDC(_hwnd);

    HBITMAP result_hbm = CreateDIBitmap(tmp_hdc, &tmp_w_bitmap.bmiHeader, CBM_INIT, _image_data_ptr, (BITMAPINFO*)&tmp_w_bitmap, DIB_RGB_COLORS);

    ReleaseDC(_hwnd, tmp_hdc);

    return result_hbm;
}