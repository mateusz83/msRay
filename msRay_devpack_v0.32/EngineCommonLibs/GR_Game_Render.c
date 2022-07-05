#include "GR_Game_Render.h"
#include "PL_Player.h"
#include "LV_Level.h"
#include "MA_Math.h"

#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------
// --- GAME RENDER - PRIVATE globals, constants, variables ---
// -----------------------------------------------------------
int16		GR_render_width, GR_render_height;
int16		GR_render_height__sub1, GR_render_height__div2;
float32		GR_render_width__1div;

int16		GR_panel_height;

float32		GR_pp_dx, GR_pp_dy;
float32		GR_pp_nsize_x__div2, GR_pp_nsize_y__div2;
float32*	GR_pp_coord_x__LUT;

// Structures for precalculated texture y coords for wall stripes.

// Higher factor increases the distance we can move to the wall.
#define		GR_WALL_STRIPE_FACTOR			1.0f

int32		GR_max_wall_stripe_lineheight;
int32		GR_max_wall_stripe_lineheight__sub1;

typedef struct
{
	u_int8* wall_stripe;
	u_int32 size_offset;
	u_int16 tex_size;	
	u_int8	tex_size_sub1;
	u_int8	tex_size_bitshift;
}_IO_BYTE_ALIGN_ sGR_Wall_Stripe;

sGR_Wall_Stripe*	GR_wall_stripe;
u_int8**			GR_wall_stripe_lightmap;

// Structures for precalculated distance intensity tables.
#define GR_DISTANCE_INTENSITY_LENGTH             128   
#define GR_DISTANCE_INTENSITY_MIN                40.0f 
#define GR_DISTANCE_INTENSITY_SCALE              20.0f 

int32**		GR_distance_shading_walls__LUT;
int32**		GR_distance_shading_flats__LUT;

// Structures for calculating and redering flats: floors and ceilings.
#define GR_MAX_PILLARS      255

typedef struct
{
	int32*	flat_texture_intensity__PTR[2];
	int8*	flat_texture__PTR[2];
	int8*	flat_lightmap__PTR[2];

	int16*	floor_left;
	int16*	floor_right;

	int16*	ceil_left;
	int16*	ceil_right;

	int16	floor_top, floor_bottom;
	int16	floor_prev_top, floor_prev_bottom;
	int16	floor_last_top, floor_last_bottom, floor_last_right;

	int16	ceil_top, ceil_bottom;
	int16	ceil_prev_top, ceil_prev_bottom;
	int16	ceil_last_top, ceil_last_bottom, ceil_last_right;

}_IO_BYTE_ALIGN_ sGR_Pillar;

sGR_Pillar* GR_pillars;


typedef struct
{
	sGR_Pillar* pillar_address;
	int8 found;

} _IO_BYTE_ALIGN_ sGR_Pillar_Map;

sGR_Pillar_Map GR_pillars_map[LV_MAP_CELLS_COUNT];


int32 GR_pillars_used;
int32 GR_pillars_reset_size;

// ----------------------------------------------------
// --- GAME RENDER - PRIVATE functions declarations ---
// ----------------------------------------------------
int32	GR_Init_Projection_Plane(void);
void	GR_Cleanup_Projection_Plane(void);

int32	GR_Init_Wall_Stripe_LUT(void);
void	GR_Cleanup_Wall_Stripe_LUT(void);

int32	GR_Init_Wall_Stripe_Lightmap_LUT(void);
void	GR_Cleanup_Wall_Stripe_Lightmap_LUT(void);

int32	GR_Init_Pillars_Structure(void);
void	GR_Cleanup_Pillars_Structure(void);

int32	GR_Init_Distance_Shading_LUT(void);
void	GR_Cleanup_Distance_Shading_LUT(void);

void	GR_Update_Input(void);
void	GR_Reset_Pillars_Structure(void);
void	GR_Raycast_Walls(void);
void	GR_Raycast_Flats(void);

// --------------------------------------------------
// --- GAME RENDER - PUBLIC functions definitions ---
// --------------------------------------------------

int32 GR_Init()
{
	int32 memory_allocated = 0;
	int32 mem_tmp = 0;
	
	// Init basic parameters and projection plane variables.
	mem_tmp = GR_Init_Projection_Plane();
	if (mem_tmp == 0) return 0;
	memory_allocated += mem_tmp;

	// Init precalculated LUT containing tex y-coords for textures. Its takes care of all mipmaps from 256 to 4.
	mem_tmp = GR_Init_Wall_Stripe_LUT();
	if (mem_tmp == 0) return 0;
	memory_allocated += mem_tmp;

	// Init precalculated LUT containing tex y-coords for 32x32 px lightmaps.
	mem_tmp = GR_Init_Wall_Stripe_Lightmap_LUT();
	if (mem_tmp == 0) return 0;
	memory_allocated += mem_tmp;

	// Init Pillars structure...
	mem_tmp = GR_Init_Pillars_Structure();
	if (mem_tmp == 0) return 0;
	memory_allocated += mem_tmp;

	// Init precalculated LUT that contains distance shading values.
	mem_tmp = GR_Init_Distance_Shading_LUT();
	if (mem_tmp == 0) return 0;
	memory_allocated += mem_tmp;

	// Init Player settings.
	PL_Player_Init();

	return memory_allocated;
}

void GR_Reset(void)
{
	// Reset the rest of player startup settings. 
	// The starting x,y position and angle is set during level loading.
	PL_Player_Reset();
}

void GR_Run()
{
	// Get keys and mouse parameters and update.
	GR_Update_Input();

	//memset(IO_prefs.output_buffer_32, 0x11111111, IO_prefs.screen_frame_buffer_size);

	// Reset pillars structure...
	GR_Reset_Pillars_Structure();

	// Raycast Walls...
	GR_Raycast_Walls();

	// Raycast Flats...
	GR_Raycast_Flats();
}

void GR_Cleanup(void)
{
	GR_Cleanup_Distance_Shading_LUT();
	GR_Cleanup_Pillars_Structure();
	GR_Cleanup_Wall_Stripe_Lightmap_LUT();
	GR_Cleanup_Wall_Stripe_LUT();
	GR_Cleanup_Projection_Plane();
}

// ---------------------------------------------------
// --- GAME RENDER - PRIVATE functions definitions ---
// ---------------------------------------------------

int32	GR_Init_Projection_Plane(void)
{
	// Calculate info panel height. The hardcoded minimal info panel size is: 320 x 40 px, so:
	GR_panel_height = (IO_prefs.screen_width / 320) * 0;

	// Get and calculate rendering size.
	GR_render_width = IO_prefs.screen_width;
	GR_render_height = IO_prefs.screen_height - GR_panel_height;

	// Prepare additional variables.
	GR_render_width__1div = 1.0f / (float32)GR_render_width;
	GR_render_height__sub1 = GR_render_height - 1;
	GR_render_height__div2 = GR_render_height / 2;


	// Init projection plane attributes, direction and its normalized size.
	// GR_pp_dx determines the FOV - for 1.0 is about 66* = 2 * atan(GR_pp_nsize_y__div2 / GR_pp_dx)
	float32 render_aspect = (float32)GR_render_width / (float32)GR_render_height;

	GR_pp_dx = 0.0f;
	GR_pp_dy = -1.0f;
	GR_pp_nsize_x__div2 = render_aspect / 2.0f;
	GR_pp_nsize_y__div2 = 0.0f;


	// Setup precalculated pp coord x LUT (used in walls raycasting).
	// pp_coord_x is a ray_x coordinate in camera space.

		// -- MALLOC --
		int32 mem_size = GR_render_width * sizeof(float32);
		GR_pp_coord_x__LUT = (float32*)malloc(mem_size);
		if (GR_pp_coord_x__LUT == NULL) return 0;

		for (int32 ray_x = 0; ray_x < GR_render_width; ray_x++)
			GR_pp_coord_x__LUT[ray_x] = 2.0f * (float32)ray_x * GR_render_width__1div - 1.0f;


	return mem_size;
}
void	GR_Cleanup_Projection_Plane(void)
{
	free(GR_pp_coord_x__LUT);
}

int32	GR_Init_Wall_Stripe_LUT(void)
{
	// Setup precalculated wall stripes structure for performance.
	// It will contain texture y coords depend on wall slice line height.

	int32 memory_allocated = 0;

	// Calculate 'GR_max_wall_stripe_lineheight' - increases when resolutinon is higher.
	// Lets assume GR_max_wall_stripe_lineheight = 2048 for GR_render_width = 320. Use 2.2f as factor.
	GR_max_wall_stripe_lineheight = (int32)(1200 + GR_WALL_STRIPE_FACTOR * (GR_render_width - 320));

	GR_max_wall_stripe_lineheight__sub1 = GR_max_wall_stripe_lineheight - 1;

	// -- MALLOC --
	int32 mem_size_01 = GR_max_wall_stripe_lineheight * sizeof(sGR_Wall_Stripe);
	GR_wall_stripe = (sGR_Wall_Stripe*)malloc(mem_size_01);
	if (GR_wall_stripe == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size_01;

	// -- MALLOC --
	GR_wall_stripe[0].wall_stripe = (u_int8*)malloc(sizeof(u_int8));
	if (GR_wall_stripe[0].wall_stripe == NULL) return 0;
	
	// for line height == 0...
	GR_wall_stripe[0].wall_stripe[0] = 0;
	GR_wall_stripe[0].size_offset = 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8;
	GR_wall_stripe[0].tex_size = 4;
	GR_wall_stripe[0].tex_size_sub1 = 3;
	GR_wall_stripe[0].tex_size_bitshift = 2;

	// counting memory...
	memory_allocated += sizeof(u_int8);

	// For 4x4 px mipmap textures. 
	// Line height from 1 to 4 pixels.
	for (int32 i = 1; i < 5; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8;
		GR_wall_stripe[i].tex_size = 4;
		GR_wall_stripe[i].tex_size_sub1 = 3;
		GR_wall_stripe[i].tex_size_bitshift = 2;

		float64 step = 4.0/ (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 8x8 px mipmap textures. 
	// Line height from 5 to 8 pixels.
	for (int32 i = 5; i < 9; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16;
		GR_wall_stripe[i].tex_size = 8;
		GR_wall_stripe[i].tex_size_sub1 = 7;
		GR_wall_stripe[i].tex_size_bitshift = 3;

		float64 step = 8.0/ (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 16x16 px mipmap textures. 
	// Line height from 9 to 16 pixels.
	for (int32 i = 9; i < 17; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32;
		GR_wall_stripe[i].tex_size = 16;
		GR_wall_stripe[i].tex_size_sub1 = 15;
		GR_wall_stripe[i].tex_size_bitshift = 4;

		float64 step = 16.0/ (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 32x32 px mipmap textures. 
	// Line height from 17 to 32 pixels.
	for (int32 i = 17; i < 33; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256 + 128 * 128 + 64 * 64;
		GR_wall_stripe[i].tex_size = 32;
		GR_wall_stripe[i].tex_size_sub1 = 31;
		GR_wall_stripe[i].tex_size_bitshift = 5;

		float64 step = 32.0 / (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 64x64 px mipmap textures. 
	// Line height from 33 to 64 pixels.
	for (int32 i = 33; i < 65; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256 + 128 * 128;
		GR_wall_stripe[i].tex_size = 64;
		GR_wall_stripe[i].tex_size_sub1 = 63;
		GR_wall_stripe[i].tex_size_bitshift = 6;

		float64 step = 64.0 / (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 128x128 px mipmap textures. 
	// Line height from 65 to 128 pixels.
	for (int32 i = 65; i < 129; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 256 * 256;
		GR_wall_stripe[i].tex_size = 128;
		GR_wall_stripe[i].tex_size_sub1 = 127;
		GR_wall_stripe[i].tex_size_bitshift = 7;

		float64 step = 128.0 / (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 256x256 px mipmap textures. 
	// Line height from >= 129 pixels.
	for (int32 i = 129; i < GR_max_wall_stripe_lineheight; i++)
	{
		// -- MALLOC --
		int32 mem = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe = (u_int8*)malloc(mem);
		if (GR_wall_stripe[i].wall_stripe == NULL) return 0;

		memory_allocated += mem;

		GR_wall_stripe[i].size_offset = 0;
		GR_wall_stripe[i].tex_size = 256;
		GR_wall_stripe[i].tex_size_sub1 = 255;
		GR_wall_stripe[i].tex_size_bitshift = 8;

		float64 step = 256.0 / (float64)i;
		float64 tex_y_pos = 0.0;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)ceil(tex_y_pos);
			tex_y_pos += step;
		}
	}


	return memory_allocated;
}
void	GR_Cleanup_Wall_Stripe_LUT(void)
{
	for (int32 i = 0; i < GR_max_wall_stripe_lineheight; i++)	free(GR_wall_stripe[i].wall_stripe);
	free(GR_wall_stripe);
}

int32	GR_Init_Wall_Stripe_Lightmap_LUT(void)
{
	// Setup precalculated wall stripes structure for performance for lightmaps 32x32.
	// It will contain texture y coords depend on wall slice line height.

	int32 memory_allocated = 0;

	// -- MALLOC --
	int32 mem_size_01 = GR_max_wall_stripe_lineheight * sizeof(u_int8*);
	GR_wall_stripe_lightmap = (u_int8**)malloc(mem_size_01);
	if (GR_wall_stripe_lightmap == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size_01;

	// -- MALLOC NR 03-b --
	GR_wall_stripe_lightmap[0] = (u_int8*)malloc(sizeof(u_int8));
	if (GR_wall_stripe_lightmap[0] == NULL) return 0;
	GR_wall_stripe_lightmap[0][0] = 0;

	// counting memory...
	memory_allocated += sizeof(u_int8);

	for (int32 i = 1; i < GR_max_wall_stripe_lineheight; i++)
	{
		// -- MALLOC NR 03-c --
		int32 mem_size_02 = sizeof(u_int8) * i;
		GR_wall_stripe_lightmap[i] = (u_int8*)malloc(mem_size_02);
		if (GR_wall_stripe_lightmap[i] == NULL) return 0;

		memory_allocated += mem_size_02;

		float32 step = 32.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe_lightmap[i][j] = (u_int8)(tex_y_pos);
			tex_y_pos += step;
		}
	}

	return memory_allocated;
}
void	GR_Cleanup_Wall_Stripe_Lightmap_LUT(void)
{
	for (int32 i = 0; i < GR_max_wall_stripe_lineheight; i++)	free(GR_wall_stripe_lightmap[i]);

	free(GR_wall_stripe_lightmap);
}

int32	GR_Init_Pillars_Structure(void)
{
	int32 mem_allocated = 0;

	GR_pillars_reset_size = GR_render_height * sizeof(int16);

	int32 mem_size = GR_MAX_PILLARS * sizeof(sGR_Pillar);
	GR_pillars = (sGR_Pillar*)malloc(mem_size);
	if (GR_pillars == NULL) return 0;

	// Counting memory...
	mem_allocated += mem_size;

	for (int32 i = 0; i < GR_MAX_PILLARS; i++)
	{
		GR_pillars[i].ceil_left = (int16*)malloc(GR_pillars_reset_size);
		mem_allocated += GR_pillars_reset_size;
		if (GR_pillars[i].ceil_left == NULL) return 0;

		GR_pillars[i].ceil_right = (int16*)malloc(GR_pillars_reset_size);
		mem_allocated += GR_pillars_reset_size;
		if (GR_pillars[i].ceil_right == NULL) return 0;

		GR_pillars[i].floor_left = (int16*)malloc(GR_pillars_reset_size);
		mem_allocated += GR_pillars_reset_size;
		if (GR_pillars[i].floor_left == NULL) return 0;

		GR_pillars[i].floor_right = (int16*)malloc(GR_pillars_reset_size);
		mem_allocated += GR_pillars_reset_size;
		if (GR_pillars[i].floor_right == NULL) return 0;
	}

	return mem_allocated;
}
void	GR_Cleanup_Pillars_Structure(void)
{
	for (int32 i = 0; i < GR_MAX_PILLARS; i++)
	{
		free(GR_pillars[i].ceil_left);
		free(GR_pillars[i].ceil_right);
		free(GR_pillars[i].floor_right);
		free(GR_pillars[i].floor_left);
	}

	free(GR_pillars);
}

int32	GR_Init_Distance_Shading_LUT(void)
{
	// Setup distance shading intensity LUT.

	int32 memory_allocated = 0;

	// -- MALLOC WALLS --
	int32 mem_size = GR_DISTANCE_INTENSITY_LENGTH * sizeof(int32*);
	GR_distance_shading_walls__LUT = (int32**)malloc(mem_size);
	if (GR_distance_shading_walls__LUT == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;


	// -- MALLOC FLATS --
	mem_size = GR_DISTANCE_INTENSITY_LENGTH * sizeof(int32*);
	GR_distance_shading_flats__LUT = (int32**)malloc(mem_size);
	if (GR_distance_shading_flats__LUT == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;


	float32 step = (1.0f - (GR_DISTANCE_INTENSITY_MIN / IO_TEXTURE_MAX_SHADES)) / GR_DISTANCE_INTENSITY_LENGTH;
	float32 factor = 1.0f;

	for (int32 i = 0; i < GR_DISTANCE_INTENSITY_LENGTH; i++)
	{
		// -- MALLOC WALLS --
		mem_size = IO_TEXTURE_MAX_SHADES * sizeof(int32);
		GR_distance_shading_walls__LUT[i] = (int32*)malloc(mem_size);
		if (GR_distance_shading_walls__LUT[i] == NULL) return 0;

		// counting memory...
		memory_allocated += mem_size;


		// -- MALLOC FLATS --
		mem_size = IO_TEXTURE_MAX_SHADES * sizeof(int32);
		GR_distance_shading_flats__LUT[i] = (int32*)malloc(mem_size);
		if (GR_distance_shading_flats__LUT[i] == NULL) return 0;

		// counting memory...
		memory_allocated += mem_size;

		for (int32 j = 0; j < IO_TEXTURE_MAX_SHADES; j++)
		{
			int32 tmp = (int32)(j * factor) * 128;

			GR_distance_shading_walls__LUT[i][j] = tmp;
			GR_distance_shading_flats__LUT[i][j] = tmp;
		}

		factor -= step;
	}

	return memory_allocated;
}
void	GR_Cleanup_Distance_Shading_LUT(void)
{
	for (int32 i = 0; i < GR_DISTANCE_INTENSITY_LENGTH; i++)	free(GR_distance_shading_walls__LUT[i]);
	free(GR_distance_shading_walls__LUT);
}

void	GR_Update_Input(void)
{
	// ESC key.
		if (IO_input.keys[IO_KEYCODE_ESC])
		{
			IO_prefs.engine_state = EN_STATE_GAMEPLAY_CLEANUP;
		}

	// Update mouse movement X - rotate direction of player and the projection plane.
		#if defined  WIN32
			PL_player.angle = (float32)IO_input.mouse_dx * IO_prefs.delta_time;
		#elif defined AMIGA
			PL_player.angle = ((float32)IO_input.mouse_dx * IO_prefs.delta_time) * 0.2f;
		#endif

		float32 tmp_sinf = sinf(PL_player.angle);
		float32 tmp_cosf = cosf(PL_player.angle);

		float32 old_pp_dx = GR_pp_dx;
		GR_pp_dx = GR_pp_dx * tmp_cosf - GR_pp_dy * tmp_sinf;
		GR_pp_dy = old_pp_dx * tmp_sinf + GR_pp_dy * tmp_cosf;

		float32 old_pp_nsize_x = GR_pp_nsize_x__div2;
		GR_pp_nsize_x__div2 = GR_pp_nsize_x__div2 * tmp_cosf - GR_pp_nsize_y__div2 * tmp_sinf;
		GR_pp_nsize_y__div2 = old_pp_nsize_x * tmp_sinf + GR_pp_nsize_y__div2 * tmp_cosf;

	// Update mouse movement Y - pitch.
		PL_player.pitch -= (float32)IO_input.mouse_dy * PL_player.pitch_speed * IO_prefs.delta_time;

		if (PL_player.pitch > PL_player.pitch_max) PL_player.pitch = PL_player.pitch_max;
		if (PL_player.pitch < -PL_player.pitch_max) PL_player.pitch = -PL_player.pitch_max;

	// Calculate player movement speed.
		float32 speed_dt = PL_MOVE_SPEED * IO_prefs.delta_time;

		u_int8 player_moved = 0;

	// Moving foreward and backward are self-excluding so we can type as..
		if (IO_input.keys[IO_KEYCODE_W])
		{
			player_moved = 1;

			PL_player.x += GR_pp_dx * speed_dt;
			PL_player.y += GR_pp_dy * speed_dt;
		}
		else if (IO_input.keys[IO_KEYCODE_S])
		{
			player_moved = 1;

			PL_player.x -= GR_pp_dx * speed_dt;
			PL_player.y -= GR_pp_dy * speed_dt;
		}

		// moving left and right are self-excluding so we can type as..
		if (IO_input.keys[IO_KEYCODE_A])
		{
			player_moved = 1;

			PL_player.x += GR_pp_dy * speed_dt;
			PL_player.y += -GR_pp_dx * speed_dt;
		}
		else if (IO_input.keys[IO_KEYCODE_D])
		{
			player_moved = 1;

			PL_player.x += -GR_pp_dy * speed_dt;
			PL_player.y += GR_pp_dx * speed_dt;
		}

		if (player_moved)
		{
			PL_player.z_accumulation += 500.0f * IO_prefs.delta_time;

			if (PL_player.z_accumulation >= PL_Z_LUT_SIZE__SUB1)
				PL_player.z_accumulation = 0.0f;

			int32 index = (int32)PL_player.z_accumulation;

			PL_player.z = PL_player.z__LUT[index];
		}		
}

void	GR_Reset_Pillars_Structure(void)
{
	// Manual loop-unrolling to increase performance. Instead of LV_MAP_CELLS_COUNT = 4096 branches, we get 32.
	int32 i = 0;

	do
	{
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  // 32


		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  // 64


		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  // 96


		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;

		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;
		GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  GR_pillars_map[i++].found = 0;  // 128   

	} while (i < LV_MAP_CELLS_COUNT);


	int32 n = (GR_pillars_used + 7) / 8;
	int32 p = 0;

	switch (GR_pillars_used % 8)
	{
		case 0:

			do
			{
				memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
				memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
				memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
				memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

				GR_pillars[p].floor_last_top = 0;
				GR_pillars[p].floor_last_bottom = 0;
				GR_pillars[p].floor_last_right = 0;

				GR_pillars[p].ceil_last_top = 0;
				GR_pillars[p].ceil_last_bottom = 0;
				GR_pillars[p].ceil_last_right = 0;

				p++;

		case 7:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 6:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 5:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 4:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 3:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 2:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

		case 1:
			memset(GR_pillars[p].ceil_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].ceil_right, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_left, -1, GR_pillars_reset_size);
			memset(GR_pillars[p].floor_right, -1, GR_pillars_reset_size);

			GR_pillars[p].floor_last_top = 0;
			GR_pillars[p].floor_last_bottom = 0;
			GR_pillars[p].floor_last_right = 0;

			GR_pillars[p].ceil_last_top = 0;
			GR_pillars[p].ceil_last_bottom = 0;
			GR_pillars[p].ceil_last_right = 0;

			p++;

			} while (--n > 0);

	}

	GR_pillars_used = 0;
}

void	GR_Raycast_Walls(void)
{
	// Lets prepare some variables before entering main loop.
	int8 player_x_integer = (int8)PL_player.x;
	int8 player_y_integer = (int8)PL_player.y;

	float32 player_x_fraction = PL_player.x - player_x_integer;
	float32 minus__player_x_fraction__add__one = -player_x_fraction + 1.0f;

	float32 player_y_fraction = PL_player.y - player_y_integer;
	float32 minus__player_y_fraction__add__one = -player_y_fraction + 1.0f;

	float32 side_dist_x, side_dist_y;
	int8 step_x = 0, step_y = 0;

	// For Pillars structure (flats).
	int16 curr_map_id = player_x_integer + (player_y_integer << LV_MAP_LENGTH_BITSHIFT);

	float32* pp_coord_x__LUT__PTR = GR_pp_coord_x__LUT;

	int16 ray_x_counter = GR_render_width;
	int16 ray_x = -1;

	while (ray_x_counter)
	{
		ray_x_counter--;
		ray_x++;

		float32 pp_coord_x = *(pp_coord_x__LUT__PTR++);

		float32 ray_dir_x = GR_pp_dx + GR_pp_nsize_x__div2 * pp_coord_x;
		float32 ray_dir_y = GR_pp_dy + GR_pp_nsize_y__div2 * pp_coord_x;

		float32 ray_dir_x__1div = 1.0f / ray_dir_x;
		float32 ray_dir_y__1div = 1.0f / ray_dir_y;

		float32 delta_dist_x = fabsf(ray_dir_x__1div);
		float32 delta_dist_y = fabsf(ray_dir_y__1div);

		int8 map_x = player_x_integer;
		int8 map_y = player_y_integer;

		// Is solid wall hit...
		int8 is_solid_hit = 0;

		// What side was hit: TOP-BOTTOM (==1) or LEFT-RIGHT (==0)...
		int8 what_side_hit = 0;

		// Prepare variables for DDA.
		if (ray_dir_x < 0.0f)
		{
			step_x = -1;
			side_dist_x = player_x_fraction * delta_dist_x;
		}
		else
		{
			step_x = 1;
			side_dist_x = minus__player_x_fraction__add__one * delta_dist_x;
		}

		if (ray_dir_y < 0.0f)
		{
			step_y = -1;
			side_dist_y = player_y_fraction * delta_dist_y;
		}
		else
		{
			step_y = 1;
			side_dist_y = minus__player_y_fraction__add__one * delta_dist_y;
		}

		// For Pillars structure(flats).
		int8 hit_number = 0;
		int16 prew_draw_end = GR_render_height__sub1;
		int16 prew_draw_start = 0;
		int32 prew_map_id = curr_map_id;

		// Start DDA.
		while (is_solid_hit == 0)
		{
			float32 wall_distance = 0.0f;

			// Jump to next map square: in x-direction OR in y-direction...
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				map_x += step_x;
				what_side_hit = 0;

				// if (what_side_hit == 0) ...               
					// 'step_tmp' equals to (1-step_x) / 2
				int8 step_tmp = (step_x > 0) ? 0 : 1;
				wall_distance = (map_x - PL_player.x + step_tmp) * ray_dir_x__1div;
			}
			else
			{
				side_dist_y += delta_dist_y;
				map_y += step_y;
				what_side_hit = 1;

				// if (what_side_hit == 0) ...                
					// 'step_tmp' equals to (1-step_y) / 2
				int8 step_tmp = (step_y > 0) ? 0 : 1;
				wall_distance = (map_y - PL_player.y + step_tmp) * ray_dir_y__1div;
			}

			// Precalc this value for performance, to get rid of dividing many times, instead we can use multiply...
			float32 wall_distance__1div = 1.0f / wall_distance;

			int32 line_height = (int32)(GR_render_height * wall_distance__1div);
			int32 line_height__div2 = line_height >> 1;

			float32 player_z__mul__wall_distance__1div = PL_player.z * wall_distance__1div;
			int32 tmp = (int32)(GR_render_height__div2 + PL_player.pitch + player_z__mul__wall_distance__1div);

			int32 wall_draw_start = -line_height__div2 + tmp;
			if (wall_draw_start < 0) wall_draw_start = 0;

			int32 wall_draw_end = line_height__div2 + tmp;
			if (wall_draw_end > GR_render_height__sub1) wall_draw_end = GR_render_height__sub1;

			int16 hit_map_id = map_x + (map_y << LV_MAP_LENGTH_BITSHIFT);

						// ------------------------------------------------------
						// Preparing Pillars structure for later FLATS rendering.
						// ------------------------------------------------------
						if ((wall_draw_end != prew_draw_end) || (wall_draw_start != prew_draw_start))
						{
							if (hit_number == 0)
							{
								prew_draw_end = GR_render_height__sub1;
								prew_draw_start = 0;
								hit_number = 1;
							}

							if (!GR_pillars_map[prew_map_id].found)
							{
								GR_pillars_used++;

								GR_pillars_map[prew_map_id].found = 1;
								GR_pillars_map[prew_map_id].pillar_address = &GR_pillars[GR_pillars_used - 1];

								sGR_Pillar* tmp = GR_pillars_map[prew_map_id].pillar_address;

								// Floor.
								tmp->flat_texture__PTR[0] = LV_map[prew_map_id].flat_texture__PTR[0];
								tmp->flat_texture_intensity__PTR[0] = LV_map[prew_map_id].flat_texture_intensity__PTR[0];
								tmp->flat_lightmap__PTR[0] = LV_map[prew_map_id].flat_lightmap__PTR[0];

								tmp->floor_top = wall_draw_end;
								tmp->floor_bottom = prew_draw_end;

								tmp->floor_last_top = wall_draw_end;
								tmp->floor_last_bottom = prew_draw_end;
								tmp->floor_last_right = ray_x;

								tmp->floor_prev_top = wall_draw_end;
								tmp->floor_prev_bottom = prew_draw_end;

								for (int yy = wall_draw_end; yy <= prew_draw_end; yy++)
								{
									tmp->floor_left[yy] = ray_x;
								}

								// Ceil.
								tmp->flat_texture__PTR[1] = LV_map[prew_map_id].flat_texture__PTR[1];
								tmp->flat_texture_intensity__PTR[1] = LV_map[prew_map_id].flat_texture_intensity__PTR[1];
								tmp->flat_lightmap__PTR[1] = LV_map[prew_map_id].flat_lightmap__PTR[1];

								tmp->ceil_top = prew_draw_start;
								tmp->ceil_bottom = wall_draw_start;

								tmp->ceil_last_top = prew_draw_start;
								tmp->ceil_last_bottom = wall_draw_start;
								tmp->ceil_last_right = ray_x;

								tmp->ceil_prev_top = prew_draw_start;
								tmp->ceil_prev_bottom = wall_draw_start;

								for (int yy = prew_draw_start; yy < wall_draw_start; yy++)
								{
									tmp->ceil_left[yy] = ray_x;
								}

								// Remember previous floor and ceil hit values.
								prew_draw_end = wall_draw_end;
								prew_draw_start = wall_draw_start;
							}
							else
							{
								sGR_Pillar* tmp = GR_pillars_map[prew_map_id].pillar_address;

								// floor
								if (tmp->floor_top > wall_draw_end) tmp->floor_top = wall_draw_end;
								if (tmp->floor_bottom < prew_draw_end) tmp->floor_bottom = prew_draw_end;

								if (tmp->floor_prev_top != wall_draw_end)
								{
									int32 diff_top = 0;
									int32 diff_top_offset = 0;

									if (tmp->floor_prev_top > wall_draw_end)
									{
										diff_top = tmp->floor_prev_top - wall_draw_end;
										diff_top_offset = wall_draw_end;
									}
									else
									{
										diff_top = wall_draw_end - tmp->floor_prev_top;
										diff_top_offset = tmp->floor_prev_top;
									}

									while (diff_top > 0)
									{
										if (tmp->floor_left[diff_top_offset] < 0) tmp->floor_left[diff_top_offset] = ray_x;
										tmp->floor_right[diff_top_offset] = ray_x;

										diff_top--;
										diff_top_offset++;
									}
								}
								else
									if (tmp->floor_left[wall_draw_end] < 0) tmp->floor_left[wall_draw_end] = ray_x;

								if (tmp->floor_prev_bottom != prew_draw_end)
								{
									int32 diff_bottom = 0;
									int32 diff_bottom_offset = 0;

									if (tmp->floor_prev_bottom > prew_draw_end)
									{
										diff_bottom = tmp->floor_prev_bottom - prew_draw_end;
										diff_bottom_offset = prew_draw_end;
									}
									else
									{
										diff_bottom = prew_draw_end - tmp->floor_prev_bottom;
										diff_bottom_offset = tmp->floor_prev_bottom;
									}

									while (diff_bottom >= 0)
									{
										if (tmp->floor_left[diff_bottom_offset] < 0) tmp->floor_left[diff_bottom_offset] = ray_x;
										tmp->floor_right[diff_bottom_offset] = ray_x;

										diff_bottom--;
										diff_bottom_offset++;
									}
								}
								else
									if (tmp->floor_left[prew_draw_end] < 0) tmp->floor_left[prew_draw_end] = ray_x;

								tmp->floor_last_top = wall_draw_end;
								tmp->floor_last_bottom = prew_draw_end;
								tmp->floor_last_right = ray_x;

								tmp->floor_prev_top = wall_draw_end;
								tmp->floor_prev_bottom = prew_draw_end;

								// ceil
								if (tmp->ceil_top > prew_draw_start) tmp->ceil_top = prew_draw_start;
								if (tmp->ceil_bottom < wall_draw_start) tmp->ceil_bottom = wall_draw_start;

								if (tmp->ceil_prev_top != prew_draw_start)
								{
									int32 diff_top = 0;
									int32 diff_top_offset = 0;

									if (tmp->ceil_prev_top > prew_draw_start)
									{
										diff_top = tmp->ceil_prev_top - prew_draw_start;
										diff_top_offset = prew_draw_start;
									}
									else
									{
										diff_top = prew_draw_start - tmp->ceil_prev_top;
										diff_top_offset = tmp->ceil_prev_top;
									}

									while (diff_top > 0)
									{
										if (tmp->ceil_left[diff_top_offset] < 0) tmp->ceil_left[diff_top_offset] = ray_x;
										tmp->ceil_right[diff_top_offset] = ray_x;

										diff_top--;
										diff_top_offset++;
									}
								}
								else
									if (tmp->ceil_left[prew_draw_start] < 0) tmp->ceil_left[prew_draw_start] = ray_x;

								if (tmp->ceil_prev_bottom != wall_draw_start)
								{
									int32 diff_bottom = 0;
									int32 diff_bottom_offset = 0;

									if (tmp->ceil_prev_bottom > wall_draw_start)
									{
										diff_bottom = tmp->ceil_prev_bottom - wall_draw_start;
										diff_bottom_offset = wall_draw_start;
									}
									else
									{
										diff_bottom = wall_draw_start - tmp->ceil_prev_bottom;
										diff_bottom_offset = tmp->ceil_prev_bottom;
									}

									while (diff_bottom > 0)
									{
										if (tmp->ceil_left[diff_bottom_offset] < 0) tmp->ceil_left[diff_bottom_offset] = ray_x;
										tmp->ceil_right[diff_bottom_offset] = ray_x;

										diff_bottom--;
										diff_bottom_offset++;
									}
								}
								else
									if (tmp->ceil_left[wall_draw_start] < 0) tmp->ceil_left[wall_draw_start] = ray_x;

								tmp->ceil_last_top = prew_draw_start;
								tmp->ceil_last_bottom = wall_draw_start;
								tmp->ceil_last_right = ray_x;

								tmp->ceil_prev_top = prew_draw_start;
								tmp->ceil_prev_bottom = wall_draw_start;

								// remember previous floor and ceil hit values
								prew_draw_end = wall_draw_end;
								prew_draw_start = wall_draw_start;
							}
						}

						prew_map_id = hit_map_id;
						// ------------------------------------------------------
						// End of Pillars preparing.
						// ------------------------------------------------------


			// Check if ray has hit a wall...
			if (LV_map[hit_map_id].is_cell_solid)
			{
				// Mark that solid cell was hit - to end the DDA.
				is_solid_hit = 1;

				// Make pointer to current cell - should boost performance.
				sLV_Cell* cell = &LV_map[hit_map_id];

				// Pointer to where current texture color data (intensity) begins.
				u_int32* texture_intensity = NULL;

				// Pointer to where current texture image data (indexes to color map) begins.
				u_int8* texture = NULL;

				// Pointer to where current lightmap begins.
				u_int8* lightmap = NULL;

				// Test what kind of wall we hit...
				switch (cell->cell_type)
				{
					case LV_C_WALL_STANDARD:
						texture = cell->wall_texture__PTR[0];
						texture_intensity = cell->wall_texture_intensity__PTR[0];

						if (what_side_hit)
						{
							if (step_y > 0)
								lightmap = cell->wall_lightmap__PTR[0];
							else
								lightmap = cell->wall_lightmap__PTR[2];
						}
						else
						{
							if (step_x > 0)
								lightmap = cell->wall_lightmap__PTR[3];
							else
								lightmap = cell->wall_lightmap__PTR[1];
						}
						break;

					case LV_C_WALL_FOURSIDE:
						if (what_side_hit)
						{
							if (step_y > 0)
							{
								texture = cell->wall_texture__PTR[0];
								texture_intensity = cell->wall_texture_intensity__PTR[0];
								lightmap = cell->wall_lightmap__PTR[0];
							}
							else
							{
								texture = cell->wall_texture__PTR[2];
								texture_intensity = cell->wall_texture_intensity__PTR[2];
								lightmap = cell->wall_lightmap__PTR[2];
							}
						}
						else
						{
							if (step_x > 0)
							{
								texture = cell->wall_texture__PTR[3];
								texture_intensity = cell->wall_texture_intensity__PTR[3];
								lightmap = cell->wall_lightmap__PTR[3];
							}
							else
							{
								texture = cell->wall_texture__PTR[1];
								texture_intensity = cell->wall_texture_intensity__PTR[1];
								lightmap = cell->wall_lightmap__PTR[1];
							}
						}
						break;
				}
			
				// Lets calculate starting Y coord of wall that is mapped on texture.
				int32 tex_y_start = (int32)(wall_draw_start - PL_player.pitch - player_z__mul__wall_distance__1div - GR_render_height__div2 + line_height__div2);

				if (line_height > GR_max_wall_stripe_lineheight__sub1) line_height = GR_max_wall_stripe_lineheight__sub1;

				// These pointers will hold precalculated starting position of Y coord in texture				
				// for lightmap 32x32 px - can be set now.
				u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

				// The X coord of point that we hit the wall.
				float32 wall_hit_x;

				// The X coord mapped on the texture and lightmap.
				int32 tex_x = 0;
				int32 tex_x_lightmap = 0;

				// Tmp pointer to used wall stripe.
				sGR_Wall_Stripe* tmp_stripe__ptr = &GR_wall_stripe[line_height];
		
				if (what_side_hit == 0)
				{
					wall_hit_x = PL_player.y + wall_distance * ray_dir_y;
					wall_hit_x -= (int32)wall_hit_x;

					// The X coord mapped on the texture and lightmap.
					tex_x = (int32)(wall_hit_x * tmp_stripe__ptr->tex_size);
					tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE);

					if (step_x > 0)
					{
						tex_x = tmp_stripe__ptr->tex_size_sub1 - tex_x;
						tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
					}
				}
				else
				{
					wall_hit_x = PL_player.x + wall_distance * ray_dir_x;
					wall_hit_x -= (int32)wall_hit_x;

					// The X coord mapped on the texture and lightmap.
					tex_x = (int32)(wall_hit_x * tmp_stripe__ptr->tex_size);
					tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE);

					if (step_y < 0)
					{
						tex_x = tmp_stripe__ptr->tex_size_sub1 - tex_x;
						tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
					}
				}

				// These pointers will hold precalculated starting position of Y coord in texture				
				// for mipmaped texture - depends on distance...
				u_int8* tex_y_pos = &tmp_stripe__ptr->wall_stripe[tex_y_start];

				// Move texture pointer to start of correct mipmap.
				texture += tmp_stripe__ptr->size_offset;

				// Will hold pointer that are already shifted to correct position.
				// Before entering main rendering loop we can pull multiplaying by texture height out.
				u_int8* texture__READY = &texture[ tex_x << tmp_stripe__ptr->tex_size_bitshift ];
				u_int8* lightmap__READY = &lightmap[ tex_x_lightmap << 5 ]; 

				// Calculate distance intensity value based on wall distance.
				int32 distance_intensity = (int32)(wall_distance * GR_DISTANCE_INTENSITY_SCALE);
				distance_intensity = MA_MIN(distance_intensity, GR_DISTANCE_INTENSITY_LENGTH - 1);

				// Move pointer to correct distance in distance shading LUT.
				int32* distance_shading_walls__LUT__READY = GR_distance_shading_walls__LUT[distance_intensity];

				// This is for performance to avoid multiplication in inner loop.
				int32 wall_draw_start_in_buffer = ray_x + wall_draw_start * GR_render_width;
				u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

				// Calculate height to be drawn and to be checked against 0 in loop.
				int32 wall_draw_height = wall_draw_end - wall_draw_start;		

				// Draw a single vertical slice of wall that was hit.
				do
				{
					// At first we are getting intensity value from lightmap pixel (0-127).
					int32 lm_intensity_value = lightmap__READY[ *tex_32lm_y_pos ];

					// Next, that pixel value should be affected by the distance -
					// so lets use precalculated GR_distance_shading_walls__LUT to get that value.
					// That value is already multiplied by 128 (IO_TEXTURE_MAX_SHADES), so we dont need to multiply it here.
					// Next, move pointer to correct color (intensity) table of texture.
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_walls__LUT__READY[ lm_intensity_value ];

					// We can put correct pixel into output buffer.
					*output_buffer_32__ptr = texture_intensity_lm[ texture__READY[ *tex_y_pos ] ];
					output_buffer_32__ptr += GR_render_width;

					// Move to the next pixel in texture.
					tex_y_pos++;
					tex_32lm_y_pos++;

					wall_draw_height -= 1;	

				} while (wall_draw_height > 0);
			}
		}

	}
}

void	GR_Raycast_Flats(void)
{
	// Ray direction for leftmost ray (x = 0) and rightmost ray (x = width).
	float32 ray_dir_x0 = GR_pp_dx - GR_pp_nsize_x__div2;
	float32 ray_dir_y0 = GR_pp_dy - GR_pp_nsize_y__div2;

	// Precalculated helpers for performance.
	float32 r_dx1_m_dx0_div_width = ((GR_pp_nsize_x__div2 + GR_pp_nsize_x__div2) * GR_render_width__1div);
	float32 r_dy1_m_dy0_div_width = ((GR_pp_nsize_y__div2 + GR_pp_nsize_y__div2) * GR_render_width__1div);

	int32 p = GR_pillars_used;

	do
	{
		// Get ptr to current pillar outside of the inner loops - for performance.
		sGR_Pillar* current_pillar__PTR = &GR_pillars[p];

		// floor
		if ((current_pillar__PTR->floor_top < 0) || (current_pillar__PTR->floor_bottom < 0) ||
			(current_pillar__PTR->floor_top > GR_render_height__sub1) || (current_pillar__PTR->floor_bottom > GR_render_height__sub1))
			goto __CEIL;

		// Update projection plane Z position.
		float32 pp_z = GR_render_height__div2 + PL_player.z;

		// Making 'while' loop - this way let us to test loop against 0 which is faster.
		int32 ray_y = current_pillar__PTR->floor_top;
		int32 draw_height = current_pillar__PTR->floor_bottom - ray_y;

		if (abs(draw_height) <= 0) goto __CEIL;

		for (int yy = current_pillar__PTR->floor_last_top; yy <= current_pillar__PTR->floor_last_bottom; yy++)
		{
			current_pillar__PTR->floor_right[yy] = current_pillar__PTR->floor_last_right + 1;
		}

		int8* texture_256__READY = current_pillar__PTR->flat_texture__PTR[0];
		int8* texture_128__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256);
		int8* texture_64__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256 + 128 * 128);
		int8* texture_32__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256 + 128 * 128 + 64 * 64);
		int8* texture_16__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
		int8* texture_8__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
		int8* texture_4__READY = current_pillar__PTR->flat_texture__PTR[0] + (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);

		int32* texture_intensity = current_pillar__PTR->flat_texture_intensity__PTR[0];
		u_int8* lightmap__READY = current_pillar__PTR->flat_lightmap__PTR[0];
		
		while (draw_height >= 0)
		{
			float32 ray_y_pos = ray_y - GR_render_height__div2 - PL_player.pitch;
			float32 straight_distance_to_point = fabsf(pp_z / ray_y_pos);

			// Calculate distance intensity value based on wall distance.
			int32 distance_intensity = (int32)(straight_distance_to_point * GR_DISTANCE_INTENSITY_SCALE);
			distance_intensity = MA_MIN(distance_intensity, GR_DISTANCE_INTENSITY_LENGTH - 1);

		//	u_int32* texture_intensity__READY = texture_intensity + 127 * 128;

			// Move pointer to correct distance in distance shading LUT.
			int32* distance_shading_flats__LUT__READY = GR_distance_shading_flats__LUT[distance_intensity]; 

			float32 floor_step_x = straight_distance_to_point * r_dx1_m_dx0_div_width;
			float32 floor_step_y = straight_distance_to_point * r_dy1_m_dy0_div_width;

			float32 floor_x = (PL_player.x + (straight_distance_to_point * ray_dir_x0)) + floor_step_x * current_pillar__PTR->floor_left[ray_y];
			float32 floor_y = (PL_player.y + (straight_distance_to_point * ray_dir_y0)) + floor_step_y * current_pillar__PTR->floor_left[ray_y];

			int32 floor_step_x__fp = (int32)(floor_step_x * 262144.0f);
			int32 floor_step_y__fp = (int32)(floor_step_y * 262144.0f);

			int32 floor_x__fp = (int32)(floor_x * 262144.0f);
			int32 floor_y__fp = (int32)(floor_y * 262144.0f);
		
			int32 draw_start_in_buffer = current_pillar__PTR->floor_left[ray_y] + ray_y * GR_render_width;
			u_int32* output_buffer_32__READY = IO_prefs.output_buffer_32 + draw_start_in_buffer;

			int32 draw_length = current_pillar__PTR->floor_right[ray_y] - current_pillar__PTR->floor_left[ray_y];

			if (draw_length > 128)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 18)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 10) & (255);
					int32 ty = (int32)(floor_y__fp >> 10) & (255);

					int32 lm_intensity_value = lightmap__READY[ lm_tx + (lm_ty << 5) ];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[ lm_intensity_value ];
					 
					u_int8 texture_pixel_index = texture_256__READY[tx + (ty << 8)];

					*output_buffer_32__READY = texture_intensity_lm[ texture_pixel_index ];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 64)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 11) & (127);
					int32 ty = (int32)(floor_y__fp >> 11) & (127);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_128__READY[tx + (ty << 7)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 32)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 12) & (63);
					int32 ty = (int32)(floor_y__fp >> 12) & (63);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_64__READY[tx + (ty << 6)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 16)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 tx = (int32)(floor_x__fp >> 13) & (31);
					int32 ty = ((int32)(floor_y__fp >> 13) & (31)) << 5;



					int32 lm_intensity_value = lightmap__READY[tx + ty];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_32__READY[tx + ty];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 8)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 14) & (15);
					int32 ty = (int32)(floor_y__fp >> 14) & (15);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_16__READY[tx + (ty << 4)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 4)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 15) & (7);
					int32 ty = (int32)(floor_y__fp >> 15) & (7);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_8__READY[tx + (ty << 3)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 16) & (3);
					int32 ty = (int32)(floor_y__fp >> 16) & (3);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_4__READY[tx + (ty << 2)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}


			draw_height--;
			ray_y++;
		}





	__CEIL:;
		
		// floor
		if ((current_pillar__PTR->ceil_top < 0) || (current_pillar__PTR->ceil_bottom < 0) ||
			(current_pillar__PTR->ceil_top > GR_render_height__sub1) || (current_pillar__PTR->ceil_bottom > GR_render_height__sub1))
			continue;

		// Update projection plane Z position.
		 pp_z = GR_render_height__div2 - PL_player.z;

		// Making 'while' loop - this way let us to test loop against 0 which is faster.
		 ray_y = current_pillar__PTR->ceil_top;
		 draw_height = current_pillar__PTR->ceil_bottom - ray_y;

		if (abs(draw_height) == 0) continue;

		for (int yy = current_pillar__PTR->ceil_last_top; yy < current_pillar__PTR->ceil_last_bottom; yy++)
		{
			current_pillar__PTR->ceil_right[yy] = current_pillar__PTR->ceil_last_right + 1;
		}

		 texture_256__READY = current_pillar__PTR->flat_texture__PTR[1];
		 texture_128__READY = current_pillar__PTR->flat_texture__PTR[1] + (256 * 256);
		 texture_64__READY = current_pillar__PTR->flat_texture__PTR[1] + (256 * 256 + 128 * 128);
		 texture_32__READY = current_pillar__PTR->flat_texture__PTR[1] + (256 * 256 + 128 * 128 + 64 * 64);
		 texture_16__READY = current_pillar__PTR->flat_texture__PTR[1] + (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32);
		 texture_8__READY = current_pillar__PTR->flat_texture__PTR[1]+ (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16);
		 texture_4__READY = current_pillar__PTR->flat_texture__PTR[1] + (256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8);

		texture_intensity = current_pillar__PTR->flat_texture_intensity__PTR[1];
		lightmap__READY = current_pillar__PTR->flat_lightmap__PTR[1];

		while (draw_height > 0)
		{
			float32 ray_y_pos = ray_y - GR_render_height__div2 - PL_player.pitch;
			float32 straight_distance_to_point = fabsf(pp_z / ray_y_pos);

			// Calculate distance intensity value based on wall distance.
			int32 distance_intensity = (int32)(straight_distance_to_point * GR_DISTANCE_INTENSITY_SCALE);
			distance_intensity = MA_MIN(distance_intensity, GR_DISTANCE_INTENSITY_LENGTH - 1);

			//	u_int32* texture_intensity__READY = texture_intensity + 127 * 128;

				// Move pointer to correct distance in distance shading LUT.
			int32* distance_shading_flats__LUT__READY = GR_distance_shading_flats__LUT[distance_intensity];

			float32 floor_step_x = straight_distance_to_point * r_dx1_m_dx0_div_width;
			float32 floor_step_y = straight_distance_to_point * r_dy1_m_dy0_div_width;

			float32 floor_x = (PL_player.x + (straight_distance_to_point * ray_dir_x0)) + floor_step_x * current_pillar__PTR->ceil_left[ray_y];
			float32 floor_y = (PL_player.y + (straight_distance_to_point * ray_dir_y0)) + floor_step_y * current_pillar__PTR->ceil_left[ray_y];

			int32 floor_step_x__fp = (int32)(floor_step_x * 262144.0f);
			int32 floor_step_y__fp = (int32)(floor_step_y * 262144.0f);

			int32 floor_x__fp = (int32)(floor_x * 262144.0f);
			int32 floor_y__fp = (int32)(floor_y * 262144.0f);

			int32 draw_start_in_buffer = current_pillar__PTR->ceil_left[ray_y] + ray_y * GR_render_width;
			u_int32* output_buffer_32__READY = IO_prefs.output_buffer_32 + draw_start_in_buffer;

			int32 draw_length = current_pillar__PTR->ceil_right[ray_y] - current_pillar__PTR->ceil_left[ray_y];

			if (draw_length > 128)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 18)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 10) & (255);
					int32 ty = (int32)(floor_y__fp >> 10) & (255);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_256__READY[tx + (ty << 8)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 64)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 11) & (127);
					int32 ty = (int32)(floor_y__fp >> 11) & (127);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_128__READY[tx + (ty << 7)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 32)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 12) & (63);
					int32 ty = (int32)(floor_y__fp >> 12) & (63);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_64__READY[tx + (ty << 6)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 16)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 tx = (int32)(floor_x__fp >> 13) & (31);
					int32 ty = ((int32)(floor_y__fp >> 13) & (31)) << 5;



					int32 lm_intensity_value = lightmap__READY[tx + ty];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_32__READY[tx + ty];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 8)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 14) & (15);
					int32 ty = (int32)(floor_y__fp >> 14) & (15);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_16__READY[tx + (ty << 4)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else if (draw_length > 4)
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 15) & (7);
					int32 ty = (int32)(floor_y__fp >> 15) & (7);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_8__READY[tx + (ty << 3)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}
			else
			{
				while (draw_length > 0)
				{
					// >> 8 because instead of: (<< IO_TEXTURE_SIZE__BITSHIFT) >> FP13_BITSHIFT, ((<< 8) >> 16)

					int32 lm_tx = (int32)(floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
					int32 lm_ty = (int32)(floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

					int32 tx = (int32)(floor_x__fp >> 16) & (3);
					int32 ty = (int32)(floor_y__fp >> 16) & (3);

					int32 lm_intensity_value = lightmap__READY[lm_tx + (lm_ty << 5)];
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_flats__LUT__READY[lm_intensity_value];

					u_int8 texture_pixel_index = texture_4__READY[tx + (ty << 2)];

					*output_buffer_32__READY = texture_intensity_lm[texture_pixel_index];
					output_buffer_32__READY++;

					floor_x__fp += floor_step_x__fp;
					floor_y__fp += floor_step_y__fp;

					draw_length--;
				}
			}


			draw_height--;
			ray_y++;
		}




	} while (p--);
}