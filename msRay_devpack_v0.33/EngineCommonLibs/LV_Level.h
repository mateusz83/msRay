#ifndef __LV_LEVEL_H__
#define __LV_LEVEL_H__

// ----------------------------------------------------------------
// --- Only for Microsoft Visual Studio to not giving warnings. ---
// ----------------------------------------------------------------
#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
// ----------------------------------------------------------------

#include "BM_Bitmap.h"

// ----------------------------------------------------
// --- LEVEL - PUBLIC globals, constants, variables ---
// ----------------------------------------------------

// Level map constants.
#define LV_MAP_LENGTH					64
#define LV_MAP_LENGTH_BITSHIFT			6
#define LV_MAP_CELLS_COUNT				4096	// LV_MAP_LENGTH * LV_MAP_LENGTH

// Map cells types.
enum { LV_C_WALL_STANDARD, LV_C_WALL_FOURSIDE };

// This CELL structure is ONLY for holding level data IN FILE.
// In game this structure will be a bit different and the data will be copied during level loading.
typedef struct 
{
	// Walls textures and lightmaps id: 0=top, 1=bottom, 2=left, 3=right.
	int8 wall_texture_id[4];
	int16 wall_lightmap_id[4];

	// Flats textures and lightmaps id: 0=ceiling, 1=floor.
	int8 flat_texture_id[2];
	int16 flat_lightmap_id[2];

	// Contains a number of elements in lists of potentially visible cells from that current cell.
	// There is 8 lists per cell. Each list covers a range of 45* = 360* / 8.
	int16 fv_list[8];

	// This value is designed to hold an overall cell intensity shade (0-255)
	// that can be used to to give correct intensity for example to an enemy or gun.
	// This value should be averaged value of floor or ceiling light map in current cell.
	// For example if enemy stands in dark place it shoud use its darken intensity table.
	int8 cell_shade;

	int8 is_cell_solid;
	int8 cell_type;

	// This value is a helper for editor only, to mark textured or default floor or ceil cells.
	int8 was_floor_negative;
	int8 was_ceil_negative;

	int8 is_lightmapped;
	int8 is_fv_marked;

} sLV_Cell__File_Only;

// This LEVEL structure is also ONLY for keeps data IN FILE.
// The data will be copied to global structures during loading.
typedef struct 
{
	// The structure made of CELLS is called a MAP.
	sLV_Cell__File_Only map[LV_MAP_CELLS_COUNT];

	// We assume max. 128 textures for each walls and flats. 
	// Let each tex filename already has an extension added. 
	// The max lenght of filename with extensino should be max. 16 characters.
	char wall_texture_filename[128][IO_MAX_STRING_TEX_FILENAME];
	char flat_texture_filename[128][IO_MAX_STRING_TEX_FILENAME];

	// We assume level two titles - each max. 32 characters.
	char title_01[IO_MAX_STRING_TITLE];
	char title_02[IO_MAX_STRING_TITLE];

	// size of flat visibility list in bytes
	int32 fv_list_size;

	// We should also know how many wall and flat lightmaps we have. The number can be big - so lets store it in 2 bytes.
	int16 wall_lightmaps_count;
	int16 flat_lightmaps_count;

	// We need information how many wall and flat textures this level uses. Assumed max. 128 per each set.
	int8 wall_textures_count;
	int8 flat_textures_count;

	// Player starting attributes.
	int8 player_starting_cell_x;
	int8 player_starting_cell_y;

	// We will 'pack' 0-360 degrees range in 1 signed byte, dividing degree by 3. - The values in here are: 0, 30, 60, 120.
	int8 player_starting_angle;

} sLV_Level__File_Only;


// --- Global LEVEL resources. ---


// LV_textures - is the pointer to all textures image data. It's 1 byte index to RGB color table called: LV_texture_intensity__LUT.
// Wall textures are stored first, flat textures are next. Wall textures are rotated 90 degrees right.
// Each texture is made from 7 mipmaps from 256x256 to 4x4.
// Each map cell (wall, floor and ceil) already contains a pointer to its texture.
u_int8*		LV_textures;	

// LV_textures_intensity__LUT - is the pointer to actual RGB color tables.
// Each texture has 128 color tables from starting intensity down to black.
u_int32*	LV_textures_intensity;

// LV_lightmaps- is the pointer to all lightmaps.
// Wall lightmaps are stored first, then floor lightmaps and ceil lightmaps as last.
// Wall lightmaps are also rotated 90 degrees right as well as wall textures.
u_int8*		LV_lightmaps;

// LV_fv_list
// is the list of potentially visible cells from all cells that player can move into.
int16*		LV_fv_list;

// This CELL structure will hold map data during run-time.
// The main difference between CELL structure for file is that,
// we will keep here the pointers to current textures - not ids or offsets.
// But the pointers can be setup only after the textures are read into memory.
typedef struct
{
	// 8 Pointers to to 8 slices of cell that contains a list of potentially visible cells:
	// 0: (0-45), 1: (45-90), 2: (90-135), 3: (135-180), 4: (180-225), 5: (225-270), 6: (270-315), 7: (315-360)
	int16*	pvc_list__PTR[8];

	// Size of each list.
	int16	pvc_list_size[8];

	// Walls textures and lightmap pointers: 0=top, 1=bottom, 2=left, 3=right.
	int32*	wall_texture_intensity__PTR[4];
	int8*	wall_texture__PTR[4];
	int8*	wall_lightmap__PTR[4];

	// Flats textures and lightmap pointers: 0=ceiling, 1=floor.
	int32*	flat_texture_intensity__PTR[2];
	int8*	flat_texture__PTR[2];
	int8*	flat_lightmap__PTR[2];

	int8	cell_shade;
	int8	is_cell_solid;
	int8	cell_type;

} _IO_BYTE_ALIGN_ sLV_Cell;

sLV_Cell LV_map[LV_MAP_CELLS_COUNT];

// ---------------------------------------------
// --- LEVEL - PUBLIC functions declarations ---
// ---------------------------------------------
int8	LV_Prepare_Level(int8);
void	LV_Free_Level_Resources(void);

#endif