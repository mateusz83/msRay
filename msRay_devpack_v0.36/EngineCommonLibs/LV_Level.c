#include "LV_Level.h"
#include "PL_Player.h"
#include "MA_Math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------
// --- LEVEL - PRIVATE globals, constants, variables ---
// -----------------------------------------------------

// Temporary for level loading. The data will be copied into final structures.
sLV_Level__File_Only LV_Level__File_Only;

// List of actual levels filenames, the same for lightmaps.
// The extensions differs and are added automatically during loading.
const char* LV_level_list[] = { "level_01" };


// ----------------------------------------------
// --- LEVEL - PRIVATE functions declarations ---
// ----------------------------------------------
int8	LV_Load_Level(int8);
int8	LV_Load_Level_Textures(void);
int8	LV_Load_Level_Lightmaps(int8);
int8	LV_Load_Level_Prepare_Cells(void);


// --------------------------------------------
// --- LEVEL - PUBLIC functions definitions ---
// --------------------------------------------
int8 LV_Prepare_Level(int8 _level_number)
{
	if (!LV_Load_Level(_level_number))
	{
		return EN_STATE_END;
	}

	if (!LV_Load_Level_Textures())
	{
		return EN_STATE_END;
	}

	if (!LV_Load_Level_Lightmaps(_level_number))
	{
		return EN_STATE_END;
	}

	if (!LV_Load_Level_Prepare_Cells()) return EN_STATE_END;

	return EN_STATE_GAMEPLAY_RUN;
}

void LV_Free_Level_Resources(void)
{
	LV_wall_textures__PTR = NULL;
	LV_wall_textures_colors__PTR = NULL;

	free(LV_lightmaps__PTR);
	free(LV_wall_textures__PTR);
	free(LV_wall_textures_colors__PTR);
}


// ---------------------------------------------
// --- LEVEL - PRIVATE functions definitions ---
// ---------------------------------------------

int8 LV_Load_Level(int8 _level_number)
{
	// Prepare string with level filename and its directory.
	char level_filename[IO_MAX_STRING_PATH_FILENAME];
	memset(level_filename, 0, IO_MAX_STRING_PATH_FILENAME);

	// Get level name from number.
	memcpy(level_filename, IO_LEVELS_DIRECTORY, strlen(IO_LEVELS_DIRECTORY));
	strcat(level_filename, LV_level_list[_level_number]);

	// Add extension to level filename.
	strcat(level_filename, IO_LEVEL_FILE_EXTENSION);

	// Try open level file and fill up the temporary LV_Level__File_Only structure.
	FILE* file_in;
	file_in = fopen(level_filename, "rb");

	if (file_in == NULL)
		return 0;

	// Read in the file header (static data).
	fread(&LV_Level__File_Only, sizeof(sLV_Level__File_Only), 1, file_in);

	// Close the file.
	fclose(file_in);

	// Copy some info that can be copied right now.

	// Player starting info.
	PL_player.x = (float32)LV_Level__File_Only.player_starting_cell_x + 0.5f;
	PL_player.y = (float32)LV_Level__File_Only.player_starting_cell_y + 0.5f;

	// Multiply by 3 to 'unpack'. Then convert to radians.
	PL_player.starting_angle_radians = (float32)LV_Level__File_Only.player_starting_angle * 3;		
	PL_player.starting_angle_radians = (PL_player.starting_angle_radians * MA_pi) / 180.0f;

	return 1;
}

int8 LV_Load_Level_Textures(void)
{
	// Get number of wall and float textures..
	int16 all_textures_count = LV_Level__File_Only.wall_textures_count + LV_Level__File_Only.flat_textures_count;

	// Try allocate enought memory for intensity color map for all textures:
	LV_wall_textures_colors__PTR = (u_int32*)malloc(all_textures_count * IO_TEXTURE_COLOR_DATA_BYTE_SIZE * sizeof(u_int32));

	if (LV_wall_textures_colors__PTR == NULL)
		return 0;

	// Try allocate enought memory for textures image data for all textures - including their mip-maps:
	LV_wall_textures__PTR = (u_int8*)malloc(all_textures_count * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE * sizeof(u_int8));

	if (LV_wall_textures__PTR == NULL)
		return 0;

	// Prepare string that hold the path + filename + ext
	char path_filename[IO_MAX_STRING_PATH_FILENAME];

	// Clone pointers as local to use them for read in and offsets..
	u_int32* tmp__LV_textures_intensity = LV_wall_textures_colors__PTR;
	u_int8* tmp__LV_textures = LV_wall_textures__PTR;

	// NOTE.
	// The texture filenames are saved without the extension.
	// We need to add proper wall or flat extension during loading.

	// First read every WALL texture...
	for (int32 i = 0; i < LV_Level__File_Only.wall_textures_count; i++)
	{
		memset(path_filename, 0, IO_MAX_STRING_PATH_FILENAME);

		strcat(path_filename, IO_WALL_TEXTURES_DIRECTORY);
		strcat(path_filename, LV_Level__File_Only.wall_texture_filename[i]);
		strcat(path_filename, IO_WALL_TEXTURE_FILE_EXTENSION);

		int32 result = BM_Read_Texture_RAW(path_filename, tmp__LV_textures_intensity, tmp__LV_textures, &IO_prefs);

		tmp__LV_textures_intensity += IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
		tmp__LV_textures += IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

		if (result == 0)
			return 0;
	}

	// Next, read every FLAT texture...
	for (int32 i = 0; i < LV_Level__File_Only.flat_textures_count; i++)
	{
		memset(path_filename, 0, IO_MAX_STRING_PATH_FILENAME);

		strcat(path_filename, IO_FLAT_TEXTURES_DIRECTORY);
		strcat(path_filename, LV_Level__File_Only.flat_texture_filename[i]);
		strcat(path_filename, IO_FLAT_TEXTURE_FILE_EXTENSION);

		int32 result = BM_Read_Texture_RAW(path_filename, tmp__LV_textures_intensity, tmp__LV_textures, &IO_prefs);

		tmp__LV_textures_intensity += IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
		tmp__LV_textures += IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

		if (result == 0)
			return 0;
	}

	int32 wall_color_data_offset = LV_Level__File_Only.wall_textures_count * IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
	int32 wall_image_data_offset = LV_Level__File_Only.wall_textures_count * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

	LV_flat_textures__PTR = LV_wall_textures__PTR + wall_image_data_offset;
	LV_flat_textures_colors__PTR = LV_wall_textures_colors__PTR + wall_color_data_offset;

	return 1;
}

int8 LV_Load_Level_Lightmaps(int8 _level_number)
{
	// Try allocate enought memory for all lightmaps in one memory block.
	// Wall lightmaps first, than floor lightmaps and ceil lightmaps as last.

		// ENDIANNES NOTE:
		// Level files are written in Big-Endian for Amiga.
		// When run under Little-Endian (PC), the 16 an 32 bit values must be swapped.
		#if defined _WIN32
			int16 lightmaps_count = MA_swap_int16(LV_Level__File_Only.wall_lightmaps_count) + MA_swap_int16(LV_Level__File_Only.flat_lightmaps_count);
		#endif

		#if defined AMIGA
			int16 lightmaps_count = LV_Level__File_Only.wall_lightmaps_count + LV_Level__File_Only.flat_lightmaps_count;
		#endif

	LV_lightmaps__PTR = (u_int8*)malloc(lightmaps_count * IO_LIGHTMAP_BYTE_SIZE * sizeof(u_int8));

	if (LV_lightmaps__PTR == NULL)
		return 0;

	// Lightmaps should be saved with the same filename as level filename and put in 'lightmaps' directory.
	// Prepare string that hold the path + filename + ext
	char path_filename[IO_MAX_STRING_PATH_FILENAME];
	memset(path_filename, 0, IO_MAX_STRING_PATH_FILENAME);

	strcat(path_filename, IO_LIGHTMAPS_DIRECTORY);
	strcat(path_filename, LV_level_list[_level_number]);
	strcat(path_filename, IO_LIGHTMAP_FILE_EXTENSION);

	// Read in all lightmaps at once...
	int32 result = BM_Read_Lightmaps_RAW_All(path_filename, LV_lightmaps__PTR, lightmaps_count);

	if (result == 0)
		return 0;

	return 1;
}

int8 LV_Load_Level_Prepare_Cells(void)
{
	// Reset door cells variables.
	memset(LV_door_list, 0, sizeof(LV_door_list));
	memset(LV_door_group, 0, sizeof(LV_door_group));
	LV_door_count = 0;

	// Go thru each map cell.
	for (int32 id = 0; id < LV_MAP_CELLS_COUNT; id++)
	{
		LV_map[id].cell_type = LV_Level__File_Only.map[id].cell_type;
		LV_map[id].cell_state = LV_Level__File_Only.map[id].cell_state;
		LV_map[id].cell_group = LV_Level__File_Only.map[id].cell_group;
		LV_map[id].cell_timer = LV_Level__File_Only.map[id].cell_timer;
		LV_map[id].cell_action = LV_Level__File_Only.map[id].cell_action;

		// ENDIANNES NOTE
		// The float values was saved in AMIGA endiannes.
		// When loading on PC need to swap it.

		#if defined _WIN32
			LV_map[id].wall_vertex[0][0] =	MA_swap_float32(LV_Level__File_Only.map[id].wall_vertex[0][0]);
			LV_map[id].wall_vertex[0][1] =	MA_swap_float32(LV_Level__File_Only.map[id].wall_vertex[0][1]);
			LV_map[id].wall_vertex[1][0] =	MA_swap_float32(LV_Level__File_Only.map[id].wall_vertex[1][0]);
			LV_map[id].wall_vertex[1][1] =	MA_swap_float32(LV_Level__File_Only.map[id].wall_vertex[1][1]);

			LV_map[id].starting_height =	MA_swap_float32(LV_Level__File_Only.map[id].starting_height);
			LV_map[id].height =				MA_swap_float32(LV_Level__File_Only.map[id].height);
		#endif
		
		#if defined AMIGA
			LV_map[id].wall_vertex[0][0] =	LV_Level__File_Only.map[id].wall_vertex[0][0];
			LV_map[id].wall_vertex[0][1] =	LV_Level__File_Only.map[id].wall_vertex[0][1];
			LV_map[id].wall_vertex[1][0] =	LV_Level__File_Only.map[id].wall_vertex[1][0];
			LV_map[id].wall_vertex[1][1] =	LV_Level__File_Only.map[id].wall_vertex[1][1];

			LV_map[id].starting_height =	LV_Level__File_Only.map[id].starting_height;
			LV_map[id].height =				LV_Level__File_Only.map[id].height;
		#endif	

		LV_map[id].wall_texture_id[0] = LV_Level__File_Only.map[id].wall_texture_id[0];
		LV_map[id].wall_texture_id[1] = LV_Level__File_Only.map[id].wall_texture_id[1];
		LV_map[id].wall_texture_id[2] = LV_Level__File_Only.map[id].wall_texture_id[2];
		LV_map[id].wall_texture_id[3] = LV_Level__File_Only.map[id].wall_texture_id[3];

		LV_map[id].flat_texture_id[0] = LV_Level__File_Only.map[id].flat_texture_id[0];
		LV_map[id].flat_texture_id[1] = LV_Level__File_Only.map[id].flat_texture_id[1];

		// Count doors and fill up the door list.
		switch (LV_map[id].cell_type)
		{
			case LV_C_DOOR_THICK_HORIZONTAL:
			case LV_C_DOOR_THICK_VERTICAL:
			case LV_C_DOOR_THIN_OBLIQUE:

			case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
			case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
			case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
			case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
				
				// If door found - add this cell ID to door list and increment door count variable.
				LV_door_list[LV_door_count] = (int16)id;
				LV_door_count++;

				// Get the door cell group number. 
				// If the door cell group != 0 then update the LV_door_group structure.
				// The "0 group" means no group.
				int8 cell_group = LV_map[id].cell_group;

				if (cell_group != 0)
				{
					// Get current number of cells assigned to this cell group.
					int32 doors_in_group_index = LV_door_group[cell_group].count;

					// Use it as index to put current door cell ID on the list.
					LV_door_group[cell_group].id_list[doors_in_group_index] = (int16)id;

					// Incerement "count" value.
					LV_door_group[cell_group].count++;
				}
		}

		// Put correct lightmap pointer in every cell..
		if (LV_Level__File_Only.map[id].is_lightmapped)
		{
			// For wall lightmaps...
		
				// ENDIANNES NOTE
				// The 'wall_lightmap_id' is 16 bit and saved for AMIGA.
				// When loading on PC swap endiannes.

				#if defined _WIN32
					LV_map[id].wall_lightmap_id[0] = MA_swap_int16(LV_Level__File_Only.map[id].wall_lightmap_id[0]);
					LV_map[id].wall_lightmap_id[1] = MA_swap_int16(LV_Level__File_Only.map[id].wall_lightmap_id[1]);
					LV_map[id].wall_lightmap_id[2] = MA_swap_int16(LV_Level__File_Only.map[id].wall_lightmap_id[2]);
					LV_map[id].wall_lightmap_id[3] = MA_swap_int16(LV_Level__File_Only.map[id].wall_lightmap_id[3]);
				#endif

				#if defined AMIGA
					LV_map[id].wall_lightmap_id[0] = LV_Level__File_Only.map[id].wall_lightmap_id[0];
					LV_map[id].wall_lightmap_id[1] = LV_Level__File_Only.map[id].wall_lightmap_id[1];
					LV_map[id].wall_lightmap_id[2] = LV_Level__File_Only.map[id].wall_lightmap_id[2];
					LV_map[id].wall_lightmap_id[3] = LV_Level__File_Only.map[id].wall_lightmap_id[3];
				#endif					

			// For flat lightmaps...

				// ENDIANNES NOTE
				// The 'wall_lightmap_id' is 16 bit and saved for AMIGA.
				// When loading on PC swap endiannes.

				#if defined _WIN32
					LV_map[id].flat_lightmap_id[0] = MA_swap_int16(LV_Level__File_Only.map[id].flat_lightmap_id[0]);
					LV_map[id].flat_lightmap_id[1] = MA_swap_int16(LV_Level__File_Only.map[id].flat_lightmap_id[1]);
				#endif

				#if defined AMIGA
					LV_map[id].flat_lightmap_id[0] = LV_Level__File_Only.map[id].flat_lightmap_id[0];
					LV_map[id].flat_lightmap_id[1] = LV_Level__File_Only.map[id].flat_lightmap_id[1];
				#endif
		}
	}

	return 1;
}