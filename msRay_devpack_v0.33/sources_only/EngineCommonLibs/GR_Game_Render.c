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
int16		GR_render_width__sub1, GR_render_width__div2;
int16		GR_render_height__sub1, GR_render_height__div2;
float32		GR_render_width__1div;

int16		GR_panel_height;

int16		GR_horizont;

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

sGR_Wall_Stripe* GR_wall_stripe;
u_int8** GR_wall_stripe_lightmap;

// Structures for floor/ceiling (flats) rendering
typedef struct
{
	u_int32 offset;
	u_int16 tex_size;
	u_int8	tex_size_sub1;
	u_int8	tex_size_bitshift;
}_IO_BYTE_ALIGN_ sGR_Flat_Mip_Map;

int16* GR_fv_buf_x[2];
int16* GR_fv_buf_x_cell_id[2];
int16 GR_proxy_cell_id;

#define GR_FV_MAX_POLY_POINTS		12
#define GR_TEST_NEAR_CLIP		100.0f
#define GR_FLATS_OFFSET			2

// Structures for precalculated distance intensity tables.
#define GR_DISTANCE_INTENSITY_LENGTH             128   
#define GR_DISTANCE_INTENSITY_MIN                40.0f 
#define GR_DISTANCE_INTENSITY_SCALE              20.0f 

int32** GR_distance_shading_walls__LUT;
int32** GR_distance_shading_flats__LUT;

// ----------------------------------------------------
// --- GAME RENDER - PRIVATE functions declarations ---
// ----------------------------------------------------

int32	GR_Init_Projection_Plane(void);
void	GR_Cleanup_Projection_Plane(void);

int32	GR_Init_Wall_Stripe_LUT(void);
void	GR_Cleanup_Wall_Stripe_LUT(void);

int32	GR_Init_Wall_Stripe_Lightmap_LUT(void);
void	GR_Cleanup_Wall_Stripe_Lightmap_LUT(void);

int32	GR_Init_FV(void);
void	GR_Cleanup_FV(void);

int32	GR_Init_Distance_Shading_LUT(void);
void	GR_Cleanup_Distance_Shading_LUT(void);

void	GR_FV_Segment_To_Buffer(const int8, int16, int16, const int16, const int16);
void	GR_FV_Process_Segment(int16, int16, int16, int16);

void	GR_FV_Line_To_Screen_Top_Intersection(int32, int32, int32, int32, int32*, int32*);
void	GR_FV_Line_To_Screen_Left_Intersection(int32, int32, int32, int32, int32*, int32*);
void	GR_FV_Line_To_Screen_Bottom_Intersection(int32, int32, int32, int32, int32*, int32*);
void	GR_FV_Line_To_Screen_Right_Intersection(int32, int32, int32, int32, int32*, int32*);
void	GR_FV_Line_To_Line_Intersection(float32, float32, float32, float32, float32, float32, float32, float32, float32*, float32*);

void	GR_FV_Clip_To_Screen_Top(int32[][2], int8*);
void	GR_FV_Clip_To_Screen_Left(int32[][2], int8*);
void	GR_FV_Clip_To_Screen_Bottom(int32[][2], int8*);
void	GR_FV_Clip_To_Screen_Right(int32[][2], int8*);

void	GR_FV_Clip_Poly_To_Screen(int32[][2], int8*, int16*, int16*);
void	GR_FV_Clip_Poly_To_Line(float32[][2], int8*, float32[][2]);

void	GR_Render_Flat_Horizontal_Stripe(int32, const u_int8, const u_int8, const u_int8, int32, int32, const int32, const int32, const u_int8*, const int8*, int32*, const int32*, u_int32*);
void	GR_Render_Single_Flat(const int16, const int8, const int16, const int16, const float32, const float32, const float32, const float32, const float32);

void	GR_Update_Input(void);
void	GR_Render_Flats(void);
void	GR_Raycast_Walls(void);


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

	// 
	mem_tmp = GR_Init_FV();
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

	// Zero our 'generator'.
	GR_proxy_cell_id = 0;
}

void GR_Run()
{
	// Get keys and mouse parameters and update.
	GR_Update_Input();

	//memset(IO_prefs.output_buffer_32, 0x11, IO_prefs.screen_frame_buffer_size);

	// Render Flats...
	GR_Render_Flats();

	// Raycast Walls...
	GR_Raycast_Walls();
}

void GR_Cleanup(void)
{
	GR_Cleanup_Distance_Shading_LUT();
	GR_Cleanup_FV();
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
	GR_render_width__sub1 = GR_render_width - 1;
	GR_render_width__div2 = GR_render_width / 2;
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
	GR_wall_stripe[0].size_offset = 256 * 256 + 128 * 128 + 64 * 64;
	GR_wall_stripe[0].tex_size = 32;
	GR_wall_stripe[0].tex_size_sub1 = 31;
	GR_wall_stripe[0].tex_size_bitshift = 5;

	// counting memory...
	memory_allocated += sizeof(u_int8);

	// For 32x32 px mipmap textures. 
	// Line height from 17 to 32 pixels.
	for (int32 i = 1; i < 48; i++)
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

		float32 step = 32.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 64x64 px mipmap textures. 
	// Line height from 33 to 64 pixels.
	for (int32 i = 48; i < 96; i++)
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

		float32 step = 64.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 128x128 px mipmap textures. 
	// Line height from 65 to 128 pixels.
	for (int32 i = 96; i < 192; i++)
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

		float32 step = 128.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)(tex_y_pos);
			tex_y_pos += step;
		}
	}

	// For 256x256 px mipmap textures. 
	// Line height from >= 129 pixels.
	for (int32 i = 192; i < GR_max_wall_stripe_lineheight; i++)
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

		float32 step = 256.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			GR_wall_stripe[i].wall_stripe[j] = (u_int8)(tex_y_pos);
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

int32	GR_Init_FV(void)
{
	int32 mem_allocated = 0;
	int32 mem_size = GR_render_height * sizeof(int16);

	// -- MALLOC --
	for (int32 i = 0; i < 2; i++)
	{
		GR_fv_buf_x[i] = (int16*)malloc(mem_size);
		if (GR_fv_buf_x[i] == NULL) return 0;

		// counting memory...
		mem_allocated += mem_size;
	}

	// -- MALLOC --
	for (int32 i = 0; i < 2; i++)
	{
		GR_fv_buf_x_cell_id[i] = (int16*)malloc(mem_size);
		if (GR_fv_buf_x_cell_id[i] == NULL) return 0;

		// counting memory...
		mem_allocated += mem_size;
	}

	return mem_allocated;
}
void	GR_Cleanup_FV(void)
{
	free(GR_fv_buf_x[0]);
	free(GR_fv_buf_x[1]);

	free(GR_fv_buf_x_cell_id[0]);
	free(GR_fv_buf_x_cell_id[1]);
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


void	GR_FV_Segment_To_Buffer(const int8 _buff_id, int16 x0, int16 y0, const int16 x1, const int16 y1)
{
	int16* fv_buf_x__READY = &GR_fv_buf_x[_buff_id][0];
	int16* fv_buf_x_cell_id__READY = &GR_fv_buf_x_cell_id[_buff_id][0];

	const int16 dy = y0 - y1;
	int16 e2;

	if (x0 < x1)
	{
		const int16 dx = x1 - x0;
		int16 err = dx + dy;

		for (;;)
		{
			e2 = err << 1;

			if (e2 >= dy)
			{
				err += dy;
				x0++;
			}

			if (e2 <= dx)
			{
				fv_buf_x__READY[y0] = x0;
				fv_buf_x_cell_id__READY[y0] = GR_proxy_cell_id;

				err += dx;
				y0++;

				if (y0 == y1)
				{
					fv_buf_x__READY[y0] = x0;
					fv_buf_x_cell_id__READY[y0] = GR_proxy_cell_id;
					break;
				}
			}
		}
	}
	else
	{
		const int16 dx = x0 - x1;
		int16 err = dx + dy;

		for (;;)
		{
			e2 = err << 1;

			if (e2 >= dy)
			{
				err += dy;
				x0--;
			}

			if (e2 <= dx)
			{
				fv_buf_x__READY[y0] = x0;
				fv_buf_x_cell_id__READY[y0] = GR_proxy_cell_id;

				err += dx;
				y0++;

				if (y0 == y1)
				{
					fv_buf_x__READY[y0] = x0;
					fv_buf_x_cell_id__READY[y0] = GR_proxy_cell_id;
					break;
				}
			}
		}
	}
}
void	GR_FV_Process_Segment(int16 _x0, int16 _y0, int16 _x1, int16 _y1)
{
	// Select left/right buffer - depends on line direction.
	const int16 direction = _y1 - _y0;

	if (_y0 > _y1)
	{
		int16 swap = _x0;
		_x0 = _x1;
		_x1 = swap;

		swap = _y0;
		_y0 = _y1;
		_y1 = swap;
	}

	if (direction < 0)
	{
		GR_FV_Segment_To_Buffer(0, _x0, _y0, _x1, _y1);
	}
	else if (direction > 0)
	{
		GR_FV_Segment_To_Buffer(1, _x0, _y0, _x1, _y1);
	}
	else
	{
		// Horizontal line - put only min and max x-values in correct buffers.
		if (GR_fv_buf_x_cell_id[0][_y0] == GR_proxy_cell_id)
			GR_fv_buf_x[0][_y0] = MA_MIN_2(_x0, MA_MIN_2(_x1, GR_fv_buf_x[0][_y0]));
		else
			GR_fv_buf_x[0][_y0] = MA_MIN_2(_x0, _x1);

		if (GR_fv_buf_x_cell_id[1][_y0] == GR_proxy_cell_id)
			GR_fv_buf_x[1][_y0] = MA_MAX_2(_x0, MA_MAX_2(_x1, GR_fv_buf_x[1][_y0]));
		else
			GR_fv_buf_x[1][_y0] = MA_MAX_2(_x0, _x1);
	}
}

void	GR_FV_Line_To_Screen_Top_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (int32)((_y0 * _x1 - _x0 * _y1) / (_y0 - _y1));
	*_iy = 0;
}
void	GR_FV_Line_To_Screen_Left_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = 0;
	*_iy = (_x0 * _y1 - _y0 * _x1) / (_x0 - _x1);
}
void	GR_FV_Line_To_Screen_Bottom_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (GR_render_height__sub1 * (_x1 - _x0) + (_x0 * _y1 - _y0 * _x1)) / (_y1 - _y0);
	*_iy = GR_render_height__sub1;
}
void	GR_FV_Line_To_Screen_Right_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (GR_render_width__sub1 * (_x1 - _x0)) / (_x1 - _x0);
	*_iy = (GR_render_width__sub1 * (_y1 - _y0) - (_x0 * _y1 - _y0 * _x1)) / (_x1 - _x0);
}
void	GR_FV_Line_To_Line_Intersection(float32 x1, float32 y1, float32 x2, float32 y2, float32 x3, float32 y3, float32 x4, float32 y4, float32* x, float32* y)
{
	float32 f1 = (x1 * y2 - y1 * x2);
	float32 f2 = (x3 * y4 - y3 * x4);

	float32 numx = f1 * (x3 - x4) - (x1 - x2) * f2;
	float32 numy = f1 * (y3 - y4) - (y1 - y2) * f2;

	float32 den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	float32 den_1div = 1.0f / den;

	*x = numx * den_1div;
	*y = numy * den_1div;
}

void	GR_FV_Clip_To_Screen_Top(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_FV_MAX_POLY_POINTS][2] = { 0 };
	int8	new_poly_size = 0;

	// (ix,iy),(kx,ky) are the co-ordinate values of
	// the points
	for (int32 i = 0; i < (*poly_size); i++)
	{
		// i and k form a line in polygon
		int32 k = (i + 1) % (*poly_size);
		int32 ix = poly_points[i][0], iy = poly_points[i][1];
		int32 kx = poly_points[k][0], ky = poly_points[k][1];

		// Calculating position of first point
		// w.r.t. clipper line
		float32 i_pos = (float32)(-GR_render_width__sub1 * iy);

		// Calculating position of second point
		// w.r.t. clipper line
		float32 k_pos = (float32)(-GR_render_width__sub1 * ky);

		// Case 1 : When both points are inside
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge
			// and the second point is added

			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Top_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added
			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Top_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;
		}

		// Case 4: When both points are outside
	//	else
		//{
			//No points are added
		//}
	}

	// Copying new points into original array
	// and changing the no. of vertices

	(*poly_size) = new_poly_size;

	for (int32 i = 0; i < (*poly_size); i++)
	{
		poly_points[i][0] = new_points[i][0];
		poly_points[i][1] = new_points[i][1];
	}
}
void	GR_FV_Clip_To_Screen_Left(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_FV_MAX_POLY_POINTS][2] = { 0 };
	int8	new_poly_size = 0;

	// (ix,iy),(kx,ky) are the co-ordinate values of
	// the points
	for (int32 i = 0; i < (*poly_size); i++)
	{
		// i and k form a line in polygon
		int32 k = (i + 1) % (*poly_size);
		int32 ix = poly_points[i][0], iy = poly_points[i][1];
		int32 kx = poly_points[k][0], ky = poly_points[k][1];

		// Calculating position of first point
		// w.r.t. clipper line
		float32 i_pos = (float32)(-GR_render_height__sub1 * ix);

		// Calculating position of second point
		// w.r.t. clipper line
		float32 k_pos = (float32)(-GR_render_height__sub1 * kx);

		// Case 1 : When both points are inside
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge
			// and the second point is added

			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Left_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added

			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Left_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;
		}

		// Case 4: When both points are outside
		//else
		//{
			//No points are added
		//}
	}

	// Copying new points into original array
	// and changing the no. of vertices

	(*poly_size) = new_poly_size;

	for (int32 i = 0; i < (*poly_size); i++)
	{
		poly_points[i][0] = new_points[i][0];
		poly_points[i][1] = new_points[i][1];
	}
}
void	GR_FV_Clip_To_Screen_Bottom(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_FV_MAX_POLY_POINTS][2] = { 0 };
	int8	new_poly_size = 0;

	for (int32 i = 0; i < (*poly_size); i++)
	{
		// i and k form a line in polygon
		int32 k = (i + 1) % (*poly_size);
		int32 ix = poly_points[i][0], iy = poly_points[i][1];
		int32 kx = poly_points[k][0], ky = poly_points[k][1];

		// Calculating position of first point
		// w.r.t. clipper line
		float32 i_pos = (float32)(GR_render_width__sub1 * (iy - GR_render_height__sub1));

		// Calculating position of second point
		// w.r.t. clipper line
		float32 k_pos = (float32)(GR_render_width__sub1 * (ky - GR_render_height__sub1));

		// Case 1 : When both points are inside
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge
			// and the second point is added
			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Bottom_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added
			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Bottom_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;
		}

		// Case 4: When both points are outside
		//else
		//{
			//No points are added
		//}
	}

	// Copying new points into original array
	// and changing the no. of vertices

	(*poly_size) = new_poly_size;

	for (int32 i = 0; i < (*poly_size); i++)
	{
		poly_points[i][0] = new_points[i][0];
		poly_points[i][1] = new_points[i][1];
	}
}
void	GR_FV_Clip_To_Screen_Right(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_FV_MAX_POLY_POINTS][2] = { 0 };
	int8	new_poly_size = 0;

	// (ix,iy),(kx,ky) are the co-ordinate values of
	  // the points
	for (int32 i = 0; i < (*poly_size); i++)
	{
		// i and k form a line in polygon
		int32 k = (i + 1) % (*poly_size);
		int32 ix = poly_points[i][0], iy = poly_points[i][1];
		int32 kx = poly_points[k][0], ky = poly_points[k][1];

		// Calculating position of first point
		// w.r.t. clipper line
		float32 i_pos = (float32)(GR_render_height__sub1 * (ix - GR_render_width__sub1));

		// Calculating position of second point
		// w.r.t. clipper line
		float32 k_pos = (float32)(GR_render_height__sub1 * (kx - GR_render_width__sub1));

		// Case 1 : When both points are inside
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge
			// and the second point is added

			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Right_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added

			int32 new_x, new_y;
			GR_FV_Line_To_Screen_Right_Intersection(ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;
			new_poly_size++;
		}

		// Case 4: When both points are outside
		//else
		//{
			//No points are added
		//}
	}

	// Copying new points into original array
	// and changing the no. of vertices
	(*poly_size) = new_poly_size;
	for (int32 i = 0; i < (*poly_size); i++)
	{
		poly_points[i][0] = new_points[i][0];
		poly_points[i][1] = new_points[i][1];
	}
}

void	GR_FV_Clip_Poly_To_Screen(int32 poly_points[][2], int8* _poly_size, int16* _y_min, int16* _y_max)
{
	GR_FV_Clip_To_Screen_Left(poly_points, _poly_size);
	GR_FV_Clip_To_Screen_Bottom(poly_points, _poly_size);
	GR_FV_Clip_To_Screen_Right(poly_points, _poly_size);
	GR_FV_Clip_To_Screen_Top(poly_points, _poly_size);

	int8 get_poly_size = (*_poly_size);

	if (get_poly_size < 2) return;

	get_poly_size--;

	// Find y-min and y-max values.
	int32	last_y_min = poly_points[0][1],
		last_y_max = poly_points[0][1];

	// Process all segments from the list.
	for (int32 i = 0; i < get_poly_size; i++)
	{
		GR_FV_Process_Segment(poly_points[i][0], poly_points[i][1], poly_points[i + 1][0], poly_points[i + 1][1]);

		last_y_min = MA_MIN_2(last_y_min, poly_points[i][1]);
		last_y_max = MA_MAX_2(last_y_max, poly_points[i][1]);
	}

	// Process the the last segment.
	GR_FV_Process_Segment(poly_points[get_poly_size][0], poly_points[get_poly_size][1], poly_points[0][0], poly_points[0][1]);

	last_y_min = MA_MIN_2(last_y_min, poly_points[get_poly_size][1]);
	last_y_max = MA_MAX_2(last_y_max, poly_points[get_poly_size][1]);

	*_y_min = last_y_min;
	*_y_max = last_y_max;
}
void	GR_FV_Clip_Poly_To_Line(float32 _poly_points[][2], int8* _poly_size, float32 _clipping_line[][2])
{
	float32	new_points[GR_FV_MAX_POLY_POINTS][2] = { 0 };
	int8	new_poly_size = 0;

	float32 x1 = _clipping_line[0][0];
	float32 y1 = _clipping_line[0][1];
	float32 x2 = _clipping_line[1][0];
	float32 y2 = _clipping_line[1][1];

	for (int i = 0; i < (*_poly_size); i++)
	{
		// i and k form a line in polygon
		int k = (i + 1) % (*_poly_size);
		float ix = _poly_points[i][0], iy = _poly_points[i][1];
		float kx = _poly_points[k][0], ky = _poly_points[k][1];

		// Calculating position of first point
		// w.r.t. clipper line
		float i_pos = (x2 - x1) * (iy - y1) - (y2 - y1) * (ix - x1);

		// Calculating position of second point
		// w.r.t. clipper line
		float k_pos = (x2 - x1) * (ky - y1) - (y2 - y1) * (kx - x1);

		// Case 1 : When both points are inside
		if (i_pos < 0 && k_pos < 0)
		{
			//Only second point is added
			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 2: When only first point is outside
		else if (i_pos >= 0 && k_pos < 0)
		{
			// Point of intersection with edge
			// and the second point is added

			float new_x, new_y;
			GR_FV_Line_To_Line_Intersection(x1, y1, x2, y2, ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;

			new_poly_size++;

			new_points[new_poly_size][0] = kx;
			new_points[new_poly_size][1] = ky;
			new_poly_size++;
		}

		// Case 3: When only second point is outside
		else if (i_pos < 0 && k_pos >= 0)
		{
			//Only point of intersection with edge is added
			float new_x, new_y;
			GR_FV_Line_To_Line_Intersection(x1, y1, x2, y2, ix, iy, kx, ky, &new_x, &new_y);

			new_points[new_poly_size][0] = new_x;
			new_points[new_poly_size][1] = new_y;

			new_poly_size++;
		}

		// Case 4: When both points are outside
		// else
		// {
				//No points are added
		// }
	}

	// Copying new points into original array
	// and changing the no. of vertices

	(*_poly_size) = new_poly_size;

	for (int i = 0; i < (*_poly_size); i++)
	{
		_poly_points[i][0] = new_points[i][0];
		_poly_points[i][1] = new_points[i][1];
	}
}


void	GR_Render_Flat_Horizontal_Stripe(	int32 _draw_length, const u_int8 _tex_size, const u_int8 _tex_size_bitshift, const u_int8 _tex_size_bitshift_fp,
											int32 _floor_x__fp, int32 _floor_y__fp, const int32 _floor_step_x__fp, const int32 _floor_step_y__fp,
											const u_int8* _lightmap__PTR, const int8* _tex__PTR, int32* _tex_colors__PTR, const int32* _distance_shading_flats__PTR, u_int32* _output_buffer_32__PTR)
{

	while (_draw_length >= 0)
	{
		//  -- Step 01. --
		//  Lets find current x,y coords in lightmap bitmap. The lightmap bitmap is 32 x 32 px.	

			// BEFORE OPTIMALIZATION
			// int32 lm_tx = (int32)((floor_x__fp << 5) >> 18) & (IO_LIGHTMAP_SIZE - 1);
			// int32 lm_ty = (int32)((floor_y__fp << 5) >> 18) & (IO_LIGHTMAP_SIZE - 1);

			// AFTER OPTIMALIZATION: 
		int32 lm_x = (int32)(_floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
		int32 lm_y = (int32)(_floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);


		//  -- Step 02. --
		//  Now we can get intensity valie from ligtmap - its 8-bit value from 0 to 127.

		int32 lm_intensity_value = _lightmap__PTR[lm_x + (lm_y << 5)];


		//  -- Step 03. --
		//  Lets find current x,y coords in texture bitmap.		

			// BEFORE OPTIMALIZATION
			// int32 tx = (int32)((floor_x__fp << 8) >> 18) & 255;
			// int32 ty = (int32)((floor_y__fp << 8) >> 18) & 255;

			// AFTER OPTIMALIZATION: 
		int32 tex_x = (int32)(_floor_x__fp >> _tex_size_bitshift_fp) & _tex_size;
		int32 tex_y = (int32)(_floor_y__fp >> _tex_size_bitshift_fp) & _tex_size;


		//  -- Step 04. --
		//  Now we can get index to color table from texture bitmap.

		u_int8 texture_pixel_index = _tex__PTR[tex_x + (tex_y << _tex_size_bitshift)];


		//  -- Step 05. --
		//  We would like also to use distance shading.
		//	Lets take our intensity value from lighmap (from 02) and use it in precalculated array "distance_shading_flats__LUT__READY" 
		//	to get lightmap intensity value affected by distance.
		//  With that final lightmap intensity value we can move pointer to correct color table for that texture

		u_int32* texture_intensity_lm = _tex_colors__PTR + _distance_shading_flats__PTR[lm_intensity_value];


		//  -- Step 06. --
		//  Use texture index (from 04) to get RGBA pixel from correct color table. Put the pixel in output_buffer.

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		_output_buffer_32__PTR++;


		// -- Step 07. --
		// Update incrementations.

		_floor_x__fp += _floor_step_x__fp;
		_floor_y__fp += _floor_step_y__fp;

		_draw_length--;
	}
}

void	GR_Render_Flat_Horizontal_Stripe__32x32(int32 _draw_length,	int32 _floor_x__fp, int32 _floor_y__fp, const int32 _floor_step_x__fp, const int32 _floor_step_y__fp,
												const u_int8* _lightmap__PTR, const int8* _tex__PTR, int32* _tex_colors__PTR, const int32* _distance_shading_flats__PTR, u_int32* _output_buffer_32__PTR)
{

	// This is optimized variant of:
	// void GR_Render_Flat_Horizontal_Stripe(...)
	// to be used when mip-map level for texture is 32x32 (the same as lightmap is).

	while (_draw_length >= 0)
	{
		// Lightmap and text ture coords for 32x32 bitmap.
		int32 x = (int32)(_floor_x__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);
		int32 y = (int32)(_floor_y__fp >> 13) & (IO_LIGHTMAP_SIZE - 1);

		// Calculate index.
		int32 index = x + (y << 5);

		int32 lm_intensity_value = _lightmap__PTR[index];

		// Use the same index for texture mip-map 32x32
		u_int8 texture_pixel_index = _tex__PTR[index];

		u_int32* texture_intensity_lm = _tex_colors__PTR + _distance_shading_flats__PTR[lm_intensity_value];
		
		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		_output_buffer_32__PTR++;

		// Update incrementations.

		_floor_x__fp += _floor_step_x__fp;
		_floor_y__fp += _floor_step_y__fp;

		_draw_length--;
	}
}

void	GR_Render_Single_Flat(	const int16 _pvc_cell_id, const int8 _flat, const int16 _y_min, const int16 _y_max,
								const float32 _pp_z, const float32 _rdx1__sub__rdx0__div__width, const float32 _rdy1__sub__rdy0__div__width, const float32 _ray_dir_x0, const float32 _ray_dir_y0)
{
	// Prepare pointers to each mip-map of current texture.
	int8* texture_256__PTR	= LV_map[_pvc_cell_id].flat_texture__PTR[_flat];
	int8* texture_128__PTR	= LV_map[_pvc_cell_id].flat_texture__PTR[_flat] + (256 * 256);
	int8* texture_64__PTR	= LV_map[_pvc_cell_id].flat_texture__PTR[_flat] + (256 * 256 + 128 * 128);
	int8* texture_32__PTR	= LV_map[_pvc_cell_id].flat_texture__PTR[_flat] + (256 * 256 + 128 * 128 + 64 * 64);

	// The "texture_colors" pointer, points to the color table of current texture.
	// But each texture doesn't have ony one color table but multiple color tables
	// with fading intensity.
	int32* texture_colors__PTR = LV_map[_pvc_cell_id].flat_texture_intensity__PTR[_flat];

	// Pointer to the lightmap bitmap.
	u_int8* lightmap__PTR = LV_map[_pvc_cell_id].flat_lightmap__PTR[_flat];

	// To render single polygon lets go from its top to bottom.
	for (int32 ray_y = _y_min; ray_y <= _y_max; ray_y++)
	{
		float32 ray_y_pos = ray_y - GR_horizont;
		float32 straight_distance_to_point = fabsf(_pp_z / ray_y_pos);

		// Calculate distance intensity value based on wall distance.
		int32 distance_intensity = (int32)(straight_distance_to_point * GR_DISTANCE_INTENSITY_SCALE);
		distance_intensity = MA_MIN_2(distance_intensity, (GR_DISTANCE_INTENSITY_LENGTH - 1));

		// Move pointer to correct distance in distance shading LUT.
		int32* distance_shading_flats__PTR = GR_distance_shading_flats__LUT[distance_intensity];

		float32 flat_x_step = (straight_distance_to_point * _rdx1__sub__rdx0__div__width);
		float32 flat_y_step = (straight_distance_to_point * _rdy1__sub__rdy0__div__width);

		float32 flat_x = (PL_player.x + (straight_distance_to_point * _ray_dir_x0)) + flat_x_step * GR_fv_buf_x[0][ray_y];
		float32 flat_y = (PL_player.y + (straight_distance_to_point * _ray_dir_y0)) + flat_y_step * GR_fv_buf_x[0][ray_y];

		// Convert float values into integer fixed point values - to speed up the performance in the most inner loop.
		int32 flat_x_step__fp = (int32)(flat_x_step * 262144.0f);
		int32 flat_y_step__fp = (int32)(flat_y_step * 262144.0f);

		int32 flat_x__fp = (int32)(flat_x * 262144.0f);
		int32 flat_y__fp = (int32)(flat_y * 262144.0f);

		// Find the position in the screen buffer, where the current horizontal line begins.
		// This is optimized way to move thru the screen buffer. In the inner loop we will only +1 to the buffer,
		// move to the next pixel.
		int32 draw_start_in_buffer = GR_fv_buf_x[0][ray_y] + ray_y * GR_render_width;
		u_int32* output_buffer_32__PTR = IO_prefs.output_buffer_32 + draw_start_in_buffer;

		// The length of the current horizontal stripe.
		int32 draw_length = GR_fv_buf_x[1][ray_y] - GR_fv_buf_x[0][ray_y];

		// Select mip-map level of texture depending in the "draw_lenght" of horizontal stripe
		// and render that single stripe.

		if (draw_length > 220 )
		{
			// for >230 use 256x256
			GR_Render_Flat_Horizontal_Stripe(	draw_length, 255, 8, 10, flat_x__fp, flat_y__fp, flat_x_step__fp, flat_y_step__fp,
												lightmap__PTR, texture_256__PTR, texture_colors__PTR, distance_shading_flats__PTR, output_buffer_32__PTR);
		}
		else if (draw_length > 160)
		{
			// for 192-255 use 128x128
			GR_Render_Flat_Horizontal_Stripe(	draw_length, 127, 7, 11, flat_x__fp, flat_y__fp, flat_x_step__fp, flat_y_step__fp,
												lightmap__PTR, texture_128__PTR, texture_colors__PTR, distance_shading_flats__PTR, output_buffer_32__PTR);
		}
		else if (draw_length > 80)
		{
			// for 64-192 use 64x64
			GR_Render_Flat_Horizontal_Stripe(	draw_length, 63, 6, 12, flat_x__fp, flat_y__fp, flat_x_step__fp, flat_y_step__fp,
												lightmap__PTR, texture_64__PTR, texture_colors__PTR, distance_shading_flats__PTR, output_buffer_32__PTR);
		}
		else if (draw_length)
		{
			// for 0-64 else use 32x32
			GR_Render_Flat_Horizontal_Stripe__32x32(	draw_length, flat_x__fp, flat_y__fp, flat_x_step__fp, flat_y_step__fp,
														lightmap__PTR, texture_32__PTR, texture_colors__PTR, distance_shading_flats__PTR, output_buffer_32__PTR);
		 }
	}
}


void	GR_Update_Input(void)
{
	// ESC key.
	if (IO_input.keys[IO_KEYCODE_ESC])
	{
		IO_prefs.engine_state = EN_STATE_GAMEPLAY_CLEANUP;
	}

	// Update mouse movement X - rotate direction of player and the projection plane.
	float player_angle_speed;

	#if defined _WIN32
		player_angle_speed = (float32)IO_input.mouse_dx * IO_prefs.delta_time;
	#elif defined AMIGA
		player_angle_speed = ((float32)IO_input.mouse_dx * IO_prefs.delta_time) * 0.2f;
	#endif

	// Update player angle.
	PL_player.angle_radians += player_angle_speed;

	// Round angle between (0, 2PI)
	if (PL_player.angle_radians < 0.0f)		PL_player.angle_radians += MA_pi_x2;
	if (PL_player.angle_radians > MA_pi_x2)	PL_player.angle_radians -= MA_pi_x2;

	// Rotate camera direction and camera projection plane.
	float32 tmp_sinf = sinf(player_angle_speed);
	float32 tmp_cosf = cosf(player_angle_speed);

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

	PL_player.curr_cell_x	= (int8)PL_player.x;
	PL_player.curr_cell_y	= (int8)PL_player.y;
	PL_player.curr_cell_id	= PL_player.curr_cell_x + (PL_player.curr_cell_y << LV_MAP_LENGTH_BITSHIFT);
}

void	GR_Render_Flats(void)
{
	//  -- Step 01. --
	//  Lets get the player position and orientation, so we can get to the correct list
	//  that contains all potentially visible cells (PVC) from that current player position and orientation.

		//  Convert player angle to degrees and round to int.
		int16 player_angle__deg = (int16)((PL_player.angle_radians * 180.0f) / MA_pi);

		//  Player orientation is divided into 8 slices. And so the list of PVC.
		int32 pvc_cell_slice = player_angle__deg / 45;

	//  -- Step 02. --
	//  Access the correct list of PVC. The list are created in Level Editor and saved to level file.

		// To access it, we need cell id that Player is curently in and its slice based on orientation.
		int16* pvc_list__PTR = LV_map[PL_player.curr_cell_id].pvc_list__PTR[pvc_cell_slice];

		// Get number of elements in that current PVC list.
		int32 pvc_list_size = LV_map[PL_player.curr_cell_id].pvc_list_size[pvc_cell_slice];

	//  -- Step 03 a. --
	//  Prepare some precalculated helpers.
	//	That values will be used to project cell vertices on screen.

		float32 pp_inv_det = 1.0f / (GR_pp_nsize_x__div2 * GR_pp_dy - GR_pp_dx * GR_pp_nsize_y__div2);

		float32 ppid_dx = pp_inv_det * GR_pp_dx;
		float32 ppid_dy = pp_inv_det * GR_pp_dy;
		float32 ppid_nx = pp_inv_det * GR_pp_nsize_x__div2;
		float32 ppid_ny = pp_inv_det * GR_pp_nsize_y__div2;	

	//  -- Step 03 b. --
	//  Prepare some precalculated helpers.
	//	That values will be used to "near clipping" the edges that are close to Player position.
	//  We are only clipping to single line.

		// The "shorten" direction vector.
		float32 pp_dx__div2 = GR_pp_dx * 0.5f;
		float32 pp_dy__div2 = GR_pp_dy * 0.5f;

		// Our clipping line coords.
		float32 near_clip_x0 = pp_dx__div2 - GR_pp_nsize_x__div2;
		float32 near_clip_y0 = pp_dy__div2 - GR_pp_nsize_y__div2;

		float32 near_clip_x1 = pp_dx__div2 + GR_pp_nsize_x__div2;
		float32 near_clip_y1 = pp_dy__div2 + GR_pp_nsize_y__div2;

		float32 near_clipping_line[2][2] = { {near_clip_x0, near_clip_y0}, {near_clip_x1, near_clip_y1} };

	//  -- Step 03 c. --
	//  Prepare some projection plane precalculated helpers.
	//	That values will be used during texturing to find texture coordinates.

		// Ray direction for leftmost ray (x = 0).
		float32 ray_dir_x0 = GR_pp_dx - GR_pp_nsize_x__div2;
		float32 ray_dir_y0 = GR_pp_dy - GR_pp_nsize_y__div2;

		// Precalculated and optimized helper values.
		float32 rdx1__sub__rdx0__div__width = ((GR_pp_nsize_x__div2 + GR_pp_nsize_x__div2) * GR_render_width__1div);
		float32 rdy1__sub__rdy0__div__width = ((GR_pp_nsize_y__div2 + GR_pp_nsize_y__div2) * GR_render_width__1div);

		// Update projection plane Z position.
		float32 pp_z__floor = GR_render_height__div2 + PL_player.z;
		float32 pp_z__ceil = GR_render_height__div2 - PL_player.z;

		// Find horizont position.
		GR_horizont = (int16)(GR_render_height__div2 + PL_player.pitch);

	//  -- Step 03 d. --
	//  Other helpers and variables.

		// Wraps around our 'generator' of proxy cell id, that helps us during lines rasterization.
		GR_proxy_cell_id &= 2047;

	//  -- Step 04. --
	//  Lets start our main loop. Lets go thru all potentially visible cells from current list.

	do
	{
		//  -- Step 05. --
		//  Setup some values before entering another loop.

			// Update list counter.
			pvc_list_size--;

			// Update our 'generator'.
			GR_proxy_cell_id++;

			// Get ID and x,y of current cell from PVC list.
			int16 pvc_cell_id = pvc_list__PTR[pvc_list_size];
			int8 pvc_cell_y = pvc_cell_id >> LV_MAP_LENGTH_BITSHIFT;
			int8 pvc_cell_x = pvc_cell_id - (pvc_cell_y << LV_MAP_LENGTH_BITSHIFT);

			// Get coordinates of that cell relative to Player position.
			float32 cell_x0 = pvc_cell_x - PL_player.x;
			float32 cell_y0 = pvc_cell_y - PL_player.y;
			float32 cell_x1 = cell_x0 + 1.0f;
			float32 cell_y1 = cell_y0 + 1.0f;

			// That optimized precalculations will be used to project cell vertices into screen.
			float32 transform_top_left_x = pp_inv_det * (GR_pp_dy * cell_x0 - GR_pp_dx * cell_y0);
			float32 transform_top_left_y = pp_inv_det * (-GR_pp_nsize_y__div2 * cell_x0 + GR_pp_nsize_x__div2 * cell_y0);

			float32 transform_top_left_y__1div = 1.0f / transform_top_left_y;
			float32 transform_top_right_y__1div = 1.0f / (transform_top_left_y + ppid_nx);
			float32 transform_bottom_left_y__1div = 1.0f / (transform_top_left_y - ppid_ny);
			float32 transform_bottom_right_y__1div = 1.0f / (transform_top_left_y - ppid_ny + ppid_nx);

			// The starting size of poly for ceil and floor should be set to 4.
			int8 floor_poly_size = 4, ceil_poly_size = 4;
		
			// The min and max values of current floor/ceil tile, we need to know the tiles height to draw it.
			// The initials values 1/0 for min and max preserve polys from rendring if that values are not calculated.
			int16 floor_y_min = 1, floor_y_max = 0;
			int16 ceil_y_min = 1, ceil_y_max = 0;		

		//  -- Step 06. --
		//  Lets project cell vertices on the screen and make clipping if neccesery.
		//  The polygons that will be created on the screen can be divided into a few cases.
		//  We can then suggest optimal algorithm for each case.

			// Lets test if any of projected points will be behind the Player.

			if (	transform_top_left_y__1div		> 0.0f	&&	transform_top_left_y__1div		< GR_TEST_NEAR_CLIP &&
					transform_top_right_y__1div		> 0.0f	&&	transform_top_right_y__1div		< GR_TEST_NEAR_CLIP &&
					transform_bottom_left_y__1div	> 0.0f	&&	transform_bottom_left_y__1div	< GR_TEST_NEAR_CLIP &&
					transform_bottom_right_y__1div	> 0.0f	&&	transform_bottom_right_y__1div	< GR_TEST_NEAR_CLIP)
			{

				// CASE 01.
				// If all points are in the safe zone lets start projecting cell vertices into screen.

				// For pitch and z moving.
				float32 vertical_move__top_left = PL_player.pitch + PL_player.z * transform_top_left_y__1div + GR_render_height__div2;
				float32 vertical_move__top_right = PL_player.pitch + PL_player.z * transform_bottom_left_y__1div + GR_render_height__div2;
				float32 vertical_move__bottom_left = PL_player.pitch + PL_player.z * transform_top_right_y__1div + GR_render_height__div2;
				float32 vertical_move__bottom_right = PL_player.pitch + PL_player.z * transform_bottom_right_y__1div + GR_render_height__div2;

				float32 wall_height__top_left__div2 = (GR_render_height * transform_top_left_y__1div) / 2.0f;
				float32 wall_height__top_right__div2 = (GR_render_height * transform_bottom_left_y__1div) / 2.0f;
				float32 wall_height__bottom_left__div2 = (GR_render_height * transform_top_right_y__1div) / 2.0f;
				float32 wall_height__bottom_right__div2 = (GR_render_height * transform_bottom_right_y__1div) / 2.0f;

				// Calculating screen coords of points.
				int32 s_00__x = (int32)(GR_render_width__div2 * (1.0f + transform_top_left_x * transform_top_left_y__1div));
				int32 s_00__y__ceil = (int32)(-wall_height__top_left__div2 + vertical_move__top_left) + 2;
				int32 s_00__y__floor = (int32)(wall_height__top_left__div2 + vertical_move__top_left) - 2;

				int32 s_01__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x + ppid_dy) * transform_bottom_left_y__1div));
				int32 s_01__y__ceil = (int32)(-wall_height__top_right__div2 + vertical_move__top_right) + 2;
				int32 s_01__y__floor = (int32)(wall_height__top_right__div2 + vertical_move__top_right) - 2;

				int32 s_10__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x - ppid_dx) * transform_top_right_y__1div));
				int32 s_10__y__ceil = (int32)(-wall_height__bottom_left__div2 + vertical_move__bottom_left) + 2;
				int32 s_10__y__floor = (int32)(wall_height__bottom_left__div2 + vertical_move__bottom_left) - 2;

				int32 s_11__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x + ppid_dy - ppid_dx) * transform_bottom_right_y__1div));
				int32 s_11__y__ceil = (int32)(-wall_height__bottom_right__div2 + vertical_move__bottom_right) + 2;
				int32 s_11__y__floor = (int32)(wall_height__bottom_right__div2 + vertical_move__bottom_right) - 2;


				// Make a few visibility tests of the all points against the screen area.

				int8 test_x_points_in_screen = (		s_00__x >= 0					&&		s_00__x < GR_render_width		&&
														s_01__x >= 0					&&		s_01__x < GR_render_width		&&
														s_10__x >= 0					&&		s_10__x < GR_render_width		&&
														s_11__x >= 0					&&		s_11__x < GR_render_width		);

				int8 test_y_floor_points_in_screen = (	s_00__y__floor > GR_horizont	&&	s_00__y__floor < GR_render_height	&&
														s_01__y__floor > GR_horizont	&&	s_01__y__floor < GR_render_height	&&
														s_10__y__floor > GR_horizont	&&	s_10__y__floor < GR_render_height	&&
														s_11__y__floor > GR_horizont	&&	s_11__y__floor < GR_render_height	);

				int8 test_y_ceil_points_in_screen = (	s_00__y__ceil > 0				&&	s_00__y__ceil < GR_horizont			&&
														s_01__y__ceil > 0				&&	s_01__y__ceil < GR_horizont			&&
														s_10__y__ceil > 0				&&	s_10__y__ceil < GR_horizont			&&
														s_11__y__ceil > 0				&&	s_11__y__ceil < GR_horizont			);


				// Check the FLOOR points againts the screen area.

				if (test_x_points_in_screen && test_y_floor_points_in_screen)
				{
					// CASE 01-A FLOOR.

					// If all points are in the screen area, the whole current polygon is visible,
					// so we can use different, faster approach and algorithm.

					// Lets rasterize all edges into buffer.
					// Note, the edges direction for floor/ceil are different and that is important.

					// Segment 00-01.
					GR_FV_Process_Segment(s_00__x, s_00__y__floor, s_01__x, s_01__y__floor);

					// Segment 01-11.
					GR_FV_Process_Segment(s_01__x, s_01__y__floor, s_11__x, s_11__y__floor);

					// Segment 11-10.
					GR_FV_Process_Segment(s_11__x, s_11__y__floor, s_10__x, s_10__y__floor);

					// Segment 10-00.
					GR_FV_Process_Segment(s_10__x, s_10__y__floor, s_00__x, s_00__y__floor);

					// Get y-min and y-max values.
					floor_y_min = MA_MIN_2(s_00__y__floor, (MA_MIN_2(s_01__y__floor, (MA_MIN_2(s_10__y__floor, s_11__y__floor)))));
					floor_y_max = MA_MAX_2(s_00__y__floor, (MA_MAX_2(s_01__y__floor, (MA_MAX_2(s_10__y__floor, s_11__y__floor)))));
				}
				else
				{
					// CASE 01-B FLOOR.

					// If some points are outside the screen area - use Sutherland-Hodgman Polygon Clipping Algorithm (optimized for fixed screen area),
					// that will clip the current polygon against screen borders and create new segments if necessary.
					// 
					// Lets clip and rasterize all edges into buffer.
					// Note, the edges direction for floor/ceil are different and that is important.

					int32 floor_poly_points[GR_FV_MAX_POLY_POINTS][2] = {{s_00__x, s_00__y__floor},
																			{s_01__x, s_01__y__floor},
																			{s_11__x, s_11__y__floor},
																			{s_10__x, s_10__y__floor}	 };

					GR_FV_Clip_Poly_To_Screen(floor_poly_points, &floor_poly_size, &floor_y_min, &floor_y_max);
				}


				// Now, check the CEILING points againts the screen area.

				if (test_x_points_in_screen && test_y_ceil_points_in_screen)
				{
					// CASE 01-A CEIL.

					// If all points are in the screen area, the whole current polygon is visible,
					// so we can use different, faster approach and algorithm.

					// Lets rasterize all edges into buffer.
					// Note, the edges direction for floor/ceil are different and that is important.

					// Segment 00-01.
					GR_FV_Process_Segment(s_00__x, s_00__y__ceil, s_10__x, s_10__y__ceil);

					// Segment 01-11.
					GR_FV_Process_Segment(s_10__x, s_10__y__ceil, s_11__x, s_11__y__ceil);

					// Segment 11-10.
					GR_FV_Process_Segment(s_11__x, s_11__y__ceil, s_01__x, s_01__y__ceil);

					// Segment 10-00.
					GR_FV_Process_Segment(s_01__x, s_01__y__ceil, s_00__x, s_00__y__ceil);

					// Get y-min and y-max values.
					ceil_y_min = MA_MIN_2(s_00__y__ceil, (MA_MIN_2(s_01__y__ceil, (MA_MIN_2(s_10__y__ceil, s_11__y__ceil)))));
					ceil_y_max = MA_MAX_2(s_00__y__ceil, (MA_MAX_2(s_01__y__ceil, (MA_MAX_2(s_10__y__ceil, s_11__y__ceil)))));
				}
				else
				{
					// CASE 01-B CEIL.

					// If some points are outside the screen area - use Sutherland-Hodgman Polygon Clipping Algorithm (optimized for fixed screen area),
					// that will clip the current polygon against screen borders and create new segments if necessary.
					 
					// Lets clip and rasterize all edges into buffer.
					// Note, the edges direction for floor/ceil are different and that is important.

					int32 ceil_poly_points[GR_FV_MAX_POLY_POINTS][2] = {{s_00__x, s_00__y__ceil},
																			{s_10__x, s_10__y__ceil},
																			{s_11__x, s_11__y__ceil},
																			{s_01__x, s_01__y__ceil}	};

					GR_FV_Clip_Poly_To_Screen(ceil_poly_points, &ceil_poly_size, &ceil_y_min, &ceil_y_max);
				}
			}
			else
			{
				// CASE 02.
				// If not all points are in the safe zone - lets use different algorithm.
				// First we must clip the cell edges by near clipping line.
				// Then the cell vertices can be projected on screen and the screen clipping can be done.

				// The polygon created from current cell coordinates.
				float32 cell_poly_points[GR_FV_MAX_POLY_POINTS][2] = { {cell_x0, cell_y0}, {cell_x1, cell_y0}, {cell_x1, cell_y1}, {cell_x0, cell_y1} };

				// Initial size of that cell poly is 4.
				int8 cell_poly_size = 4;

				// Lets clip the cell poly by our "near clipping line".
				GR_FV_Clip_Poly_To_Line(cell_poly_points, &cell_poly_size, near_clipping_line);
	
				// Initial size of floor and ceil poly.
				floor_poly_size = 0;
				ceil_poly_size = 0;

				// Further calculations makes sense only if clipped cell poly has more edges than 2.
				if (cell_poly_size > 2)
				{
					// Initial size of floor and ceil poly - should be the same as result cell poly size.
					floor_poly_size = cell_poly_size;
					ceil_poly_size = cell_poly_size;

					// Lest put coordinates of our projecteed points into this kind of structure.
					int32 screen_projected_points[GR_FV_MAX_POLY_POINTS][3] = { 0 };

					// Start projecting the points of near clipped cell poly.
					for (int32 i = 0; i < cell_poly_size; i++)
					{
						float32 transform_x = pp_inv_det * (GR_pp_dy * cell_poly_points[i][0] - GR_pp_dx * cell_poly_points[i][1]);
						float32 transform_y = pp_inv_det * (-GR_pp_nsize_y__div2 * cell_poly_points[i][0] + GR_pp_nsize_x__div2 * cell_poly_points[i][1]);
						float32 transform_y__1div = 1.0f / transform_y;

						float32 vertical_move = (float32)(PL_player.pitch + PL_player.z * transform_y__1div) + GR_render_height__div2;
						float32 wall_height__div2 = ((float32)(GR_render_height * transform_y__1div)) / 2.0f;

						// The ceil and floor points a offset by a little value - just to hide the gaps between walls and floor/ceils.
						// This is because walls and floor/ceil doesnt share the same math and drawing algorithm.
						screen_projected_points[i][0] = (int32)(GR_render_width__div2 * (1.0f + transform_x * transform_y__1div));
						screen_projected_points[i][1] = (int32)(wall_height__div2 + vertical_move) - GR_FLATS_OFFSET;
						screen_projected_points[i][2] = (int32)(-wall_height__div2 + vertical_move) + GR_FLATS_OFFSET;
					}

					// Lets setup edges of our new "near clipped" floor poligon.
					// The edges order is important.

					int32 floor_screen_clipped_points[GR_FV_MAX_POLY_POINTS][2] = {	{ screen_projected_points[0][0], screen_projected_points[0][1]	},
																					{ screen_projected_points[1][0], screen_projected_points[1][1]	},
																					{ screen_projected_points[2][0], screen_projected_points[2][1]	},
																					{ screen_projected_points[3][0], screen_projected_points[3][1]	},
																					{ screen_projected_points[4][0], screen_projected_points[4][1]	} };

					// Perform the screen clipping.
					GR_FV_Clip_Poly_To_Screen(floor_screen_clipped_points, &floor_poly_size, &floor_y_min, &floor_y_max);


					// Now lets setup our "near clipped" ceil poligon.
					// The edges order is important.
					// For some reason I could't make this work well as in floor case. I have to split it depend on ceil_poly_size.

					int32 screen_clipped_ceil_points[GR_FV_MAX_POLY_POINTS][2] = { { screen_projected_points[0][0], screen_projected_points[0][2]	} };

					for (int32 i = 1; i < ceil_poly_size; i++)
					{
						screen_clipped_ceil_points[i][0] = screen_projected_points[ceil_poly_size - i][0];
						screen_clipped_ceil_points[i][1] = screen_projected_points[ceil_poly_size - i][2];
					}

					// Perform the screen clipping.
					GR_FV_Clip_Poly_To_Screen(screen_clipped_ceil_points, &ceil_poly_size, &ceil_y_min, &ceil_y_max);
				}
			}

			//  -- Step 07. --
			//  After points are projected, segments clipped and rasterized into x0 and x1 buffers,
			//  the current floor and ceil polygons can be rendered.

				// Render the FLOOR if the final floor poly has more than 2 segments.

				if (floor_poly_size > 2)
				{
					GR_Render_Single_Flat(pvc_cell_id, 0, floor_y_min, floor_y_max, pp_z__floor, rdx1__sub__rdx0__div__width, rdy1__sub__rdy0__div__width, ray_dir_x0, ray_dir_y0);
				}

				// Render the CEILING if the final ceil poly has more than 2 segments.

				if (ceil_poly_size > 2)
				{
					GR_Render_Single_Flat(pvc_cell_id, 1, ceil_y_min, ceil_y_max, pp_z__ceil, rdx1__sub__rdx0__div__width, rdy1__sub__rdy0__div__width, ray_dir_x0, ray_dir_y0);
				}

		} while (pvc_list_size);
}

void	GR_Raycast_Walls(void)
{
	// Lets prepare some variables before entering main loop.
	float32 player_x_fraction = PL_player.x - PL_player.curr_cell_x;
	float32 minus__player_x_fraction__add__one = -player_x_fraction + 1.0f;

	float32 player_y_fraction = PL_player.y - PL_player.curr_cell_y;
	float32 minus__player_y_fraction__add__one = -player_y_fraction + 1.0f;

	float32 side_dist_x, side_dist_y;
	int8 step_x = 0, step_y = 0;

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

		int8 map_x = PL_player.curr_cell_x;
		int8 map_y = PL_player.curr_cell_y;

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
				u_int8* texture__READY = &texture[tex_x << tmp_stripe__ptr->tex_size_bitshift];
				u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

				// Calculate distance intensity value based on wall distance.
				int32 distance_intensity = (int32)(wall_distance * GR_DISTANCE_INTENSITY_SCALE);
				distance_intensity = MA_MIN_2(distance_intensity, (GR_DISTANCE_INTENSITY_LENGTH - 1));

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
					int32 lm_intensity_value = lightmap__READY[*tex_32lm_y_pos];

					// Next, that pixel value should be affected by the distance -
					// so lets use precalculated GR_distance_shading_walls__LUT to get that value.
					// That value is already multiplied by 128 (IO_TEXTURE_MAX_SHADES), so we dont need to multiply it here.
					// Next, move pointer to correct color (intensity) table of texture.
					u_int32* texture_intensity_lm = texture_intensity + distance_shading_walls__LUT__READY[lm_intensity_value];

					// We can put correct pixel into output buffer.
					*output_buffer_32__ptr = texture_intensity_lm[texture__READY[*tex_y_pos]];
					output_buffer_32__ptr += GR_render_width;

					// Move to the next pixel in texture.
					tex_y_pos++;
					tex_32lm_y_pos++;

					wall_draw_height--;

				} while (wall_draw_height >= 0);
			}
		}
	}
}
