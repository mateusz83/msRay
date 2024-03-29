﻿#include "main.h"

// ---------------------------------
// --- APP FUNCTIONS DEFINITIONS ---
// ---------------------------------


int APIENTRY        wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
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
ATOM                APP_MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };
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
BOOL                APP_InitInstance(HINSTANCE hInstance, int nCmdShow)
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

LRESULT CALLBACK    APP_WndProc(HWND _hwnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
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

            // ALL COMBOS AND LISTS.
            switch (wm_event)
            {
                case CBN_SELCHANGE:
                    switch (wm_id)
                    {
                        case IDL_LAYERS_LIST:
                            gui_current_layer = SendMessage((HWND)_lParam, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                            gui_current_list_index = 0;

                            GUI_Update_List_Box(_hwnd);
                            GUI_Update_Properties(_hwnd);
                            Map_Update_Colors();
                            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                            SetFocus(_hwnd);
                            break;

                        case IDL_LIST_LIST:
                            gui_current_list_index = SendMessage((HWND)_lParam, (UINT)LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                            GUI_Update_Properties(_hwnd);
                            SetFocus(_hwnd);
                            break;

                        case IDC_PROP_GROUP_COMBO:
                        case IDC_PROP_TIMER_COMBO:
                        case IDC_PROP_ACTION_COMBO:
                            GUI_Update_Properties_Values(_hwnd, wm_id);
                            SetFocus(_hwnd);
                            break;
                    }
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
                    break;

                case IDB_RES_FILE_REFRESH:
                    GUI_Update_Resources_File(_hwnd);
                    SetFocus(_hwnd);
                    break;

                case IDB_RES_MEM_REFRESH:
                    APP_Count_or_Export_Lightmaps(1);
                    TEX_Count_Textures();
                    GUI_Update_Resources_Memory(_hwnd);
                    SetFocus(_hwnd);
                    break;


                // --- properties ---
 

                    // player
                    case IDB_PROP_PLAYER_RIGHT:     
                        APP_level_file.player_starting_angle = 90; // for 270*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;

                    case IDB_PROP_PLAYER_UP:    
                        APP_level_file.player_starting_angle = 0;  // for 0*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;

                    case IDB_PROP_PLAYER_LEFT: 
                        APP_level_file.player_starting_angle = 30; // for 90*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;

                    case IDB_PROP_PLAYER_DOWN:      
                        APP_level_file.player_starting_angle = 60; // for 180*
                        GUI_Update_Properties_Player(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;
                        
                    // select top, right, bottom, left texture
                    case IDB_PROP_WALL_IMAGE_TOP_SELECT:
                    case IDB_PROP_WALL_IMAGE_RIGHT_SELECT:
                    case IDB_PROP_WALL_IMAGE_BOTTOM_SELECT:
                    case IDB_PROP_WALL_IMAGE_LEFT_SELECT:

                        gui_selected_image = wm_id;
                        DialogBox(APP_hInst, MAKEINTRESOURCE(IDD_SELECT_TEXTURE), _hwnd, APP_Wnd_Select_Texture_Proc);

                        TEX_Count_Textures();
                        GUI_Update_Resources_Memory(_hwnd);
                        GUI_Update_Properties(_hwnd);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;

                    // for non regular walls drawing:
                    case IDS_PROP_WALL_DRAWING_PLUS1:
                    case IDS_PROP_WALL_DRAWING_MINUS1:
                    case IDS_PROP_WALL_DRAWING_PLUS2:
                    case IDS_PROP_WALL_DRAWING_MINUS2:
                    case IDS_PROP_WALL_DRAWING_SLASH:
                    case IDS_PROP_WALL_DRAWING_BSLASH:

                    case IDS_PROP_WALL_DRAWING_HOR_UP:	
                    case IDS_PROP_WALL_DRAWING_HOR_TURN:
                    case IDS_PROP_WALL_DRAWING_HOR_DOWN:	

                    case IDS_PROP_WALL_DRAWING_VER_LEFT:	
                    case IDS_PROP_WALL_DRAWING_VER_TURN:
                    case IDS_PROP_WALL_DRAWING_VER_RIGHT:	

                    case IDS_PROP_WALL_DRAWING_UP_1:		
                    case IDS_PROP_WALL_DRAWING_UP_2:	
                    case IDS_PROP_WALL_DRAWING_RIGHT_1:
                    case IDS_PROP_WALL_DRAWING_RIGHT_2:	
                    case IDS_PROP_WALL_DRAWING_BOTTOM_1:	
                    case IDS_PROP_WALL_DRAWING_BOTTOM_2:	
                    case IDS_PROP_WALL_DRAWING_LEFT_1:	
                    case IDS_PROP_WALL_DRAWING_LEFT_2:	
                    case IDS_PROP_WALL_DRAWING_OBL_TURN:

                        GUI_Update_Properties_Drawing_Window(_hwnd, wm_id);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
                        break;

                    // for walls height drawing:
                    case IDB_PROP_WALL_HEIGHT_SH_UP:
                    case IDB_PROP_WALL_HEIGHT_SH_DOWN:
                    case IDB_PROP_WALL_HEIGHT_H_UP:	
                    case IDB_PROP_WALL_HEIGHT_H_DOWN:	

                        GUI_Update_Properties_Drawing_Height(_hwnd, wm_id);
                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        SetFocus(_hwnd);
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
                        Map_Update_Colors();

                        RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                        break;

                    case ID_FILE_OPEN:
                        if (APP_Open())
                        {
                            TEX_Count_Textures();
                            APP_Count_or_Export_Lightmaps(1);
                            GUI_Reset(_hwnd);

                            GUI_Update_List_Box(_hwnd);
                            GUI_Update_Properties(_hwnd);
                            Map_Update_Colors();

                            RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
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

        // for LEFT ALT-KEY
        case WM_SYSKEYDOWN:
            if (HIWORD(_lParam) && KF_ALTDOWN)
            {
                if (!APP_control_pressed)
                    APP_alt_pressed = true;

                RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            }
            break;

        case WM_SYSKEYUP:
            if (HIWORD(_lParam) && KF_ALTDOWN)
            {
                    APP_alt_pressed = false;

                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
            }
            break;

        case WM_KEYDOWN:
            switch ((UINT)_wParam)
            {
                case VK_CONTROL:
                    APP_control_pressed = true;
                    APP_alt_pressed = false;
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;
            }
            break;

         case WM_KEYUP:
         {
             switch ((UINT)_wParam)
             {
                 case VK_CONTROL:
                     APP_control_pressed = false;
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case VK_SPACE:
                     gui_texprew_mode = !gui_texprew_mode;
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case '1':
                     gui_current_layer = LAYERS_PLAYER_LAYER;
                     gui_current_list_index = 0;

                     GUI_Update_List_Box(_hwnd);
                     GUI_Update_Properties(_hwnd);
                     Map_Update_Colors();
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case '2':
                     gui_current_layer = LAYERS_WALL_LAYER;
                     gui_current_list_index = 0;

                     GUI_Update_List_Box(_hwnd);
                     GUI_Update_Properties(_hwnd);
                     Map_Update_Colors();
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case '3':
                     gui_current_layer = LAYERS_FLOOR_LAYER;
                     gui_current_list_index = 0;

                     GUI_Update_List_Box(_hwnd);
                     GUI_Update_Properties(_hwnd);
                     Map_Update_Colors();
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case '4':
                     gui_current_layer = LAYERS_CEIL_LAYER;
                     gui_current_list_index = 0;

                     GUI_Update_List_Box(_hwnd);
                     GUI_Update_Properties(_hwnd);
                     Map_Update_Colors();
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;

                 case '5':
                     gui_current_layer = LAYERS_LIGHTMAPS_LAYER;
                     gui_current_list_index = 0;

                     GUI_Update_List_Box(_hwnd);
                     GUI_Update_Properties(_hwnd);
                     Map_Update_Colors();
                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                     break;
             }
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
                         Map_Add_Current_Object();
                       
                     }
                     else if (APP_alt_pressed)
                     {
                         Map_Clear_Current_Cell();
                     }

                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                 }
                 else if (APP_is_middle_hold)
                 {
                     f_offset_x -= (APP_f_mouse_x - f_start_pan_x) / f_scale_x;
                     f_offset_y -= (APP_f_mouse_y - f_start_pan_y) / f_scale_y;

                     f_start_pan_x = APP_f_mouse_x;
                     f_start_pan_y = APP_f_mouse_y;

                     RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                 }
                 else
                 {
                     if (APP_control_pressed || APP_alt_pressed)
                         RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                 }
             }

             GUI_Update_Map_Coords(is_map);
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
                    Map_Add_Current_Object();
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                }
                else if (APP_alt_pressed)
                {
                    Map_Clear_Current_Cell();   
                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                }
                else
                {
                    Map_Read_Current_Cell();

                    GUI_Update_Layers_Combo(_hwnd);
                    GUI_Update_List_Box(_hwnd);
                    GUI_Update_Properties(_hwnd);

                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                }

                GUI_Update_Map_Coords(is_map);
            }
        }
        break;

        case WM_LBUTTONUP:
        {
            APP_is_left_hold = false;

            bool is_map = GUI_is_Map_Region();

            if (is_map)
            {
                TEX_Count_Textures();
                GUI_Update_Resources_Memory(_hwnd);
            }

            break;
        }

        case WM_MBUTTONDOWN:
            APP_is_middle_hold = true;
            f_start_pan_x = APP_f_mouse_x;
            f_start_pan_y = APP_f_mouse_y;
            break;

        case WM_MBUTTONUP:
            APP_is_middle_hold = false;
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
                Map_Draw_Mode(hdc_tmp_buffer);
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

            // redraw properties drawing window for non regular walls
            GUI_Draw_Properties_Drawing_Window(_hwnd);

            // redraw properties drawing window for walls height
            GUI_Draw_Properties_Drawing_Height(_hwnd);
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

INT_PTR CALLBACK    APP_Wnd_Help_Controls_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
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
INT_PTR CALLBACK    APP_Wnd_Help_Lightmaps_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
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

INT_PTR CALLBACK    APP_Wnd_Select_Texture_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:
        {
            // Set positions of preview windows.
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[0]), HWND_TOP, 200, 10, 128 + 2, 128 + 2, SWP_SHOWWINDOW);

            // Alloc memory for raw texture when loaded.
            cnv_raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            cnv_raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            // restore to default directory
            SetCurrentDirectory(APP_curr_dir);

            // get files from directory
            TCHAR directory[APP_MAX_FILENAME];
            ZeroMemory(directory, sizeof(directory));

            if (gui_current_layer == LAYERS_WALL_LAYER || gui_current_layer == LAYERS_DOOR_LAYER)
            {
                wcscpy(directory, TEXT(APP_WALL_TEXTURES_DIRECTORY));
                wcscat(directory, L"\\*");
                wcscat(directory, TEXT(IO_WALL_TEXTURE_FILE_EXTENSION));
            }
            else
            {
                wcscpy(directory, TEXT(APP_FLAT_TEXTURES_DIRECTORY));
                wcscat(directory, L"\\*");
                wcscat(directory, TEXT(IO_FLAT_TEXTURE_FILE_EXTENSION));
            }

            WIN32_FIND_DATA data;
            HANDLE hFind = FindFirstFile(directory, &data);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    // Show only files with correct filename length (without extension).
                    if (wcslen(data.cFileName) > (IO_MAX_STRING_TEX_FILENAME + strlen(IO_WALL_TEXTURE_FILE_EXTENSION)) )
                        continue;

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
                            index = (int)SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_GETCURSEL, 0, 0);
                            SendMessage(GetDlgItem(_hwnd, IDC_SELECT_TEXTURE_LIST), LB_GETTEXT, index, (LPARAM)TEX_selected_file_name);

                            TCHAR path_file_name[APP_MAX_FILENAME];
                            ZeroMemory(path_file_name, sizeof(path_file_name));

                            if (gui_current_layer == LAYERS_WALL_LAYER || gui_current_layer == LAYERS_DOOR_LAYER)
                            {
                                wcscpy(path_file_name, TEXT(APP_WALL_TEXTURES_DIRECTORY));
                            }
                            else
                                wcscpy(path_file_name, TEXT(APP_FLAT_TEXTURES_DIRECTORY));

                            wcscat(path_file_name, L"\\");
                            wcscat(path_file_name, TEX_selected_file_name);

                            // convert TCHAR path_filename to CHAR path_filename
                            ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                            wcstombs_s(NULL, cnv_input_bitmap_filename, path_file_name, wcslen(path_file_name) + 1);

                            sIO_Prefs prefs = { 0 };
                            prefs.ch1 = 8;
                            prefs.ch2 = 16;
                            prefs.ch3 = 24;

                            int read_raw_result = BM_Read_Texture_RAW(cnv_input_bitmap_filename, cnv_raw_table, cnv_raw_index, &prefs);

                            if (read_raw_result <= 0)
                                MessageBox(_hwnd, L"Can't open that file.", L"Error.", MB_ICONWARNING | MB_OK);
                            else
                            {
                                cnv_hbm_texture[0] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS,
                                                                                      cnv_raw_table + IO_TEXTURE_MAX_COLORS * (IO_TEXTURE_MAX_SHADES - 1), cnv_raw_index);
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
                    case LAYERS_WALL_LAYER:
                    case LAYERS_DOOR_LAYER:
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
        PAINTSTRUCT ps_texture_128;
        HDC hdc_texture_128 = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture_128);

        HDC hdc_tmp_128 = CreateCompatibleDC(hdc_texture_128);
        SelectObject(hdc_tmp_128, cnv_hbm_texture[0]);
        BitBlt(hdc_texture_128, 0, 0, 128, 128, hdc_tmp_128, 0, 0, SRCCOPY);
        DeleteObject(cnv_hbm_texture[0]);
        DeleteDC(hdc_tmp_128);

        EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture_128);
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

int                 APP_Wnd_Convert_TGA(HWND _hwnd, char* _ch_filename, sBM_Bitmap_Indexed* _input_bitmap, sIO_Prefs* _prefs, int _max_width, int _max_height, int _max_colors, int _max_filename)
{
    int tga_result = BM_Read_Bitmap_Indexed_TGA(_ch_filename, _input_bitmap, _prefs);

    // check filename length
    char fname[32];
    ZeroMemory(fname, sizeof(fname));

    _splitpath(_ch_filename, NULL, NULL, fname, NULL);

    if (strlen(fname) > _max_filename)
        tga_result = -7;

    // check input width and height
    if ((_input_bitmap->width != _max_width) || (_input_bitmap->height != _max_height))
        tga_result = -5;

    // check num of colors
    if (_input_bitmap->num_colors > _max_colors)
        tga_result = -6;

    switch (tga_result)
    {
        case 0:
            MessageBox(_hwnd, L"File not found.", L"Error.", MB_ICONWARNING | MB_OK);
            break;

        case -1:
            MessageBox(_hwnd, L"This TGA is not indexed bitmap.", L"Error.", MB_ICONWARNING | MB_OK);
            break;

        case -2:
            MessageBox(_hwnd, L"Can't alloc memory for indexes.", L"Error.", MB_ICONWARNING | MB_OK);
            break;

        case -3:
            MessageBox(_hwnd, L"Can't alloc memory for color map.", L"Error.", MB_ICONWARNING | MB_OK);
            break;

        case -4:
            MessageBox(_hwnd, L"Can't read this TGA format.", L"Error.", MB_ICONWARNING | MB_OK);
            break;

        case -5:
        {
                TCHAR output[128];
                ZeroMemory(output, sizeof(output));
                swprintf_s(output, L"The input TGA bitmap should be %d x %d pixels.", INPUT_TEX_MAX_WIDTH, INPUT_TEX_MAX_HEIGHT);

                MessageBox(_hwnd, output, L"Error.", MB_ICONWARNING | MB_OK);
                break;
        }
     
        case -6:
        {
            TCHAR output[128];
            ZeroMemory(output, sizeof(output));
            swprintf_s(output, L"The maximum number of colors shoud be %d.", IO_TEXTURE_MAX_COLORS);

            MessageBox(_hwnd, output, L"Error.", MB_ICONWARNING | MB_OK);
            break;
        }

        case -7:
        {
            TCHAR output[128];
            ZeroMemory(output, sizeof(output));
            swprintf_s(output, L"Filename too long, must be less than %d charactes.", IO_MAX_STRING_TEX_FILENAME);

            MessageBox(_hwnd, output, L"Error.", MB_ICONWARNING | MB_OK);
            break;
        }
    }

    return tga_result;
}
INT_PTR CALLBACK    APP_Wnd_Convert_Tex_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
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
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[0]), HWND_TOP, 625, 10, INPUT_TEX_MAX_WIDTH + 2, INPUT_TEX_MAX_HEIGHT + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[1]), HWND_TOP, 24, 315, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[2]), HWND_TOP, 24 + 128 + 10, 315, 128 + 2, 128 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[3]), HWND_TOP, 24 + 128 + 10 + 128 + 10, 315, 128 + 2, 128 + 2, SWP_SHOWWINDOW);

            // Drag drop text
            TCHAR output[128];
            ZeroMemory(output, sizeof(output));
            swprintf_s(output, L"\n\n\n         --- Drag && drop TGA bitmap here ---\n\n             %d x %d, %d indexed colors", INPUT_TEX_MAX_WIDTH, INPUT_TEX_MAX_HEIGHT, IO_TEXTURE_MAX_COLORS);
            SetWindowText(GetDlgItem(_hwnd, IDS_DC_TEXTURE), output);

            // INFO TEXT
            TCHAR output_2[256];
            ZeroMemory(output_2, sizeof(output_2));
            swprintf_s(output_2, L"INPUT:\nAn %d indexed color, %d x %d TGA bitmap, that contains original 128x128 texture and its mipmaps down to 32x32,\nthat should be top aligned next to eachother.", IO_TEXTURE_MAX_COLORS, INPUT_TEX_MAX_WIDTH, INPUT_TEX_MAX_HEIGHT);
            SetWindowText(GetDlgItem(_hwnd, IDS_DC_INFO_2), output_2);

            // Init combo.
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"01. No light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"02. Solid light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"03. Dimmed light aware");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_MODE), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

            // Init slider.
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, IO_TEXTURE_MAX_SHADES-1));
            SendMessage(GetDlgItem(_hwnd, IDC_DC_SLIDER), TBM_SETPOS, TRUE, IO_TEXTURE_MAX_SHADES-1);

            // Reset current visible filename to -none-
            ZeroMemory(output, sizeof(output));
            swprintf_s(output, L"%s", L"- no input bitmap -");
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
            ZeroMemory(output2, sizeof(output2));
            swprintf_s(output2, L"%ld", IO_TEXTURE_MAX_SHADES - 1);
            SendMessage(GetDlgItem(_hwnd, IDS_DC_SLIDER_RIGHT), WM_SETTEXT, 0, (LPARAM)output2);
            SendMessage(GetDlgItem(_hwnd, IDC_DC_INTENSITY), WM_SETTEXT, 0, (LPARAM)output2);

            // Init other parameters.
            ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));

            cnv_current_light_mode = 0;
            cnv_current_light_treshold = 240;

            cnv_is_input_loaded = false;
            cnv_is_raw_loaded = false;            

            // Alloc memory for raw texture when loaded.
            cnv_raw_index = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
            cnv_raw_table = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

            return (INT_PTR)TRUE;


        case WM_DROPFILES:

            HDROP file_drop;
            file_drop = (HDROP)_wparam;

            TCHAR dropped_path_filename[APP_MAX_PATH];
            ZeroMemory(dropped_path_filename, sizeof(dropped_path_filename));

            if (DragQueryFile(file_drop, 0xFFFFFFFF, NULL, APP_MAX_PATH) != 1)
            {
                MessageBox(_hwnd, L"Drag & drop problem.", L"Error.", MB_ICONWARNING | MB_OK);
            }
            else
            {
                DragQueryFile(file_drop, 0, dropped_path_filename, APP_MAX_PATH);

                // Reject if filename if longer than correct fixed value.
                TCHAR dropped_filename[APP_MAX_FILENAME];
                ZeroMemory(dropped_filename, sizeof(dropped_filename));

                _wsplitpath(dropped_path_filename, NULL, NULL, dropped_filename, NULL);

                if (wcslen(dropped_filename) > (IO_MAX_STRING_TEX_FILENAME-1))
                {
                    TCHAR output[256];
                    swprintf(output, 256, L"The filename is too long.\nMax length is: %d characters.\nDo not include extension length.", (IO_MAX_STRING_TEX_FILENAME-1));

                    MessageBox(_hwnd, output, L"Error.", MB_ICONWARNING | MB_OK);
                }

                // convert TCHAR path_filename to CHAR path_filename
                ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                wcstombs_s(NULL, cnv_input_bitmap_filename, dropped_path_filename, wcslen(dropped_path_filename) + 1);

                // Lets read TGA textue with BGRA color order
                // - it will be visible correctly in preview window because of Windows Api format
                // and we could also use memcpy for fast color copying.
                sIO_Prefs prefs = { 0 };
                prefs.ch1 = 16;
                prefs.ch2 = 8;
                prefs.ch3 = 0;

                int dropped_tga_result = APP_Wnd_Convert_TGA( _hwnd, cnv_input_bitmap_filename, &cnv_input_bitmap_BGRA, &prefs,
                                                              INPUT_TEX_MAX_WIDTH, INPUT_TEX_MAX_HEIGHT, IO_TEXTURE_MAX_COLORS, IO_MAX_STRING_TEX_FILENAME);

                if (dropped_tga_result > 0)
                {
                    cnv_hbm_texture[0] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, cnv_input_bitmap_BGRA.width, cnv_input_bitmap_BGRA.height, cnv_input_bitmap_BGRA.num_colors,
                                                                       cnv_input_bitmap_BGRA.color_data, cnv_input_bitmap_BGRA.image_data);

                    // Get only filename.
                    char fname[32], ext[16];
                    ZeroMemory(fname, sizeof(fname));
                    _splitpath(cnv_input_bitmap_filename, NULL, NULL, fname, ext);

                    TCHAR output[256];
                    swprintf(output, 256, L"%hs%hs", fname, ext);

                    // Print filename.
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
                            cnv_current_light_mode = (int)SendMessage((HWND)_lparam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
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
                    // read in TGA as ARGB - its gonna be saved that way - so on Amiga with ARGB mode - no conversion will be need.
                    sIO_Prefs prefs = { 0 };
                    prefs.ch1 = 24;
                    prefs.ch2 = 16;
                    prefs.ch3 = 8;

                    int input_tga_result = APP_Wnd_Convert_TGA(_hwnd, cnv_input_bitmap_filename, &cnv_input_bitmap_ARGB, &prefs,
                                                               INPUT_TEX_MAX_WIDTH, INPUT_TEX_MAX_HEIGHT, IO_TEXTURE_MAX_COLORS, IO_MAX_STRING_TEX_FILENAME);
                 
                    if (input_tga_result > 0)
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

                        raw_result = BM_Convert_And_Save_Bitmap_Indexed_To_Texture_RAW(  output_filename, &cnv_input_bitmap_ARGB, texture_type,
                                                                                         cnv_current_light_mode, cnv_current_light_treshold  );

                        
                        // We can free RGBA TGA now because it is not needed anymore.
                        BM_Free_Bitmap_Indexed(&cnv_input_bitmap_ARGB);


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
                           
                            sIO_Prefs prefs = { 0 };
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
                                wsprintf(output, L"%d", (int)slider_pos);

                                SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

                                int intensity = _wtoi(output);
                                int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;
                 
                                cnv_hbm_texture[1] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                                cnv_hbm_texture[2] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 128 * 128);
                                cnv_hbm_texture[3] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 128 * 128 + 64 * 64);
                                
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
            wsprintf(output, L"%d", (int)slider_pos);

            SetWindowText(GetDlgItem(_hwnd, IDC_DC_INTENSITY), output);

            int intensity = _wtoi(output);
            int intensity_offset = intensity * IO_TEXTURE_MAX_COLORS;

            if (cnv_is_raw_loaded)
            {
                cnv_hbm_texture[1] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 128, 128, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index);
                cnv_hbm_texture[2] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 64, 64, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 128 * 128);
                cnv_hbm_texture[3] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, 32, 32, IO_TEXTURE_MAX_COLORS, cnv_raw_table + intensity_offset, cnv_raw_index + 128 * 128 + 64 * 64);
                
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
                PAINTSTRUCT ps_texture[4] = { 0 };
                HDC hdc_texture[4] = { 0 };
                HDC hdc_tmp[4] = { 0 };

                hdc_texture[0] = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture[0]);

                    hdc_tmp[0] = CreateCompatibleDC(hdc_texture[0]);
                    SelectObject(hdc_tmp[0], cnv_hbm_texture[0]);
                    BitBlt(hdc_texture[0], 0, 0, 224, 128, hdc_tmp[0], 0, 0, SRCCOPY);
                    DeleteObject(cnv_hbm_texture[0]);
                    DeleteDC(hdc_tmp[0]);

                EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture[0]);

                for (int i = 1; i < 4; i++)
                {
                    hdc_texture[i] = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[i]), &ps_texture[i]);

                        hdc_tmp[i] = CreateCompatibleDC(hdc_texture[i]);
                        SelectObject(hdc_tmp[i], cnv_hbm_texture[i]);

                        switch (i)
                        {
                            case 1:
                                StretchBlt(hdc_texture[i], 0, 0, 128, 128, hdc_tmp[i], 0, 0, 128, 128, SRCCOPY);
                                break;

                            case 2:
                                StretchBlt(hdc_texture[i], 0, 0, 128, 128, hdc_tmp[i], 0, 0, 64, 64, SRCCOPY);
                                break;

                            case 3:
                                StretchBlt(hdc_texture[i], 0, 0, 128, 128, hdc_tmp[i], 0, 0, 32, 32, SRCCOPY);
                                break;
                        }

                        DeleteObject(cnv_hbm_texture[i]);
                        DeleteDC(hdc_tmp[i]);

                    EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[i]), &ps_texture[i]);
                }
            }
            
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
INT_PTR CALLBACK    APP_Wnd_Convert_Lightmap_Proc(HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam)
{
    UNREFERENCED_PARAMETER(_lparam);

    switch (_message)
    {
        case WM_INITDIALOG:
            if (APP_never_saved)
            {
                MessageBox(_hwnd, L"You should save the the level first. The converted lightmap file will have the same name as level file.", L"Error.", MB_ICONWARNING | MB_OK);

                EndDialog(_hwnd, LOWORD(_wparam));
                return (INT_PTR)TRUE;
            }

            SetCurrentDirectory(APP_curr_dir);

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
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[0]), HWND_TOP, 570, 10, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[1]), HWND_TOP, 24, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[2]), HWND_TOP, 24 + 256 + 10, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(_hwnd, cnv_hbm_ID[3]), HWND_TOP, 24 + 256 + 10 + 256 + 10, 380, 256 + 2, 256 + 2, SWP_SHOWWINDOW);

            // Reset current visible filename to -none-
            TCHAR output[128];
            ZeroMemory(&output, sizeof(output));
            swprintf_s(output, L"%s", L"- no input TGA file -");
            SendMessage(GetDlgItem(_hwnd, IDC_DC_FILE), WM_SETTEXT, 0, (LPARAM)output);

            // INFO TEXT
            TCHAR output_2[256];
            ZeroMemory(&output_2, sizeof(output_2));
            swprintf_s(output_2, L" INPUT:\nAn indexed, %d x %d TGA bitmap, that contains baked lightmaps from 3D map.", INPUT_LIGHTMAP_MAX_WIDTH, INPUT_LIGHTMAP_MAX_HEIGHT);
            SetWindowText(GetDlgItem(_hwnd, IDS_DC_INFO_2), output_2);       

            // Init other windows.
            EnableWindow(GetDlgItem(_hwnd, IDB_DC_CONVERT_LIGHTMAP), FALSE);
            EnableWindow(GetDlgItem(_hwnd, IDC_DC_OUTPUT), FALSE);

            // Init other parameters.
            ZeroMemory(&cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));

            cnv_is_input_loaded = false;
            cnv_is_raw_loaded = false;

            // Alloc memory for raw texture when loaded.
            cnv_raw_lm_wall = (u_int8*)malloc((size_t)(IO_LIGHTMAP_BYTE_SIZE * APP_level_file.wall_lightmaps_count));
            cnv_raw_lm_floor = (u_int8*)malloc((size_t)(IO_LIGHTMAP_BYTE_SIZE * (APP_level_file.flat_lightmaps_count / 2)));
            cnv_raw_lm_ceil = (u_int8*)malloc((size_t)(IO_LIGHTMAP_BYTE_SIZE * (APP_level_file.flat_lightmaps_count / 2)));

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

                // convert TCHAR path_filename to CHAR path_filename
                ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
                wcstombs_s(NULL, cnv_input_bitmap_filename, file_name, wcslen(file_name) + 1);

                sIO_Prefs prefs = { 0 };
                prefs.ch1 = 16;
                prefs.ch2 = 8;
                prefs.ch3 = 0;

                // read in texture and keep it
                int dropped_tga_result = APP_Wnd_Convert_TGA(_hwnd, cnv_input_bitmap_filename, &cnv_input_bitmap_BGRA, &prefs,
                                                             INPUT_LIGHTMAP_MAX_WIDTH, INPUT_LIGHTMAP_MAX_HEIGHT, IO_TEXTURE_MAX_SHADES, 100);


                if (dropped_tga_result > 0)
                {
                    cnv_hbm_texture[0] = Make_HBITMAP_From_Bitmap_Indexed(_hwnd, cnv_input_bitmap_BGRA.width, cnv_input_bitmap_BGRA.height, cnv_input_bitmap_BGRA.num_colors,
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

            return (INT_PTR)TRUE;


        case WM_COMMAND:
        {
            int wm_id = LOWORD(_wparam);
            int wm_event = HIWORD(_wparam);

            switch (wm_id)
            {
                case IDB_DC_CONVERT_LIGHTMAP:
                {
                    sIO_Prefs prefs = { 0 };
                    prefs.ch1 = 24;
                    prefs.ch2 = 16;
                    prefs.ch3 = 8;

                    int input_tga_result = APP_Wnd_Convert_TGA(_hwnd, cnv_input_bitmap_filename, &cnv_input_bitmap_ARGB, &prefs,
                                                                INPUT_LIGHTMAP_MAX_WIDTH, INPUT_LIGHTMAP_MAX_HEIGHT, IO_TEXTURE_MAX_SHADES, 100);

                    if (input_tga_result > 0)
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

                        raw_result = BM_Convert_And_Save_Bitmap_Indexed_To_Lightmap_RAW(output_filename, &cnv_input_bitmap_ARGB,
                                                                                        APP_level_file.wall_lightmaps_count, APP_level_file.flat_lightmaps_count);

                        // We can free RGBA TGA now because it is not needed anymore.
                        BM_Free_Bitmap_Indexed(&cnv_input_bitmap_ARGB);

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

                            int read_raw_result = BM_Read_Lightmaps_RAW_Separate(output_filename, cnv_raw_lm_wall, cnv_raw_lm_floor, cnv_raw_lm_ceil,
                                                                                APP_level_file.wall_lightmaps_count, APP_level_file.flat_lightmaps_count);

                            if (read_raw_result <= 0)
                                MessageBox(_hwnd, L"Can't open converted file. Did you run the Level Editor from compilator? You must run it as standalone from its original directory.", L"Error.", MB_ICONWARNING | MB_OK);
                            else
                            {
                                cnv_raw_lm_wall_lenght = (int)(ceilf(sqrtf(APP_level_file.wall_lightmaps_count))) * 32;
                                cnv_hbm_texture[1] = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall);

                                cnv_raw_lm_flat_lenght = (int)(ceilf(sqrtf((float)(APP_level_file.flat_lightmaps_count / 2)))) * 32;
                                cnv_hbm_texture[2] = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, cnv_raw_lm_floor);
                                cnv_hbm_texture[3] = Make_HBITMAP_From_Bitmap_Indexed_Grayscale(_hwnd, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, cnv_raw_lm_ceil);

                                // Set that raw file is loaded.
                                cnv_is_raw_loaded = true;
                            }
                        }
                    }

                    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE);
                    break;
                }

                case IDB_DC_MAKE_EMPTY_LM:
                    Make_Empty_Lightmap();
                    break;
            }
            return (INT_PTR)TRUE;
        }
   
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);

            // display input texture
            if (cnv_is_input_loaded)
            {
                PAINTSTRUCT ps_texture[4] = { 0 };
                HDC hdc_texture[4] = { 0 };
                HDC hdc_tmp[4] = { 0 };

                // original
                hdc_texture[0] = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture[0]);
                    hdc_tmp[0] = CreateCompatibleDC(hdc_texture[0]);
                    SelectObject(hdc_tmp[0], cnv_hbm_texture[0]);
                    StretchBlt(hdc_texture[0], 0, 0, 256, 256, hdc_tmp[0], 0, 0, INPUT_LIGHTMAP_MAX_WIDTH, INPUT_LIGHTMAP_MAX_HEIGHT, SRCCOPY);
                    DeleteObject(cnv_hbm_texture[0]);
                    DeleteDC(hdc_tmp[0]);
                EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[0]), &ps_texture[0]);

                // wall lm
                hdc_texture[1] = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[1]), &ps_texture[1]);
                    hdc_tmp[1] = CreateCompatibleDC(hdc_texture[1]);
                    SelectObject(hdc_tmp[1], cnv_hbm_texture[1]);
                    StretchBlt(hdc_texture[1], 0, 0, 256, 256, hdc_tmp[1], 0, 0, cnv_raw_lm_wall_lenght, cnv_raw_lm_wall_lenght, SRCCOPY);
                    DeleteObject(cnv_hbm_texture[1]);
                    DeleteDC(hdc_tmp[1]);
                EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[1]), &ps_texture[1]);

                // flat lm
                for (int i = 2; i < 4; i++)
                {
                    hdc_texture[i] = BeginPaint(GetDlgItem(_hwnd, cnv_hbm_ID[i]), &ps_texture[i]);
                        hdc_tmp[i] = CreateCompatibleDC(hdc_texture[i]);
                        SelectObject(hdc_tmp[i], cnv_hbm_texture[i]);
                        StretchBlt(hdc_texture[i], 0, 0, 256, 256, hdc_tmp[i], 0, 0, cnv_raw_lm_flat_lenght, cnv_raw_lm_flat_lenght, SRCCOPY);
                        DeleteObject(cnv_hbm_texture[i]);
                        DeleteDC(hdc_tmp[i]);
                    EndPaint(GetDlgItem(_hwnd, cnv_hbm_ID[i]), &ps_texture[i]);
                }
            }

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


void    APP_Init_Once()
{
    // save path to current directory
    GetCurrentDirectory(MAX_PATH, APP_curr_dir);

    // save path to level directory
    SetCurrentDirectory(TEXT(APP_LEVELS_DIRECTORY));
    GetCurrentDirectory(MAX_PATH, APP_level_dir);
}
void    APP_Reset()
{
    // --- APP settings ---
    SetCurrentDirectory(APP_curr_dir);

    ZeroMemory(APP_filename, sizeof(APP_filename));
    wcscpy(APP_filename, APP_level_dir);
    wcscat(APP_filename, L"\\new-level.m");
    APP_never_saved = 1;

    APP_control_pressed = false;
    APP_alt_pressed = false;
    APP_is_left_hold = false;
    APP_is_middle_hold = false;

    // --- level structure settings ---
    ZeroMemory(&APP_level_file, sizeof(APP_level_file));

    APP_level_file.player_starting_angle = 0;
    APP_level_file.player_starting_cell_x = MAP_LENGTH / 2;
    APP_level_file.player_starting_cell_y = MAP_LENGTH / 2;
}
int     APP_Open()
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

            // first read in the header of file (static data)
            fread(&APP_level_file, sizeof(APP_level_file), 1, file_in);
            fclose(file_in);

            APP_never_saved = 0;

            // make swaps
            APP_level_file.wall_lightmaps_count = MA_swap_int16(APP_level_file.wall_lightmaps_count);
            APP_level_file.flat_lightmaps_count = MA_swap_int16(APP_level_file.flat_lightmaps_count);

            // restore opened filename
            wcscpy(APP_filename, old_filename);

            // restore current dir to default
            SetCurrentDirectory(APP_curr_dir);

            // Read in wall textures. 
            // They are stored without the extensions so addthem before putting on the list.
            for (int i = 0; i < APP_level_file.wall_textures_count; i++)
            {
                ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));
                mbstowcs_s(NULL, TEX_selected_file_name, IO_MAX_STRING_TITLE, APP_level_file.wall_texture_filename[i], _TRUNCATE);
                wcscat(TEX_selected_file_name, TEXT(IO_WALL_TEXTURE_FILE_EXTENSION));

                TEX_Add_Texture_To_List(&TEX_wall_list, 0);
            }

            // read in flat textures
            for (int i = 0; i < APP_level_file.flat_textures_count; i++)
            {
                ZeroMemory(TEX_selected_file_name, sizeof(TEX_selected_file_name));
                mbstowcs_s(NULL, TEX_selected_file_name, IO_MAX_STRING_TITLE, APP_level_file.flat_texture_filename[i], _TRUNCATE);
                wcscat(TEX_selected_file_name, TEXT(IO_FLAT_TEXTURE_FILE_EXTENSION));

                TEX_Add_Texture_To_List(&TEX_flat_list, 1);
            }

            // restore negative floor and ceil cells
            for (int cell_id = 0; cell_id < LV_MAP_CELLS_COUNT; cell_id++)
            {
                int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
                int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

                switch (APP_level_file.map[cell_id].cell_type)
                {
                    case LV_C_WALL_THIN_HORIZONTAL:
                    case LV_C_WALL_THIN_VERTICAL:
                    case LV_C_WALL_THIN_OBLIQUE:

                    case LV_C_WALL_BOX:
                    case LV_C_WALL_BOX_FOURSIDE:
                    case LV_C_WALL_BOX_SHORT:

                    case LV_C_DOOR_THICK_HORIZONTAL:
                    case LV_C_DOOR_THICK_VERTICAL:
                    case LV_C_DOOR_THIN_OBLIQUE:

                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:

                        // Also wwap endiannes. It was saved in different endiannes for AMIGA.
                        APP_level_file.map[cell_id].starting_height     = MA_swap_float32(APP_level_file.map[cell_id].starting_height);
                        APP_level_file.map[cell_id].height              = MA_swap_float32(APP_level_file.map[cell_id].height);

                        APP_level_file.map[cell_id].wall_vertex[0][0]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[0][0]) - cell_x;
                        APP_level_file.map[cell_id].wall_vertex[0][1]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[0][1]) - cell_y;
                        APP_level_file.map[cell_id].wall_vertex[1][0]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[1][0]) - cell_x;
                        APP_level_file.map[cell_id].wall_vertex[1][1]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[1][1]) - cell_y;
                        break;
                }
            }

            return 1;
        }
    }
    else
        return 0;
}
void    APP_Save()
{
    if (APP_never_saved)    APP_Save_As();
    else                    APP_Saving_File();
}
void    APP_Save_As()
{
    if (GUI_OpenDialog(APP_filename, APP_SAVE_FILE, APP_MAP_FILTER, APP_DEFEXT_MAP))
    {
        APP_never_saved = 0;
        APP_Saving_File();
    }
    else
        MessageBox(APP_hWnd, L"Nothing saved...", L"Info...", MB_OK);
}
void    APP_Saving_File()
{
    // Open level file to be saved.
    FILE* file_out;
    file_out = _wfopen(APP_filename, L"wb");

    if (file_out == NULL)
    {
        MessageBox(APP_hWnd, L"Could not open file to save...", L"Info...", MB_OK);
        return;
    }
    else
    {
        // Lets count all textures.
        TEX_Count_Textures();
        
        // Put number of textures on level file.
        APP_level_file.wall_textures_count = TEX_wall_count;
        APP_level_file.flat_textures_count = TEX_flat_count;

        // Lets update WALL texture list.
        // Clear unused textures, sort, put filenames in level file, give new IDs.
        TEX_Prepare_Wall_List_To_Save(&TEX_wall_list, TEX_wall_table, 0);
         
        // Lets update FLAT texture list.
        // Clear unused textures, sort, put filenames in level file, give new IDs.
        TEX_Prepare_Flat_List_To_Save(&TEX_flat_list, TEX_flat_table, 1);
        
        // Update lightmaps info.
        APP_Count_or_Export_Lightmaps(1);

        // Change ranges of vertices.
        // This is for non standard WALLS and DOORS that use vertices because in editor we are using ranges (0.0-1.0) for vertices.
        // In game engine we must know the integer part (the cell x,y) - for example: (32.5, 32.5) not (0.5, 0.5).

            for (int cell_id = 0; cell_id < LV_MAP_CELLS_COUNT; cell_id++)
            {
                // Get cell_x, and cell_Y from cell_id.
                int cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
                int cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

                switch (APP_level_file.map[cell_id].cell_type)
                {
                    case LV_C_WALL_THIN_HORIZONTAL:
                    case LV_C_WALL_THIN_VERTICAL:
                    case LV_C_WALL_THIN_OBLIQUE:

                    case LV_C_WALL_BOX:
                    case LV_C_WALL_BOX_FOURSIDE:
                    case LV_C_WALL_BOX_SHORT:

                    case LV_C_DOOR_THICK_HORIZONTAL:
                    case LV_C_DOOR_THICK_VERTICAL:
                    case LV_C_DOOR_THIN_OBLIQUE:

                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:

                        // Update cell_state according to cell_action.
                        if (APP_level_file.map[cell_id].cell_action == LV_A_PUSH)        APP_level_file.map[cell_id].cell_state = LV_S_DOOR_CLOSED;
                        if (APP_level_file.map[cell_id].cell_action == LV_A_PROXMITY)    APP_level_file.map[cell_id].cell_state = LV_S_DOOR_CLOSED_PROXMITY;

                        // Swap endiannes for floats for AMIGA.
                        APP_level_file.map[cell_id].starting_height     = MA_swap_float32(APP_level_file.map[cell_id].starting_height);
                        APP_level_file.map[cell_id].height              = MA_swap_float32(APP_level_file.map[cell_id].height);

                        APP_level_file.map[cell_id].wall_vertex[0][0]   = MA_swap_float32(cell_x + (APP_level_file.map[cell_id].wall_vertex[0][0]));
                        APP_level_file.map[cell_id].wall_vertex[0][1]   = MA_swap_float32(cell_y + (APP_level_file.map[cell_id].wall_vertex[0][1]));
                        APP_level_file.map[cell_id].wall_vertex[1][0]   = MA_swap_float32(cell_x + (APP_level_file.map[cell_id].wall_vertex[1][0]));
                        APP_level_file.map[cell_id].wall_vertex[1][1]   = MA_swap_float32(cell_y + (APP_level_file.map[cell_id].wall_vertex[1][1]));

                        break;
                }
            }

        // Swap endiannes for 16 and 32 bit values before saving.
            
            APP_level_file.wall_lightmaps_count = MA_swap_int16(APP_level_file.wall_lightmaps_count);
            APP_level_file.flat_lightmaps_count = MA_swap_int16(APP_level_file.flat_lightmaps_count);
            
            for (int cell_id = 0; cell_id < LV_MAP_CELLS_COUNT; cell_id++)
            {
                APP_level_file.map[cell_id].wall_lightmap_id[0] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[0]);
                APP_level_file.map[cell_id].wall_lightmap_id[1] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[1]);
                APP_level_file.map[cell_id].wall_lightmap_id[2] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[2]);
                APP_level_file.map[cell_id].wall_lightmap_id[3] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[3]);

                APP_level_file.map[cell_id].flat_lightmap_id[0] = MA_swap_int16(APP_level_file.map[cell_id].flat_lightmap_id[0]);
                APP_level_file.map[cell_id].flat_lightmap_id[1] = MA_swap_int16(APP_level_file.map[cell_id].flat_lightmap_id[1]);
            }

        // Try save the header structure with all data to file.
        
            int saving_result = (int)fwrite(&APP_level_file, sizeof(sLV_Level__File_Only), 1, file_out);            
            fclose(file_out);
            if (!saving_result) MessageBox(APP_hWnd, L"Could not save to file...", L"Info...", MB_OK);            
            

        // ----------------------------------------------------------------
        // After the file is saved (or not) let restore values for editor. 
        // ----------------------------------------------------------------
         

        // Lets restore the endiannes back.

            APP_level_file.wall_lightmaps_count = MA_swap_int16(APP_level_file.wall_lightmaps_count);
            APP_level_file.flat_lightmaps_count = MA_swap_int16(APP_level_file.flat_lightmaps_count);

            for (int cell_id = 0; cell_id < LV_MAP_CELLS_COUNT; cell_id++)
            {
                APP_level_file.map[cell_id].wall_lightmap_id[0] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[0]);
                APP_level_file.map[cell_id].wall_lightmap_id[1] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[1]);
                APP_level_file.map[cell_id].wall_lightmap_id[2] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[2]);
                APP_level_file.map[cell_id].wall_lightmap_id[3] = MA_swap_int16(APP_level_file.map[cell_id].wall_lightmap_id[3]);

                APP_level_file.map[cell_id].flat_lightmap_id[0] = MA_swap_int16(APP_level_file.map[cell_id].flat_lightmap_id[0]);
                APP_level_file.map[cell_id].flat_lightmap_id[1] = MA_swap_int16(APP_level_file.map[cell_id].flat_lightmap_id[1]);
            }

        // Lets restore the ranges of non standard WALLS and DOORS back to (0.0-1.0) for editor.

            for (int cell_id = 0; cell_id < LV_MAP_CELLS_COUNT; cell_id++)
            {
                // Get cell_x, and cell_Y from cell_id.
                int cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
                int cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

                switch (APP_level_file.map[cell_id].cell_type)
                {
                    case LV_C_WALL_THIN_HORIZONTAL:
                    case LV_C_WALL_THIN_VERTICAL:
                    case LV_C_WALL_THIN_OBLIQUE:
                    case LV_C_WALL_BOX:
                    case LV_C_WALL_BOX_FOURSIDE:
                    case LV_C_WALL_BOX_SHORT:

                    case LV_C_DOOR_THICK_HORIZONTAL:
                    case LV_C_DOOR_THICK_VERTICAL:
                    case LV_C_DOOR_THIN_OBLIQUE:

                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:

                        // Restore also endiannes after saving. 
                        APP_level_file.map[cell_id].starting_height     = MA_swap_float32(APP_level_file.map[cell_id].starting_height);
                        APP_level_file.map[cell_id].height              = MA_swap_float32(APP_level_file.map[cell_id].height);

                        APP_level_file.map[cell_id].wall_vertex[0][0]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[0][0]) - cell_x;
                        APP_level_file.map[cell_id].wall_vertex[0][1]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[0][1]) - cell_y;
                        APP_level_file.map[cell_id].wall_vertex[1][0]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[1][0]) - cell_x;
                        APP_level_file.map[cell_id].wall_vertex[1][1]   = MA_swap_float32(APP_level_file.map[cell_id].wall_vertex[1][1]) - cell_y;

                        break;
                }
            }
    }
}            
void    APP_Count_or_Export_Lightmaps(int _mode)
{
    // _mode == 0 is for exporting to OBJ file and counting lightmaps and memory
    // _mode == 1 is for counting lightmaps and memory only

    // First of all - check if there are any lightmaps.
    // If no lightmaps then just return - when _mode == 0 give also a message.
    int is_lightmap = 0;

    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
        if (APP_level_file.map[i].is_lightmapped)
        {
            is_lightmap = 1;
            break;
        }
    }

    if (is_lightmap == 0)
    {
        if (_mode == 0)
            MessageBox(APP_hWnd, L"No lightmaps found.\nYou must first select the lightmaps in the LIGHTMAPS LAYER.", L"Info...", MB_OK);

        return;
    }

    // This will hold the OBJ output filename - selected by user from listener.
    TCHAR tc_output_obj_filename[MAX_PATH];
    ZeroMemory(tc_output_obj_filename, sizeof(tc_output_obj_filename));

    // This will hold the result of selecting filename.
    bool output_obj_filename_result = 0;

    // If saving to file mode - display listener to select file name.
    if (_mode == 0)
        output_obj_filename_result = GUI_OpenDialog(tc_output_obj_filename, APP_SAVE_FILE, APP_OBJ_FILTER, APP_DEFEXT_OBJ);
    else
        output_obj_filename_result = 1;


    if (!output_obj_filename_result)
    {
        // If user selcted nothing - just return.
        MessageBox(APP_hWnd, L"Nothing saved...", L"Info...", MB_OK);
        return;
    }
    else
    {
        // If the user selected file or we are only in counting mode do the rest.

        // This is OBJ output file handle.
        FILE* output_obj_file = NULL;
        bool output_obj_file_result = 0;

        // If we are exporting the file - open the selected file to save.
        if (_mode == 0)
        {
            output_obj_file = _wfopen(tc_output_obj_filename, L"wb");

            if (output_obj_file)    output_obj_file_result = 1;
            else                    output_obj_file_result = 0;
        }
        else
            output_obj_file_result = 1;


        if (!output_obj_file_result)
        {
            // Return if the file cant be opened.
            MessageBox(APP_hWnd, L"Could not open file...", L"Info...", MB_OK);
            return;
        }
        else
        {
            // If the file is OK or we are in the counting mode - do the rest.

            // 1. Each wall/door/floor/ceil is based on quads - one quad has 4 vertices.
            // 2. In OBJ the vertices list comes first, one vertex is: "v 0.00 0.00 0.00" - so we can write them to file during calculations.
            // 3. After the vertices list - the list of vertex texture coords comes in, one texture coord is: "vt 0.0000 0.0000 0.0" 
            // - this will be saved to "tex_coords_list" and write to file after list of vertices is written.
            // 4. At the end we must put list of quads, one line is: "f 0/0 1/1 2/2 3/3"

            // Lets create this "too big" tex_coords_buffer - for holding all list of texture coords
            // that will be written to file after the list of vertex is written.
            
            // Its string data - lets assume max 32 bytes for one line:  "vt 0.0000 0.0000 0.0" 
            // maximum case: 6 walls * 4 vertices * 32 bytes per line * cell count
            char* tex_coords_buffer = NULL;

            if (_mode == 0)
            {
                int tex_coords_buffer_size = sizeof(char) * 6 * 4 * 16 * LV_MAP_CELLS_COUNT;

                tex_coords_buffer = (char*)malloc(tex_coords_buffer_size);
                if (tex_coords_buffer == NULL)
                {
                    MessageBox(APP_hWnd, L"Can't alloc memory for texture coords buffer.", L"Info...", MB_OK);
                    return;
                }

                memset(tex_coords_buffer, 0, sizeof(tex_coords_buffer_size));
            }
          
            // Assuming big fixed texture 2048x2048px, so 64 lightmaps in a row when one lightmap is 32x32px
            double fraction = 1.0 / 64.0;

            int16 v_num = 0;
            int16 q_num = 0;

            double grid = 10.0;
            double mirror_grid = -1.0 * grid;

            // For exporting lightmaps UV only - starting coords.
            double curr_x = 0.0;
            double curr_y = 0.0;

            APP_level_file.wall_lightmaps_count = 0;
            APP_level_file.flat_lightmaps_count = 0;

            // Help bufer for calculating one line of vt.
            char tmp_buffer[32];
            memset(tmp_buffer, 0, sizeof(tmp_buffer));

            // The main loop - lets go thru all the map.
            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;

                    // Make action only when current map cell has a lightmap selected.
                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    // What CELL type are we dealing with.
                    // The actions will depend on the CELL type.

                    switch (APP_level_file.map[index].cell_type)
                    {

                        // --------------------------------------------------------------------------
                        // 01. The basic standard walls - that have 4 sides and fills the WHOLE CELL.
                        // --------------------------------------------------------------------------

                        case LV_C_WALL_STANDARD:
                        case LV_C_WALL_FOURSIDE:
                        {
                            // In this case check the neightbour of each side.

                            // Should we export TOP WALL quad? 
                            // Check if cell that is higher is solid or marked as lightmap, or this cell is the topmost.
                            int top_index = x + (y - 1) * MAP_LENGTH;

                            if (
                                   (APP_level_file.map[top_index].cell_type == LV_C_NOT_SOLID                           ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_THIN_HORIZONTAL                ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_THIN_VERTICAL                  ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_THIN_OBLIQUE                   ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_BOX                            ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_BOX_FOURSIDE                   ||
                                    APP_level_file.map[top_index].cell_type == LV_C_WALL_BOX_SHORT                      ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_THICK_HORIZONTAL                ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_THICK_VERTICAL                  ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_THIN_OBLIQUE                   ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT   ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT  ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT    ||
                                    APP_level_file.map[top_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)    &&
                                    APP_level_file.map[top_index].is_lightmapped                                        &&
                                    (y > 0)
                                )
                            {
                                if (_mode == 0)
                                {
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

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

                                    curr_x += fraction;
                                    v_num += 4;
                                }

                                APP_level_file.map[index].wall_lightmap_id[id_top] = q_num;
                                q_num += 1;

                                APP_level_file.wall_lightmaps_count++;
                            }

                            // Should we export BOTTOM WALL quad? 
                            // Check if cell that is lower  is solid or marked as lightmap, or this cell is the bottom.
                            int bottom_index = x + (y + 1) * MAP_LENGTH;

                            if (
                                   (APP_level_file.map[bottom_index].cell_type == LV_C_NOT_SOLID                            ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_THIN_HORIZONTAL                 ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_THIN_VERTICAL                   ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_THIN_OBLIQUE                    ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_BOX                             ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_BOX_FOURSIDE                    ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_WALL_BOX_SHORT                       ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_THICK_HORIZONTAL                 ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_THICK_VERTICAL                   ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_THIN_OBLIQUE                    ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT    ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT   ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT     ||
                                    APP_level_file.map[bottom_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)     &&
                                    APP_level_file.map[bottom_index].is_lightmapped                                         && 
                                    (y < LV_MAP_LENGTH - 1)
                                 )
                            {
                                if (_mode == 0)
                                {
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));

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

                                    curr_x += fraction;
                                    v_num += 4;
                                }

                                APP_level_file.map[index].wall_lightmap_id[id_bottom] = q_num;
                                q_num += 1;

                                APP_level_file.wall_lightmaps_count++;
                            }

                            // Should we export LEFT WALL quad? 
                            // Check if cell that is on th left is solid or marked as lightmap, or this cell is the leftmost.
                            int left_index = x - 1 + y * MAP_LENGTH;

                            if (
                                   (APP_level_file.map[left_index].cell_type == LV_C_NOT_SOLID                             ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_THIN_HORIZONTAL                  ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_THIN_VERTICAL                    ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_THIN_OBLIQUE                     ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_BOX                              ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_BOX_FOURSIDE                     ||
                                    APP_level_file.map[left_index].cell_type == LV_C_WALL_BOX_SHORT                        ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_THICK_HORIZONTAL                  ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_THICK_VERTICAL                    ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_THIN_OBLIQUE                     ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT     ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT    ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT      ||
                                    APP_level_file.map[left_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)      &&
                                    APP_level_file.map[left_index].is_lightmapped                                          &&
                                    (x > 0)
                                )
                            {
                                if (_mode == 0)
                                {
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

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

                                    curr_x += fraction;
                                    v_num += 4;
                                }

                                APP_level_file.map[index].wall_lightmap_id[id_left] = q_num;                                
                                q_num += 1;

                                APP_level_file.wall_lightmaps_count++;
                            }

                            // Should we export RIGHT WALL quad? 
                            // Check if cell that is on the right is solid or marked as lightmap, or this cell is the rightmost.
                            int right_index = x + 1 + y * MAP_LENGTH;

                            if (
                                   (APP_level_file.map[right_index].cell_type == LV_C_NOT_SOLID                             ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_THIN_HORIZONTAL                  ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_THIN_VERTICAL                    ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_THIN_OBLIQUE                     ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_BOX                              ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_BOX_FOURSIDE                     ||
                                    APP_level_file.map[right_index].cell_type == LV_C_WALL_BOX_SHORT                        ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_THICK_HORIZONTAL                  ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_THICK_VERTICAL                    ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_THIN_OBLIQUE                     ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT     ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT    ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT      ||
                                    APP_level_file.map[right_index].cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)      &&
                                    APP_level_file.map[right_index].is_lightmapped &&
                                    (x < LV_MAP_LENGTH - 1)
                                )
                            {
                                if (_mode == 0)
                                {
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                                    fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));

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

                                    curr_x += fraction;
                                    v_num += 4;
                                }

                                APP_level_file.map[index].wall_lightmap_id[id_right] = q_num;                              
                                q_num += 1;

                                APP_level_file.wall_lightmaps_count++;
                            }

                            break;
                        }

                        // ------------------------------------------------
                        // 02. The thin walls and doors - that have 1 side.
                        // ------------------------------------------------

                        case LV_C_WALL_THIN_HORIZONTAL:
                        case LV_C_WALL_THIN_VERTICAL:
                        case LV_C_WALL_THIN_OBLIQUE:

                        case LV_C_DOOR_THIN_OBLIQUE:
                        {

                            // Export only only side/quad/lightmap. Don't forget also to export floor and ceil quads/lightmap later.

                            if (_mode == 0)
                            {
                                double real_x0 = x + APP_level_file.map[index].wall_vertex[0][0];
                                double real_y0 = y + APP_level_file.map[index].wall_vertex[0][1];
                                double real_x1 = x + APP_level_file.map[index].wall_vertex[1][0];
                                double real_y1 = y + APP_level_file.map[index].wall_vertex[1][1];


                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(1.0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(1.0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(0.0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(0.0 * grid));

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

                                curr_x += fraction;
                                v_num += 4;
                            }

                             APP_level_file.map[index].wall_lightmap_id[id_top] = q_num;                            
                             q_num += 1;

                             APP_level_file.wall_lightmaps_count++;

                             break;
                        }

                        // --------------------------------------------------------------------
                        // 03. The THICK DOORS - that have 2 sides and are non-regular.
                        // --------------------------------------------------------------------
                        case LV_C_DOOR_THICK_HORIZONTAL:
                        {
                            if (_mode == 0)
                            {
                                double real_x0 = (double)x + (double)APP_level_file.map[index].wall_vertex[0][0];
                                double real_y0 = (double)y + (double)APP_level_file.map[index].wall_vertex[0][1];
                                double real_x1 = (double)x + (double)APP_level_file.map[index].wall_vertex[1][0];
                                double real_y1 = (double)y + (double)APP_level_file.map[index].wall_vertex[1][1];

                                double real_z0 = (1.0 - (double)APP_level_file.map[index].starting_height - (double)APP_level_file.map[index].height);
                                double real_z1 = (1.0 - (double)APP_level_file.map[index].starting_height);

                                // top
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z0 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z0 * grid);

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

                                curr_x += fraction;

                                // bottom
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));

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

                                curr_x += fraction;

                                v_num += (4 * 2);
                            }

                            APP_level_file.map[index].wall_lightmap_id[id_top] = q_num;
                            q_num++;

                            APP_level_file.map[index].wall_lightmap_id[id_bottom] = q_num;
                            q_num++;

                            APP_level_file.wall_lightmaps_count += 2;
                        }
                        break;

                        case LV_C_DOOR_THICK_VERTICAL:
                        {
                            if (_mode == 0)
                            {
                                double real_x0 = (double)x + (double)APP_level_file.map[index].wall_vertex[0][0];
                                double real_y0 = (double)y + (double)APP_level_file.map[index].wall_vertex[0][1];
                                double real_x1 = (double)x + (double)APP_level_file.map[index].wall_vertex[1][0];
                                double real_y1 = (double)y + (double)APP_level_file.map[index].wall_vertex[1][1];

                                double real_z0 = (1.0 - (double)APP_level_file.map[index].starting_height - (double)APP_level_file.map[index].height);
                                double real_z1 = (1.0 - (double)APP_level_file.map[index].starting_height);

                                // left
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(real_z0 * grid));

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

                                curr_x += fraction;

                                // right
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y0 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y0 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));

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

                                curr_x += fraction;

                                v_num += (4 * 2);
                            }

                            APP_level_file.map[index].wall_lightmap_id[id_left] = q_num;
                            q_num++;

                            APP_level_file.map[index].wall_lightmap_id[id_right] = q_num;
                            q_num++;

                            APP_level_file.wall_lightmaps_count += 2;
                        }
                        break;

                        // --------------------------------------------------------------------
                        // 04. The BOX WALLS and DOORS - that have 4 sides and are non-regular.
                        // --------------------------------------------------------------------

                        case LV_C_WALL_BOX:
                        case LV_C_WALL_BOX_FOURSIDE:
                        case LV_C_WALL_BOX_SHORT:

                        case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                        case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                        case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                        case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                        {
                            if (_mode == 0)
                            {
                                double real_x0 = (double)x + (double)APP_level_file.map[index].wall_vertex[0][0];
                                double real_y0 = (double)y + (double)APP_level_file.map[index].wall_vertex[0][1];
                                double real_x1 = (double)x + (double)APP_level_file.map[index].wall_vertex[1][0];
                                double real_y1 = (double)y + (double)APP_level_file.map[index].wall_vertex[1][1];

                                double real_z0 = (1.0 - (double)APP_level_file.map[index].starting_height - (double)APP_level_file.map[index].height);
                                double real_z1 = (1.0 - (double)APP_level_file.map[index].starting_height);

                                // top
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z0 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z0 * grid);

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

                                curr_x += fraction;

                                // bottom
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));

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

                                curr_x += fraction;

                                // left
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1* grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(real_z1* grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0* grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x0 * grid), (double)(real_y0 * mirror_grid), (double)(real_z0* grid));

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

                                curr_x += fraction;

                                // right
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y0 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y0 * mirror_grid), (double)(real_z1 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z0 * grid));
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(real_x1 * grid), (double)(real_y1 * mirror_grid), (double)(real_z1 * grid));

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

                                curr_x += fraction;

                                v_num += (4 * 4);
                            }

                            APP_level_file.map[index].wall_lightmap_id[id_top] = q_num;
                            q_num++;

                            APP_level_file.map[index].wall_lightmap_id[id_bottom] = q_num;
                            q_num++;

                            APP_level_file.map[index].wall_lightmap_id[id_left] = q_num;
                            q_num++;

                            APP_level_file.map[index].wall_lightmap_id[id_right] = q_num;
                            q_num++;             

                            APP_level_file.wall_lightmaps_count += 4;

                            break;
                        }
                    }
                }
            }

            // ----------------------------------------
            // Now lets add only FLOOR quads and UVs...
            // ----------------------------------------
            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;
                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    // Make floor quad only if cell type id not solid OR there is non regular wall.

                    if (
                            APP_level_file.map[index].cell_type == LV_C_NOT_SOLID ||
                           (APP_level_file.map[index].cell_type >= LV_C_WALL_THIN_HORIZONTAL && APP_level_file.map[index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
                       )
                    {
                        if (_mode == 0)
                        {
                            // add floor quad
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(0.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(0.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(0.0 * grid));

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

                            curr_x += fraction;
                            v_num += 4;
                        }

                        APP_level_file.map[index].flat_lightmap_id[id_floor] = q_num;
                        q_num++;

                        APP_level_file.flat_lightmaps_count++;
                    }
                }
            }

            // ---------------------------------------
            // Now lets add only CEIL quads and UVs...
            // ---------------------------------------
            for (int y = 0; y < MAP_LENGTH; y++)
            {
                for (int x = 0; x < MAP_LENGTH; x++)
                {
                    int index = x + y * MAP_LENGTH;
                    if (!APP_level_file.map[index].is_lightmapped) continue;

                    // Make ceil quad only if cell type id not solid OR there is non regular wall

                    if (
                            APP_level_file.map[index].cell_type == LV_C_NOT_SOLID ||
                           (APP_level_file.map[index].cell_type >= LV_C_WALL_THIN_HORIZONTAL && APP_level_file.map[index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
                       )
                    {
                        if (_mode == 0)
                        {
                            // add ceil quad
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)(y * mirror_grid), (double)(1.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)(x * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));
                            fprintf(output_obj_file, "v %.2f %.2f %.2f\n", (double)((x + 1.0) * grid), (double)((y + 1.0) * mirror_grid), (double)(1.0 * grid));

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

                            curr_x += fraction;
                            v_num += 4;
                        }

                        APP_level_file.map[index].flat_lightmap_id[id_ceil] = q_num;
                        q_num++;                       

                        APP_level_file.flat_lightmaps_count++;
                    }
                }
            }

            // Finalize the OBJ file.
            if (_mode == 0)
            {
                // Write the number vertices.
                fprintf(output_obj_file, "# %d vertices\n\n", v_num);

                // copy tex coords buffor to file
                fwrite(tex_coords_buffer, sizeof(char), strlen(tex_coords_buffer), output_obj_file);

                // Write the number of tex coords.
                fprintf(output_obj_file, "# %d texture coords\n\n", v_num);

                // Name of map model.
                fprintf(output_obj_file, "o MAP\ng MAP\n");

                // Write list of quads.
                for (int v = 0; v < v_num; v += 4)
                {
                    fprintf(output_obj_file, "f %d/%d %d/%d %d/%d %d/%d\n", v + 3, v + 3, v + 1, v + 1, v + 2, v + 2, v + 4, v + 4);
                }

                // Write the number of quads.
                fprintf(output_obj_file, "# %d polygons\n\n", q_num);

                // --- END of exporting basic map geometry ---


                // --- BEGIN of exporting helper object ---

                // We also need take care of the WALL_BOX_SHORT. We need to export only top and bottom caps for wach wall ofthat type.
                // We will write this geometry to the OBJ file but as a separate helper object. It will be used to generate light/shadows correctly,
                // so we dont want that kind of wall be "empty" inside. This geometry will not have any lightmaps, its just helper.
                // We dont need texture coords for this. Only vertices and faces of the top and bottom cap of the short wall.

                int new_v_num = 0;
                int new_q_num = 0;

                for (int y = 0; y < MAP_LENGTH; y++)
                {
                    for (int x = 0; x < MAP_LENGTH; x++)
                    {
                        int index = x + y * MAP_LENGTH;

                        switch (APP_level_file.map[index].cell_type)
                        {
                            case LV_C_WALL_BOX_SHORT:
                            {
                                double real_x0 = (double)x + (double)APP_level_file.map[index].wall_vertex[0][0];
                                double real_y0 = (double)y + (double)APP_level_file.map[index].wall_vertex[0][1];
                                double real_x1 = (double)x + (double)APP_level_file.map[index].wall_vertex[1][0];
                                double real_y1 = (double)y + (double)APP_level_file.map[index].wall_vertex[1][1];

                                double real_z0 = (1.0 - (double)APP_level_file.map[index].starting_height - (double)APP_level_file.map[index].height);
                                double real_z1 = (1.0 - (double)APP_level_file.map[index].starting_height);

                                // top cap
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z0 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z0 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y1 * mirror_grid, real_z0 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y1 * mirror_grid, real_z0 * grid);

                                // bottom cap
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x0 * grid, real_y1 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y0 * mirror_grid, real_z1 * grid);
                                fprintf(output_obj_file, "v %.2f %.2f %.2f\n", real_x1 * grid, real_y1 * mirror_grid, real_z1 * grid);

                                v_num += 8;
                                q_num += 2;

                                new_v_num += 8;
                                new_q_num += 2;
                            }
                            break;
                        }
                    }
                }

                // If we exported new model write info to file.
                if (new_v_num > 0)
                {
                    // Put some additional starting info to the file..
                    fprintf(output_obj_file, "#\n# object OTHER_GEOMETRY\n#\n\n");

                    // Name of helper model.
                    fprintf(output_obj_file, "\no OTHER_GEOMETRY\ng OTHER_GEOMETRY\n\n");

                    // Write list of quads.
                    for (int v = (v_num - new_v_num); v < v_num; v += 4)
                    {
                        fprintf(output_obj_file, "f %d %d %d %d\n", v + 3, v + 1, v + 2, v + 4);
                    }

                    // Write the number of quads of helper model.
                    fprintf(output_obj_file, "\n# %d polygons\n\n", q_num);
                }


                // --- END of exporting helper object ---


                // Close the OBJ file.
                fclose(output_obj_file);

                // Free buffers.
                free(tex_coords_buffer);
            }
        }
    }
}


// ---------------------------------
// --- GUI FUNCTIONS DEFINITIONS ---
// ---------------------------------


void GUI_Create(HWND _hwnd)
{
    // init gui stuff
    h_font_1 = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Segoe UI"));

    hp_none = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
    hp_map_not_found_texture = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

    hp_main_grid = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    hp_floor_grid = CreatePen(PS_SOLID, 1, RGB(45, 36, 28));
    hp_ceil_grid = CreatePen(PS_SOLID, 1, RGB(31, 38, 50));
    hp_lightmaps_grid = CreatePen(PS_SOLID, 1, RGB(20, 20, 20));

    hp_floor_dotted = CreatePen(PS_DOT, 0, RGB(230, 230, 230));
    hp_ceil_dotted = CreatePen(PS_DOT, 0, RGB(230, 230, 230));
    hp_lightmaps_dotted = CreatePen(PS_DOT, 0, RGB(230, 230, 230));

    hp_map_walls_dotted = CreatePen(PS_DOT, 0, RGB(200, 200, 200)); 
    hp_map_doors_dotted = CreatePen(PS_DOT, 0, RGB(240, 240, 240));

    hbr_map_not_found_texture = CreateSolidBrush(RGB(200, 0, 0));
    hbr_black = CreateSolidBrush(RGB(0, 0, 0));
    hbr_map_player = CreateSolidBrush(RGB(0, 200, 0));

    hbr_map_walls_standard = CreateSolidBrush(RGB(100, 100, 100));
    hbr_map_walls_fourside_01 = hbr_map_walls_standard;
    hbr_map_walls_fourside_02 = CreateSolidBrush(RGB(110, 110, 110));
    hbr_map_walls_short = CreateSolidBrush(RGB(80, 80, 80));

    hp_map_walls_thin = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));

    hbr_map_doors = CreateSolidBrush(RGB(180, 180, 180));
    hp_map_doors_thin = CreatePen(PS_SOLID, 2, RGB(180, 180, 180));

    hbr_drawing_bg = CreateSolidBrush(RGB(70, 70, 70));
    hp_drawing_grid = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    hp_drawing_line = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
    
    hbr_drawing_door_direction = CreateSolidBrush(RGB(250, 250, 250));

    hbr_map_floor = CreateSolidBrush(RGB(110, 8, 8));
    hbr_map_ceil = CreateSolidBrush(RGB(40, 60, 100));
    hbr_map_lightmaps = CreateSolidBrush(RGB(100, 100, 100));

    hbr_main_bg = CreateSolidBrush(RGB(30, 30, 30));
    hbr_floor_bg = CreateSolidBrush(RGB(34, 27, 21));
    hbr_ceil_bg = CreateSolidBrush(RGB(23, 28, 37));
    hbr_lightmaps_bg = CreateSolidBrush(RGB(45, 45, 45));

    hbr_selected_cell = CreateSolidBrush(RGB(255, 255, 0));

    hbr_mode_brush_add = CreateSolidBrush(RGB(0, 255, 0));
    hbr_mode_brush_clear = CreateSolidBrush(RGB(255, 0, 0));

    // ---- EDITOR WINDOW -----

    int window_margin = 5;

    // graphic editor window
    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        window_margin, window_margin, MAP_WINDOW_SIZE + 2, MAP_WINDOW_SIZE + 2, _hwnd, (HMENU)IDS_MAP, GetModuleHandle(NULL), nullptr);

    // fit button
    CreateWindowEx(0, L"BUTTON", L"Fit to window", WS_CHILD | WS_VISIBLE,
        window_margin, 972, 140, 44, _hwnd, (HMENU)IDB_MAP_FIT, GetModuleHandle(NULL), nullptr);

    // info
    CreateWindowEx(0, L"STATIC", L"selected: ", WS_CHILD | WS_VISIBLE | SS_LEFT,
        810, 973, 110, 40, _hwnd, (HMENU)IDS_MAP_SELECTED_COORDS_L, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
        868, 973, 110, 40, _hwnd, (HMENU)IDS_MAP_SELECTED_COORDS, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"hover: ", WS_CHILD | WS_VISIBLE | SS_LEFT,
        825, 993, 110, 40, _hwnd, (HMENU)IDS_MAP_HOVER_COORDS_L, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
        868, 993, 110, 40, _hwnd, (HMENU)IDS_MAP_HOVER_COORDS, GetModuleHandle(NULL), nullptr);

    // font assign
    SendDlgItemMessage(_hwnd, IDB_MAP_FIT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDB_MAP_CONTROLS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_MAP_INFO1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_MAP_SELECTED_COORDS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_MAP_SELECTED_COORDS_L, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_MAP_HOVER_COORDS, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_MAP_HOVER_COORDS_L, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // --- LEVEL FILE SIZE INFO ----

    int file_groupbox_x = 980, file_groupbox_y = 12;

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        file_groupbox_x, file_groupbox_y, 210, 118, _hwnd, (HMENU)-1, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L" Level file info:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        file_groupbox_x + 5, file_groupbox_y - 10, 90, 20, _hwnd, (HMENU)IDS_RES_FILE_LABEL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Header size:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        file_groupbox_x + 10, file_groupbox_y + 18, 75, 20, _hwnd, (HMENU)IDS_RES_FILEH_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        file_groupbox_x + 110, file_groupbox_y + 18, 90, 20, _hwnd, (HMENU)IDS_RES_FILEH, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Data size:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        file_groupbox_x + 10, file_groupbox_y + 36, 70, 20, _hwnd, (HMENU)IDS_RES_FILED_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        file_groupbox_x + 110, file_groupbox_y + 36, 90, 20, _hwnd, (HMENU)IDS_RES_FILED, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Total:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        file_groupbox_x + 10, file_groupbox_y + 60, 90, 20, _hwnd, (HMENU)IDS_RES_FILE_TOTAL_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        file_groupbox_x + 110, file_groupbox_y + 60, 90, 20, _hwnd, (HMENU)IDS_RES_FILE_TOTAL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE,
        file_groupbox_x + 8, file_groupbox_y + 84, 193, 24, _hwnd, (HMENU)IDB_RES_FILE_REFRESH, GetModuleHandle(NULL), NULL);


    SendDlgItemMessage(_hwnd, IDB_RES_FILE_REFRESH, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_FILE_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_FILEH_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_FILEH, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_FILED_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_FILED, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_FILE_TOTAL_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_FILE_TOTAL, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // --- LEVEL MEM SIZE INFO ----

    int mem_groupbox_x = 980, mem_groupbox_y = 145;

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        mem_groupbox_x, mem_groupbox_y, 210, 190, _hwnd, (HMENU)-1, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L" Level memory info:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 5, mem_groupbox_y - 10, 120, 20, _hwnd, (HMENU)IDS_RES_MEM_LABEL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Cells data:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 18, 90, 20, _hwnd, (HMENU)IDS_RES_CELLS_DATA_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 18, 90, 20, _hwnd, (HMENU)IDS_RES_CELLS_DATA, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Wall textures:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 42, 90, 20, _hwnd, (HMENU)IDS_RES_WALL_TEX_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 42, 90, 20, _hwnd, (HMENU)IDS_RES_WALL_TEX, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Flat textures:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 60, 90, 20, _hwnd, (HMENU)IDS_RES_FLAT_TEX_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 60, 90, 20, _hwnd, (HMENU)IDS_RES_FLAT_TEX, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"STATIC", L"Wall lightmaps:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 84, 90, 20, _hwnd, (HMENU)IDS_RES_W_LIGHTMAPS_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 84, 90, 20, _hwnd, (HMENU)IDS_RES_W_LIGHTMAPS, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"Flat lightmaps:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 102, 90, 20, _hwnd, (HMENU)IDS_RES_F_LIGHTMAPS_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 102, 90, 20, _hwnd, (HMENU)IDS_RES_F_LIGHTMAPS, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"Total:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        mem_groupbox_x + 10, mem_groupbox_y + 126, 90, 20, _hwnd, (HMENU)IDS_RES_MEM_TOTAL_LABEL, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        mem_groupbox_x + 105, mem_groupbox_y + 126, 90, 20, _hwnd, (HMENU)IDS_RES_MEM_TOTAL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE,
        mem_groupbox_x + 8, mem_groupbox_y + 150, 193, 24, _hwnd, (HMENU)IDB_RES_MEM_REFRESH, GetModuleHandle(NULL), NULL);


    SendDlgItemMessage(_hwnd, IDB_RES_MEM_REFRESH, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_MEM_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_CELLS_DATA_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_CELLS_DATA, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_WALL_TEX, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_FLAT_TEX_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_FLAT_TEX, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_W_LIGHTMAPS_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_W_LIGHTMAPS, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_F_LIGHTMAPS_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_F_LIGHTMAPS, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDS_RES_MEM_TOTAL_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_RES_MEM_TOTAL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    // ---- LAYERS ----

    int layers_groupbox_x = 1200, layers_groupbox_y = 12;

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        layers_groupbox_x, layers_groupbox_y, 200, 152, _hwnd, (HMENU)IDS_LAYERS_GROUPBOX, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L" Layers:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        layers_groupbox_x + 5, layers_groupbox_y - 10, 50, 20, _hwnd, (HMENU)IDS_LAYERS_LABEL1, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        layers_groupbox_x + 15, layers_groupbox_y + 15, 170, 147, _hwnd, (HMENU)IDL_LAYERS_LIST, GetModuleHandle(NULL), NULL);

    SendDlgItemMessage(_hwnd, IDS_LAYERS_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"01. Player");
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"02. Walls");
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"03. Floors");
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"04. Ceilings");
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"05. Lightmaps");
    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), (UINT)LB_ADDSTRING, 0, (LPARAM)L"06. Doors");

    SendMessage(GetDlgItem(_hwnd, IDL_LAYERS_LIST), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    // ---- LIST BOX ----

    int list_groupbox_x = 1410, list_groupbox_y = 12;

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        list_groupbox_x, list_groupbox_y, 320, 152, _hwnd, (HMENU)IDS_LIST_GROUPBOX, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L" List of elements:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        list_groupbox_x + 5, list_groupbox_y - 10, 100, 20, _hwnd, (HMENU)IDS_LIST_LABEL1, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        list_groupbox_x + 12, list_groupbox_y + 15, 295, 147, _hwnd, (HMENU)IDL_LIST_LIST, GetModuleHandle(NULL), nullptr);

    SendDlgItemMessage(_hwnd, IDS_LIST_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // --- PROPERTIES ---

    int prop_groupbox_x = 1200, prop_groupbox_y = 176;

    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        prop_groupbox_x, prop_groupbox_y, 530, 520, _hwnd, (HMENU)IDS_PROPERTIES_GROUPBOX, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"STATIC", L" Properties:", WS_CHILD | WS_VISIBLE | SS_LEFT,
        prop_groupbox_x + 5, prop_groupbox_y - 10, 80, 20, _hwnd, (HMENU)IDS_PROPERTIES_LABEL1, GetModuleHandle(NULL), nullptr);

    SendDlgItemMessage(_hwnd, IDS_PROPERTIES_LABEL1, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // PROPERTIES --- player start
    CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
        prop_groupbox_x + 15, prop_groupbox_y + 25, 250, 60, _hwnd, (HMENU)IDS_PROP_PLAYER_L1, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"BUTTON", L"0* - RIGHT", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 310, prop_groupbox_y + 180, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_RIGHT, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"90* - UP", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 180, prop_groupbox_y + 110, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_UP, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"180* - LEFT", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 50, prop_groupbox_y + 180, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_LEFT, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"270* - DOWN", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 180, prop_groupbox_y + 250, 150, 60, _hwnd, (HMENU)IDB_PROP_PLAYER_DOWN, GetModuleHandle(NULL), NULL);

    SendDlgItemMessage(_hwnd, IDS_PROP_PLAYER_L1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_RIGHT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_UP, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_LEFT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDB_PROP_PLAYER_DOWN, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // PROPERTIES --- 4 textures window for walls etc.

    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 190, prop_groupbox_y + 20, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_TOP, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 167, prop_groupbox_y + 20, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_TOP_SELECT, GetModuleHandle(NULL), NULL);

    CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                     prop_groupbox_x + 180, prop_groupbox_y + 150, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_TOP_LABEL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 370, prop_groupbox_y + 170, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_RIGHT, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 348, prop_groupbox_y + 170, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_RIGHT_SELECT, GetModuleHandle(NULL), NULL);

    CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                       prop_groupbox_x + 360, prop_groupbox_y + 300, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_RIGHT_LABEL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 190, prop_groupbox_y + 320, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_BOTTOM, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 167, prop_groupbox_y + 320, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_BOTTOM_SELECT, GetModuleHandle(NULL), NULL);

       CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                      prop_groupbox_x + 180, prop_groupbox_y + 450, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_BOTTOM_LABEL, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 33, prop_groupbox_y + 170, 130, 130, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_LEFT, GetModuleHandle(NULL), nullptr);

    CreateWindowEx(0, L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + 10, prop_groupbox_y + 170, 22, 22, _hwnd, (HMENU)IDB_PROP_WALL_IMAGE_LEFT_SELECT, GetModuleHandle(NULL), NULL);

     CreateWindowEx( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
                     prop_groupbox_x + 20, prop_groupbox_y + 300, 150, 20, _hwnd, (HMENU)IDS_PROP_WALL_IMAGE_LEFT_LABEL, GetModuleHandle(NULL), nullptr);

    SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_RIGHT_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_BOTTOM_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    SendDlgItemMessage(_hwnd, IDB_PROP_WALL_IMAGE_LEFT_SELECT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // PROPERTIES -- WALL DRAWING

    int wall_draw_group_x = 30;
    int wall_draw_group_y = 35;


    // thin wall horizintal

    CreateWindowEx(0, L"BUTTON", L"˄", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 167, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_HOR_UP, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"÷", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 183, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_HOR_TURN, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"˅", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 200, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_HOR_DOWN, GetModuleHandle(NULL), NULL);


    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_HOR_UP, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_HOR_TURN, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_HOR_DOWN, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // thin wall vertical

    CreateWindowEx(0, L"BUTTON", L"<", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 202, prop_groupbox_y + wall_draw_group_y + 232, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_VER_LEFT, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"÷", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 218, prop_groupbox_y + wall_draw_group_y + 232, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_VER_TURN, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L">", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 235, prop_groupbox_y + wall_draw_group_y + 232, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_VER_RIGHT, GetModuleHandle(NULL), NULL);


    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_VER_LEFT, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_VER_TURN, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_VER_RIGHT, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    // thin wall oblique

    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 185, prop_groupbox_y + wall_draw_group_y + 152, 80, 80, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING, GetModuleHandle(NULL), nullptr);


    CreateWindowEx(0, L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 151, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_PLUS1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"-", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 165, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_MINUS1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 266, prop_groupbox_y + wall_draw_group_y + 151, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_PLUS2, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"-", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 266, prop_groupbox_y + wall_draw_group_y + 165, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_MINUS2, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"/", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 185, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_SLASH, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"\\", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 205, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_BSLASH, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"÷", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 250, prop_groupbox_y + wall_draw_group_y + 232, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_OBL_TURN, GetModuleHandle(NULL), NULL);

    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_PLUS1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_MINUS1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_PLUS2, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_MINUS2, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_SLASH, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_BSLASH, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_OBL_TURN, WM_SETFONT, (WPARAM)h_font_1, TRUE);


    // drawing window - boxwall - buttons

    CreateWindowEx(0, L"BUTTON", L"-", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 151, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_UP_1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"--", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 170, prop_groupbox_y + wall_draw_group_y + 165, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_UP_2, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"-", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 266, prop_groupbox_y + wall_draw_group_y + 181, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_BOTTOM_1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"--", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 266, prop_groupbox_y + wall_draw_group_y + 195, 15, 15, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_BOTTOM_2, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L">", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 225, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_RIGHT_1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L">|", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 245, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_RIGHT_2, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"<", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 205, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_LEFT_1, GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, L"BUTTON", L"|<", WS_CHILD | WS_VISIBLE,
        prop_groupbox_x + wall_draw_group_x + 185, prop_groupbox_y + wall_draw_group_y + 232, 18, 18, _hwnd, (HMENU)IDS_PROP_WALL_DRAWING_LEFT_2, GetModuleHandle(NULL), NULL);

    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_UP_1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_UP_2, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1, WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendDlgItemMessage(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2, WM_SETFONT, (WPARAM)h_font_1, TRUE);

    //  group
    CreateWindowEx(0, L"STATIC", L"Group: ", WS_CHILD | WS_VISIBLE | SS_LEFT, prop_groupbox_x + 340, prop_groupbox_y + 23, 60, 20, _hwnd, (HMENU)IDC_PROP_GROUP_LABEL, GetModuleHandle(NULL), nullptr);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_LABEL), WM_SETFONT, (WPARAM)h_font_1, TRUE);

    CreateWindow(WC_COMBOBOX, TEXT(""), CBS_DROPDOWNLIST | WS_VSCROLL | CBS_NOINTEGRALHEIGHT | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                 prop_groupbox_x + 390, prop_groupbox_y + 20, 125, 200, _hwnd, (HMENU)IDC_PROP_GROUP_COMBO, GetModuleHandle(NULL), NULL);

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), WM_SETFONT, (WPARAM)h_font_1, TRUE);    
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"0 - no group");

    TCHAR text[3];
    ZeroMemory(text, sizeof(text));

    for (int i = 1; i < 32; i++)
    {
        ZeroMemory(text, sizeof(text));
        swprintf_s(text, L"%d", i);
        SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)text);
    }

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    //  timer
    CreateWindowEx(0, L"STATIC", L"Timer: ", WS_CHILD | WS_VISIBLE | SS_LEFT, prop_groupbox_x + 340, prop_groupbox_y + 53, 60, 20, _hwnd, (HMENU)IDC_PROP_TIMER_LABEL, GetModuleHandle(NULL), nullptr);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_LABEL), WM_SETFONT, (WPARAM)h_font_1, TRUE);

    CreateWindow(WC_COMBOBOX, TEXT(""), CBS_DROPDOWNLIST | WS_VSCROLL | CBS_NOINTEGRALHEIGHT | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, prop_groupbox_x + 390, prop_groupbox_y + 50, 125, 200, _hwnd, (HMENU)IDC_PROP_TIMER_COMBO, GetModuleHandle(NULL), NULL);

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"0 - stays opened");
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"1 - will close");

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

    //  action
    CreateWindowEx(0, L"STATIC", L"Action: ", WS_CHILD | WS_VISIBLE | SS_LEFT, prop_groupbox_x + 340, prop_groupbox_y + 83, 60, 20, _hwnd, (HMENU)IDC_PROP_ACTION_LABEL, GetModuleHandle(NULL), nullptr);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_LABEL), WM_SETFONT, (WPARAM)h_font_1, TRUE);

    CreateWindow(WC_COMBOBOX, TEXT(""), CBS_DROPDOWNLIST | WS_VSCROLL | CBS_NOINTEGRALHEIGHT | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, prop_groupbox_x + 390, prop_groupbox_y + 80, 125, 200, _hwnd, (HMENU)IDC_PROP_ACTION_COMBO, GetModuleHandle(NULL), NULL);

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"0 - push");
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"1 - proxmity");

    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    // PROPERTIES -- HEIGHT WALL DRAWING

    int wall_height_group_x = 15;
    int wall_height_group_y = 45;

    CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE, prop_groupbox_x + wall_height_group_x, prop_groupbox_y + wall_height_group_y, 80, 80, _hwnd, (HMENU)IDS_PROP_WALL_HEIGHT, GetModuleHandle(NULL), nullptr);
    CreateWindowEx(0, L"STATIC", L"Wall height:", WS_CHILD | WS_VISIBLE | SS_LEFT, prop_groupbox_x + wall_height_group_x, prop_groupbox_y + wall_height_group_y - 20, 80, 20, _hwnd, (HMENU)IDS_PROP_WALL_HEIGHT_LABEL, GetModuleHandle(NULL), nullptr);
    CreateWindowEx(0, L"BUTTON", L"˄", WS_CHILD | WS_VISIBLE, prop_groupbox_x + wall_height_group_x + 82, prop_groupbox_y + wall_height_group_y, 15, 15, _hwnd, (HMENU)IDB_PROP_WALL_HEIGHT_SH_UP, GetModuleHandle(NULL), NULL);
    CreateWindowEx(0, L"BUTTON", L"˅", WS_CHILD | WS_VISIBLE, prop_groupbox_x + wall_height_group_x + 82, prop_groupbox_y + wall_height_group_y + 15, 15, 15, _hwnd, (HMENU)IDB_PROP_WALL_HEIGHT_SH_DOWN, GetModuleHandle(NULL), NULL);
    CreateWindowEx(0, L"BUTTON", L"˄", WS_CHILD | WS_VISIBLE, prop_groupbox_x + wall_height_group_x + 82, prop_groupbox_y + wall_height_group_y + 50, 15, 15, _hwnd, (HMENU)IDB_PROP_WALL_HEIGHT_H_UP, GetModuleHandle(NULL), NULL);
    CreateWindowEx(0, L"BUTTON", L"˅", WS_CHILD | WS_VISIBLE, prop_groupbox_x + wall_height_group_x + 82, prop_groupbox_y + wall_height_group_y + 50 + 15, 15, 15, _hwnd, (HMENU)IDB_PROP_WALL_HEIGHT_H_DOWN, GetModuleHandle(NULL), NULL);

    SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT_LABEL), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_UP), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_DOWN), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_UP), WM_SETFONT, (WPARAM)h_font_1, TRUE);
    SendMessage(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_DOWN), WM_SETFONT, (WPARAM)h_font_1, TRUE);
}
void GUI_Reset(HWND _hwnd)
{
    gui_current_layer = 0;
    gui_current_list_index = -1;

    gui_texprew_mode = 0;

    //  get from file structure ---
    SetWindowText(_hwnd, APP_filename);

    GUI_Update_Resources_File(_hwnd);
    GUI_Update_Resources_Memory(_hwnd);
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

void GUI_Update_Resources_File(HWND _hwnd)
{
    TCHAR output[16];

    // File header size.
        TCHAR t_file_header_size[16];
        ZeroMemory(t_file_header_size, sizeof(t_file_header_size));

        int file_header_size = sizeof(sLV_Level__File_Only);
        Add_Number_Spaces(file_header_size, t_file_header_size);

        ZeroMemory(output, sizeof(output));
        swprintf_s(output, L"%s Bytes", t_file_header_size);
        SendMessage(GetDlgItem(_hwnd, IDS_RES_FILEH), WM_SETTEXT, 0, (LPARAM)output);

    // File header size.
        TCHAR t_file_data_size[16];
        ZeroMemory(t_file_data_size, sizeof(t_file_data_size));

        int file_data_size = 0;
        Add_Number_Spaces(file_data_size, t_file_data_size);

        ZeroMemory(output, sizeof(output));
        swprintf_s(output, L"%s Bytes", t_file_data_size);
        SendMessage(GetDlgItem(_hwnd, IDS_RES_FILED), WM_SETTEXT, 0, (LPARAM)output);

    // File total size.
        TCHAR t_file_total_size[16];
        ZeroMemory(t_file_total_size, sizeof(t_file_total_size));
        Add_Number_Spaces(file_header_size + file_data_size, t_file_total_size);

        ZeroMemory(output, sizeof(output));
        swprintf_s(output, L"%s Bytes", t_file_total_size);
        SendMessage(GetDlgItem(_hwnd, IDS_RES_FILE_TOTAL), WM_SETTEXT, 0, (LPARAM)output);
}
void GUI_Update_Resources_Memory(HWND _hwnd)
{
    TCHAR output[16];

    // Cell data.
    TCHAR t_file_header_size[16];
    ZeroMemory(t_file_header_size, sizeof(t_file_header_size));

    int32 cell_data_size = sizeof(sLV_Cell) * LV_MAP_CELLS_COUNT;
    Add_Number_Spaces(cell_data_size, t_file_header_size);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%s Bytes", t_file_header_size);
    SendMessage(GetDlgItem(_hwnd, IDS_RES_CELLS_DATA), WM_SETTEXT, 0, (LPARAM)output);

    // Wall textures count and memory.
    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%d:   %.2f MB", TEX_wall_count, TEX_wall_mem / (1024.0f*1024.0f) );
    SendMessage(GetDlgItem(_hwnd, IDS_RES_WALL_TEX), WM_SETTEXT, 0, (LPARAM)output);

    // Flat textures count and memory.
    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%d:   %.2f MB", TEX_flat_count, TEX_flat_mem / (1024.0f * 1024.0f) );
    SendMessage(GetDlgItem(_hwnd, IDS_RES_FLAT_TEX), WM_SETTEXT, 0, (LPARAM)output);

    // Lightmapas count and memory.
    int wall_lm_mem = (APP_level_file.wall_lightmaps_count * IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE);
    int flat_lm_mem = (APP_level_file.flat_lightmaps_count * IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%d:  %.2f MB", APP_level_file.wall_lightmaps_count, (float)wall_lm_mem / (1024.0f * 1024.0f));
    SendMessage(GetDlgItem(_hwnd, IDS_RES_W_LIGHTMAPS), WM_SETTEXT, 0, (LPARAM)output);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%d:  %.2f MB", APP_level_file.flat_lightmaps_count, (float)flat_lm_mem / (1024.0f * 1024.0f));
    SendMessage(GetDlgItem(_hwnd, IDS_RES_F_LIGHTMAPS), WM_SETTEXT, 0, (LPARAM)output);
      
    // Total.
    TCHAR t_total_mem_bytes[16];
    ZeroMemory(t_total_mem_bytes, sizeof(t_total_mem_bytes));

    int32 total_mem_bytes = TEX_wall_mem + TEX_flat_mem + wall_lm_mem + flat_lm_mem + cell_data_size;
    Add_Number_Spaces(total_mem_bytes, t_total_mem_bytes);

    ZeroMemory(output, sizeof(output));
    swprintf_s(output, L"%s Bytes", t_total_mem_bytes);
    SendMessage(GetDlgItem(_hwnd, IDS_RES_MEM_TOTAL), WM_SETTEXT, 0, (LPARAM)output);
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
    SendMessage(GetDlgItem(APP_hWnd, IDL_LAYERS_LIST), LB_SETCURSEL, (WPARAM)gui_current_layer, (LPARAM)0);
}
void GUI_Update_List_Box(HWND _hwnd)
{
    SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), (UINT)LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    switch (gui_current_layer)
    {
        case LAYERS_PLAYER_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Player start");
            break;

        case LAYERS_WALL_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Wall standard");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"02. Wall fourside");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"03. Wall thin horizontal");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"04. Wall thin vertical");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"05. Wall thin oblique");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"06. Wall box");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"07. Wall box fourside");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"08. Wall box short");
            break;

        case LAYERS_FLOOR_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Floor");
            break;

        case LAYERS_CEIL_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Ceiling");
            break;

        case LAYERS_LIGHTMAPS_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Lightmaps selection");
            break;

        case LAYERS_DOOR_LAYER:
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"01. Door thick horizontal");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"02. Door thick vertical");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"03. Door thin oblique");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"04. Door box fourside horizontal-left");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"05. Door box fourside horizontal-right");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"06. Door box fourside vertical-left");
            SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_ADDSTRING, 0, (LPARAM)L"07. Door box fourside vertical-right");
            break;
    }

    SendMessage(GetDlgItem(_hwnd, IDL_LIST_LIST), LB_SETCURSEL, (WPARAM)gui_current_list_index, (LPARAM)0);
}

void GUI_Update_Properties(HWND _hwnd)
{
    GUI_Hide_Properties(_hwnd);

    switch (gui_current_layer)
    {
        case LAYERS_PLAYER_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_PLAYER:
                    GUI_Update_Properties_Player(_hwnd);
                    break;
            }
            break;

        case LAYERS_WALL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_WALL_STANDARD:
                case LIST_WALL_FOURSIDE:
                case LIST_WALL_THIN_HORIZONTAL:
                case LIST_WALL_THIN_VERTICAL:
                case LIST_WALL_THIN_OBLIQUE:
                case LIST_WALL_BOX:
                case LIST_WALL_BOX_FOURSIDE:
                case LIST_WALL_BOX_SHORT:
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
                    GUI_Update_Properties_Lightmaps(_hwnd);
                    break;
            }
            break;

        case LAYERS_DOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_DOOR_THICK_HORIZONTAL:
                case LIST_DOOR_THICK_VERTICAL:
                case LIST_DOOR_THIN_OBLIQUE:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    GUI_Update_Properties_Doors(_hwnd);
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
        case LIST_WALL_STANDARD:
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
            break;

        case LIST_WALL_FOURSIDE:
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_right);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_bottom);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_left);

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
            break;


        case LIST_WALL_THIN_HORIZONTAL:  

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_WALL_THIN_HORIZONTAL)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.0f;
                MAP_working_cell.wall_vertex[0][1] = 0.5f;
                MAP_working_cell.wall_vertex[1][0] = 1.0f;
                MAP_working_cell.wall_vertex[1][1] = 0.5f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_UP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_TURN), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_DOWN), TRUE);
            break;


        case LIST_WALL_THIN_VERTICAL:

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_WALL_THIN_VERTICAL)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.5f;
                MAP_working_cell.wall_vertex[0][1] = 0.0f;
                MAP_working_cell.wall_vertex[1][0] = 0.5f;
                MAP_working_cell.wall_vertex[1][1] = 1.0f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_LEFT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_TURN), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_RIGHT), TRUE);
            break;


        case LIST_WALL_THIN_OBLIQUE:

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_WALL_THIN_OBLIQUE)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.0f;
                MAP_working_cell.wall_vertex[0][1] = 0.0f;
                MAP_working_cell.wall_vertex[1][0] = 1.0f;
                MAP_working_cell.wall_vertex[1][1] = 1.0f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_SLASH), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BSLASH), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_OBL_TURN), TRUE);
            break;


        case LIST_WALL_BOX:

            // validate current values
            if ((MAP_working_cell.cell_type != LV_C_WALL_BOX) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_FOURSIDE) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_SHORT))
            {
                MAP_working_cell.wall_vertex[0][0] = 0.2f;
                MAP_working_cell.wall_vertex[0][1] = 0.2f;
                MAP_working_cell.wall_vertex[1][0] = 0.8f;
                MAP_working_cell.wall_vertex[1][1] = 0.8f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2), TRUE);
            break;


        case LIST_WALL_BOX_FOURSIDE:

            // validate current values
            if ((MAP_working_cell.cell_type != LV_C_WALL_BOX) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_FOURSIDE) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_SHORT))
            {
                MAP_working_cell.wall_vertex[0][0] = 0.2f;
                MAP_working_cell.wall_vertex[0][1] = 0.2f;
                MAP_working_cell.wall_vertex[1][0] = 0.8f;
                MAP_working_cell.wall_vertex[1][1] = 0.8f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_right);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_bottom);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_left);

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

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2), TRUE);
            break;


        case LIST_WALL_BOX_SHORT:

            // validate current values
            if ((MAP_working_cell.cell_type != LV_C_WALL_BOX) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_FOURSIDE) && (MAP_working_cell.cell_type != LV_C_WALL_BOX_SHORT))
            {
                MAP_working_cell.wall_vertex[0][0] = 0.2f;
                MAP_working_cell.wall_vertex[0][1] = 0.2f;
                MAP_working_cell.wall_vertex[1][0] = 0.8f;
                MAP_working_cell.wall_vertex[1][1] = 0.8f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT_LABEL), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_UP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_DOWN), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_UP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_DOWN), TRUE);

            break;
    }
}
void GUI_Update_Properties_Floor(HWND _hwnd)
{
    GUI_Update_Properties_Flat_Tex_Name(_hwnd, id_floor);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
}
void GUI_Update_Properties_Ceil(HWND _hwnd)
{
    GUI_Update_Properties_Flat_Tex_Name(_hwnd, id_ceil);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);
}
void GUI_Update_Properties_Lightmaps(HWND _hwnd)
{
    switch (gui_current_list_index)
    {
        case LIST_LIGHTMAPS_SELECT:
            break;
    }
}
void GUI_Update_Properties_Doors(HWND _hwnd)
{
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_GROUP_LABEL), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), TRUE);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), CB_SETCURSEL, (WPARAM)MAP_working_cell.cell_group, (LPARAM)0);

    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_TIMER_LABEL), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), TRUE);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), CB_SETCURSEL, (WPARAM)MAP_working_cell.cell_timer, (LPARAM)0);

    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_ACTION_LABEL), TRUE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), TRUE);
    SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), CB_SETCURSEL, (WPARAM)MAP_working_cell.cell_action, (LPARAM)0);

    switch (gui_current_list_index)
    {
        case LIST_DOOR_THICK_HORIZONTAL:

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_DOOR_THICK_HORIZONTAL)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.0f;
                MAP_working_cell.wall_vertex[0][1] = 0.5f;
                MAP_working_cell.wall_vertex[1][0] = 1.0f;
                MAP_working_cell.wall_vertex[1][1] = 0.6f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_bottom);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_BOTTOM_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_UP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_DOWN), TRUE);
            break;


        case LIST_DOOR_THICK_VERTICAL:

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_DOOR_THICK_VERTICAL)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.5f;
                MAP_working_cell.wall_vertex[0][1] = 0.0f;
                MAP_working_cell.wall_vertex[1][0] = 0.6f;
                MAP_working_cell.wall_vertex[1][1] = 1.0f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_right);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_left);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_LEFT_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_RIGHT_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_LEFT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_RIGHT), TRUE);
            break;


        case LIST_DOOR_THIN_OBLIQUE:

            // validate current values
            if (MAP_working_cell.cell_type != LV_C_DOOR_THIN_OBLIQUE)
            {
                MAP_working_cell.wall_vertex[0][0] = 0.0f;
                MAP_working_cell.wall_vertex[0][1] = 0.0f;
                MAP_working_cell.wall_vertex[1][0] = 1.0f;
                MAP_working_cell.wall_vertex[1][1] = 1.0f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_IMAGE_TOP_SELECT), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_SLASH), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BSLASH), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_OBL_TURN), TRUE);
            break;

        case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
        case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:

            // validate current values
            if ( (MAP_working_cell.cell_type != LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT) && (MAP_working_cell.cell_type != LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT) )
            {
                MAP_working_cell.wall_vertex[0][0] = 0.0f;
                MAP_working_cell.wall_vertex[0][1] = 0.2f;
                MAP_working_cell.wall_vertex[1][0] = 1.0f;
                MAP_working_cell.wall_vertex[1][1] = 0.8f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_right);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_bottom);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_left);

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

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2), TRUE);
            break;


        case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
        case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:

            // validate current values
            if ((MAP_working_cell.cell_type != LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT) && (MAP_working_cell.cell_type != LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT))
            {
                MAP_working_cell.wall_vertex[0][0] = 0.2f;
                MAP_working_cell.wall_vertex[0][1] = 0.0f;
                MAP_working_cell.wall_vertex[1][0] = 0.8f;
                MAP_working_cell.wall_vertex[1][1] = 1.0f;
            }
            else
            {
                MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
                MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
                MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
                MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];
            }

            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_top);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_right);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_bottom);
            GUI_Update_Properties_Wall_Tex_Name(_hwnd, id_left);

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

            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1), TRUE);
            ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2), TRUE);
            break;
    }
}

void GUI_Update_Properties_Drawing_Window(HWND _hwnd, int _wm_id)
{
    if (gui_current_layer == LAYERS_WALL_LAYER)
    {
        switch (gui_current_list_index)
        {
            case LIST_WALL_THIN_HORIZONTAL:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_HOR_UP:                        
                        if ((MAP_working_cell.wall_vertex[0][1] > 0.1f+0.0001f) && (MAP_working_cell.wall_vertex[1][1] > 0.1f+0.0001f))
                        {
                            MAP_working_cell.wall_vertex[0][1] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_HOR_TURN:
                    {
                        float tmp = MAP_working_cell.wall_vertex[0][0];
                        MAP_working_cell.wall_vertex[0][0] = MAP_working_cell.wall_vertex[1][0];
                        MAP_working_cell.wall_vertex[1][0] = tmp;
                        break;
                    }

                    case IDS_PROP_WALL_DRAWING_HOR_DOWN:
                        if ( (MAP_working_cell.wall_vertex[0][1] < 0.9f-0.0001f) && (MAP_working_cell.wall_vertex[1][1] < 0.9f-0.0001))
                        {
                            MAP_working_cell.wall_vertex[0][1] += 0.1f;
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        }
                        break;
                }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_THIN_HORIZONTAL)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }

                break;
            }

            case LIST_WALL_THIN_VERTICAL:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_VER_LEFT:
                        if ((MAP_working_cell.wall_vertex[0][0] > 0.1f + 0.0001f) && (MAP_working_cell.wall_vertex[1][0] > 0.1f + 0.0001f))
                        {
                            MAP_working_cell.wall_vertex[0][0] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_VER_TURN:
                    {
                        float tmp = MAP_working_cell.wall_vertex[0][1];
                        MAP_working_cell.wall_vertex[0][1] = MAP_working_cell.wall_vertex[1][1];
                        MAP_working_cell.wall_vertex[1][1] = tmp;
                        break;
                    }

                    case IDS_PROP_WALL_DRAWING_VER_RIGHT:
                        if ((MAP_working_cell.wall_vertex[0][0] < 0.9f - 0.0001f) && (MAP_working_cell.wall_vertex[1][0] < 0.9f - 0.0001))
                        {
                            MAP_working_cell.wall_vertex[0][0] += 0.1f;
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        }
                        break;
                    }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_THIN_VERTICAL)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }

                break;
            }

            case LIST_WALL_THIN_OBLIQUE:
            {
                int index_0 = Get_Wall_Drawing_Coords_Index(MAP_working_cell.wall_vertex[0][0], MAP_working_cell.wall_vertex[0][1]);
                int index_1 = Get_Wall_Drawing_Coords_Index(MAP_working_cell.wall_vertex[1][0], MAP_working_cell.wall_vertex[1][1]);

                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_PLUS1:

                        index_0++;                    
                        if (index_0 > 39) index_0 = 0;

                        MAP_working_cell.wall_vertex[0][0] = wall_drawing_coords[index_0][0];
                        MAP_working_cell.wall_vertex[0][1] = wall_drawing_coords[index_0][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_MINUS1:

                        index_0--;
                        if (index_0 < 0) index_0 = 39;

                        MAP_working_cell.wall_vertex[0][0] = wall_drawing_coords[index_0][0];
                        MAP_working_cell.wall_vertex[0][1] = wall_drawing_coords[index_0][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_PLUS2:

                        index_1++;
                        if (index_1 > 39) index_1 = 0;

                        MAP_working_cell.wall_vertex[1][0] = wall_drawing_coords[index_1][0];
                        MAP_working_cell.wall_vertex[1][1] = wall_drawing_coords[index_1][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_MINUS2:

                        index_1--;
                        if (index_1 < 0) index_1 = 39;

                        MAP_working_cell.wall_vertex[1][0] = wall_drawing_coords[index_1][0];
                        MAP_working_cell.wall_vertex[1][1] = wall_drawing_coords[index_1][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_SLASH:
                        MAP_working_cell.wall_vertex[0][0] = 1.0f;
                        MAP_working_cell.wall_vertex[0][1] = 0.0f;
                        MAP_working_cell.wall_vertex[1][0] = 0.0f;
                        MAP_working_cell.wall_vertex[1][1] = 1.0f;
                        break;

                    case IDS_PROP_WALL_DRAWING_BSLASH:
                        MAP_working_cell.wall_vertex[0][0] = 0.0f;
                        MAP_working_cell.wall_vertex[0][1] = 0.0f;
                        MAP_working_cell.wall_vertex[1][0] = 1.0f;
                        MAP_working_cell.wall_vertex[1][1] = 1.0f;
                        break;

                    case IDS_PROP_WALL_DRAWING_OBL_TURN:
                    {
                        float tmp = MAP_working_cell.wall_vertex[0][0];
                        MAP_working_cell.wall_vertex[0][0] = MAP_working_cell.wall_vertex[1][0];
                        MAP_working_cell.wall_vertex[1][0] = tmp;

                        tmp = MAP_working_cell.wall_vertex[0][1];
                        MAP_working_cell.wall_vertex[0][1] = MAP_working_cell.wall_vertex[1][1];
                        MAP_working_cell.wall_vertex[1][1] = tmp;
                        break;
                    }
                }
            
                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_THIN_OBLIQUE)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }

                break;
            }


            case LIST_WALL_BOX:
            case LIST_WALL_BOX_FOURSIDE:
            case LIST_WALL_BOX_SHORT:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_UP_1:
                        if (MAP_working_cell.wall_vertex[0][1] >= 0.1f)
                        {
                            MAP_working_cell.wall_vertex[0][1] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_UP_2:
                        if (MAP_working_cell.wall_vertex[1][1] - 0.1f > MAP_working_cell.wall_vertex[0][0])
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_RIGHT_1:
                        if (MAP_working_cell.wall_vertex[1][0] < 1.0f)
                        {
                            MAP_working_cell.wall_vertex[0][0] += 0.1f;
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_RIGHT_2:
                        if (MAP_working_cell.wall_vertex[1][0] < 1.0f)
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_BOTTOM_1:
                        if (MAP_working_cell.wall_vertex[1][1] < 1.0f)
                        {
                            MAP_working_cell.wall_vertex[0][1] += 0.1f;
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_BOTTOM_2:
                        if (MAP_working_cell.wall_vertex[1][1] < 1.0f)
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_LEFT_1:
                        if (MAP_working_cell.wall_vertex[0][0] >= 0.1f)
                        {
                            MAP_working_cell.wall_vertex[0][0] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_LEFT_2:
                        if (MAP_working_cell.wall_vertex[1][0] - 0.1f > MAP_working_cell.wall_vertex[0][0])
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        break;
                }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_BOX          ||
                    APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_BOX_FOURSIDE ||
                    APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_BOX_SHORT)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }
            }
            break;
        }
    }


    else if (gui_current_layer == LAYERS_DOOR_LAYER)
    {
        switch (gui_current_list_index)
        {
            case LIST_DOOR_THICK_HORIZONTAL:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_HOR_UP:
                        if ((MAP_working_cell.wall_vertex[0][1] > 0.0f + 0.0001f) && (MAP_working_cell.wall_vertex[1][1] > 0.0f + 0.0001f))
                        {
                            MAP_working_cell.wall_vertex[0][1] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_HOR_TURN:
                        {
                            float tmp = MAP_working_cell.wall_vertex[0][0];
                            MAP_working_cell.wall_vertex[0][0] = MAP_working_cell.wall_vertex[1][0];
                            MAP_working_cell.wall_vertex[1][0] = tmp;
                            break;
                        }

                    case IDS_PROP_WALL_DRAWING_HOR_DOWN:
                        if ((MAP_working_cell.wall_vertex[0][1] < 1.0f - 0.0001f) && (MAP_working_cell.wall_vertex[1][1] < 1.0f - 0.0001))
                        {
                            MAP_working_cell.wall_vertex[0][1] += 0.1f;
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        }
                        break;
                }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_DOOR_THICK_HORIZONTAL)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }

                break;
            }

            case LIST_DOOR_THICK_VERTICAL:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_VER_LEFT:
                        if ((MAP_working_cell.wall_vertex[0][0] > 0.0f + 0.0001f) && (MAP_working_cell.wall_vertex[1][0] > 0.0f + 0.0001f))
                        {
                            MAP_working_cell.wall_vertex[0][0] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_VER_TURN:
                        {
                            float tmp = MAP_working_cell.wall_vertex[0][1];
                            MAP_working_cell.wall_vertex[0][1] = MAP_working_cell.wall_vertex[1][1];
                            MAP_working_cell.wall_vertex[1][1] = tmp;
                            break;
                        }

                    case IDS_PROP_WALL_DRAWING_VER_RIGHT:
                        if ((MAP_working_cell.wall_vertex[0][0] < 1.0f - 0.0001f) && (MAP_working_cell.wall_vertex[1][0] < 1.0f - 0.0001))
                        {
                            MAP_working_cell.wall_vertex[0][0] += 0.1f;
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        }
                        break;
                }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_DOOR_THICK_VERTICAL)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }
                
                break;
            }

            case LIST_DOOR_THIN_OBLIQUE:
            {
                int index_0 = Get_Wall_Drawing_Coords_Index(MAP_working_cell.wall_vertex[0][0], MAP_working_cell.wall_vertex[0][1]);
                int index_1 = Get_Wall_Drawing_Coords_Index(MAP_working_cell.wall_vertex[1][0], MAP_working_cell.wall_vertex[1][1]);

                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_PLUS1:

                        index_0++;
                        if (index_0 > 39) index_0 = 0;

                        MAP_working_cell.wall_vertex[0][0] = wall_drawing_coords[index_0][0];
                        MAP_working_cell.wall_vertex[0][1] = wall_drawing_coords[index_0][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_MINUS1:

                        index_0--;
                        if (index_0 < 0) index_0 = 39;

                        MAP_working_cell.wall_vertex[0][0] = wall_drawing_coords[index_0][0];
                        MAP_working_cell.wall_vertex[0][1] = wall_drawing_coords[index_0][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_PLUS2:

                        index_1++;
                        if (index_1 > 39) index_1 = 0;

                        MAP_working_cell.wall_vertex[1][0] = wall_drawing_coords[index_1][0];
                        MAP_working_cell.wall_vertex[1][1] = wall_drawing_coords[index_1][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_MINUS2:

                        index_1--;
                        if (index_1 < 0) index_1 = 39;

                        MAP_working_cell.wall_vertex[1][0] = wall_drawing_coords[index_1][0];
                        MAP_working_cell.wall_vertex[1][1] = wall_drawing_coords[index_1][1];
                        break;

                    case IDS_PROP_WALL_DRAWING_SLASH:
                        MAP_working_cell.wall_vertex[0][0] = 1.0f;
                        MAP_working_cell.wall_vertex[0][1] = 0.0f;
                        MAP_working_cell.wall_vertex[1][0] = 0.0f;
                        MAP_working_cell.wall_vertex[1][1] = 1.0f;
                        break;

                    case IDS_PROP_WALL_DRAWING_BSLASH:
                        MAP_working_cell.wall_vertex[0][0] = 0.0f;
                        MAP_working_cell.wall_vertex[0][1] = 0.0f;
                        MAP_working_cell.wall_vertex[1][0] = 1.0f;
                        MAP_working_cell.wall_vertex[1][1] = 1.0f;
                        break;

                    case IDS_PROP_WALL_DRAWING_OBL_TURN:
                        {
                            float tmp = MAP_working_cell.wall_vertex[0][0];
                            MAP_working_cell.wall_vertex[0][0] = MAP_working_cell.wall_vertex[1][0];
                            MAP_working_cell.wall_vertex[1][0] = tmp;

                            tmp = MAP_working_cell.wall_vertex[0][1];
                            MAP_working_cell.wall_vertex[0][1] = MAP_working_cell.wall_vertex[1][1];
                            MAP_working_cell.wall_vertex[1][1] = tmp;
                            break;
                        }
                }

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_DOOR_THIN_OBLIQUE)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }

                break;
            }


            case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
            case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
            case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
            case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
            {
                switch (_wm_id)
                {
                    case IDS_PROP_WALL_DRAWING_UP_1:
                        if (MAP_working_cell.wall_vertex[0][1] >= 0.1f)
                        {
                            MAP_working_cell.wall_vertex[0][1] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_UP_2:
                        if (MAP_working_cell.wall_vertex[1][1] - 0.1f > MAP_working_cell.wall_vertex[0][0])
                            MAP_working_cell.wall_vertex[1][1] -= 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_RIGHT_1:
                        if (MAP_working_cell.wall_vertex[1][0] < 1.0f)
                        {
                            MAP_working_cell.wall_vertex[0][0] += 0.1f;
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_RIGHT_2:
                        if (MAP_working_cell.wall_vertex[1][0] < 1.0f)
                            MAP_working_cell.wall_vertex[1][0] += 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_BOTTOM_1:
                        if (MAP_working_cell.wall_vertex[1][1] < 1.0f)
                        {
                            MAP_working_cell.wall_vertex[0][1] += 0.1f;
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_BOTTOM_2:
                        if (MAP_working_cell.wall_vertex[1][1] < 1.0f)
                            MAP_working_cell.wall_vertex[1][1] += 0.1f;
                        break;

                    case IDS_PROP_WALL_DRAWING_LEFT_1:
                        if (MAP_working_cell.wall_vertex[0][0] >= 0.1f)
                        {
                            MAP_working_cell.wall_vertex[0][0] -= 0.1f;
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        }
                        break;

                    case IDS_PROP_WALL_DRAWING_LEFT_2:
                        if (MAP_working_cell.wall_vertex[1][0] - 0.1f > MAP_working_cell.wall_vertex[0][0])
                            MAP_working_cell.wall_vertex[1][0] -= 0.1f;
                        break;
                }

                if (APP_level_file.map[selected_cell_index].cell_type >= LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT ||
                    APP_level_file.map[selected_cell_index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
                {
                    APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                    APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];
                }
            }
            break;
        }
    }
}
void GUI_Update_Properties_Drawing_Height(HWND _hwnd, int _wm_id)
{
    if (gui_current_layer == LAYERS_WALL_LAYER)
    {
        switch (gui_current_list_index)
        {
            case LIST_WALL_BOX_SHORT:
            {
                float end_point = MAP_working_cell.starting_height + MAP_working_cell.height;

                switch (_wm_id)
                {
                    case IDB_PROP_WALL_HEIGHT_SH_UP:
                        if ((MAP_working_cell.starting_height > 0.0f + 0.0001f))
                            MAP_working_cell.starting_height -= 0.1f;
                        break;

                    case IDB_PROP_WALL_HEIGHT_SH_DOWN:
                        if ((MAP_working_cell.starting_height < 0.3f + 0.0001f))
                            MAP_working_cell.starting_height += 0.1f;
                        break;

                    case IDB_PROP_WALL_HEIGHT_H_UP:
                        if ( end_point > 0.6f + 0.0001f)
                            end_point -= 0.1f;
                        break;

                    case IDB_PROP_WALL_HEIGHT_H_DOWN:
                        if (end_point < 0.9f + 0.0001f)
                            end_point += 0.1f;
                        break;
                }

                MAP_working_cell.height = end_point - MAP_working_cell.starting_height;

                if (APP_level_file.map[selected_cell_index].cell_type == LV_C_WALL_BOX_SHORT)
                {
                    APP_level_file.map[selected_cell_index].starting_height = MAP_working_cell.starting_height;
                    APP_level_file.map[selected_cell_index].height = MAP_working_cell.height;
                }
            }
            break;
        }
    }
}

void GUI_Update_Properties_Wall_Tex_Name(HWND _hwnd, int _side)
{
    HWND h_label = GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL);
    
    switch (_side)
    {
        case id_top:
            h_label = GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL);
            break;

        case id_right:
            h_label = GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT_LABEL);
            break;

        case id_bottom:
            h_label = GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM_LABEL);
            break;

        case id_left:
            h_label = GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_LEFT_LABEL);
            break;
    }

    int tex_id = MAP_working_cell.wall_texture_id[_side];
    int found = 0;

    std::list<s_Texture>::iterator tex_iterator;

    for (tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); ++tex_iterator)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if (tmp_texture.id == tex_id)
        {
            SendMessage(h_label, WM_SETTEXT, 0, (LPARAM)tmp_texture.filename);

            found = 1;
            break;
        }
    }

    if (found == 0)
        SendMessage(h_label, WM_SETTEXT, 0, (LPARAM)L"-none-");
}
void GUI_Update_Properties_Flat_Tex_Name(HWND _hwnd, int _flat)
{
    int tex_id = MAP_working_cell.flat_texture_id[_flat];
    int found = 0;

    std::list<s_Texture>::iterator tex_iterator;

    for (tex_iterator = TEX_flat_list.begin(); tex_iterator != TEX_flat_list.end(); ++tex_iterator)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if (tmp_texture.id == tex_id)
        {
            SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), WM_SETTEXT, 0, (LPARAM)tmp_texture.filename);

            found = 1;
            break;
        }
    }

    if (found == 0)
        SendMessage(GetDlgItem(_hwnd, IDS_PROP_WALL_IMAGE_TOP_LABEL), WM_SETTEXT, 0, (LPARAM)L"-none-");
}
void GUI_Update_Properties_Values(HWND _hwnd, int _wm_id)
{
    // When user changes the values.

    switch (_wm_id)
    {
        // When GROUP combo box changes.
        case IDC_PROP_GROUP_COMBO:
        {
            // Get new value.
            int group = (int)SendMessage(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

            // Update working cell with selected value.
            MAP_working_cell.cell_group = group;

            // If cell is already a DOOR update the value in map.
            if (APP_level_file.map[selected_cell_index].cell_type >= LV_C_DOOR_THICK_HORIZONTAL &&
                APP_level_file.map[selected_cell_index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
            {
                APP_level_file.map[selected_cell_index].cell_group = group;
            }
        }
        break;

        // When TIMER combo box changes.
        case IDC_PROP_TIMER_COMBO:
        {
            // Get new value.
            int timer = (int)SendMessage(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

            // Update working cell with selected value.
            MAP_working_cell.cell_timer = timer;

            // If cell is already a DOOR update the value in map.
            if (APP_level_file.map[selected_cell_index].cell_type >= LV_C_DOOR_THICK_HORIZONTAL &&
                APP_level_file.map[selected_cell_index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
            {
                APP_level_file.map[selected_cell_index].cell_timer = timer;
            }
        }
        break;

        // When ACTION combo box changes.
        case IDC_PROP_ACTION_COMBO:
        {
            // Get new value.
            int action = (int)SendMessage(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

            // Update working cell with selected value.
            MAP_working_cell.cell_action = action;

            // If cell is already a DOOR update the value in map.
            if (APP_level_file.map[selected_cell_index].cell_type >= LV_C_DOOR_THICK_HORIZONTAL &&
                APP_level_file.map[selected_cell_index].cell_type <= LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)
            {
                APP_level_file.map[selected_cell_index].cell_action = action;
            }
        }
        break;
    }
}

void GUI_Draw_Properites_Textures(HWND _hwnd)
{
    switch (gui_current_layer)
    {
        case LAYERS_WALL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_WALL_STANDARD:
                case LIST_WALL_THIN_HORIZONTAL:
                case LIST_WALL_THIN_VERTICAL:
                case LIST_WALL_THIN_OBLIQUE:
                case LIST_WALL_BOX:
                case LIST_WALL_BOX_SHORT:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    break;

                case LIST_WALL_FOURSIDE:
                case LIST_WALL_BOX_FOURSIDE:
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

        case LAYERS_DOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_DOOR_THICK_HORIZONTAL:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM, APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom], &TEX_wall_list);
                    break;

                case LIST_DOOR_THICK_VERTICAL:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT, APP_level_file.map[selected_cell_index].wall_texture_id[id_right], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_LEFT, APP_level_file.map[selected_cell_index].wall_texture_id[id_left], &TEX_wall_list);
                    break;

                case LIST_DOOR_THIN_OBLIQUE:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    break;

                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_TOP, APP_level_file.map[selected_cell_index].wall_texture_id[id_top], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_RIGHT, APP_level_file.map[selected_cell_index].wall_texture_id[id_right], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_BOTTOM, APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom], &TEX_wall_list);
                    GUI_Draw_Properties_Textures_Window(_hwnd, IDS_PROP_WALL_IMAGE_LEFT, APP_level_file.map[selected_cell_index].wall_texture_id[id_left], &TEX_wall_list);
                    break;
            }
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

void GUI_Draw_Properties_Drawing_Window(HWND _hwnd)
{
    switch (gui_current_layer)
    {
        case LAYERS_WALL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_WALL_THIN_HORIZONTAL:
                case LIST_WALL_THIN_VERTICAL:
                case LIST_WALL_THIN_OBLIQUE:
                    GUI_Draw_Properties_Drawing_Window_Line(_hwnd, IDS_PROP_WALL_DRAWING);
                    break;

                case LIST_WALL_BOX:
                case LIST_WALL_BOX_FOURSIDE:
                case LIST_WALL_BOX_SHORT:
                    GUI_Draw_Properties_Drawing_Window_Box(_hwnd, IDS_PROP_WALL_DRAWING, FALSE);
                    break;
            }
            break;

        case LAYERS_DOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_DOOR_THIN_OBLIQUE:
                    GUI_Draw_Properties_Drawing_Window_Line(_hwnd, IDS_PROP_WALL_DRAWING);
                    break;

                case LIST_DOOR_THICK_HORIZONTAL:
                case LIST_DOOR_THICK_VERTICAL:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    GUI_Draw_Properties_Drawing_Window_Box(_hwnd, IDS_PROP_WALL_DRAWING, TRUE);
                    break;
            }
            break;
    }
}
void GUI_Draw_Properties_Drawing_Window_BG(HDC _hdc)
{
    SelectObject(_hdc, hbr_drawing_bg);
    Rectangle(_hdc, 0, 0, 78, 78);

    SelectObject(_hdc, hp_drawing_grid);

    for (float i = 0.1f; i < 1.0f; i += 0.1f)
    {
        int ii = (int)(i * 78.0f);
        MoveToEx(_hdc, ii, 0, NULL);
        LineTo(_hdc, ii, 78);

        MoveToEx(_hdc, 0, ii, NULL);
        LineTo(_hdc, 78, ii);
    }
}
void GUI_Draw_Properties_Drawing_Window_Line(HWND _hwnd, int _wm_id)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetDlgItem(_hwnd, _wm_id), &ps);

    GUI_Draw_Properties_Drawing_Window_BG(hdc);

    SelectObject(hdc, hp_drawing_line);

    int x0 = (int)(MAP_working_cell.wall_vertex[0][0] * 78.0f);
    int y0 = (int)(MAP_working_cell.wall_vertex[0][1] * 78.0f);

    int x1 = (int)(MAP_working_cell.wall_vertex[1][0] * 78.0f);
    int y1 = (int)(MAP_working_cell.wall_vertex[1][1] * 78.0f);
   
    int vx = x1 - x0;
    int vy = y1 - y0;

    int xs = vx / 2;
    int ys = vy / 2;
       
    xs = x0 + xs - (-vy / 10);
    ys = y0 + ys - (vx / 10);

    MoveToEx(hdc, x0, y0, NULL);
    LineTo(hdc, x1, y1);

    Ellipse(hdc, xs - 2, ys - 2, xs + 2, ys + 2);

    EndPaint(GetDlgItem(_hwnd, _wm_id), &ps);
}
void GUI_Draw_Properties_Drawing_Window_Box(HWND _hwnd, int _wm_id, bool _is_door)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(GetDlgItem(_hwnd, _wm_id), &ps);

        int x0 = (int)(MAP_working_cell.wall_vertex[0][0] * 78.0f);
        int y0 = (int)(MAP_working_cell.wall_vertex[0][1] * 78.0f);

        int x1 = (int)(MAP_working_cell.wall_vertex[1][0] * 78.0f);
        int y1 = (int)(MAP_working_cell.wall_vertex[1][1] * 78.0f);

        GUI_Draw_Properties_Drawing_Window_BG(hdc);

        SelectObject(hdc, hp_drawing_line);
        Rectangle(hdc, x0, y0, x1, y1);

        // If we are dealing with doors - draw the direction arrow.
        if (_is_door)
        {
            int vx = x1 - x0;
            int vy = y1 - y0;

            int xs = vx / 2;
            int ys = vy / 2;

            xs = x0 + xs;
            ys = y0 + ys;

            SelectObject(hdc, hbr_drawing_door_direction);
            SelectObject(hdc, hp_none);

            switch (gui_current_list_index)
            {
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                {
                    xs = x0 + 10;

                    POINT vertices[] = { {xs - 5, ys}, {xs + 10, ys - 10}, {xs + 10, ys + 10} };
                    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
                    break;
                }

                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                {
                    xs = x1 - 10;

                    POINT vertices[] = { {xs + 5, ys}, {xs - 10, ys - 10}, {xs - 10, ys + 10} };
                    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
                    break;
                }

                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                {
                    ys = y0 + 10;

                    POINT vertices[] = { {xs, ys - 5}, {xs + 10, ys + 10}, {xs - 10, ys + 10} };
                    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
                    break;
                }

                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                {
                    ys = y1 - 10;

                    POINT vertices[] = { {xs, ys + 5}, {xs + 10, ys - 10}, {xs - 10, ys - 10} };
                    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
                    break;
                }
            }
        }

    EndPaint(GetDlgItem(_hwnd, _wm_id), &ps);
}

void GUI_Draw_Properties_Drawing_Height(HWND _hwnd)
{
    switch (gui_current_layer)
    {
        case LAYERS_WALL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_WALL_BOX_SHORT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT), &ps);

                    int x0 = (int)(0.2f * 78.0f);
                    int y0 = (int)(MAP_working_cell.starting_height * 78.0f);

                    int x1 = (int)(0.8f * 78.0f);
                    int y1 = (int)((MAP_working_cell.starting_height + MAP_working_cell.height) * 78.0f);

                    GUI_Draw_Properties_Drawing_Window_BG(hdc);

                    SelectObject(hdc, hp_drawing_line);
                    Rectangle(hdc, x0, y0, x1, y1);

                    SelectObject(hdc, CreatePen(PS_SOLID, 2, RGB(200, 100, 100)));
                    MoveToEx(hdc, 0, (int)(0.5f * 78.0f), NULL);
                    LineTo(hdc, (int)(1.0f * 78.0f),(int)(0.5f * 78.0f));

                    EndPaint(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT), &ps);
                }
                break;
            }
            break;
    }
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

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_PLUS2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_MINUS2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_SLASH), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BSLASH), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_UP), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_TURN), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_HOR_DOWN), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_LEFT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_TURN), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_VER_RIGHT), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_UP_2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_RIGHT_2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_BOTTOM_2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_1), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_LEFT_2), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_DRAWING_OBL_TURN), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_GROUP_LABEL), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_GROUP_COMBO), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_TIMER_LABEL), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_TIMER_COMBO), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_ACTION_LABEL), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDC_PROP_ACTION_COMBO), FALSE);

    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT_LABEL), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDS_PROP_WALL_HEIGHT), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_UP), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_SH_DOWN), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_UP), FALSE);
    ShowWindow(GetDlgItem(_hwnd, IDB_PROP_WALL_HEIGHT_H_DOWN), FALSE);
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
        APP_level_file.map[i].cell_type = LV_C_NOT_SOLID;
        APP_level_file.map[i].cell_state = 0;
        APP_level_file.map[i].cell_group = 0;
        APP_level_file.map[i].cell_timer = 1;
        APP_level_file.map[i].cell_action = 0;

        APP_level_file.map[i].floor_used = 0;
        APP_level_file.map[i].ceil_used = 0;

        APP_level_file.map[i].wall_vertex[0][0] = 0.0f;
        APP_level_file.map[i].wall_vertex[0][1] = 0.0f;
        APP_level_file.map[i].wall_vertex[1][0] = 1.0f;
        APP_level_file.map[i].wall_vertex[1][1] = 1.0f;

        APP_level_file.map[i].starting_height = 0.0f;
        APP_level_file.map[i].height = 1.0f;

        APP_level_file.map[i].wall_texture_id[id_top] = 0;
        APP_level_file.map[i].wall_texture_id[id_right] = 0;
        APP_level_file.map[i].wall_texture_id[id_bottom] = 0;
        APP_level_file.map[i].wall_texture_id[id_left] = 0;

        APP_level_file.map[i].flat_texture_id[id_floor] = 0;
        APP_level_file.map[i].flat_texture_id[id_ceil] = 0;

        APP_level_file.map[i].wall_lightmap_id[id_top] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_right] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_bottom] = -1;
        APP_level_file.map[i].wall_lightmap_id[id_left] = -1;

        APP_level_file.map[i].flat_lightmap_id[id_floor] = -1;
        APP_level_file.map[i].flat_lightmap_id[id_ceil] = -1;
    }

    MAP_working_cell.cell_type = LV_C_NOT_SOLID;
    MAP_working_cell.cell_state = 0;
    MAP_working_cell.cell_group = 0;
    MAP_working_cell.cell_timer = 1;
    MAP_working_cell.cell_action = 0;

    MAP_working_cell.wall_vertex[0][0] = 0.0f;
    MAP_working_cell.wall_vertex[0][1] = 0.0f;
    MAP_working_cell.wall_vertex[1][0] = 1.0f;
    MAP_working_cell.wall_vertex[1][1] = 1.0f;

    MAP_working_cell.starting_height = 0.0f;
    MAP_working_cell.height = 1.0f;

    MAP_working_cell.wall_texture_id[id_top] = 0;
    MAP_working_cell.wall_texture_id[id_right] = 0;
    MAP_working_cell.wall_texture_id[id_bottom] = 0;
    MAP_working_cell.wall_texture_id[id_left] = 0;

    MAP_working_cell.flat_texture_id[id_floor] = 0;
    MAP_working_cell.flat_texture_id[id_ceil] = 0;

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
        case LAYERS_PLAYER_LAYER:
        case LAYERS_WALL_LAYER:
        case LAYERS_DOOR_LAYER:
            hbr_active_bg = hbr_main_bg;
            hp_active_grid = hp_main_grid;
            hp_active_dotted = hp_lightmaps_dotted;
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
void Map_Draw_Mode(HDC _hdc)
{
    if (APP_alt_pressed || APP_control_pressed)
    {
        int cx0, cx1, cy0, cy1;

        World_To_Screen((float)hover_cell_x, (float)hover_cell_y, cx0, cy0);
        World_To_Screen((float)hover_cell_x + 1.0f, (float)hover_cell_y + 1.0f, cx1, cy1);

        RECT tmp_cell = { cx0, cy0, cx1, cy1 };

        HBRUSH hbr_mode_brush = hbr_mode_brush_add;

        if (APP_control_pressed)  hbr_mode_brush = hbr_mode_brush_add;
        else if (APP_alt_pressed) hbr_mode_brush = hbr_mode_brush_clear;

        FillRect(_hdc, &tmp_cell, hbr_mode_brush);
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
    case 90:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 0.5f, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 0:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 1.0f, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 0.5f, (float)APP_level_file.player_starting_cell_y, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 30:
    {
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y, px0, py0);
        World_To_Screen((float)APP_level_file.player_starting_cell_x, (float)APP_level_file.player_starting_cell_y + 0.5f, px1, py1);
        World_To_Screen((float)APP_level_file.player_starting_cell_x + 1.0f, (float)APP_level_file.player_starting_cell_y + 1.0f, px2, py2);

        POINT vertices[] = { {px0, py0}, {px1, py1}, {px2, py2} };
        Polygon(_hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
    }
    break;

    case 60:
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
                        case LAYERS_PLAYER_LAYER:
                            if (APP_level_file.map[index].cell_type) Map_Draw_Objects_Dotted_Wall_Layer(_hdc, x, y, index);
                            break;


                        case LAYERS_WALL_LAYER:
                        case LAYERS_DOOR_LAYER:
                            if (APP_level_file.map[index].cell_type)
                            {
                                switch (APP_level_file.map[index].cell_type)
                                {
                                    case LV_C_WALL_STANDARD:
                                        Map_Draw_Objects_Walls_Standard(_hdc, x, y, index, hbr_map_walls_standard);
                                        break;

                                    case LV_C_WALL_FOURSIDE:
                                        Map_Draw_Objects_Walls_Fourside(_hdc, x, y, index);
                                        break;

                                    case LV_C_WALL_THIN_HORIZONTAL:
                                    case LV_C_WALL_THIN_VERTICAL:
                                    case LV_C_WALL_THIN_OBLIQUE:
                                        Map_Draw_Objects_Thin_Wall(_hdc, x, y, index, hp_map_walls_thin);

                                        if (f_scale_x > 50.0f)
                                            Map_Draw_Objects_Thin_Wall_Side(_hdc, x, y, index, hp_map_walls_thin);
                                        break;

                                    case LV_C_WALL_BOX:
                                    case LV_C_WALL_BOX_FOURSIDE:
                                        Map_Draw_Objects_Walls_Box(_hdc, x, y, index, hbr_map_walls_standard);
                                        break;

                                    case LV_C_WALL_BOX_SHORT:
                                        Map_Draw_Objects_Walls_Box(_hdc, x, y, index, hbr_map_walls_short);
                                        break;

                                    case LV_C_DOOR_THIN_OBLIQUE:
                                        Map_Draw_Objects_Thin_Wall(_hdc, x, y, index, hp_map_doors_thin);

                                        if (f_scale_x > 50.0f)
                                            Map_Draw_Objects_Thin_Wall_Side(_hdc, x, y, index, hp_map_walls_thin);
                                        break;

                                    case LV_C_DOOR_THICK_HORIZONTAL:
                                    case LV_C_DOOR_THICK_VERTICAL:
                                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                                    case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                                    case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                                        Map_Draw_Objects_Walls_Box(_hdc, x, y, index, hbr_map_doors);
                                        break;
                                }
                            }
                        break;


                        case LAYERS_FLOOR_LAYER:
                            Map_Draw_Objects_Floor(_hdc, x, y, index);
                            if (APP_level_file.map[index].cell_type) Map_Draw_Objects_Dotted_Wall_Layer(_hdc, x, y, index);
                            break;


                        case LAYERS_CEIL_LAYER:
                            Map_Draw_Objects_Ceil(_hdc, x, y, index);
                            if (APP_level_file.map[index].cell_type) Map_Draw_Objects_Dotted_Wall_Layer(_hdc, x, y, index);
                            break;


                        case LAYERS_LIGHTMAPS_LAYER:
                            Map_Draw_Objects_Lightmaps(_hdc, x, y, index);
                            if (APP_level_file.map[index].cell_type) Map_Draw_Objects_Dotted_Wall_Layer(_hdc, x, y, index);
                            break;
                    }
                }
            }
        }
    }
}
void Map_Draw_Objects_Walls_Standard(HDC _hdc, int _mx, int _my, int _index, HBRUSH _hbr)
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
        FillRect(_hdc, &r, _hbr);
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
void Map_Draw_Objects_Thin_Wall(HDC _hdc, int _mx, int _my, int _index, HPEN _pen)
{
    int cx0, cx1, cy0, cy1;
    
    float x0 = (float)_mx + APP_level_file.map[_index].wall_vertex[0][0];
    float y0 = (float)_my + APP_level_file.map[_index].wall_vertex[0][1];

    float x1 = (float)_mx + APP_level_file.map[_index].wall_vertex[1][0];
    float y1 = (float)_my + APP_level_file.map[_index].wall_vertex[1][1];

    World_To_Screen(x0, y0, cx0, cy0);
    World_To_Screen(x1, y1, cx1, cy1);

    SelectObject(_hdc, _pen);
    MoveToEx(_hdc, cx0, cy0, NULL);
    LineTo(_hdc, cx1, cy1);
}
void Map_Draw_Objects_Thin_Wall_Side(HDC _hdc, int _mx, int _my, int _index, HPEN _pen)
{
    int cx0, cx1, cy0, cy1;

    float x0 = (float)_mx + APP_level_file.map[_index].wall_vertex[0][0];
    float y0 = (float)_my + APP_level_file.map[_index].wall_vertex[0][1];

    float x1 = (float)_mx + APP_level_file.map[_index].wall_vertex[1][0];
    float y1 = (float)_my + APP_level_file.map[_index].wall_vertex[1][1];

    World_To_Screen(x0, y0, cx0, cy0);
    World_To_Screen(x1, y1, cx1, cy1);

    int vx = cx1 - cx0;
    int vy = cy1 - cy0;

    int xs = vx / 2;
    int ys = vy / 2;

    xs = cx0 + xs - (-vy / 10);
    ys = cy0 + ys - (vx / 10);

    SelectObject(_hdc, _pen);
    Ellipse(_hdc, xs - 2, ys - 2, xs + 2, ys + 2);
}
void Map_Draw_Objects_Walls_Box(HDC _hdc, int _mx, int _my, int _index, HBRUSH _hbr)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx + APP_level_file.map[_index].wall_vertex[0][0], (float)_my + APP_level_file.map[_index].wall_vertex[0][1], cx0, cy0);
    World_To_Screen((float)_mx + APP_level_file.map[_index].wall_vertex[1][0], (float)_my + APP_level_file.map[_index].wall_vertex[1][1], cx1, cy1);

    if (gui_texprew_mode)
    {
        int id = APP_level_file.map[_index].wall_texture_id[id_top];
        Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_wall_list, id);
    }
    else
    {
        RECT r = { cx0, cy0, cx1, cy1 };
        FillRect(_hdc, &r, _hbr);
    }
}
void Map_Draw_Objects_Floor(HDC _hdc, int _mx, int _my, int _index)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_mx, (float)_my, cx0, cy0);
    World_To_Screen((float)_mx + 1.0f, (float)_my + 1.0f, cx1, cy1);

    if (APP_level_file.map[_index].floor_used)
    {
        if (gui_texprew_mode)
            Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_flat_list, APP_level_file.map[_index].flat_texture_id[id_floor]);
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

    if (APP_level_file.map[_index].ceil_used)
    {
        if (gui_texprew_mode)
            Map_Draw_Textured_Square(_hdc, cx0, cy0, cx1, cy1, &TEX_flat_list, APP_level_file.map[_index].flat_texture_id[id_ceil]);
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
void Map_Draw_Objects_Dotted_Rectangle(HDC _hdc, float _x0, float _y0, float _x1, float _y1, HPEN _hpen)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen((float)_x0, (float)_y0, cx0, cy0);
    World_To_Screen((float)_x1, (float)_y1, cx1, cy1);

    SelectObject(_hdc, _hpen);
    SetBkMode(_hdc, TRANSPARENT);

    MoveToEx(_hdc, cx0, cy0, NULL);
    LineTo(_hdc, cx1, cy0);
    LineTo(_hdc, cx1, cy1);
    LineTo(_hdc, cx0, cy1);
    LineTo(_hdc, cx0, cy0);
}
void Map_Draw_Objects_Dotted_Line(HDC _hdc, float _x0, float _y0, float _x1, float _y1, HPEN _hpen)
{
    int cx0, cx1, cy0, cy1;

    World_To_Screen(_x0, _y0, cx0, cy0);
    World_To_Screen(_x1, _y1, cx1, cy1);

    SelectObject(_hdc, _hpen);
    SetBkMode(_hdc, TRANSPARENT);

    MoveToEx(_hdc, cx0, cy0, NULL);
    LineTo(_hdc, cx1, cy1);
}
void Map_Draw_Objects_Dotted_Wall_Layer(HDC _hdc, int _mx, int _my, int _index)
{
    switch (APP_level_file.map[_index].cell_type)
    {
        case LV_C_WALL_STANDARD:
        case LV_C_WALL_FOURSIDE:
            Map_Draw_Objects_Dotted_Rectangle(_hdc, (float)_mx + 0.04f, (float)_my + 0.04f, (float)_mx + 0.96f, (float)_my + 0.96f, hp_map_walls_dotted);
            break;


        case LV_C_WALL_THIN_HORIZONTAL:
        case LV_C_WALL_THIN_VERTICAL:
        case LV_C_WALL_THIN_OBLIQUE:
            Map_Draw_Objects_Dotted_Line(   _hdc,
                                            (float)_mx + APP_level_file.map[_index].wall_vertex[0][0],
                                            (float)_my + APP_level_file.map[_index].wall_vertex[0][1],
                                            (float)_mx + APP_level_file.map[_index].wall_vertex[1][0],
                                            (float)_my + APP_level_file.map[_index].wall_vertex[1][1],
                                            hp_map_walls_dotted);
            break;


        case LV_C_WALL_BOX:
        case LV_C_WALL_BOX_FOURSIDE:
        case LV_C_WALL_BOX_SHORT:
            Map_Draw_Objects_Dotted_Rectangle(  _hdc,
                                                (float)_mx + 0.04f + APP_level_file.map[_index].wall_vertex[0][0],
                                                (float)_my + 0.04f + APP_level_file.map[_index].wall_vertex[0][1],
                                                (float)_mx - 0.04f + APP_level_file.map[_index].wall_vertex[1][0],
                                                (float)_my - 0.04f + APP_level_file.map[_index].wall_vertex[1][1],
                                                hp_map_walls_dotted);
            break;


        case LV_C_DOOR_THIN_OBLIQUE:
            Map_Draw_Objects_Dotted_Line(   _hdc,
                                            (float)_mx + APP_level_file.map[_index].wall_vertex[0][0],
                                            (float)_my + APP_level_file.map[_index].wall_vertex[0][1],
                                            (float)_mx + APP_level_file.map[_index].wall_vertex[1][0],
                                            (float)_my + APP_level_file.map[_index].wall_vertex[1][1],
                                            hp_map_doors_dotted);
            break;

        case LV_C_DOOR_THICK_HORIZONTAL:
        case LV_C_DOOR_THICK_VERTICAL:

        case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
        case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
        case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
        case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
            Map_Draw_Objects_Dotted_Rectangle(  _hdc,
                                                (float)_mx + 0.04f + APP_level_file.map[_index].wall_vertex[0][0],
                                                (float)_my + 0.04f + APP_level_file.map[_index].wall_vertex[0][1],
                                                (float)_mx - 0.04f + APP_level_file.map[_index].wall_vertex[1][0],
                                                (float)_my - 0.04f + APP_level_file.map[_index].wall_vertex[1][1],
                                                hp_map_doors_dotted);
            break;
    }
}

void Map_Add_Current_Object()
{
    switch (gui_current_layer)
    {
        case LAYERS_PLAYER_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_PLAYER:
                    APP_level_file.player_starting_cell_x = selected_cell_x;
                    APP_level_file.player_starting_cell_y = selected_cell_y;
                    break;
            }
            break;

        case LAYERS_WALL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_WALL_STANDARD:
                case LIST_WALL_FOURSIDE:
                case LIST_WALL_THIN_HORIZONTAL:
                case LIST_WALL_THIN_VERTICAL:
                case LIST_WALL_THIN_OBLIQUE:
                case LIST_WALL_BOX:
                case LIST_WALL_BOX_FOURSIDE:
                case LIST_WALL_BOX_SHORT:
                    if (selected_cell_x != APP_level_file.player_starting_cell_x || selected_cell_y != APP_level_file.player_starting_cell_y)
                    {
                        if (gui_current_list_index == LIST_WALL_STANDARD)           APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_STANDARD;
                        if (gui_current_list_index == LIST_WALL_FOURSIDE)           APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_FOURSIDE;
                        if (gui_current_list_index == LIST_WALL_THIN_HORIZONTAL)    APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_THIN_HORIZONTAL;
                        if (gui_current_list_index == LIST_WALL_THIN_VERTICAL)      APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_THIN_VERTICAL;
                        if (gui_current_list_index == LIST_WALL_THIN_OBLIQUE)       APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_THIN_OBLIQUE;
                        if (gui_current_list_index == LIST_WALL_BOX)                APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_BOX;
                        if (gui_current_list_index == LIST_WALL_BOX_FOURSIDE)       APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_BOX_FOURSIDE;
                        if (gui_current_list_index == LIST_WALL_BOX_SHORT)          APP_level_file.map[selected_cell_index].cell_type = LV_C_WALL_BOX_SHORT;

                        APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                        APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                        APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                        APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];

                        APP_level_file.map[selected_cell_index].starting_height = MAP_working_cell.starting_height;
                        APP_level_file.map[selected_cell_index].height = MAP_working_cell.height;

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
                    APP_level_file.map[selected_cell_index].floor_used = 1;
                    break;
            }
            break;

        case LAYERS_CEIL_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_CEIL:
                    APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil] = MAP_working_cell.flat_texture_id[id_ceil];
                    APP_level_file.map[selected_cell_index].ceil_used = 1;
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

        case LAYERS_DOOR_LAYER:
            switch (gui_current_list_index)
            {
                case LIST_DOOR_THICK_HORIZONTAL:
                case LIST_DOOR_THICK_VERTICAL:
                case LIST_DOOR_THIN_OBLIQUE:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    if (selected_cell_x != APP_level_file.player_starting_cell_x || selected_cell_y != APP_level_file.player_starting_cell_y)
                    {
                        if (gui_current_list_index == LIST_DOOR_THICK_HORIZONTAL)                APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_THICK_HORIZONTAL;
                        if (gui_current_list_index == LIST_DOOR_THICK_VERTICAL)                  APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_THICK_VERTICAL;
                        if (gui_current_list_index == LIST_DOOR_THIN_OBLIQUE)                   APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_THIN_OBLIQUE;
                        if (gui_current_list_index == LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT)   APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT;
                        if (gui_current_list_index == LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT)  APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT;
                        if (gui_current_list_index == LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)     APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT;
                        if (gui_current_list_index == LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT)    APP_level_file.map[selected_cell_index].cell_type = LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT;

                        APP_level_file.map[selected_cell_index].wall_vertex[0][0] = MAP_working_cell.wall_vertex[0][0];
                        APP_level_file.map[selected_cell_index].wall_vertex[0][1] = MAP_working_cell.wall_vertex[0][1];
                        APP_level_file.map[selected_cell_index].wall_vertex[1][0] = MAP_working_cell.wall_vertex[1][0];
                        APP_level_file.map[selected_cell_index].wall_vertex[1][1] = MAP_working_cell.wall_vertex[1][1];

                        APP_level_file.map[selected_cell_index].cell_group = MAP_working_cell.cell_group;
                        APP_level_file.map[selected_cell_index].cell_timer = MAP_working_cell.cell_timer;
                        APP_level_file.map[selected_cell_index].cell_action = MAP_working_cell.cell_action;

                        APP_level_file.map[selected_cell_index].wall_texture_id[id_top] = MAP_working_cell.wall_texture_id[id_top];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_right] = MAP_working_cell.wall_texture_id[id_right];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom] = MAP_working_cell.wall_texture_id[id_bottom];
                        APP_level_file.map[selected_cell_index].wall_texture_id[id_left] = MAP_working_cell.wall_texture_id[id_left];
                    }
                    break;
            }
            break;
    }
}
void Map_Read_Current_Cell()
{
    switch (gui_current_layer)
    {
        case LAYERS_PLAYER_LAYER:
            if (selected_cell_x == APP_level_file.player_starting_cell_x && selected_cell_y == APP_level_file.player_starting_cell_y)
            {
                gui_current_list_index = 0;
            }
            break;

        case LAYERS_WALL_LAYER:
            switch (APP_level_file.map[selected_cell_index].cell_type)
            {
                case LV_C_WALL_STANDARD:
                    gui_current_list_index = LIST_WALL_STANDARD;
                    break;

                case LV_C_WALL_FOURSIDE:
                    gui_current_list_index = LIST_WALL_FOURSIDE;
                    break;

                case LV_C_WALL_THIN_HORIZONTAL:
                    gui_current_list_index = LIST_WALL_THIN_HORIZONTAL;
                    break;

                case LV_C_WALL_THIN_VERTICAL:
                    gui_current_list_index = LIST_WALL_THIN_VERTICAL;
                    break;

                case LV_C_WALL_THIN_OBLIQUE:
                    gui_current_list_index = LIST_WALL_THIN_OBLIQUE;
                    break;

                case LV_C_WALL_BOX:
                    gui_current_list_index = LIST_WALL_BOX;
                    break;

                case LV_C_WALL_BOX_FOURSIDE:
                    gui_current_list_index = LIST_WALL_BOX_FOURSIDE;
                    break;

                case LV_C_WALL_BOX_SHORT:
                    gui_current_list_index = LIST_WALL_BOX_SHORT;
                    break;

                // If DOOR CELL selected while we are in WALL LAYER - switch to DOOR LAYER.

                case LV_C_DOOR_THICK_HORIZONTAL:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_THICK_HORIZONTAL;
                    break;

                case LV_C_DOOR_THICK_VERTICAL:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_THICK_VERTICAL;
                    break;

                case LV_C_DOOR_THIN_OBLIQUE:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_THIN_OBLIQUE;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    gui_current_layer = LAYERS_DOOR_LAYER;
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT;
                    break;

                // If cell is empty nothing in the list box should be highlighted or selected.

                default:
                    gui_current_list_index = -1;
                    break;
            }
            break;


        case LAYERS_FLOOR_LAYER:
                gui_current_list_index = LIST_FLOOR;
                break;


        case LAYERS_CEIL_LAYER:
                gui_current_list_index = LIST_CEIL;
                break;


        case LAYERS_DOOR_LAYER:
            switch (APP_level_file.map[selected_cell_index].cell_type)
            {
                // If WALL CELL is selected while we are in DOOR LAYER - switch to WALL LAYER.

                case LV_C_WALL_STANDARD:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_STANDARD;
                    break;

                case LV_C_WALL_FOURSIDE:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_FOURSIDE;
                    break;

                case LV_C_WALL_THIN_HORIZONTAL:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_THIN_HORIZONTAL;
                    break;

                case LV_C_WALL_THIN_VERTICAL:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_THIN_VERTICAL;
                    break;

                case LV_C_WALL_THIN_OBLIQUE:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_THIN_OBLIQUE;
                    break;

                case LV_C_WALL_BOX:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_BOX;
                    break;

                case LV_C_WALL_BOX_FOURSIDE:
                    gui_current_layer = LAYERS_WALL_LAYER;
                    gui_current_list_index = LIST_WALL_BOX_FOURSIDE;
                    break;

                //

                case LV_C_DOOR_THICK_HORIZONTAL:
                    gui_current_list_index = LIST_DOOR_THICK_HORIZONTAL;
                    break;

                case LV_C_DOOR_THICK_VERTICAL:
                    gui_current_list_index = LIST_DOOR_THICK_VERTICAL;
                    break;

                case LV_C_DOOR_THIN_OBLIQUE:
                    gui_current_list_index = LIST_DOOR_THIN_OBLIQUE;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT;
                    break;

                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    gui_current_list_index = LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT;
                    break;

                default:
                    // if cell is empty nothing in the list box should be highlighted or selected
                    gui_current_list_index = -1;
                    break;
            }
            break;
    }

    // copy selected cell from map to working cell
    MAP_working_cell.cell_type = APP_level_file.map[selected_cell_index].cell_type;
    MAP_working_cell.cell_group = APP_level_file.map[selected_cell_index].cell_group;
    MAP_working_cell.cell_state = APP_level_file.map[selected_cell_index].cell_state;
    MAP_working_cell.cell_timer = APP_level_file.map[selected_cell_index].cell_timer;
    MAP_working_cell.cell_action = APP_level_file.map[selected_cell_index].cell_action;

    MAP_working_cell.starting_height = APP_level_file.map[selected_cell_index].starting_height;
    MAP_working_cell.height = APP_level_file.map[selected_cell_index].height;

    MAP_working_cell.wall_vertex[0][0] = APP_level_file.map[selected_cell_index].wall_vertex[0][0];
    MAP_working_cell.wall_vertex[0][1] = APP_level_file.map[selected_cell_index].wall_vertex[0][1];
    MAP_working_cell.wall_vertex[1][0] = APP_level_file.map[selected_cell_index].wall_vertex[1][0];
    MAP_working_cell.wall_vertex[1][1] = APP_level_file.map[selected_cell_index].wall_vertex[1][1];

    MAP_working_cell.wall_texture_id[id_top] = APP_level_file.map[selected_cell_index].wall_texture_id[id_top];
    MAP_working_cell.wall_texture_id[id_right] = APP_level_file.map[selected_cell_index].wall_texture_id[id_right];
    MAP_working_cell.wall_texture_id[id_bottom] = APP_level_file.map[selected_cell_index].wall_texture_id[id_bottom];
    MAP_working_cell.wall_texture_id[id_left] = APP_level_file.map[selected_cell_index].wall_texture_id[id_left];

    MAP_working_cell.flat_texture_id[id_floor] = APP_level_file.map[selected_cell_index].flat_texture_id[id_floor];
    MAP_working_cell.flat_texture_id[id_ceil] = APP_level_file.map[selected_cell_index].flat_texture_id[id_ceil];
}
void Map_Clear_Current_Cell()
{
    switch (gui_current_layer)
    {
        case LAYERS_WALL_LAYER:
        case LAYERS_DOOR_LAYER:
            APP_level_file.map[hover_cell_index].cell_type = LV_C_NOT_SOLID;
            APP_level_file.map[hover_cell_index].cell_group = 0;
            APP_level_file.map[hover_cell_index].cell_state = 0;
            APP_level_file.map[hover_cell_index].cell_timer = 1;
            APP_level_file.map[hover_cell_index].cell_action = 0;

            APP_level_file.map[hover_cell_index].starting_height = 0.0f;
            APP_level_file.map[hover_cell_index].height = 1.0f;

            APP_level_file.map[hover_cell_index].wall_texture_id[id_top] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_right] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_bottom] = -1;
            APP_level_file.map[hover_cell_index].wall_texture_id[id_left] = -1;

            APP_level_file.map[hover_cell_index].wall_vertex[0][0] = 0.0f;
            APP_level_file.map[hover_cell_index].wall_vertex[0][1] = 0.0f;
            APP_level_file.map[hover_cell_index].wall_vertex[1][0] = 0.0f;
            APP_level_file.map[hover_cell_index].wall_vertex[1][1] = 0.0f;
            break;

        case LAYERS_FLOOR_LAYER:
            APP_level_file.map[hover_cell_index].flat_texture_id[id_floor] = 0;
            APP_level_file.map[hover_cell_index].floor_used = 0;
            break;

        case LAYERS_CEIL_LAYER:
            APP_level_file.map[hover_cell_index].flat_texture_id[id_ceil] = 0;
            APP_level_file.map[hover_cell_index].ceil_used = 0;
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


void    TEX_Reset()
{
    TEX_wall_current_id = 0;
    TEX_flat_current_id = 0;

    TEX_Reset_Texture_List(&TEX_wall_list);
    TEX_Reset_Texture_List(&TEX_flat_list);
}
bool    TEX_List_Sort_Function(const s_Texture& a, const s_Texture& b)
{
    return (wcscmp(a.filename, b.filename) < 0);
}
void    TEX_Prepare_List_To_Save(std::list<s_Texture>* _list, int* _table, int _mode)
{
    // MODE == 0 for TEX WALL LIST
    // MODE == 1 for TEX FLAT LIST
    
    // This function removes unused textures from texture list.

    // clear new id in the list
    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); ++tex_iterator)
        (*tex_iterator).new_id = -1;

    // update list
    int counter = 0;

    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); tex_iterator++)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if (_table[counter] <= 0)
        {
            // Mark as "to be removed".
            (*tex_iterator).new_id = -2;
        }

        counter++;
    }

    // Remove marked items.
    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); tex_iterator++)
    {
        s_Texture tmp_texture;
        tmp_texture = *tex_iterator;

        if ((*tex_iterator).new_id == -2)
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
    }

    // first sort by name - then add new id
    _list->sort(TEX_List_Sort_Function);

    int new_id = 0;

    // NOTE.
    // We saving only filenames without extensions.

    for (auto tex_iterator = _list->begin(); tex_iterator != _list->end(); tex_iterator++)
    {
        (*tex_iterator).new_id = new_id;

        // Convert texture filename from TCHAR to CHAR to be written in file structure. Also save the filename without extension.
        if (_mode == 0)
        {
            TCHAR file_name_no_ext[32];
            ZeroMemory(file_name_no_ext, sizeof(file_name_no_ext));

            _wsplitpath((*tex_iterator).filename, NULL, NULL, file_name_no_ext, NULL);

            wcstombs_s(NULL, APP_level_file.wall_texture_filename[new_id], file_name_no_ext, wcslen(file_name_no_ext) + 1);
        }
        else
        {
            TCHAR file_name_no_ext[32];
            ZeroMemory(file_name_no_ext, sizeof(file_name_no_ext));

            _wsplitpath((*tex_iterator).filename, NULL, NULL, file_name_no_ext, NULL);

            wcstombs_s(NULL, APP_level_file.flat_texture_filename[new_id], file_name_no_ext, wcslen(file_name_no_ext) + 1);
        }

        new_id++;
    }

    // If no texture was selected - the 0 texture on the list is empty - it should be replaced with "default" texture.
    // We are saving without the file extensions.
    if (_mode == 0)
    {
        if (strlen(APP_level_file.wall_texture_filename[0]) == 0)
            strcpy(APP_level_file.wall_texture_filename[0], "_default");
    }
    else
    {
        if (strlen(APP_level_file.flat_texture_filename[0]) == 0)
            strcpy(APP_level_file.flat_texture_filename[0], "_default");
    }
}
void    TEX_Prepare_Wall_List_To_Save(std::list<s_Texture>* _list, int* _table, int _mode)
{
    // Update WALL TEXTURE lists.
    // Clear unused textures, sort in A-Z order, put filenames in level file.
    TEX_Prepare_List_To_Save(_list, _table, 0);

    // Switch old ids to new ids in wall list.
    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
        if (APP_level_file.map[i].cell_type)
        {
            switch (APP_level_file.map[i].cell_type)
            {
                // For cells with only TOP side texture.
                case LV_C_WALL_STANDARD:
                case LV_C_WALL_BOX:

                case LV_C_WALL_THIN_HORIZONTAL:
                case LV_C_WALL_THIN_VERTICAL:
                case LV_C_WALL_THIN_OBLIQUE:

                case LV_C_DOOR_THICK_HORIZONTAL:
                case LV_C_DOOR_THICK_VERTICAL:
                case LV_C_DOOR_THIN_OBLIQUE:

                    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
                    {
                        if (APP_level_file.map[i].wall_texture_id[id_top] == (*tex_iterator).id)
                        {
                            APP_level_file.map[i].wall_texture_id[id_top] = (*tex_iterator).new_id;
                            break;
                        }
                    }
                    break;

                // For cells with FOUR sides texture.
                case LV_C_WALL_FOURSIDE:
                case LV_C_WALL_BOX_FOURSIDE:

                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:

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

    // Update new ids on WALL TEXTURE list.
    for (auto tex_iterator = TEX_wall_list.begin(); tex_iterator != TEX_wall_list.end(); tex_iterator++)
        (*tex_iterator).id = (*tex_iterator).new_id;

    TEX_wall_current_id = TEX_wall_list.size();
}
void    TEX_Prepare_Flat_List_To_Save(std::list<s_Texture>* _list, int* _table, int _mode)
{
    // Update FLAT TEXTURE lists.
    // Clear unused textures, sort in A-Z order, put filenames in level file.
    TEX_Prepare_List_To_Save(_list, _table, 1);

    // Switch old ids to new ids in wall list.
    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
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


    // Update new ids on list.
    for (auto tex_iterator = TEX_flat_list.begin(); tex_iterator != TEX_flat_list.end(); tex_iterator++)
        (*tex_iterator).id = (*tex_iterator).new_id;

    TEX_flat_current_id = TEX_flat_list.size();
}
int     TEX_Add_Texture_To_List(std::list<s_Texture>* _list, int _mode)
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
            s_Texture tmp_texture = { 0 };

            if (_mode == 0) tmp_texture.id = TEX_wall_current_id;
            else            tmp_texture.id = TEX_flat_current_id;

            tmp_texture.new_id = -1;
            wcscpy(tmp_texture.filename, TEX_selected_file_name);

            // Load raw texture and convert it to windows HBITMAP
            TCHAR path_file_name[APP_MAX_PATH];
            ZeroMemory(path_file_name, sizeof(path_file_name));

            if (_mode == 0) wcscpy(path_file_name, TEXT(APP_WALL_TEXTURES_DIRECTORY));
            else            wcscpy(path_file_name, TEXT(APP_FLAT_TEXTURES_DIRECTORY));

            wcscat(path_file_name, L"\\");
            wcscat(path_file_name, TEX_selected_file_name);

            // convert TCHAR path_filename to CHAR path_filename
            ZeroMemory(cnv_input_bitmap_filename, sizeof(cnv_input_bitmap_filename));
            wcstombs_s(NULL, cnv_input_bitmap_filename, path_file_name, wcslen(path_file_name) + 1);

            sIO_Prefs prefs = { 0 };
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
                MessageBox(GetActiveWindow(), L"Texture load error.", L"Error.", MB_ICONWARNING | MB_OK);
            else
            {
                tmp_texture.bitmap_128 = Make_HBITMAP_From_Bitmap_Indexed(  GetActiveWindow(), 128, 128, IO_TEXTURE_MAX_COLORS,
                                                                            raw_table + IO_TEXTURE_MAX_COLORS * (IO_TEXTURE_MAX_SHADES- 1), raw_index);
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
void    TEX_Reset_Texture_List(std::list<s_Texture>* _list)
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
void    TEX_Count_Textures()
{
    ZeroMemory(TEX_wall_table, sizeof(TEX_wall_table));
    ZeroMemory(TEX_flat_table, sizeof(TEX_flat_table));

    TEX_wall_mem = 0; 
    TEX_flat_mem = 0;

    TEX_wall_count = 0;
    TEX_flat_count = 0;    

    for (int i = 0; i < LV_MAP_CELLS_COUNT; i++)
    {
        if (APP_level_file.map[i].cell_type)
        {
            switch (APP_level_file.map[i].cell_type)
            {
                case LV_C_WALL_STANDARD:
                case LV_C_WALL_THIN_HORIZONTAL:
                case LV_C_WALL_THIN_VERTICAL:
                case LV_C_WALL_THIN_OBLIQUE:
                case LV_C_DOOR_THIN_OBLIQUE:
                case LV_C_WALL_BOX:
                case LV_C_WALL_BOX_SHORT:
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_top]]++;
                    break;

                case LV_C_DOOR_THICK_HORIZONTAL:
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_top]]++;
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_bottom]]++;
                    break;

                case LV_C_DOOR_THICK_VERTICAL:
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_left]]++;
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_right]]++;
                    break;

                case LV_C_WALL_FOURSIDE:
                case LV_C_WALL_BOX_FOURSIDE:
                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
                case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
                case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_top]]++;
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_right]]++;
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_bottom]]++;
                    TEX_wall_table[APP_level_file.map[i].wall_texture_id[id_left]]++;
                    break;
            }
        }

        if (APP_level_file.map[i].floor_used) TEX_flat_table[APP_level_file.map[i].flat_texture_id[id_floor]]++;
        if (APP_level_file.map[i].ceil_used) TEX_flat_table[APP_level_file.map[i].flat_texture_id[id_ceil]]++;
    }

    for (int p = 0; p < TEX_MAX_TABLE_SIZE; p++)
    {
        if (TEX_wall_table[p] > 0)  TEX_wall_count++;
        if (TEX_flat_table[p] > 0)  TEX_flat_count++;
    }

    TEX_wall_mem = TEX_wall_count * (IO_TEXTURE_IMAGE_DATA_BYTE_SIZE + IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
    TEX_flat_mem = TEX_flat_count * (IO_TEXTURE_IMAGE_DATA_BYTE_SIZE + IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
}
void    TEX_Draw_Not_Found_Texture(HDC _hdc, int _cx0, int _cy0, int _cx1, int _cy1)
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

    int length = (int)wcslen(tmp);
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

    sAPP_Windows_Bitmap tmp_w_bitmap = { 0 };

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

    sAPP_Windows_Bitmap tmp_w_bitmap = { 0 };

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
int		Get_Wall_Drawing_Coords_Index(float _x, float _y)
{
    for (int i = 0; i < 40; i++)
    {
        if ( (_x >= wall_drawing_coords[i][0] - 0.00001) && (_x <= wall_drawing_coords[i][0] + 0.00001) && (_y >= wall_drawing_coords[i][1] - 0.0001) && (_y<= wall_drawing_coords[i][1] + 0.0001))
            return i;
    }

    return -1;
}
float   Round_To_2_Places(float var)
{
    // 37.66666 * 100 =3766.66
    // 3766.66 + .5 =3767.16    for rounding off value
    // then type cast to int so value is 3767
    // then divided by 100 so the value converted into 37.67
    /*float value = (int)(var * 100 + .5);
    return (float)value / 100;*/
    return 0.0f;
}
void    Make_Empty_Lightmap(void)
{
    SetCurrentDirectory(APP_curr_dir);

    // Create output filename for lightmap by: lightmap_path + level_filename + lightmap_extension.
    TCHAR output_filename[256];
    ZeroMemory(output_filename, sizeof(output_filename));

    wcscat(output_filename, TEXT(APP_LIGHTMAPS_DIRECTORY));
    wcscat(output_filename, L"\\");

    // convert TCHAR to CHAR and split path to get filename
    TCHAR fname[32], app_filename_char[260];
    ZeroMemory(fname, sizeof(fname));
    ZeroMemory(app_filename_char, sizeof(app_filename_char));

    //wcstombs_s(NULL, app_filename_char, APP_filename, wcslen(APP_filename) + 1);
    _wsplitpath(APP_filename, NULL, NULL, fname, NULL);

    wcscat(output_filename, fname);
    wcscat(output_filename, TEXT(IO_LIGHTMAP_FILE_EXTENSION));

    // Test if lightmap file exist.
  /*  WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile(output_filename, &FindFileData);

    if (handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle);
        return;
    }*/

    // Light map not found lets create empty one.
    FILE* lightmap_file;
    lightmap_file = _wfopen(output_filename, L"wb");

    if (lightmap_file == NULL)
        return;

    u_int32 mem_size = IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE * (APP_level_file.wall_lightmaps_count + APP_level_file.flat_lightmaps_count);

     unsigned char* lightmap_block = (unsigned char*)malloc(mem_size);

     if (lightmap_block != NULL)
     {
         memset(lightmap_block, 0xFF, mem_size);
         fwrite(lightmap_block, mem_size, 1, lightmap_file);
         free(lightmap_block);
     }
    
     fclose(lightmap_file);
}