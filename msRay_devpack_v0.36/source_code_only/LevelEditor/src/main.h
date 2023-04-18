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

#define APP_MAX_PATH			1024	
#define APP_MAX_FILENAME		256

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
TCHAR		APP_filename[APP_MAX_PATH];
int			APP_never_saved;

// mouse control
POINT APP_mp;
float APP_f_mouse_x, APP_f_mouse_y;
bool APP_is_left_hold, APP_is_middle_hold;

// keys
bool APP_control_pressed, APP_alt_pressed;

// ----------------------------------
// --- APP FUNCTIONS declarations ---
// ----------------------------------
ATOM                APP_MyRegisterClass(HINSTANCE hInstance);
BOOL                APP_InitInstance(HINSTANCE, int);

LRESULT CALLBACK    APP_WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Help_Controls_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Help_Lightmaps_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Convert_Tex_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Convert_Lightmap_Proc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	APP_Wnd_Select_Texture_Proc(HWND, UINT, WPARAM, LPARAM);

void				APP_Init_Once();
void				APP_Reset();
int					APP_Open();
void				APP_Save();
void				APP_Save_As();
void				APP_Saving_File();
void				APP_Count_or_Export_Lightmaps(int);

// ---------------------------
// --- MAP CONTROL GLOBALS ---
// ---------------------------
#define	MAP_LENGTH			LV_MAP_LENGTH
#define	MAP_LENGTH_pow2		(MAP_LENGTH * MAP_LENGTH);
#define	MAP_CELL_SIZE_PX	15
#define	MAP_WINDOW_SIZE		(MAP_LENGTH * MAP_CELL_SIZE_PX + 2)

enum LAYERS				{	LAYERS_PLAYER_LAYER, LAYERS_WALL_LAYER, LAYERS_FLOOR_LAYER, LAYERS_CEIL_LAYER, LAYERS_LIGHTMAPS_LAYER, LAYERS_DOOR_LAYER	};
enum LIST_PLAYER		{	LIST_PLAYER		};
enum LIST_WALL			{	LIST_WALL_STANDARD, LIST_WALL_FOURSIDE, LIST_WALL_THIN_HORIZONTAL, LIST_WALL_THIN_VERTICAL, LIST_WALL_THIN_OBLIQUE, LIST_WALL_BOX, LIST_WALL_BOX_FOURSIDE, LIST_WALL_BOX_SHORT	};
enum LIST_FLOOR			{	LIST_FLOOR	};
enum LIST_CEIL			{	LIST_CEIL	};
enum LIST_LIGHTMAPS		{	LIST_LIGHTMAPS_SELECT	};
enum LIST_DOORS			{	LIST_DOOR_THICK_HORIZONTAL, LIST_DOOR_THICK_VERTICAL, LIST_DOOR_THIN_OBLIQUE, LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT, LIST_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT,
							LIST_DOOR_BOX_FOURSIDE_VERTICAL_LEFT, LIST_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT	};

sLV_Cell__File_Only MAP_working_cell;

int selected_cell_x, selected_cell_y, selected_cell_index;
int hover_cell_x, hover_cell_y, hover_cell_index;

float f_offset_x, f_offset_y;
float f_start_pan_x, f_start_pan_y;
float f_scale_x, f_scale_y;

// text info
TCHAR map_text_layers[6][32] = { L"Player layer", L"Wall layer", L"Floor layer", L"Ceiling layer", L"Lightmaps layer", L"Door layer"};
TCHAR map_text_texture_mode[2][16] = { L"(solid view)", L"(textured view)" };

// rects
RECT map_rc = { 0, 0, MAP_WINDOW_SIZE - 1, MAP_WINDOW_SIZE - 1 };
RECT map_rc_text_layer = { 10, 10, 20, 10 };
RECT map_rc_text_mode = { 10, 30, 20, 10 };

// pens, brushes
HPEN	hp_none, hp_map_not_found_texture;
HPEN	hp_active_grid, hp_main_grid, hp_floor_grid, hp_ceil_grid, hp_lightmaps_grid;
HPEN	hp_active_dotted, hp_floor_dotted, hp_ceil_dotted, hp_lightmaps_dotted;

HPEN	hp_map_walls_dotted, hp_map_doors_dotted;

HBRUSH	hbr_map_not_found_texture, hbr_black;
HBRUSH	hbr_map_player, hbr_map_walls_standard, hbr_map_walls_fourside_01, hbr_map_walls_fourside_02, hbr_map_walls_short, hbr_map_doors;
HPEN	hp_map_walls_thin, hp_map_doors_thin;
HBRUSH	hbr_map_floor, hbr_map_ceil, hbr_map_lightmaps;

HBRUSH	hbr_drawing_bg, hbr_drawing_door_direction;
HPEN	hp_drawing_grid, hp_drawing_line;

HBRUSH	hbr_active_bg, hbr_main_bg, hbr_floor_bg, hbr_ceil_bg, hbr_lightmaps_bg;
HBRUSH	hbr_selected_cell;
HBRUSH	hbr_mode_brush_add, hbr_mode_brush_clear;

// -------------------
// --- GUI GLOBALS ---
// -------------------
HFONT h_font_1;

// can be integer, returns something from combos, boxes etc.
LRESULT gui_current_layer;
LRESULT gui_current_list_index;
bool gui_texprew_mode;
int gui_selected_image;

// -----------------------
// --- CONVERT GLOBALS ---
// -----------------------
char				cnv_input_bitmap_filename[APP_MAX_FILENAME];
sBM_Bitmap_Indexed	cnv_input_bitmap_BGRA, cnv_input_bitmap_ARGB;
u_int8				*cnv_raw_index, *cnv_raw_lm_wall, *cnv_raw_lm_floor, *cnv_raw_lm_ceil;
u_int32*			cnv_raw_table;
bool				cnv_is_input_loaded, cnv_is_raw_loaded;
int					cnv_current_light_mode, cnv_current_light_treshold;
int					cnv_raw_lm_wall_lenght, cnv_raw_lm_flat_lenght;

int					cnv_hbm_ID[4] = { IDS_DC_TEXTURE_00, IDS_DC_TEXTURE_01, IDS_DC_TEXTURE_02, IDS_DC_TEXTURE_03 };
HBITMAP				cnv_hbm_texture[4];

// -------------------------------
// --- TEXTURE CONTROL GLOBALS ---
// -------------------------------
#define id_top			0
#define id_right		1
#define id_bottom		2
#define id_left			3
#define id_floor		0
#define id_ceil			1

#define INPUT_TEX_MAX_WIDTH			224  // 128+64+32
#define INPUT_TEX_MAX_HEIGHT		128
#define INPUT_LIGHTMAP_MAX_WIDTH	2048
#define INPUT_LIGHTMAP_MAX_HEIGHT	2048

#define TEX_MAX_TABLE_SIZE			512

typedef struct
{
	TCHAR filename[32];
	int id;
	int new_id;
	HBITMAP bitmap_128;
} s_Texture;

std::list<s_Texture, std::allocator<s_Texture>> TEX_wall_list, TEX_flat_list;

int				TEX_wall_current_id, TEX_flat_current_id, TEX_prev_id, TEX_wall_count, TEX_flat_count, TEX_wall_mem, TEX_flat_mem;
int				TEX_wall_is_default;
TCHAR			TEX_selected_file_name[32];
int				TEX_wall_table[TEX_MAX_TABLE_SIZE], TEX_flat_table[TEX_MAX_TABLE_SIZE];

// -------------------------------------------------
// --- DRAWING NON REGULAR WALLS CONTROL GLOBALS ---
// -------------------------------------------------

float wall_drawing_coords[40][2] =	{	{0.0f, 0.0f}, {0.0f, 0.1f}, {0.0f, 0.2f}, {0.0f, 0.3f}, {0.0f, 0.4f}, {0.0f, 0.5f}, {0.0f, 0.6f}, {0.0f, 0.7f}, {0.0f, 0.8f}, {0.0f, 0.9f},
										{0.0f, 1.0f}, {0.1f, 1.0f}, {0.2f, 1.0f}, {0.3f, 1.0f}, {0.4f, 1.0f}, {0.5f, 1.0f}, {0.6f, 1.0f}, {0.7f, 1.0f}, {0.8f, 1.0f}, {0.9f, 1.0f},
										{1.0f, 1.0f}, {1.0f, 0.9f}, {1.0f, 0.8f}, {1.0f, 0.7f}, {1.0f, 0.6f}, {1.0f, 0.5f}, {1.0f, 0.4f}, {1.0f, 0.3f}, {1.0f, 0.2f}, {1.0f, 0.1f},
										{1.0f, 0.0f}, {0.9f, 0.0f}, {0.8f, 0.0f}, {0.7f, 0.0f}, {0.6f, 0.0f}, {0.5f, 0.0f}, {0.4f, 0.0f}, {0.3f, 0.0f}, {0.2f, 0.0f}, {0.1f, 0.0f} };

// ----------------------------------
// --- GUI FUNCTIONS declarations ---
// ----------------------------------

void GUI_Create(HWND);
void GUI_Reset(HWND);
BOOL GUI_OpenDialog(TCHAR*, bool, LPCWSTR, LPCWSTR);
bool GUI_is_Map_Region(void);

void GUI_Update_Resources_File(HWND);
void GUI_Update_Resources_Memory(HWND);
void GUI_Update_Map_Coords(bool);
void GUI_Update_Layers_Combo(HWND);
void GUI_Update_List_Box(HWND);

void GUI_Update_Properties(HWND);
void GUI_Update_Properties_Player(HWND);
void GUI_Update_Properties_Walls(HWND);
void GUI_Update_Properties_Floor(HWND);
void GUI_Update_Properties_Ceil(HWND);
void GUI_Update_Properties_Lightmaps(HWND);
void GUI_Update_Properties_Doors(HWND);

void GUI_Update_Properties_Drawing_Window(HWND, int);
void GUI_Update_Properties_Drawing_Height(HWND, int);

void GUI_Update_Properties_Wall_Tex_Name(HWND, int);
void GUI_Update_Properties_Flat_Tex_Name(HWND, int);
void GUI_Update_Properties_Values(HWND, int);

void GUI_Draw_Properites_Textures(HWND);
void GUI_Draw_Properties_Textures_Window(HWND, int, int, std::list<s_Texture>*);

void GUI_Draw_Properties_Drawing_Window(HWND);
void GUI_Draw_Properties_Drawing_Window_BG(HDC);
void GUI_Draw_Properties_Drawing_Window_Line(HWND, int);
void GUI_Draw_Properties_Drawing_Window_Box(HWND, int, bool);

void GUI_Draw_Properties_Drawing_Height(HWND);

void GUI_Hide_Properties(HWND);

// ----------------------------------
// --- MAP FUNCTIONS DECLARATIONS ---
// ----------------------------------

void Map_Reset(void);
void Map_Update_Colors(void);

void Map_Draw_BG(HDC);
void Map_Draw_Grid(HDC);
void Map_Draw_Selected_Cell(HDC);
void Map_Draw_Mode(HDC);
void Map_Draw_Layer_Info(HDC);
void Map_Draw_Textured_Square(HDC, int, int, int, int, std::list<s_Texture>*, int);
void Map_Draw_Player(HDC);

void Map_Draw_Objects(HDC);
void Map_Draw_Objects_Walls_Standard(HDC, int, int, int, HBRUSH);
void Map_Draw_Objects_Walls_Fourside(HDC _hdc, int, int, int);
void Map_Draw_Objects_Thin_Wall(HDC, int, int, int, HPEN);
void Map_Draw_Objects_Thin_Wall_Side(HDC, int, int, int, HPEN);
void Map_Draw_Objects_Walls_Box(HDC, int, int, int, HBRUSH);
void Map_Draw_Objects_Floor(HDC, int, int, int);
void Map_Draw_Objects_Ceil(HDC, int, int, int);
void Map_Draw_Objects_Lightmaps(HDC, int, int, int);
void Map_Draw_Objects_Dotted_Rectangle(HDC, float, float, float, float, HPEN);
void Map_Draw_Objects_Dotted_Line(HDC, float, float, float, float, HPEN);
void Map_Draw_Objects_Dotted_Wall_Layer(HDC, int, int, int);

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
void	TEX_Prepare_List_To_Save(std::list<s_Texture>*, int*, int);
void	TEX_Prepare_Wall_List_To_Save(std::list<s_Texture>*, int*, int);
void	TEX_Prepare_Flat_List_To_Save(std::list<s_Texture>*, int*, int);
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
int		Get_Wall_Drawing_Coords_Index(float _x, float _y);
float	Round_To_2_Places(float);
void    Make_Empty_Lightmap(void);
