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
	free(LV_lightmaps);
	free(LV_textures);
	free(LV_textures_intensity);
	free(LV_fv_list);
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
	memcpy(level_filename, IO_LEVELS_DIRECTORY, sizeof(IO_LEVELS_DIRECTORY));
	strcat(level_filename, LV_level_list[_level_number]);

	// Add extension to level filename.
	strcat(level_filename, IO_LEVEL_FILE_EXTENSION);

	// Try open level file and fill up the temporary LV_Level__File_Only structure.
	FILE* file_in;
	file_in = fopen(level_filename, "rb");

	if (file_in == NULL)
		return 0;

	// Read in the file header (static data).
	fread(&LV_Level__File_Only, sizeof(LV_Level__File_Only), 1, file_in);

	// Get size if flat visibility list and alloc memory for LV_fv_list.
	int32 list_size_bytes = 0;

	#if defined _WIN32
		list_size_bytes = MA_swap_int32(LV_Level__File_Only.fv_list_size);
	#endif

	#if defined AMIGA
		list_size_bytes = LV_Level__File_Only.fv_list_size;
	#endif

	LV_fv_list = (int16*)malloc(list_size_bytes);
	if (LV_fv_list == NULL) return 0;

	// Read in dynamic data - put all fv list from file into LV_fv_list buffer.
	fread(LV_fv_list, list_size_bytes, 1, file_in);

	// Close the file.
	fclose(file_in);

	// FV - for PC/WIN swap bits for just read id cells from list..
	#if defined _WIN32
		for (int cell = 0; cell < (list_size_bytes / 2); cell++)
			LV_fv_list[cell] = MA_swap_int16(LV_fv_list[cell]);
	#endif

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
	int16 textures_count = LV_Level__File_Only.wall_textures_count + LV_Level__File_Only.flat_textures_count;

	// Try allocate enought memory for intensity color map for all textures:
	LV_textures_intensity = (u_int32*)malloc(textures_count * IO_TEXTURE_COLOR_DATA_BYTE_SIZE * sizeof(u_int32));

	if (LV_textures_intensity == NULL)
		return 0;

	// Try allocate enought memory for textures image data for all textures - including their mip-maps:
	LV_textures = (u_int8*)malloc(textures_count * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE * sizeof(u_int8));

	if (LV_textures == NULL)
		return 0;

	// Prepare string that hold the path + filename + ext
	char path_filename[IO_MAX_STRING_PATH_FILENAME];

	// Clone pointers as local to use them for read in and offsets..
	u_int32* tmp__LV_textures_intensity = LV_textures_intensity;
	u_int8* tmp__LV_textures = LV_textures;

	// First read every WALL texture...
	for (int32 i = 0; i < LV_Level__File_Only.wall_textures_count; i++)
	{
		memset(path_filename, 0, IO_MAX_STRING_PATH_FILENAME);

		strcat(path_filename, IO_WALL_TEXTURES_DIRECTORY);
		strcat(path_filename, LV_Level__File_Only.wall_texture_filename[i]);

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

		int32 result = BM_Read_Texture_RAW(path_filename, tmp__LV_textures_intensity, tmp__LV_textures, &IO_prefs);

		tmp__LV_textures_intensity += IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
		tmp__LV_textures += IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

		if (result == 0)
			return 0;
	}

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


	LV_lightmaps = (u_int8*)malloc(lightmaps_count * IO_LIGHTMAP_BYTE_SIZE * sizeof(u_int8));

	if (LV_lightmaps == NULL)
		return 0;

	// Lightmaps should be saved with the same filename as level filename and put in 'lightmaps' directory.
	// Prepare string that hold the path + filename + ext
	char path_filename[IO_MAX_STRING_PATH_FILENAME];
	memset(path_filename, 0, IO_MAX_STRING_PATH_FILENAME);

	strcat(path_filename, IO_LIGHTMAPS_DIRECTORY);
	strcat(path_filename, LV_level_list[_level_number]);
	strcat(path_filename, IO_LIGHTMAP_FILE_EXTENSION);

	// Read in all lightmaps at once...
	int32 result = BM_Read_Lightmaps_RAW_All(path_filename, LV_lightmaps, lightmaps_count);

	if (result == 0)
		return 0;

	return 1;
}

int8 LV_Load_Level_Prepare_Cells(void)
{
	// Offset helper for creating pointers list for flat visibility list.
	int32 fv_offset = 0;

	#if defined _WIN32
		fv_offset = 0;
	#endif

	#if defined AMIGA
		fv_offset = 1;
	#endif

	for (int32 i = 0; i < LV_MAP_CELLS_COUNT; i++)
	{
		// Copy cell data.
		LV_map[i].cell_shade = LV_Level__File_Only.map[i].cell_shade;
		LV_map[i].is_cell_solid = LV_Level__File_Only.map[i].is_cell_solid;
		LV_map[i].cell_type = LV_Level__File_Only.map[i].cell_type;

		if (LV_Level__File_Only.map[i].is_cell_solid)
		{
			// In every wall put already a pointer to correct texture - instead of its id...
			switch (LV_Level__File_Only.map[i].cell_type)
			{
				case LV_C_WALL_STANDARD:
					LV_map[i].wall_texture_intensity__PTR[0] = LV_textures_intensity + (LV_Level__File_Only.map[i].wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
					LV_map[i].wall_texture__PTR[0] = LV_textures + (LV_Level__File_Only.map[i].wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
					break;

				case LV_C_WALL_FOURSIDE:
					LV_map[i].wall_texture_intensity__PTR[0] = LV_textures_intensity + (LV_Level__File_Only.map[i].wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
					LV_map[i].wall_texture__PTR[0] = LV_textures + (LV_Level__File_Only.map[i].wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);

					LV_map[i].wall_texture_intensity__PTR[1] = LV_textures_intensity + (LV_Level__File_Only.map[i].wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
					LV_map[i].wall_texture__PTR[1] = LV_textures + (LV_Level__File_Only.map[i].wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);

					LV_map[i].wall_texture_intensity__PTR[2] = LV_textures_intensity + (LV_Level__File_Only.map[i].wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
					LV_map[i].wall_texture__PTR[2] = LV_textures + (LV_Level__File_Only.map[i].wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);

					LV_map[i].wall_texture_intensity__PTR[3] = LV_textures_intensity + (LV_Level__File_Only.map[i].wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
					LV_map[i].wall_texture__PTR[3] = LV_textures + (LV_Level__File_Only.map[i].wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
					break;
			}
		}

		// Put correct texture pointer in every flat. Flat textures are stored right after wall textures - so make additinal offset.
		int32 wall_color_data_offset = LV_Level__File_Only.wall_textures_count * IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
		int32 wall_image_data_offset = LV_Level__File_Only.wall_textures_count * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

		LV_map[i].flat_texture_intensity__PTR[0] = LV_textures_intensity + wall_color_data_offset + (LV_Level__File_Only.map[i].flat_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
		LV_map[i].flat_texture__PTR[0] = LV_textures + wall_image_data_offset + (LV_Level__File_Only.map[i].flat_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);

		LV_map[i].flat_texture_intensity__PTR[1] = LV_textures_intensity + wall_color_data_offset + (LV_Level__File_Only.map[i].flat_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
		LV_map[i].flat_texture__PTR[1] = LV_textures + wall_image_data_offset + (LV_Level__File_Only.map[i].flat_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);

		// Put correct lightmap pointer in every cell..
		if (LV_Level__File_Only.map[i].is_lightmapped)
		{
			// For wall lightmaps...
			if (LV_Level__File_Only.map[i].is_cell_solid)
			{			
				// ENDIANNES NOTE
				// The 'wall_lightmap_id' is 16 bit and saved for AMIGA.
				// When loading on PC swap endiannes.

				#if defined _WIN32
						LV_map[i].wall_lightmap__PTR[0] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].wall_lightmap_id[0]) * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[1] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].wall_lightmap_id[1]) * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[2] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].wall_lightmap_id[2]) * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[3] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].wall_lightmap_id[3]) * IO_LIGHTMAP_BYTE_SIZE);
				#endif

				#if defined AMIGA
						LV_map[i].wall_lightmap__PTR[0] = LV_lightmaps + (LV_Level__File_Only.map[i].wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[1] = LV_lightmaps + (LV_Level__File_Only.map[i].wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[2] = LV_lightmaps + (LV_Level__File_Only.map[i].wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);
						LV_map[i].wall_lightmap__PTR[3] = LV_lightmaps + (LV_Level__File_Only.map[i].wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);
				#endif					
			}


			// For flat lightmaps...

			// ENDIANNES NOTE
			// The 'wall_lightmap_id' is 16 bit and saved for AMIGA.
			// When loading on PC swap endiannes.

			#if defined _WIN32
				LV_map[i].flat_lightmap__PTR[0] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].flat_lightmap_id[0]) * IO_LIGHTMAP_BYTE_SIZE);
				LV_map[i].flat_lightmap__PTR[1] = LV_lightmaps + ( MA_swap_int16(LV_Level__File_Only.map[i].flat_lightmap_id[1]) * IO_LIGHTMAP_BYTE_SIZE);
			#endif

			#if defined AMIGA
				LV_map[i].flat_lightmap__PTR[0] = LV_lightmaps + (LV_Level__File_Only.map[i].flat_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);
				LV_map[i].flat_lightmap__PTR[1] = LV_lightmaps + (LV_Level__File_Only.map[i].flat_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);
			#endif
		}

		// Flat visibility data - lists of potential visible cells for each cell marked as fv_marked.
		// Lets create a list of pointers for each slice for each cell.
		if (LV_Level__File_Only.map[i].is_fv_marked)
		{
			for (int32 slice = 0; slice < 8; slice++)
			{
				#if defined _WIN32
					LV_map[i].pvc_list_size[slice] = MA_swap_int16(LV_Level__File_Only.map[i].fv_list[slice]);
				#endif

				#if defined AMIGA
					LV_map[i].pvc_list_size[slice] = LV_Level__File_Only.map[i].fv_list[slice];
				#endif

				LV_map[i].pvc_list__PTR[slice] = LV_fv_list + fv_offset;

				// Update global list offset.
				fv_offset += LV_map[i].pvc_list_size[slice];
			}
		}
	}

	return 1;
}