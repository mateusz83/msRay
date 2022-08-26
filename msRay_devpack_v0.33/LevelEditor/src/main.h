#pragma once

// for enabling button NOTIFY + turning on visual style
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"comctl32.lib")

#define _CRT_SECURE_NO_WARNINGS

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             

// Windows Header Files
#include <windows.h>
#include <commctrl.h>  
#include <Commdlg.h>
#include <shellapi.h>
#include <Shlwapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include <tchar.h>
#include <list>
#include <algorithm>

#include "resource.h"

// using engine libs - using C functions in CPP
extern "C"
{
	#include "../../EngineCommonLibs/LV_Level.h"
	#include "../../EngineCommonLibs/MA_Math.h"
	#include "../../EngineCommonLibs/EN_Engine_Main.h"
}

// -------------------
// --- APP GLOBALS ---
// -------------------
#define APP_NAME				L"msRay - Level Editor (v0.32)"
#define APP_WIN_SIZEX			1760
#define APP_WIN_SIZEY			1080
#define APP_Open_FILE			0
#define APP_SAVE_FILE			1

#if defined _DEBUG
	#define APP_WALL_TEXTURES_DIRECTORY	"..\\..\\TheGameOutput\\data\\wall_textures" 
	#define APP_FLAT_TEXTURES_DIRECTORY	"..\\..\\TheGameOutput\\data\\flat_textures" 
	#define APP_LIGHTMAPS_DIRECTORY		"..\\..\\TheGameOutput\\data\\lightmaps" 
	#define APP_LEVELS_DIRECTORY		"..\\..\\TheGameOutput\\data\\levels" 
#else
	#define APP_WALL_TEXTURES_DIRECTORY	"..\\TheGameOutput\\data\\wall_textures" 
	#define APP_FLAT_TEXTURES_DIRECTORY	"..\\TheGameOutput\\data\\flat_textures" 
	#define APP_LIGHTMAPS_DIRECTORY		"..\\TheGameOutput\\data\\lightmaps" 
	#define APP_LEVELS_DIRECTORY		"..\\TheGameOutput\\data\\levels" 
#endif

HINSTANCE	APP_hInst;
HWND		APP_hWnd;
WCHAR		APP_szTitle[]		= APP_NAME;
WCHAR		APP_szWindowClass[] = APP_NAME;

TCHAR		APP_curr_dir[MAX_PATH], APP_level_dir[MAX_PATH];

LPCTSTR		APP_MAP_FILTER = L"Level maps files (*.lv)\0*.lv\0\0";
LPCTSTR		APP_OBJ_FILTER = L"Obj files (*.obj)\0*.obj\0\0";
LPCTSTR		APP_DEFEXT_MAP = L"lv";
LPCTSTR		APP_DEFEXT_OBJ = L"obj";

// main Level structure to be saved to file
sLV_Level__File_Only	APP_level_file;

// Current map filename
TCHAR		APP_filename[MAX_PATH];
int			APP_never_saved;

// mouse control
POINT APP_mp;
float APP_f_mouse_x, APP_f_mouse_y;
bool APP_is_left_hold, APP_is_middle_hold;

// keys
bool APP_control_pressed;

// ----------------------------------
// --- APP FUNCTIONS declarations ---
// ----------------------------------
ATOM                APP_MyRegisterClass(HINSTANCE hInstance);
BOOL                APP_InitInstance(HINSTANCE, int);

LRESULT CALLBACK    APP_WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Help_Controls_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Help_Lightmaps_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Convert_Tex_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_View_Texture_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Convert_Lightmap_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Select_Texture_Proc(HWND, UINT, WPARAM, LPARAM);

void				APP_Init_Once();
void				APP_Reset();
int					APP_Open();
void				APP_Save();
void				APP_Save_As();
void				APP_Count_or_Export_Lightmaps(int);
void				APP_Generate_Flats_Visibility_Lists();

// ---------------------------
// --- MAP CONTROL GLOBALS ---
// ---------------------------
#define	MAP_LENGTH			LV_MAP_LENGTH
#define	MAP_LENGTH_pow2		(MAP_LENGTH * MAP_LENGTH);
#define	MAP_CELL_SIZE_PX	15
#define	MAP_WINDOW_SIZE		(MAP_LENGTH * MAP_CELL_SIZE_PX + 2)

enum LAYERS							{ LAYERS_MAIN_LAYER, LAYERS_FLOOR_LAYER, LAYERS_CEIL_LAYER, LAYERS_LIGHTMAPS_LAYER, LAYERS_FLATS_VISIBILITY_LAYER };
enum LIST_MAIN_LAYER				{ LIST_MAIN_PLAYER, LIST_MAIN_WALL_STANDARD, LIST_MAIN_WALL_FOURSIDE };
enum LIST_FLOOR_LAYER				{ LIST_FLOOR };
enum LIST_CEIL_LAYER				{ LIST_CEIL };
enum LIST_LIGHTMAPS_LAYER			{ LIST_LIGHTMAPS_SELECT };
enum LIST_FLATS_VISIBILITY_LAYER	{ LIST_FV_SELECT_AND_GENERATE, LIST_FV_VIEW };

sLV_Cell__File_Only MAP_working_cell;

// The std::list will help to create 8 lists for each used map cell
// that contains potentially visible cells.
std::list<short> MAP_fv_list[LV_MAP_CELLS_COUNT][8];

int selected_cell_x, selected_cell_y, selected_cell_index;
int hover_cell_x, hover_cell_y, hover_cell_index;

float f_offset_x, f_offset_y;
float f_start_pan_x, f_start_pan_y;
float f_scale_x, f_scale_y;

// text info
TCHAR map_text_layers[5][32] = { L"Main layer", L"Floor layer", L"Ceiling layer", L"Lightmaps layer", L"Flats visibility layer"};
TCHAR map_text_texture_mode[2][16] = { L"(solid view)", L"(textured view)" };

// rects
RECT map_rc = { 0, 0, MAP_WINDOW_SIZE - 1, MAP_WINDOW_SIZE - 1 };
RECT map_rc_text_layer = { 10, 10, 20, 10 };
RECT map_rc_text_mode = { 10, 30, 20, 10 };

// pens, brushes
HPEN	hp_none, hp_map_not_found_texture;
HPEN	hp_active_grid, hp_main_grid, hp_floor_grid, hp_ceil_grid, hp_lightmaps_grid, hp_flats_visibility_grid;
HPEN	hp_active_dotted, hp_floor_dotted, hp_ceil_dotted, hp_lightmaps_dotted, hp_flats_visibility_dotted;

HBRUSH	hbr_map_not_found_texture, hbr_black;
HBRUSH	hbr_map_player, hbr_map_walls_standard, hbr_map_walls_fourside_01, hbr_map_walls_fourside_02;
HBRUSH	hbr_map_floor, hbr_map_ceil, hbr_map_lightmaps, hbr_map_flats_visibility, hbr_map_flats_visibility_list;

HBRUSH	hbr_active_bg, hbr_main_bg, hbr_floor_bg, hbr_ceil_bg, hbr_lightmaps_bg, hbr_flats_visibility_bg;
HBRUSH	hbr_selected_cell;

// -------------------
// --- GUI GLOBALS ---
// -------------------
HFONT h_font_1;

// can be integer, returns something from combos, boxes etc.
LRESULT gui_current_layer;
LRESULT gui_current_list_index;
bool gui_texprew_mode, gui_add_mode;
int gui_selected_image, gui_selected_fv_list;

// -----------------------
// --- CONVERT GLOBALS ---
// -----------------------
char				cnv_input_bitmap_filename[256];
sBM_Bitmap_Indexed	cnv_input_bitmap_BGRA, cnv_input_bitmap_RGBA;
u_int8				*cnv_raw_index, *cnv_raw_lm_wall, *cnv_raw_lm_floor, *cnv_raw_lm_ceil;
u_int32*			cnv_raw_table;
bool				cnv_is_input_loaded, cnv_is_raw_loaded;
int					cnv_current_light_mode, cnv_current_light_treshold;
int					cnv_raw_lm_wall_lenght, cnv_raw_lm_flat_lenght;

HBITMAP				cnv_hbm_texture, cnv_hbm_texture_256, cnv_hbm_texture_128, cnv_hbm_texture_64,
					cnv_hbm_texture_32, cnv_hbm_texture_16, cnv_hbm_texture_8, cnv_hbm_texture_4;

// -------------------------------
// --- TEXTURE CONTROL GLOBALS ---
// -------------------------------
#define id_top			0
#define id_right		1
#define id_bottom		2
#define id_left			3
#define id_floor		0
#define id_ceil			1

typedef struct
{
	TCHAR filename[IO_MAX_STRING_TEX_FILENAME];
	int id;
	int new_id;
	HBITMAP bitmap_128;
} s_Texture;

std::list<s_Texture, std::allocator<s_Texture>> TEX_wall_list, TEX_flat_list;

int				TEX_wall_current_id, TEX_flat_current_id, TEX_prev_id, TEX_wall_count, TEX_flat_count, TEX_wall_mem, TEX_flat_mem;
int				TEX_wall_is_default;
TCHAR			TEX_selected_file_name[IO_MAX_STRING_TEX_FILENAME];
unsigned char	TEX_wall_table[512], TEX_flat_table[512];

// ----------------------------------
// --- GUI FUNCTIONS declarations ---
// ----------------------------------

void GUI_Create(HWND);
void GUI_Reset(HWND);
BOOL GUI_OpenDialog(TCHAR*, bool, LPCWSTR, LPCWSTR);
bool GUI_is_Map_Region(void);

void GUI_Update_Resources(HWND);
void GUI_Update_Map_Coords(bool);
void GUI_Update_Layers_Combo(HWND);
void GUI_Update_List_Box(HWND);
void GUI_Update_Add_Mode_Button(HWND);

void GUI_Update_Properties(HWND);
void GUI_Update_Properties_Player(HWND);
void GUI_Update_Properties_Walls(HWND);
void GUI_Update_Properties_Floor(HWND);
void GUI_Update_Properties_Ceil(HWND);
void GUI_Update_Properties_Lightmaps(HWND);
void GUI_Update_Properties_Flats_Visibility(HWND);

void GUI_Draw_Properites_Textures(HWND);
void GUI_Draw_Properties_Textures_Window(HWND, int, int, std::list<s_Texture>*);

void GUI_Hide_Properties(HWND);

// ----------------------------------
// --- MAP FUNCTIONS DECLARATIONS ---
// ----------------------------------

void Map_Reset(void);
void Map_Update_Colors(void);

void Map_Draw_BG(HDC);
void Map_Draw_Grid(HDC);
void Map_Draw_Selected_Cell(HDC);
void Map_Draw_Layer_Info(HDC);
void Map_Draw_Textured_Square(HDC, int, int, int, int, std::list<s_Texture>*, int);
void Map_Draw_Player(HDC);

void Map_Draw_Objects(HDC);
void Map_Draw_Objects_Walls_Standard(HDC _hdc, int, int, int);
void Map_Draw_Objects_Walls_Fourside(HDC _hdc, int, int, int);
void Map_Draw_Objects_Floor(HDC, int, int, int);
void Map_Draw_Objects_Ceil(HDC, int, int, int);
void Map_Draw_Objects_Lightmaps(HDC, int, int, int);
void Map_Draw_Objects_Flats_Visibility(HDC, int, int, int);
void Map_Draw_Objects_Flats_Visibility_List(HDC);
void Map_Draw_Objects_Dotted_Rectangle(HDC, float, float, float, float);

void Map_Draw_Selected_Cell(HDC);
void Map_Draw_Layer_Info(HDC);

void Map_Add_Current_Object(void);
void Map_Read_Current_Cell(void);
void Map_Clear_Current_Cell(void);

// ----------------------------------------------
// --- TEXTURE CONTROL FUNCTIONS declarations ---
// ----------------------------------------------

void	TEX_Reset();
bool	TEX_List_Sort_Function(const s_Texture&, const s_Texture&);
void	TEX_Prepare_List_To_Save(std::list<s_Texture>*, unsigned char*, int);
int		TEX_Add_Texture_To_List(std::list<s_Texture>*, int);
void	TEX_Reset_Texture_List(std::list<s_Texture>*);
void	TEX_Count_Textures();
void	TEX_Draw_Not_Found_Texture(HDC, int, int, int, int);

// -------------------------------------
// --- HELPER FUNCTIONS DECLARATIONS ---
// -------------------------------------

void	World_To_Screen(float, float, int&, int&);
void	Screen_To_World(int, int, float&, float&);
void	Add_Number_Spaces(int, wchar_t*);
void	Center_Window(HWND);
HBITMAP	Make_HBITMAP_From_Bitmap_Indexed(HWND, int, int, int, u_int32*, u_int8*);
HBITMAP Make_HBITMAP_From_Bitmap_Indexed_Grayscale(HWND, int, int, u_int8*);