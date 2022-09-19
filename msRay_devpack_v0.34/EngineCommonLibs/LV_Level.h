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
enum { LV_C_NOT_SOLID, LV_C_WALL_STANDARD, LV_C_WALL_FOURSIDE };

// This CELL structure is ONLY for holding level data IN FILE.
// In game this structure will be a bit different and the data will be copied during level loading.
typedef struct 
{
	// Walls textures and lightmaps IDs: 0=top, 1=bottom, 2=left, 3=right.
	int8 wall_texture_id[4];
	int16 wall_lightmap_id[4];

	// Flats textures and lightmaps IDs: 0=ceiling, 1=floor.
	int8 flat_texture_id[2];
	int16 flat_lightmap_id[2];

	// Cell type info. Not solid = 0;
	int8 cell_type;

	// This value is a helper for editor only, to mark textured or default floor or ceil cells.
	int8 was_floor_negative;
	int8 was_ceil_negative;

	int8 is_lightmapped;

} sLV_Cell__File_Only;

// This LEVEL structure is also ONLY for keeps data IN FILE.
// The data will be copied to global structures during loading.
typedef struct 
{
	// The structure made of CELLS is called a MAP.
	sLV_Cell__File_Only map[LV_MAP_CELLS_COUNT];

	// We assume max. 128 textures for each walls and flats. 
	// Let each tex filename already has an extension added. 
	// The max lenght of filename with extensinon should be max. 16 characters.
	char wall_texture_filename[128][IO_MAX_STRING_TEX_FILENAME];
	char flat_texture_filename[128][IO_MAX_STRING_TEX_FILENAME];

	// We assume level two titles - each max. 32 characters.
	char title_01[IO_MAX_STRING_TITLE];
	char title_02[IO_MAX_STRING_TITLE];

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


// LV_wall_textures__PTR and LV_flat_textures__PTR - are the pointers to all textures image data.
// The data is 1 byte index to RGB color table called: LV_wall_textures_colors__PTR and LV_flat_textures_colors__PTR.
// Wall textures are stored first, flat textures are next. Wall textures are rotated 90 degrees right.
// Each texture is made from 3 mipmaps from 128x128 to 32x32.
u_int8*		LV_wall_textures__PTR;	
u_int8*		LV_flat_textures__PTR;

// LV_wall_textures_colors__PTR and LV_flat_textures_colors__PTR - are the pointers to actual RGB color tables.
// Each texture has 256 color tables from starting intensity down to black.
u_int32*	LV_wall_textures_colors__PTR;
u_int32*	LV_flat_textures_colors__PTR;

// LV_lightmaps - is the pointer to all lightmaps.
// Wall lightmaps are stored first, then floor lightmaps and ceil lightmaps as last.
// Wall lightmaps are also rotated 90 degrees right as well as wall textures.
u_int8*		LV_lightmaps__PTR;

// This CELL structure will hold map data during run-time.
typedef struct
{
	u_int16	wall_lightmap_id[4];
	u_int8	wall_texture_id[4];

	u_int16	flat_lightmap_id[2];
	u_int8	flat_texture_id[2];

	// Cell type info. Not solid = 0;
	int8	cell_type;

} _IO_BYTE_ALIGN_ sLV_Cell;

sLV_Cell LV_map[LV_MAP_CELLS_COUNT];

// ---------------------------------------------
// --- LEVEL - PUBLIC functions declarations ---
// ---------------------------------------------
int8	LV_Prepare_Level(int8);
void	LV_Free_Level_Resources(void);

#endif
