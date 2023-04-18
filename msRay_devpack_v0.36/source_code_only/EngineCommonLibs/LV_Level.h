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

#define LV_MAX_DOORS					32
#define LV_MAX_DOOR_GROUPS				32
#define LV_MAX_DOORS_IN_GROUP			5

// Map cell types.
enum {	LV_C_NOT_SOLID, 
		LV_C_WALL_STANDARD, LV_C_WALL_FOURSIDE, 
		LV_C_WALL_THIN_HORIZONTAL, LV_C_WALL_THIN_VERTICAL, LV_C_WALL_THIN_OBLIQUE, 
		LV_C_WALL_BOX, LV_C_WALL_BOX_FOURSIDE, LV_C_WALL_BOX_SHORT,
		LV_C_DOOR_THICK_HORIZONTAL, LV_C_DOOR_THICK_VERTICAL, LV_C_DOOR_THIN_OBLIQUE,
		LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT, LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT, 
		LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT, LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT };

// Cell DOOR states.
enum {	LV_S_DOOR_OPENED_FOREVER = -1, LV_S_DOOR_CLOSED = 0, LV_S_DOOR_CLOSED_PROXMITY, LV_S_DOOR_OPENING, LV_S_DOOR_OPENED_TEMP, LV_S_DOOR_CLOSING };

// Cell DOOR ACTION states.
enum { LV_A_PUSH = 0, LV_A_PROXMITY = 1 };

// This CELL structure is ONLY for holding level data IN FILE.
// In game this structure will be a bit different and the data will be copied during level loading.
typedef struct 
{
	// Keeps wall vertices coords for non regular walls like: thin wall or box wall: 
	// (x0, y0) = (wall_vertex[0][0], wall_vertex[0][1])
	// (x1, y1) = (wall_vertex[1][0], wall_vertex[1][1])
	float32 wall_vertex[2][2];

	// For additional non regular walls and thin-doors that opens up.
	float32 starting_height;
	float32 height;

	// Walls textures and lightmaps IDs: 0=top, 1=bottom, 2=left, 3=right.
	int16 wall_lightmap_id[4];
	int8 wall_texture_id[4];

	// Flats textures and lightmaps IDs: 0=ceiling, 1=floor.
	int16 flat_lightmap_id[2];
	int8 flat_texture_id[2];

	// Cell type info. Not solid = 0;
	int8 cell_type;

	// Cell state info. For doors, switches, etc.
	int8 cell_state;

	// Tag is for connecting things, like doors-to-doors or doors-to-switches.
	int8 cell_group;

	// Timer can be used for doors, switches ect. When =0 doors stays open, when >0 they will close.
	int8 cell_timer;

	int8 cell_action;

	// This value is a helper for editor only, to mark textured or default floor or ceil cells.
	int8 floor_used;
	int8 ceil_used;

	int8 is_lightmapped;

} _IO_BYTE_ALIGN_ sLV_Cell__File_Only;

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

} _IO_BYTE_ALIGN_ sLV_Level__File_Only;



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

// Door helpers.
// While loading a level file - lets create a helper door group list,
// se we can find fast all id door cells groupped with given door/switch etc.
typedef struct
{
	int16 id_list[LV_MAX_DOORS_IN_GROUP];
	int16 count;								
} _IO_BYTE_ALIGN_ sLV_Door_Group;								

sLV_Door_Group	LV_door_group[LV_MAX_DOOR_GROUPS];

// LV_door_list - the list will contain indexes of cells that are doors.
// LV_door_count - the number of doors in map, must be less than LV_MAX_DOORS.
int32		LV_door_timer[LV_MAX_DOORS];
int16		LV_door_list[LV_MAX_DOORS];
int8		LV_door_count;

// This CELL structure will hold map data during run-time.
typedef struct
{
	// Keeps wall vertices coords for non regular walls like: thin wall or box wall: 
	// (x0, y0) = (wall_vertex[0][0], wall_vertex[0][1])
	// (x1, y1) = (wall_vertex[1][0], wall_vertex[1][1])
	float32 wall_vertex[2][2];

	// For additional non regular walls and thin-doors that opens up.
	float32 starting_height;
	float32 height;

	u_int16	wall_lightmap_id[4];
	u_int8	wall_texture_id[4];

	u_int16	flat_lightmap_id[2];
	u_int8	flat_texture_id[2];

	// Cell type info. Not solid = 0;
	int8	cell_type;

	// Cell state info. For doors, switches, etc.
	int8	cell_state;

	// Tag is for connecting things, like doors-to-doors or doors-to-switches.
	int8	cell_group;

	// Timer can be used for doors, switches ect. When =0 doors stays open, when >0 they will close.
	int8	cell_timer;

	int8 cell_action;

} _IO_BYTE_ALIGN_ sLV_Cell;

sLV_Cell LV_map[LV_MAP_CELLS_COUNT];

// ---------------------------------------------
// --- LEVEL - PUBLIC functions declarations ---
// ---------------------------------------------
int8	LV_Prepare_Level(int8);
void	LV_Free_Level_Resources(void);

#endif
