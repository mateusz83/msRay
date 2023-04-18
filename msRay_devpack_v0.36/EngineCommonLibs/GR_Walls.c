#include "GR_Walls.h"
#include "GR_Projection_Plane.h"
#include "GR_Visible_Cells_List.h"

#include "PL_Player.h"

#include <time.h>

// -------------------------------------------------------------------
// --- GAME RENDER - WALLS / DOORS - PRIVATE globals, constants, variables ---
// -------------------------------------------------------------------
typedef struct
{
	u_int8* wall_stripe__ptr;
	int16   mip_map_offset;
	u_int8  tex_size;
	int8	tex_size_bitshift;
}_IO_BYTE_ALIGN_ sGR_Wall_Stripe;

static sGR_Wall_Stripe* GR_wall_stripe;
static u_int8**			GR_wall_stripe_lightmap;

static int32		GR_max_wall_stripe_lineheight;
static int32		GR_max_wall_stripe_lineheight__sub1;

// Render pass structure.
// This structure will hold informations gathered during raycast.
// The informations will be later used to render full-height walls in 1st pass
// and non-full-height (short walls, up-doors) in the 2nd pass.
typedef struct
{
	u_int8*		lightmap__READY;
	u_int8*		texture__READY;
	u_int8*		tex_y_pos;
	u_int8*		tex_32lm_y_pos;
	u_int32*	texture_intensity;
	u_int32*	output_buffer_32__ptr;
	int32		wall_draw_height;
}_IO_BYTE_ALIGN_ sGR_Wall_Column;

static sGR_Wall_Column* GR_wall_column_1st_pass;
static sGR_Wall_Column* GR_wall_column_2nd_pass;
static int32			GR_wall_column_2nd_pass_count;


// DOORS are also WALLS.
// - The THICK and THIN DOORS always opens upwards and opens when player is coming near - theese are proxmity doors.
// - The BOX DOORS always opens to sides and requires player to push keyboard.
// - All DOORS closes after GR_DOOR_CLOSE_TIMER time or can stay opened forever, THINK OBLIQUE walls hould stay opened because they dont have backface.
#define	GR_DOOR_PROXMITY_DISTANCE		2.0f				// test distance for opening THIN/THICK DOORS
#define	GR_DOOR_PUSH_DISTANCE			0.9f				// test distance for opening BOX DOORS doors using keyboard
#define	GR_DOOR_UP_SPEED				2.5f				// THICK/THIN DOORS opening/closing speed
#define	GR_DOOR_SIDE_SPEED				1.0f				// BOX DOORS opening/closing speed
#define GR_DOOR_CLOSE_TIMER				4 * CLOCKS_PER_SEC	// time to close in miliseconds

static int32	GR_door_box_nearest_id;
static float32	GR_door_box_nearest_distance;

// -----------------------------------------------------------
// --- GAME RENDER - WALLS / DOORS - PRIVATE functions definitions ---
// -----------------------------------------------------------
static int32	GR_Init_Once_Wall_Stripe_LUT(void)
{
	// Setup precalculated wall stripes structure for performance.
	// It will contain texture y coords depend on wall slice line height.

	int32 memory_allocated = 0;

	// The size of the wall stripe buffer depends on the resolution.
	// If the size will be too small, the textures will be distorted when player will be close to the wall.
	// For example for 320x180 screen size the value GR_max_wall_stripe_lineheight = 850 is enough.
	// I wanted to automate this process for different resolutions and also I wanted to keep this value as low as possible to save the memory.
	// Making tests with different resolutions and aspects, I calculated a factor for each screen aspect that can be used to calculate good size.

	float32 screen_aspect = (float32)IO_prefs.screen_width / (float32)IO_prefs.screen_height;
	float32 factor;

	if ( (screen_aspect > ((16.0f / 9.0f) + 0.0001f)) )														factor = 0.2f;				// for bigger than 16:9 (1.77), the factor = 0.2
	if ( (screen_aspect >= ((16.0f / 9.0f) - 0.0001f)) && (screen_aspect <= ((16.0f / 9.0f) + 0.0001f)) )	factor = 180.0f / 850.0f;	// for 16:9, the factor = 0.211
	if ( (screen_aspect >= ((16.0f / 10.0f) - 0.0001f)) && (screen_aspect <= ((16.0f / 10.0f) + 0.0001f)) )	factor = 200.0f / 900.0f;	// for 16:10, the factor = 0.222
	if ( (screen_aspect >= ((4.0f / 3.0f) - 0.0001f)) && (screen_aspect <= ((4.0f / 3.0f) + 0.0001f)) )		factor = 240.0f / 1020.0f;	// for 4:3, the factor = 0.235
	if ( (screen_aspect >= ((5.0f / 4.0f) - 0.0001f)) && (screen_aspect <= ((5.0f / 4.0f) + 0.0001f)) )		factor = 256.0f / 1070.0f;	// for 5:4, the factor = 0.239
	if ( (screen_aspect < ((5.0f / 4.0f) - 0.0001f)))														factor = 0.245;				// for less than 5:4 (1.25), the factor = 0.245

	// Now we can automatically calculate/estimate the 'GR_max_wall_stripe_lineheight' value.
	GR_max_wall_stripe_lineheight = (int32)( ((float32)IO_prefs.screen_height / factor) + 0.5f );
	GR_max_wall_stripe_lineheight__sub1 = GR_max_wall_stripe_lineheight - 1;

	// -- MALLOC --
	int32 mem_size = GR_max_wall_stripe_lineheight * sizeof(sGR_Wall_Stripe);
	GR_wall_stripe = (sGR_Wall_Stripe*)malloc(mem_size);
	if (GR_wall_stripe == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;

	mem_size = sizeof(u_int8);
	GR_wall_stripe[0].wall_stripe__ptr = (u_int8*)malloc(mem_size);
	if (GR_wall_stripe[0].wall_stripe__ptr == NULL) return 0;

	// for line height == 0...
	GR_wall_stripe[0].wall_stripe__ptr[0] = 0;
	GR_wall_stripe[0].mip_map_offset = 128 * 128 + 64 * 64;
	GR_wall_stripe[0].tex_size = 32;
	GR_wall_stripe[0].tex_size_bitshift = 5;

	// counting memory...
	memory_allocated += mem_size;

	// For 32x32 px mipmap textures. 
	for (int32 i = 1; i < 48; i++)
	{
		// -- MALLOC --
		int32 mem_size = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe__ptr = (u_int8*)malloc(mem_size);
		if (GR_wall_stripe[i].wall_stripe__ptr == NULL) return 0;

		memory_allocated += mem_size;

		GR_wall_stripe[i].mip_map_offset = 128 * 128 + 64 * 64;
		GR_wall_stripe[i].tex_size = 32;
		GR_wall_stripe[i].tex_size_bitshift = 5;
		/*
		GR_wall_stripe[i].mip_map_offset = 0;
		GR_wall_stripe[i].tex_size = 128;
		GR_wall_stripe[i].tex_size_bitshift = 7;*/

	//	float32 step = 128.0f / (float32)i;
		float32 step = 32.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			// Adding 0.5f for rounding.
			u_int8 value = (u_int8)(tex_y_pos + 0.5f);

			// Check ranges.
			value = MA_MIN_2(value, 31);
		//	value = MA_MIN_2(value, 127);

			GR_wall_stripe[i].wall_stripe__ptr[j] = value;
			tex_y_pos += step;
		}
	}

	// For 64x64 px mipmap textures. 
	for (int32 i = 48; i < 96; i++)
	{
		// -- MALLOC --
		int32 mem_size = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe__ptr = (u_int8*)malloc(mem_size);
		if (GR_wall_stripe[i].wall_stripe__ptr == NULL) return 0;

		memory_allocated += mem_size;

		GR_wall_stripe[i].mip_map_offset = 128 * 128;
		GR_wall_stripe[i].tex_size = 64;
		GR_wall_stripe[i].tex_size_bitshift = 6;

		float32 step = 64.0f / (float32)i;

	/*	GR_wall_stripe[i].mip_map_offset = 0;
		GR_wall_stripe[i].tex_size = 128;
		GR_wall_stripe[i].tex_size_bitshift = 7;

		float32 step = 128.0f / (float32)i;*/

		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			// Adding 0.5f for rounding.
			u_int8 value = (u_int8)(tex_y_pos + 0.5f);

			// Check ranges.
			value = MA_MIN_2(value, 63);
			//value = MA_MIN_2(value, 127);

			GR_wall_stripe[i].wall_stripe__ptr[j] = value;
			tex_y_pos += step;
		}
	}

	// For 128x128 px mipmap textures. 
	for (int32 i = 96; i < GR_max_wall_stripe_lineheight; i++)
	{
		// -- MALLOC --
		int32 mem_size = sizeof(u_int8) * i;
		GR_wall_stripe[i].wall_stripe__ptr = (u_int8*)malloc(mem_size);
		if (GR_wall_stripe[i].wall_stripe__ptr == NULL) return 0;

		memory_allocated += mem_size;

		GR_wall_stripe[i].mip_map_offset = 0;
		GR_wall_stripe[i].tex_size = 128;
		GR_wall_stripe[i].tex_size_bitshift = 7;

		float32 step = 128.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			// Adding 0.5f for rounding.
			u_int8 value = (u_int8)(tex_y_pos + 0.5f);

			// Check ranges.
			value = MA_MIN_2(value, 127);

			GR_wall_stripe[i].wall_stripe__ptr[j] = value;
			tex_y_pos += step;
		}
	}

	return memory_allocated;
}
static void		GR_Cleanup_Wall_Stripe_LUT(void)
{
	for (int32 i = 0; i < GR_max_wall_stripe_lineheight; i++)
		free(GR_wall_stripe[i].wall_stripe__ptr);

	free(GR_wall_stripe);
}

static int32	GR_Init_Once_Wall_Stripe_Lightmap_LUT(void)
{
	// Setup precalculated wall stripes structure for performance for lightmaps 32x32.
	// It will contain texture y coords depend on wall slice line height.

	int32 memory_allocated = 0;

	// -- MALLOC --
	int32 mem_size = GR_max_wall_stripe_lineheight * sizeof(u_int8*);
	GR_wall_stripe_lightmap = (u_int8**)malloc(mem_size);
	if (GR_wall_stripe_lightmap == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;

	// -- MALLOC --
	mem_size = sizeof(u_int8);
	GR_wall_stripe_lightmap[0] = (u_int8*)malloc(mem_size);
	if (GR_wall_stripe_lightmap[0] == NULL) return 0;
	GR_wall_stripe_lightmap[0][0] = 0;

	// counting memory...
	memory_allocated += mem_size;

	for (int32 i = 1; i < GR_max_wall_stripe_lineheight; i++)
	{
		// -- MALLOC NR 03-c --
		mem_size = sizeof(u_int8) * i;
		GR_wall_stripe_lightmap[i] = (u_int8*)malloc(mem_size);
		if (GR_wall_stripe_lightmap[i] == NULL) return 0;

		memory_allocated += mem_size;

		float32 step = 32.0f / (float32)i;
		float32 tex_y_pos = 0.0f;

		for (int32 j = 0; j < i; j++)
		{
			// Adding 0.5f for rounding.
			u_int8 value = (u_int8)(tex_y_pos + 0.5f);

			// Check ranges.
			value = MA_MIN_2(value, 31);

			GR_wall_stripe_lightmap[i][j] = value;
			tex_y_pos += step;
		}
	}

	return memory_allocated;
}
static void		GR_Cleanup_Wall_Stripe_Lightmap_LUT(void)
{
	for (int32 i = 0; i < GR_max_wall_stripe_lineheight; i++)	free(GR_wall_stripe_lightmap[i]);

	free(GR_wall_stripe_lightmap);
}

static void		GR_Segment_vs_Ray_Intersection(float32 sx0, float32 sy0, float32 sx1, float32 sy1, float32 ray_dir_x, float32 ray_dir_y, float32* _ix, float32* _iy, int8* _intersect)
{
	// Control segment.
	float32 a1 = sy1 - sy0;
	float32 b1 = sx0 - sx1;
	float32 c1 = a1 * sx0 + b1 * sy0;

	// Ray. Player (x,y) position is the ray origin.
	float32 c2 = ray_dir_x * PL_player.x + ray_dir_y * PL_player.y;

	float32 det = a1 * ray_dir_y - ray_dir_x * b1;

	// check if lines are pararell
	if (det < 0.001f && det > -0.001f)
	{
		// SEGMENT and RAY are pararell - return 2.
		*_intersect = 2;
	}
	else
	{
		// lines are not pararell - calculate intersection point
		float32 det__1div = 1.0f / det;

		float32 tmp_x = (ray_dir_y * c1 - b1 * c2) * det__1div;
		float32 tmp_y = (a1 * c2 - ray_dir_x * c1) * det__1div;

		float32 min_x, max_x, min_y, max_y;

		if (sx0 > sx1)
		{
			min_x = sx1 - 0.0001f;
			max_x = sx0 + 0.0001f;
		}
		else
		{
			min_x = sx0 - 0.0001f;
			max_x = sx1 + 0.0001f;
		}

		if (sy0 > sy1)
		{
			min_y = sy1 - 0.0001f;
			max_y = sy0 + 0.0001f;
		}
		else
		{
			min_y = sy0 - 0.0001f;
			max_y = sy1 + 0.0001f;
		}

		if ((tmp_x >= min_x) && (tmp_x <= max_x) && (tmp_y >= min_y) && (tmp_y <= max_y))
		{
			// Intersection point is inside the SEGMENT - return 0.
			*_intersect = 0;

			*_ix = tmp_x;
			*_iy = tmp_y;
		}
		else
		{
			// Intersection point is outside the SEGMENT - return 1.
			*_intersect = 1;
		}
	}
}

static void		GR_Horizontal_Segment_vs_Ray_Intersection(float32 _sx0, float32 _sx1, float32 _sy, float32 _ray_dir_x, float32 _ray_dir_y, float32* _ix, float32* _iy, int8* _intersect)
{
	// Control segment.
	float32 b1 = _sx0 - _sx1;
	float32 c1 = b1 * _sy;

	// Ray. Player (x,y) position is the ray origin.
	float32 c2 = _ray_dir_x * PL_player.x + _ray_dir_y * PL_player.y;

	float32 det = -_ray_dir_x * b1;

	// check if lines are pararell
	if (det < 0.001f && det > -0.001f)
	{
		// SEGMENT and RAY are pararell - return 2.
		*_intersect = 2;
	}
	else
	{
		// lines are not pararell - calculate intersection point
		float32 tmp_x = (_ray_dir_y * c1 - b1 * c2) / det;

		float32 min_x, max_x;

		if (_sx0 > _sx1)
		{
			min_x = _sx1 - 0.0001f;
			max_x = _sx0 + 0.0001f;
		}
		else
		{
			min_x = _sx0 - 0.0001f;
			max_x = _sx1 + 0.0001f;
		}

		if ((tmp_x >= min_x) && (tmp_x <= max_x))
		{
			// Intersection point is inside the SEGMENT - return 0.
			*_intersect = 0;

			*_ix = tmp_x;
			*_iy = _sy;
		}
		else
		{
			// Intersection point is outside the SEGMENT - return 1.
			*_intersect = 1;
		}
	}
}
static void		GR_Vertical_Segment_vs_Ray_Intersection(float32 _sy0, float32 _sy1, float32 _sx, float32 _ray_dir_x, float32 _ray_dir_y, float32* _ix, float32* _iy, int8* _intersect)
{
	// Control segment.
	float32 a1 = _sy1 - _sy0;
	float32 c1 = a1 * _sx;

	// Ray. Player (x,y) position is the ray origin.
	float32 c2 = _ray_dir_x * PL_player.x + _ray_dir_y * PL_player.y;

	float32 det = a1 * _ray_dir_y;

	// check if lines are pararell
	if (det < 0.001f && det > -0.001f)
	{
		// SEGMENT and RAY are pararell - return 2.
		*_intersect = 2;
	}
	else
	{
		// lines are not pararell - calculate intersection point
		float32 tmp_y = (a1 * c2 - _ray_dir_x * c1) / det;

		float32 min_y, max_y;

		if (_sy0 > _sy1)
		{
			min_y = _sy1 - 0.0001f;
			max_y = _sy0 + 0.0001f;
		}
		else
		{
			min_y = _sy0 - 0.0001f;
			max_y = _sy1 + 0.0001f;
		}

		if ((tmp_y >= min_y) && (tmp_y <= max_y))
		{
			// Intersection point is inside the SEGMENT - return 0.
			*_intersect = 0;

			*_ix = _sx;
			*_iy = tmp_y;
		}
		else
		{
			// Intersection point is outside the SEGMENT - return 1.
			*_intersect = 1;
		}
	}
}

static void		GR_Wall_Box_Top_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal (top) line.
	int8	intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and horizontal line.
	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 x_len = cell->wall_vertex[1][0] - cell->wall_vertex[0][0];
		float32 wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Bottom_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];


		float32 x_len = cell->wall_vertex[1][0] - cell->wall_vertex[0][0];
		float32 wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Left_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Right_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}

static void		GR_Wall_Box_Fourside_Bottom_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 x_len = cell->wall_vertex[1][0] - cell->wall_vertex[0][0];
		float32 wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
		//	float32 wall_hit_x = (cell->wall_vertex[1][0]-ix);
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Fourside_Left_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Fourside_Right_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}

static void		GR_Wall_Box_Short_Top_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start += (int32)(original_line_height__f * cell->starting_height + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end -= (int32)(original_line_height__f * (1.0f - cell->starting_height - cell->height) + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 x_len = cell->wall_vertex[1][0] - cell->wall_vertex[0][0];
		float32 wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		float top_fr = 0.5f - cell->starting_height;
		float end_fr = cell->starting_height + cell->height - 0.5f;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start - (original_line_height__f * (end_fr - top_fr) / 2.0f) + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter.
		GR_wall_column_2nd_pass_count++;

		// Set solid hit to 0 - because we want the ray to move further.
		// The doors in this state (moving) will be render in second pass.
		*_is_solid_hit = 0;
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Short_Bottom_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;


		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start += (int32)(original_line_height__f * cell->starting_height + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end -= (int32)(original_line_height__f * (1.0f - cell->starting_height - cell->height) + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);


		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];


		float32 x_len = cell->wall_vertex[1][0] - cell->wall_vertex[0][0];
		float32 wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
		
		float top_fr = 0.5f - cell->starting_height;
		float end_fr = cell->starting_height + cell->height-0.5f;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start   - (original_line_height__f*(end_fr-top_fr)/2.0f) + line_height__div2__f - GR_render_height__div2  - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);



		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter.
		GR_wall_column_2nd_pass_count++;

		// Set solid hit to 0 - because we want the ray to move further.
		// The doors in this state (moving) will be render in second pass.
		*_is_solid_hit = 0;
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Short_Left_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start += (int32)(original_line_height__f * cell->starting_height + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end -= (int32)(original_line_height__f * (1.0f - cell->starting_height - cell->height) + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		tex_x = tmp_stripe->tex_size - tex_x - 1;
		tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;


		float top_fr = 0.5f - cell->starting_height;
		float end_fr = cell->starting_height + cell->height - 0.5f;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start - (original_line_height__f * (end_fr - top_fr) / 2.0f) + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter.
		GR_wall_column_2nd_pass_count++;

		// Set solid hit to 0 - because we want the ray to move further.
		// The doors in this state (moving) will be render in second pass.
		*_is_solid_hit = 0;
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Wall_Box_Short_Right_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Will hol the intersection result between a wall line and a ray.
	int8 intersect_result;

	// Will hold the intersection point values if found.
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start += (int32)(original_line_height__f * cell->starting_height + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end -= (int32)(original_line_height__f * (1.0f - cell->starting_height - cell->height) + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 y_len = cell->wall_vertex[1][1] - cell->wall_vertex[0][1];
		float32 wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		float top_fr = 0.5f - cell->starting_height;
		float end_fr = cell->starting_height + cell->height - 0.5f;

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start - (original_line_height__f * (end_fr - top_fr) / 2.0f) + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);


		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter.
		GR_wall_column_2nd_pass_count++;

		// Set solid hit to 0 - because we want the ray to move further.
		// The doors in this state (moving) will be render in second pass.
		*_is_solid_hit = 0;
	}
	else
	{
		*_is_solid_hit = 0;
	}
}

static void		GR_Door_Thick_Horizontal_Top_Ray_Intersection_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// For PUSH DOORS. Test the nearest door distance condition and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = ix - cell->wall_vertex[0][0];
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);




		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Horizontal_Top_Ray_Intersection_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;
		if (wall_distance <= 0.0f) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		// Lets get original values - before they will be modyfied by cell height.
		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		// Get difference between the lines.
		float32 lh_difference = original_line_height__f - line_height__f;

		// Calculate where wall starts and ends on screen.
		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__f - original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		// Use original line height to get to correct wall stripe pointer.
		sGR_Wall_Stripe* tmp = &GR_wall_stripe[original_line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = ix - cell->wall_vertex[0][0];
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		// Adding 1.0f for array safety, adding 0.5f for int rounding. (= 1.5f)
		int32 tex_y_start = (int32)(wall_draw_start + original_line_height__div2__f + lh_difference - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);
	
		int32 tex_y_start_lm = (int32)(wall_draw_start + original_line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[original_line_height][tex_y_start_lm];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter. Check limit because sometimes the counter can pass screen size which is buffer limit.
		GR_wall_column_2nd_pass_count++;
	}

	// Set solid hit to 0 - because we want the ray to move further.
	// The doors in this state (moving) will be render in second pass.
	*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Horizontal_Bottom_Ray_Intersection_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// For PUSH DOORS. Test the nearest door distance condition and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];

		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = cell->wall_vertex[1][0] - ix;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);


		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Horizontal_Bottom_Ray_Intersection_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;
		if (wall_distance <= 0.0f) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		// Lets get original values - before they will be modyfied by cell height.
		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		// Get difference between the lines.
		float32 lh_difference = original_line_height__f - line_height__f;

		// Calculate where wall starts and ends on screen.
		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__f - original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		// Use original line height to get to correct wall stripe pointer.
		sGR_Wall_Stripe* tmp = &GR_wall_stripe[original_line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = cell->wall_vertex[1][0] - ix;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		// Adding 1.0f for array safety, adding 0.5f for int rounding. (= 1.5f)
		int32 tex_y_start = (int32)(wall_draw_start + original_line_height__div2__f + lh_difference - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);
		
		
		int32 tex_y_start_lm = (int32)(wall_draw_start + original_line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[original_line_height][tex_y_start_lm];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter. Check limit because sometimes the counter can pass screen size which is buffer limit.
		GR_wall_column_2nd_pass_count++;
	}

	// Set solid hit to 0 - because we want the ray to move further.
	// The doors in this state (moving) will be render in second pass.
	*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Vertical_Left_Ray_Intersection_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// For PUSH DOORS. Test the nearest door distance condition and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = cell->wall_vertex[1][1] - iy;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);




		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Vertical_Left_Ray_Intersection_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		if (wall_distance <= 0.0f) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		// Lets get original values - before they will be modyfied by cell height.
		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		// Get difference between the lines.
		float32 lh_difference = original_line_height__f - line_height__f;

		// Calculate where wall starts and ends on screen.
		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__f - original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		// Use original line height to get to correct wall stripe pointer.
		sGR_Wall_Stripe* tmp = &GR_wall_stripe[original_line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = cell->wall_vertex[1][1] - iy;
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		// Adding 1.0f for array safety, adding 0.5f for int rounding. (= 1.5f)
		int32 tex_y_start = (int32)(wall_draw_start + original_line_height__div2__f + lh_difference - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		int32 tex_y_start_lm = (int32)(wall_draw_start + original_line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[original_line_height][tex_y_start_lm];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter. Check limit because sometimes the counter can pass screen size which is buffer limit.
		GR_wall_column_2nd_pass_count++;
	}

	// Set solid hit to 0 - because we want the ray to move further.
	// The doors in this state (moving) will be render in second pass.
	*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Vertical_Right_Ray_Intersection_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// For PUSH DOORS. Test the nearest door distance condition and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];

		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = iy - cell->wall_vertex[0][1];
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);




		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Door_Thick_Vertical_Right_Ray_Intersection_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		if (wall_distance <= 0.0f) return;

		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		// Lets get original values - before they will be modyfied by cell height.
		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		// Get difference between the lines.
		float32 lh_difference = original_line_height__f - line_height__f;

		// Calculate where wall starts and ends on screen.
		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__f - original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		// Use original line height to get to correct wall stripe pointer.
		sGR_Wall_Stripe* tmp = &GR_wall_stripe[original_line_height];


		// Convert X intersection point into X tex coord.
		float32 wall_hit_x = iy - cell->wall_vertex[0][1];
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));

		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		// Adding 1.0f for array safety, adding 0.5f for int rounding. (= 1.5f)
		int32 tex_y_start = (int32)(wall_draw_start + original_line_height__div2__f + lh_difference - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);


		int32 tex_y_start_lm = (int32)(wall_draw_start + original_line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[original_line_height][tex_y_start_lm];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter. Check limit because sometimes the counter can pass screen size which is buffer limit.
		GR_wall_column_2nd_pass_count++;
	}

	// Set solid hit to 0 - because we want the ray to move further.
	// The doors in this state (moving) will be render in second pass.
	*_is_solid_hit = 0;
}

static void		GR_Raycast_Door_Thin_Oblique_Closed(int32 _ray_x, int32 _hit_map_id, int8 _what_side_hit, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[0][1], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance;

		if (_what_side_hit == 0)	wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		else						wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;

		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// For PUSH DOORS. Test the nearest door distance condition and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}


		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);


		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];


		float32 wall_hit_x;

		float x_len = fabsf(cell->wall_vertex[0][0] - cell->wall_vertex[1][0]);
		float y_len = fabsf(cell->wall_vertex[0][1] - cell->wall_vertex[1][1]);

		if (x_len > y_len)
		{
			if (ix > cell->wall_vertex[0][0])	wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
			else								wall_hit_x = (cell->wall_vertex[0][0] - ix) / x_len;
		}
		else
		{
			if (iy > cell->wall_vertex[0][1])	wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
			else								wall_hit_x = (cell->wall_vertex[0][1] - iy) / y_len;
		}

		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);




		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);


	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Raycast_Door_Thin_Oblique_Moving(int32 _ray_x, int32 _hit_map_id, int8 _what_side_hit, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8 intersect_result;
	float32 ix, iy;

	GR_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[0][1], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance;

		if (_what_side_hit == 0)	wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		else						wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;

		if (wall_distance <= 0.0f) return;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List
		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		// Lets get original values - before they will be modyfied by cell height.
		float32 original_line_height__f = GR_render_height * wall_distance__1div;
		float32 original_line_height__div2__f = original_line_height__f / 2.0f;

		int32 original_line_height = (int32)(original_line_height__f + 0.5f);
		original_line_height = MA_MIN_2(original_line_height, GR_max_wall_stripe_lineheight__sub1);

		// Calculate current line height - that is modified by cell height.
		float32 line_height__f = GR_render_height * wall_distance__1div * cell->height;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		// Get difference between the lines.
		float32 lh_difference = original_line_height__f - line_height__f;

		// Calculate where wall starts and ends on screen.
		int32 wall_draw_start = (int32)(-original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__f - original_line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		// Use original line height to get to correct wall stripe pointer.
		sGR_Wall_Stripe* tmp = &GR_wall_stripe[original_line_height];


		float32 wall_hit_x;

		float x_len = fabsf(cell->wall_vertex[0][0] - cell->wall_vertex[1][0]);
		float y_len = fabsf(cell->wall_vertex[0][1] - cell->wall_vertex[1][1]);

		if (x_len > y_len)
		{
			if (ix > cell->wall_vertex[0][0])	wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
			else								wall_hit_x = (cell->wall_vertex[0][0] - ix) / x_len;
		}
		else
		{
			if (iy > cell->wall_vertex[0][1])	wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
			else								wall_hit_x = (cell->wall_vertex[0][1] - iy) / y_len;
		}

		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));


		// Lets calculate starting Y coord of wall that is mapped on texture.
		// Adding 1.0f for array safety, adding 0.5f for int rounding. (= 1.5f)
		int32 tex_y_start = (int32)(wall_draw_start + original_line_height__div2__f + lh_difference - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		int32 tex_y_start_lm = (int32)(wall_draw_start + original_line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[original_line_height][tex_y_start_lm];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);




		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;


		// Lets remember theese values per ray_x - we gonna need them for wall rendering.
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].wall_draw_height = wall_draw_height;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].lightmap__READY = lightmap__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture__READY = texture__READY;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_y_pos = tex_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].texture_intensity = texture_intensity;
		GR_wall_column_2nd_pass[GR_wall_column_2nd_pass_count].output_buffer_32__ptr = output_buffer_32__ptr;

		// Add this slice/ray to second pass counter.
		GR_wall_column_2nd_pass_count++;

		// Set solid hit to 0 - because we want the ray to move further.
		// The doors in this state (moving) will be render in second pass.
		*_is_solid_hit = 0;
	}
	else
		*_is_solid_hit = 0;
}

static void		GR_Door_Box_Top_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal (bottom) line.
	int8 intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and horizontal line.
	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// Test the nearest door distance condition 
		// and get current cell ID if distance is shorter.
		if (cell->cell_action == LV_A_PUSH && wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 wall_hit_x, wall_hit_x_lm;
		int32 tex_x, tex_x_lightmap;

		if (cell->cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT)
		{
			wall_hit_x = (cell->wall_vertex[1][0] - ix);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
			tex_x = tmp_stripe->tex_size - tex_x - 1;

			wall_hit_x_lm = (ix + cell->wall_vertex[0][0]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
		}
		else
		{
			wall_hit_x = (ix - cell->wall_vertex[0][0]);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));

			wall_hit_x_lm = (ix + cell->wall_vertex[1][0]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
		}

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Door_Box_Fourside_Bottom_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	int8 intersect_result;
	float32 ix, iy;

	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * ray_dir_y__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		GR_VCL_Add_Cell(_hit_map_id);

		// Test the nearest door distance condition 
		// and get current cell ID if distance is shorter.
		if (wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 wall_hit_x, wall_hit_x_lm;
		int32 tex_x, tex_x_lightmap;

		if (cell->cell_type == LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT)
		{
			wall_hit_x = (cell->wall_vertex[1][0] - ix);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));

			wall_hit_x_lm = (ix + cell->wall_vertex[0][0]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
		}
		else
		{
			wall_hit_x = (ix - cell->wall_vertex[0][0]);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
			tex_x = tmp_stripe->tex_size - tex_x - 1;

			wall_hit_x_lm = (ix + cell->wall_vertex[1][0]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
		}

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Door_Box_Fourside_Left_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and vertical (left) line.
	int8 intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and vertical line.
	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// Test the nearest door distance condition 
		// and get current cell ID if distance is shorter.
		if (wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 wall_hit_x, wall_hit_x_lm;
		int32 tex_x, tex_x_lightmap;

		if (cell->cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)
		{
			wall_hit_x = (cell->wall_vertex[1][1] - iy);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));

			wall_hit_x_lm = (iy + cell->wall_vertex[0][1]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
		}
		else
		{
			wall_hit_x = (iy - cell->wall_vertex[0][1]);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
			tex_x = tmp_stripe->tex_size - tex_x - 1;

			wall_hit_x_lm = (iy + cell->wall_vertex[1][1]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;
		}

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}
static void		GR_Door_Box_Fourside_Right_Ray_Intersection(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and vertical (right) line.
	int8 intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and vertical line.
	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[1][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * ray_dir_x__1div;
		if (wall_distance <= 0) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		// Test the nearest door distance condition 
		// and get current cell ID if distance is shorter.
		if (wall_distance < GR_door_box_nearest_distance)
		{
			GR_door_box_nearest_distance = wall_distance;
			GR_door_box_nearest_id = _hit_map_id;
		}

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp_stripe = &GR_wall_stripe[line_height];

		float32 wall_hit_x, wall_hit_x_lm;
		int32 tex_x, tex_x_lightmap;

		if (cell->cell_type == LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT)
		{
			wall_hit_x = (cell->wall_vertex[1][1] - iy);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));
			tex_x = tmp_stripe->tex_size - tex_x - 1;

			wall_hit_x_lm = (iy + cell->wall_vertex[0][1]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
		}
		else
		{
			wall_hit_x = (iy - cell->wall_vertex[0][1]);
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * tmp_stripe->tex_size + 0.5f);
			tex_x = MA_MIN_2(tex_x, (tmp_stripe->tex_size - 1));

			wall_hit_x_lm = (iy + cell->wall_vertex[1][1]);
			wall_hit_x_lm -= (int32)wall_hit_x_lm;

			tex_x_lightmap = (int32)(wall_hit_x_lm * IO_LIGHTMAP_SIZE + 0.5f);
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));
		}

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp_stripe->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp_stripe->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp_stripe->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
	{
		*_is_solid_hit = 0;
	}
}

static void		GR_Raycast_Wall_Standard(int32 _ray_x, int32 _hit_map_id, int8 _what_side_hit, int8 _map_x, int8 _map_y, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, float32 _ray_dir_y__1div)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Declare variables.
	int32 wall_draw_start, wall_draw_end, line_height;
	int32 tex_x, tex_x_lightmap;
	float32 wall_distance, wall_distance__1div;
	float32 line_height__div2__f;
	sGR_Wall_Stripe* wall_stripe_tex__ptr;
	u_int8* lightmap;

	// Threat every side-case separately - to avoid multiple checkin the same conditions.
	// It's REDUNDANT but might work a bit faster.

	if (_what_side_hit)
	{
		if (_step_y > 0)
		{
			wall_distance = (_map_y - PL_player.y) * _ray_dir_y__1div;
			wall_distance__1div = 1.0f / wall_distance;


			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------


			float32 wall_hit_x = PL_player.x + wall_distance * _ray_dir_x;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			// Lightmap texture is different for each side.
			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);
		}
		else
		{
			wall_distance = (_map_y - PL_player.y + 1) * _ray_dir_y__1div;
			wall_distance__1div = 1.0f / wall_distance;


			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------


			float32 wall_hit_x = PL_player.x + wall_distance * _ray_dir_x;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			tex_x = wall_stripe_tex__ptr->tex_size - tex_x - 1;
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

			// Lightmap texture is different for each side.
			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);
		}
	}
	else
	{
		if (_step_x > 0)
		{
			wall_distance = (_map_x - PL_player.x) * _ray_dir_x__1div;
			wall_distance__1div = 1.0f / wall_distance;


			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------


			float32 wall_hit_x = PL_player.y + wall_distance * _ray_dir_y;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			tex_x = GR_wall_stripe[line_height].tex_size - tex_x - 1;
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

			// Lightmap texture is different for each side.
			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);
		}
		else
		{
			wall_distance = (_map_x - PL_player.x + 1) * _ray_dir_x__1div;
			wall_distance__1div = 1.0f / wall_distance;


			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------


			float32 wall_hit_x = PL_player.y + wall_distance * _ray_dir_y;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			// Lightmap texture is different for each side.
			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);
		}
	}

	// Lets calculate starting Y coord of wall that is mapped on texture.
	int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

	// These pointers will hold precalculated starting position of Y coord in texture
	// for lightmap 32x32 px - can be set now.
	u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

	// These pointers will hold precalculated starting position of Y coord in texture
	// for mipmaped texture - depends on distance...
	u_int8* tex_y_pos = &wall_stripe_tex__ptr->wall_stripe__ptr[tex_y_start];

	u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
	u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

	// Move texture pointer to start of correct mipmap.
	texture += wall_stripe_tex__ptr->mip_map_offset;

	// Will hold pointer that are already shifted to correct position.
	// Before entering main rendering loop we can pull multiplaying by texture height out.
	u_int8* texture__READY = &texture[tex_x << wall_stripe_tex__ptr->tex_size_bitshift];
	u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

	// This is for performance to avoid multiplication in inner loop.
	int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
	u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

	// Calculate height to be drawn and to be checked against 0 in loop.
	int32 wall_draw_height = wall_draw_end - wall_draw_start;

	// Put informations into 1st pass render stucture. It will be used for rendering.
	GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
	GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
	GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
	GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
	GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
	GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
	GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

	// Update wall min/max values.
	GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
	GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
}
static void		GR_Raycast_Wall_Foursided(int32 _ray_x, int32 _hit_map_id, int8 _what_side_hit, int8 _map_x, int8 _map_y, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, float32 _ray_dir_y__1div)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Declare variables.
	int32 wall_draw_start, wall_draw_end, line_height;
	int32 tex_x, tex_x_lightmap;
	float32 wall_distance, wall_distance__1div;
	float32 line_height__div2__f;

	sGR_Wall_Stripe* wall_stripe_tex__ptr;
	u_int8* lightmap;
	u_int8* texture;
	u_int32* texture_intensity;

	// Threat every side-case separately - to avoid multiple checkin the same conditions.
	// It's REDUNDANT but might work a bit faster.
	if (_what_side_hit)
	{
		if (_step_y > 0)
		{
			wall_distance = (_map_y - PL_player.y) * _ray_dir_y__1div;
			wall_distance__1div = 1.0f / wall_distance;

			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------

			float32 wall_hit_x = PL_player.x + wall_distance * _ray_dir_x;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			// Texture and lightmap texture is different for each side.
			texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
			texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);
		}
		else
		{
			wall_distance = (_map_y - PL_player.y + 1) * _ray_dir_y__1div;
			wall_distance__1div = 1.0f / wall_distance;

			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------

			float32 wall_hit_x = PL_player.x + wall_distance * _ray_dir_x;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			tex_x = wall_stripe_tex__ptr->tex_size - tex_x - 1;
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

			// Texture and lightmap texture is different for each side.
			texture = LV_wall_textures__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
			texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[2] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[2] * IO_LIGHTMAP_BYTE_SIZE);
		}
	}
	else
	{
		if (_step_x > 0)
		{
			wall_distance = (_map_x - PL_player.x) * _ray_dir_x__1div;
			wall_distance__1div = 1.0f / wall_distance;

			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------

			float32 wall_hit_x = PL_player.y + wall_distance * _ray_dir_y;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			tex_x = wall_stripe_tex__ptr->tex_size - tex_x - 1;
			tex_x_lightmap = (IO_LIGHTMAP_SIZE - 1) - tex_x_lightmap;

			// Texture and lightmap texture is different for each side.
			texture = LV_wall_textures__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
			texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[3] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[3] * IO_LIGHTMAP_BYTE_SIZE);
		}
		else
		{
			wall_distance = (_map_x - PL_player.x + 1) * _ray_dir_x__1div;
			wall_distance__1div = 1.0f / wall_distance;

			// -- Common section for all sides --

				float32 line_height__f = GR_render_height * wall_distance__1div;
				line_height__div2__f = line_height__f / 2.0f;

				line_height = (int32)(line_height__f + 0.5f);
				line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

				wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_start = MA_MAX_2(wall_draw_start, 0);

				wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
				wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

				wall_stripe_tex__ptr = &GR_wall_stripe[line_height];

			// ------------------------------------

			float32 wall_hit_x = PL_player.y + wall_distance * _ray_dir_y;
			wall_hit_x -= (int32)wall_hit_x;

			tex_x = (int32)(wall_hit_x * wall_stripe_tex__ptr->tex_size + 0.5f);
			tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

			tex_x = MA_MIN_2(tex_x, (wall_stripe_tex__ptr->tex_size - 1));
			tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

			// Texture and lightmap texture is different for each side.
			texture = LV_wall_textures__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
			texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[1] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

			lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[1] * IO_LIGHTMAP_BYTE_SIZE);
		}
	}

	// Lets calculate starting Y coord of wall that is mapped on texture.
	int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

	// These pointers will hold precalculated starting position of Y coord in texture
	// for lightmap 32x32 px - can be set now.
	u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

	// These pointers will hold precalculated starting position of Y coord in texture
	// for mipmaped texture - depends on distance...
	u_int8* tex_y_pos = &wall_stripe_tex__ptr->wall_stripe__ptr[tex_y_start];

	// Move texture pointer to start of correct mipmap.
	texture += wall_stripe_tex__ptr->mip_map_offset;

	// Will hold pointer that are already shifted to correct position.
	// Before entering main rendering loop we can pull multiplaying by texture height out.
	u_int8* texture__READY = &texture[tex_x << wall_stripe_tex__ptr->tex_size_bitshift];
	u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

	// This is for performance to avoid multiplication in inner loop.
	int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
	u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

	// Calculate height to be drawn and to be checked against 0 in loop.
	int32 wall_draw_height = wall_draw_end - wall_draw_start;

	// Put informations into 1st pass render stucture. It will be used for rendering.
	GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
	GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
	GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
	GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
	GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
	GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
	GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

	// Update wall min/max values.
	GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
	GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
}
static void		GR_Raycast_Wall_Thin_Horizontal(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and horizontal line.
	int8	intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and horizontal line.
	GR_Horizontal_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[1][0], cell->wall_vertex[0][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];

		float32 wall_hit_x = fabsf(cell->wall_vertex[0][0] - ix);
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Raycast_Wall_Thin_Vertical(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and vertical line.
	int8 intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and vertical line.
	GR_Vertical_Segment_vs_Ray_Intersection(cell->wall_vertex[0][1], cell->wall_vertex[1][1], cell->wall_vertex[0][0], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];

		float32 wall_hit_x = fabsf(iy - cell->wall_vertex[0][1]);
		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];


		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];


		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Raycast_Wall_Thin_Oblique(int32 _ray_x, int32 _hit_map_id, int8 _what_side_hit, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get current cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Intersection result and intersection points with ray_x and oblique line.
	int8 intersect_result;
	float32 ix, iy;

	// Lets test intersection with ray_x and oblique line.
	GR_Segment_vs_Ray_Intersection(cell->wall_vertex[0][0], cell->wall_vertex[0][1], cell->wall_vertex[1][0], cell->wall_vertex[1][1], -_ray_dir_y, _ray_dir_x, &ix, &iy, &intersect_result);

	if (intersect_result == 0)
	{
		float32 wall_distance;

		if (_what_side_hit == 0)	wall_distance = (ix - PL_player.x) * _ray_dir_x__1div;
		else						wall_distance = (iy - PL_player.y) * _ray_dir_y__1div;

		if (wall_distance <= 0.0f) return;

		*_is_solid_hit = 1;

		// Because its not full-cell standard wall the floor and ceil might be visible.
		// Add floor and ceil to the Visible Cell List.
		GR_VCL_Add_Cell(_hit_map_id);

		float32 wall_distance__1div = 1.0f / wall_distance;

		float32 line_height__f = GR_render_height * wall_distance__1div;
		float32 line_height__div2__f = line_height__f / 2.0f;

		int32 line_height = (int32)(line_height__f + 0.5f);
		line_height = MA_MIN_2(line_height, GR_max_wall_stripe_lineheight__sub1);

		int32 wall_draw_start = (int32)(-line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_start = MA_MAX_2(wall_draw_start, 0);

		int32 wall_draw_end = (int32)(line_height__div2__f + GR_render_height__div2 + PL_player.pitch + PL_player.z * wall_distance__1div + 0.5f);
		wall_draw_end = MA_MIN_2((wall_draw_end - 1), GR_render_height);

		sGR_Wall_Stripe* tmp = &GR_wall_stripe[line_height];

		float32 x_len = fabsf(cell->wall_vertex[0][0] - cell->wall_vertex[1][0]);
		float32 y_len = fabsf(cell->wall_vertex[0][1] - cell->wall_vertex[1][1]);

		float32 wall_hit_x;

		if (x_len > y_len)
		{
			if (ix > cell->wall_vertex[0][0])	wall_hit_x = (ix - cell->wall_vertex[0][0]) / x_len;
			else								wall_hit_x = (cell->wall_vertex[0][0] - ix) / x_len;
		}
		else
		{
			if (iy > cell->wall_vertex[0][1])	wall_hit_x = (iy - cell->wall_vertex[0][1]) / y_len;
			else								wall_hit_x = (cell->wall_vertex[0][1] - iy) / y_len;
		}

		wall_hit_x -= (int32)wall_hit_x;

		int32 tex_x = (int32)(wall_hit_x * tmp->tex_size + 0.5f);
		int32 tex_x_lightmap = (int32)(wall_hit_x * IO_LIGHTMAP_SIZE + 0.5f);

		// Check ranges, use MIN functino instead of IF.
		tex_x = MA_MIN_2(tex_x, (tmp->tex_size - 1));
		tex_x_lightmap = MA_MIN_2(tex_x_lightmap, (IO_LIGHTMAP_SIZE - 1));

		// Lets calculate starting Y coord of wall that is mapped on texture.
		int32 tex_y_start = (int32)(wall_draw_start + line_height__div2__f - GR_render_height__div2 - PL_player.pitch - PL_player.z * wall_distance__1div + 0.5f);

		// These pointers will hold precalculated starting position of Y coord in texture
		// for lightmap 32x32 px - can be set now.
		u_int8* tex_32lm_y_pos = &GR_wall_stripe_lightmap[line_height][tex_y_start];

		// These pointers will hold precalculated starting position of Y coord in texture
		// for mipmaped texture - depends on distance...
		u_int8* tex_y_pos = &tmp->wall_stripe__ptr[tex_y_start];

		u_int8* texture = LV_wall_textures__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
		u_int32* texture_intensity = LV_wall_textures_colors__PTR + (cell->wall_texture_id[0] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

		u_int8* lightmap = LV_lightmaps__PTR + (cell->wall_lightmap_id[0] * IO_LIGHTMAP_BYTE_SIZE);

		// Move texture pointer to start of correct mipmap.
		texture += tmp->mip_map_offset;

		// Will hold pointer that are already shifted to correct position.
		// Before entering main rendering loop we can pull multiplaying by texture height out.
		u_int8* texture__READY = &texture[tex_x << tmp->tex_size_bitshift];
		u_int8* lightmap__READY = &lightmap[tex_x_lightmap << 5];

		// This is for performance to avoid multiplication in inner loop.
		int32 wall_draw_start_in_buffer = _ray_x + wall_draw_start * GR_render_width;
		u_int32* output_buffer_32__ptr = IO_prefs.output_buffer_32 + wall_draw_start_in_buffer;

		// Calculate height to be drawn and to be checked against 0 in loop.
		int32 wall_draw_height = wall_draw_end - wall_draw_start;

		// Put informations into 1st pass render stucture. It will be used for rendering.
		GR_wall_column_1st_pass[_ray_x].wall_draw_height = wall_draw_height;
		GR_wall_column_1st_pass[_ray_x].lightmap__READY = lightmap__READY;
		GR_wall_column_1st_pass[_ray_x].texture__READY = texture__READY;
		GR_wall_column_1st_pass[_ray_x].tex_y_pos = tex_y_pos;
		GR_wall_column_1st_pass[_ray_x].tex_32lm_y_pos = tex_32lm_y_pos;
		GR_wall_column_1st_pass[_ray_x].texture_intensity = texture_intensity;
		GR_wall_column_1st_pass[_ray_x].output_buffer_32__ptr = output_buffer_32__ptr;

		// Update wall min/max values.
		GR_wall_end_min = MA_MIN_2(wall_draw_end, GR_wall_end_min);
		GR_wall_start_max = MA_MAX_2(wall_draw_start, GR_wall_start_max);
	}
	else
		*_is_solid_hit = 0;
}
static void		GR_Raycast_Wall_Box(int32 _ray_x, int8 _what_side_hit, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, float32 ray_dir_y__1div, int32 _hit_map_id, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Test the hit side first.
	if (_what_side_hit)
	{
		// We are hitting TOP/BOTTOM side.
		// Find out is it a TOP or BOTTOM using step_y.

		if (_step_y > 0)
		{
			// We are hitting TOP side.

			// Check if Player X position is between TOP side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting BOTTOM side.

			// Check if Player X position is between BOTTOM side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
	}
	else
	{
		// We are hitting LEFT/RIGHT side.
		// Find out is it a LEFT or RIGHT side.

		if (_step_x > 0)
		{
			// We are hitting LEFT side.
			// Check if Player X position is between BOTTOM side X vertices.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting RIGHT side.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
	}
}
static void		GR_Raycast_Wall_Box_Fourside(int32 _ray_x, int8 _what_side_hit, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, float32 ray_dir_y__1div, int32 _hit_map_id, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Test the hit side first.
	if (_what_side_hit)
	{
		// We are hitting TOP/BOTTOM side.
		// Find out is it a TOP or BOTTOM using step_y.

		if (_step_y > 0)
		{
			// We are hitting TOP side.

			// Check if Player X position is between TOP side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting BOTTOM side.

			// Check if Player X position is between BOTTOM side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
	}
	else
	{
		// We are hitting LEFT/RIGHT side.
		// Find out is it a LEFT or RIGHT side.

		if (_step_x > 0)
		{
			// We are hitting LEFT side.
			// Check if Player X position is between BOTTOM side X vertices.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting RIGHT side.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}

	}
}
static void		GR_Raycast_Wall_Box_Short(int32 _ray_x, int8 _what_side_hit, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, float32 ray_dir_y__1div, int32 _hit_map_id, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Test the hit side first.
	if (_what_side_hit)
	{
		// We are hitting TOP/BOTTOM side.
		// Find out is it a TOP or BOTTOM using step_y.

		if (_step_y > 0)
		{
			// We are hitting TOP side.

			// Check if Player X position is between TOP side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Short_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Short_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Short_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting BOTTOM side.

			// Check if Player X position is between BOTTOM side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Short_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				GR_Wall_Box_Short_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				GR_Wall_Box_Short_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
	}
	else
	{
		// We are hitting LEFT/RIGHT side.
		// Find out is it a LEFT or RIGHT side.

		if (_step_x > 0)
		{
			// We are hitting LEFT side.
			// Check if Player X position is between BOTTOM side X vertices.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Short_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Short_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Short_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting RIGHT side.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.
				GR_Wall_Box_Short_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				GR_Wall_Box_Short_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				GR_Wall_Box_Short_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				if (*_is_solid_hit == 0)
					GR_Wall_Box_Short_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
	}
}

static void		GR_Raycast_Door_Thick_Horizontal_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	if (PL_player.y < cell->wall_vertex[0][1] + 0.05f)
	{
		// We are hitting TOP side.
		GR_Door_Thick_Horizontal_Top_Ray_Intersection_Closed(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_y__1div, _is_solid_hit);
	}
	else
	{
		// We are hitting BOTTOM side.
		GR_Door_Thick_Horizontal_Bottom_Ray_Intersection_Closed(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_y__1div, _is_solid_hit);
	}
}
static void		GR_Raycast_Door_Thick_Horizontal_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_y__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	if (PL_player.y < cell->wall_vertex[0][1] + 0.05f)
	{
		// We are hitting TOP side.
		GR_Door_Thick_Horizontal_Top_Ray_Intersection_Moving(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_y__1div, _is_solid_hit);
	}
	else
	{
		// We are hitting BOTTOM side.
		GR_Door_Thick_Horizontal_Bottom_Ray_Intersection_Moving(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_y__1div, _is_solid_hit);
	}
}
static void		GR_Raycast_Door_Thick_Vertical_Closed(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	if (PL_player.x < cell->wall_vertex[0][0] + 0.05f)
	{
		// We are hitting LEFT side.
		GR_Door_Thick_Vertical_Left_Ray_Intersection_Closed(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_x__1div, _is_solid_hit);
	}
	else
	{
		// We are hitting RIGHT side.
		GR_Door_Thick_Vertical_Right_Ray_Intersection_Closed(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_x__1div, _is_solid_hit);
	}
}
static void		GR_Raycast_Door_Thick_Vertical_Moving(int32 _ray_x, int32 _hit_map_id, float32 _ray_dir_x, float32 _ray_dir_y, float32 _ray_dir_x__1div, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	if (PL_player.x < cell->wall_vertex[0][0] + 0.05f)
	{
		// We are hitting LEFT side.
		GR_Door_Thick_Vertical_Left_Ray_Intersection_Moving(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_x__1div, _is_solid_hit);
	}
	else
	{
		// We are hitting RIGHT side.
		GR_Door_Thick_Vertical_Right_Ray_Intersection_Moving(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, _ray_dir_x__1div, _is_solid_hit);
	}
}
static void		GR_Raycast_Door_Box_Fourside_Horizontal(int32 _ray_x, int8 _what_side_hit, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, float32 ray_dir_y__1div, int32 _hit_map_id, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Because we are dealing with HORIZONTAL DOORS, we want the sides of the doors (left/right)
	// to have textures strechted to its size as well as in the WALLS case. So lets use WALLS functions here for every left/right render.

	// Test the hit side first.
	if (_what_side_hit)
	{
		// We are hitting TOP/BOTTOM side.
		// Find out is it a TOP or BOTTOM using step_y.

		if (_step_y > 0)
		{
			// We are hitting TOP side.

			// Check if Player X position is between TOP side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Door render function here.
				GR_Door_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				// Door render function here.
				GR_Door_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				// Door render function here.
				GR_Door_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting BOTTOM side.

			// Check if Player X position is between BOTTOM side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Door render function here.
				GR_Door_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
	}
	else
	{
		// We are hitting LEFT/RIGHT side.
		// Find out is it a LEFT or RIGHT side.

		if (_step_x > 0)
		{
			// We are hitting LEFT side.
			// Check if Player X position is between BOTTOM side X vertices.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Wall render function here.
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting RIGHT side.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Wall render function here.
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
	}
}
static void		GR_Raycast_Door_Box_Fourside_Vertical(int32 _ray_x, int8 _what_side_hit, int8 _step_x, int8 _step_y, float32 _ray_dir_x, float32 _ray_dir_y, float32 ray_dir_x__1div, float32 ray_dir_y__1div, int32 _hit_map_id, int8* _is_solid_hit)
{
	// Get hit cell pointer.
	sLV_Cell* cell = &LV_map[_hit_map_id];

	// Because we are dealing with VERTICAL DOORS, we want the sides of the doors (top/bottom)
	// to have textures strechted to its size as well as in the WALLS case. So lets use WALLS functions here for every top/bottom render.

	// Test the hit side first.
	if (_what_side_hit)
	{
		// We are hitting TOP/BOTTOM side.
		// Find out is it a TOP or BOTTOM using step_y.

		if (_step_y > 0)
		{
			// We are hitting TOP side.

			// Check if Player X position is between TOP side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Wall render function here.
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				// Wall render function here.
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				// Wall render function here.
				GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting BOTTOM side.

			// Check if Player X position is between BOTTOM side X vertices.
			if (PL_player.x >= cell->wall_vertex[0][0] && PL_player.x <= cell->wall_vertex[1][0])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Wall render function here.
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.x < cell->wall_vertex[0][0])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.x > cell->wall_vertex[1][0])
			{
				// Wall render function here.
				GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				// Door render function here.
				if (*_is_solid_hit == 0)
					GR_Door_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				return;
			}
		}
	}
	else
	{
		// We are hitting LEFT/RIGHT side.
		// Find out is it a LEFT or RIGHT side.

		if (_step_x > 0)
		{
			// We are hitting LEFT side.
			// Check if Player X position is between BOTTOM side X vertices.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Door render function here.
				GR_Door_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Left_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
		else
		{
			// We are hitting RIGHT side.

			// Check if Player Y position is between LEFT side Y vertices.
			if (PL_player.y >= cell->wall_vertex[0][1] && PL_player.y <= cell->wall_vertex[1][1])
			{
				// If so, we will see BOTTOM side only - so only one intersection to check.

				// Door render function here.
				GR_Door_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);
				return;

			}
			else if (PL_player.y < cell->wall_vertex[0][1])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Top_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
			else if (PL_player.y > cell->wall_vertex[1][1])
			{
				// Door render function here.
				GR_Door_Box_Fourside_Right_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_x__1div, _is_solid_hit);

				// Wall render function here.
				if (*_is_solid_hit == 0)
					GR_Wall_Box_Fourside_Bottom_Ray_Intersection(_ray_x, _hit_map_id, _ray_dir_x, _ray_dir_y, ray_dir_y__1div, _is_solid_hit);

				return;
			}
		}
	}
}

// ----------------------------------------------------------
// --- GAME RENDER - WALLS / DOORS - PUBLIC functions definitions ---
// ----------------------------------------------------------
int32	GR_Walls_Init_Once(void)
{
	int32 mem_size;
	int32 memory_allocated = 0;

	// Init precalculated LUT containing tex y-coords for textures. Its takes care of all mipmaps from 256 to 4.
	mem_size = GR_Init_Once_Wall_Stripe_LUT();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;

	// Init precalculated LUT containing tex y-coords for 32x32 px lightmaps.
	mem_size = GR_Init_Once_Wall_Stripe_Lightmap_LUT();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;

	// Screen column structure for 1st rendering pass.
	mem_size = sizeof(sGR_Wall_Column) * GR_render_width;
	GR_wall_column_1st_pass = (sGR_Wall_Column*)malloc(mem_size);
	if (GR_wall_column_1st_pass == NULL) return 0;
	memory_allocated += mem_size;

	// Screen column structure for 2nd rendering pass.
	// The buffer for 2nd pass need to be bigger than GR_render_width, because there may be more irregular in one line.
	mem_size = sizeof(sGR_Wall_Column) * GR_render_width * 2;
	GR_wall_column_2nd_pass = (sGR_Wall_Column*)malloc(mem_size);
	if (GR_wall_column_2nd_pass == NULL) return 0;
	memory_allocated += mem_size;

	return memory_allocated;
}
void	GR_Walls_Cleanup(void)
{
	free(GR_wall_column_2nd_pass);
	free(GR_wall_column_1st_pass);
	GR_Cleanup_Wall_Stripe_Lightmap_LUT();
	GR_Cleanup_Wall_Stripe_LUT();
}

int32	GR_Doors_Get_List_Index(int32 _cell_id)
{
	// This function takes CELL_ID of door and return its position on the DOOR LIST.

	// Get number of doors im the list;
	int32 door_counter = LV_door_count;

	// Upate every door in group.
	while (door_counter--)
	{
		if (LV_door_list[door_counter] == _cell_id)
			return door_counter;
	}

	return 0;
}
void	GR_Doors_Open(void)
{
	// If "GR_door_nearest_id" is set and the "GR_door_nearest_distance" is "<" then test distance,
	// change the state of that door to "OPENING".
	if ((GR_door_box_nearest_id != 0) && (GR_door_box_nearest_distance < GR_DOOR_PUSH_DISTANCE))
	{
		if (LV_map[GR_door_box_nearest_id].cell_state == LV_S_DOOR_CLOSED)
		{
			// Get the cell group number.
			int32 cell_group = LV_map[GR_door_box_nearest_id].cell_group;

			if (cell_group == 0)
			{
				// If group == 0 (no group assigned), set the new state only for this one door cell.
				LV_map[GR_door_box_nearest_id].cell_state = LV_S_DOOR_OPENING;
			}
			else
			{
				// Otherwise, set the new state only for all door cells with this group.

				// Get number of doors in current group.
				int32 counter = LV_door_group[cell_group].count;

				// Upate every door in group.
				while (counter--)
					LV_map[LV_door_group[cell_group].id_list[counter]].cell_state = LV_S_DOOR_OPENING;
			}
		}
	}
}
void	GR_Doors_Update(void)
{
	// Reset door test variables.
	GR_door_box_nearest_distance = 100.0f;
	GR_door_box_nearest_id = 0;

	// This function go thru the list of existing doors in the map
	// and updates door values if needed.

	for (int32 i = 0; i < LV_door_count; i++)
	{
		// Get a pointer to curent door cell.
		int32 cell_id = LV_door_list[i];
		sLV_Cell* door_cell = &LV_map[cell_id];

		// If doors are not closed or not opened forever - update it.
		if (door_cell->cell_state > LV_S_DOOR_CLOSED)
		{
			switch (door_cell->cell_state)
			{
				// DOORS ARE IN PROXMITY STATE (THIN DOORS) - CHECKING IF SOMEONE IS NEAR - TO AUTO OPEN.

				case LV_S_DOOR_CLOSED_PROXMITY:
				{
					int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
					int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

					// Check Player "simplified distance".
					float32 diff_x = PL_player.x - (cell_x + 0.5f);
					float32 diff_y = PL_player.y - (cell_y + 0.5f);
					float32 player_distance = diff_x * diff_x + diff_y * diff_y;

					// If player is near - open all doors in group.
					if (player_distance < GR_DOOR_PROXMITY_DISTANCE)
					{
						// Update the state of current door.
						LV_map[cell_id].cell_state = LV_S_DOOR_OPENING;

						// Also upate every door in group.
						int32 counter = LV_door_group[door_cell->cell_group].count;

						while (counter--)
							LV_map[LV_door_group[door_cell->cell_group].id_list[counter]].cell_state = LV_S_DOOR_OPENING;
					}
				}
				break;

				// DOORS ARE OPENING

				case LV_S_DOOR_OPENING:
				{
					switch (door_cell->cell_type)
					{
						case LV_C_DOOR_THICK_HORIZONTAL:
						case LV_C_DOOR_THICK_VERTICAL:
						case LV_C_DOOR_THIN_OBLIQUE:
						{
							door_cell->height -= (IO_prefs.delta_time * GR_DOOR_UP_SPEED);

							if (door_cell->height < 0.0f)
							{
								door_cell->height = 0.0f;

								if (door_cell->cell_timer == 0)
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_FOREVER;
								}
								else
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_TEMP;
									LV_door_timer[i] = clock();
								}
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
							int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);
							
							door_cell->wall_vertex[1][0] -= IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[1][0] < (cell_x + 0.05f))
							{
								door_cell->wall_vertex[1][0] = cell_x + 0.05f;

								if (door_cell->cell_timer == 0)
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_FOREVER;
								}
								else
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_TEMP;
									LV_door_timer[i] = clock();
								}
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
							int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

							door_cell->wall_vertex[0][0] += IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[0][0] > (cell_x + 0.95f))
							{
								door_cell->wall_vertex[0][0] = cell_x + 0.95f;

								if (door_cell->cell_timer == 0)
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_FOREVER;
								}
								else
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_TEMP;
									LV_door_timer[i] = clock();
								}
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;

							door_cell->wall_vertex[1][1] -= IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[1][1] < (cell_y + 0.05f))
							{
								door_cell->wall_vertex[1][1] = cell_y + 0.05f;

								if (door_cell->cell_timer == 0)
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_FOREVER;
								}
								else
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_TEMP;
									LV_door_timer[i] = clock();
								}
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;

							door_cell->wall_vertex[0][1] += IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[0][1] > (cell_y + 0.95f))
							{
								door_cell->wall_vertex[0][1] = cell_y + 0.95f;

								if (door_cell->cell_timer == 0)
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_FOREVER;
								}
								else
								{
									door_cell->cell_state = LV_S_DOOR_OPENED_TEMP;
									LV_door_timer[i] = clock();
								}
							}
						}
						break;
					}
				}
				break;

				// DOORS TEMPORARY OPENED

				case LV_S_DOOR_OPENED_TEMP:
				{
					// If door is opened temprarly - check its timer.
					if ((clock() - LV_door_timer[i]) > GR_DOOR_CLOSE_TIMER)
						door_cell->cell_state = LV_S_DOOR_CLOSING;
				}
				break;

				// DOORS ARE CLOSING

				case LV_S_DOOR_CLOSING:
				{
					// If doors are in closing state - check PLAYERS ID CELL.
					// If PLAYER is in the same CELL as doors change their status to OPENING immiedietaly.
					// Update all doors in group.
					if (PL_player.curr_cell_id == cell_id)
					{
						// Get number of doors in current group.
						int32 counter = LV_door_group[door_cell->cell_group].count;

						// Upate every door in group.
						while (counter--)
							LV_map[LV_door_group[door_cell->cell_group].id_list[counter]].cell_state = LV_S_DOOR_OPENING;

						break;
					}

					switch (door_cell->cell_type)
					{
						case LV_C_DOOR_THICK_HORIZONTAL:
						case LV_C_DOOR_THICK_VERTICAL:
						case LV_C_DOOR_THIN_OBLIQUE:
						{
							door_cell->height += (IO_prefs.delta_time * GR_DOOR_UP_SPEED);

							if (door_cell->height > 1.0f)
							{
								door_cell->height = 1.0f;

								if (door_cell->cell_action == LV_A_PUSH)	door_cell->cell_state = LV_S_DOOR_CLOSED;
								else										door_cell->cell_state = LV_S_DOOR_CLOSED_PROXMITY;
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
							int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

							door_cell->wall_vertex[1][0] += IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[1][0] > (cell_x + 1.0f))
							{
								door_cell->wall_vertex[1][0] = cell_x + 1.0f;

								if (door_cell->cell_action == LV_A_PUSH)	door_cell->cell_state = LV_S_DOOR_CLOSED;
								else										door_cell->cell_state = LV_S_DOOR_CLOSED_PROXMITY;
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;
							int8 cell_x = cell_id - (cell_y << LV_MAP_LENGTH_BITSHIFT);

							door_cell->wall_vertex[0][0] -= IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[0][0] < cell_x)
							{
								door_cell->wall_vertex[0][0] = cell_x;

								if (door_cell->cell_action == LV_A_PUSH)	door_cell->cell_state = LV_S_DOOR_CLOSED;
								else										door_cell->cell_state = LV_S_DOOR_CLOSED_PROXMITY;
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;

							door_cell->wall_vertex[1][1] += IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[1][1] > (cell_y + 1.0f))
							{
								door_cell->wall_vertex[1][1] = cell_y + 1.0f;

								if (door_cell->cell_action == LV_A_PUSH)	door_cell->cell_state = LV_S_DOOR_CLOSED;
								else										door_cell->cell_state = LV_S_DOOR_CLOSED_PROXMITY;
							}
						}
						break;

						case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
						{
							int8 cell_y = cell_id >> LV_MAP_LENGTH_BITSHIFT;

							door_cell->wall_vertex[0][1] -= IO_prefs.delta_time * GR_DOOR_SIDE_SPEED;

							if (door_cell->wall_vertex[0][1] < cell_y)
							{
								door_cell->wall_vertex[0][1] = cell_y;

								if (door_cell->cell_action == LV_A_PUSH)	door_cell->cell_state = LV_S_DOOR_CLOSED;
								else										door_cell->cell_state = LV_S_DOOR_CLOSED_PROXMITY;
							}
						}
						break;
					}
				}
				break;
			}
		}
	}
}

void	GR_Walls_Raycast(void)
{
	// Reset visible cells list.
	GR_VCL_Reset();

	// Reset previous max and min wall parametes.
	GR_wall_end_min = GR_render_height;
	GR_wall_start_max = 0;

	// Reset 2nd pass counter;
	GR_wall_column_2nd_pass_count = 0;

	// Lets prepare some variables before entering main loop.
	float32 player_x_fraction = PL_player.x - PL_player.curr_cell_x;
	float32 player_y_fraction = PL_player.y - PL_player.curr_cell_y;

	float32 side_dist_x, side_dist_y;
	int8 step_x = 0, step_y = 0;

	// Start raycasting from right to left.
	int32 ray_x = GR_render_width;

	while (ray_x--)
	{
		float32 pp_coord_x = (ray_x << 1) * GR_render_width__1div - 1.0f;

		float32 ray_dir_x = GR_pp_dx + GR_pp_nsize_x__div2 * pp_coord_x;
		float32 ray_dir_y = GR_pp_dy + GR_pp_nsize_y__div2 * pp_coord_x;

		float32 ray_dir_x__1div = 1.0f / ray_dir_x;
		float32 ray_dir_y__1div = 1.0f / ray_dir_y;

		float32 delta_dist_x = fabsf(ray_dir_x__1div);
		float32 delta_dist_y = fabsf(ray_dir_y__1div);

		int8 map_x = PL_player.curr_cell_x;
		int8 map_y = PL_player.curr_cell_y;

		int32 prew_map_id = PL_player.curr_cell_id;

		// Is solid wall hit.
		int8 is_solid_hit = 0;

		// What side was hit: TOP-BOTTOM (==1) or LEFT-RIGHT (==0)...
		int8 what_side_hit = 0;

		// Init variables for DDA.
		if (ray_dir_x < 0.0f)
		{
			step_x = -1;
			side_dist_x = player_x_fraction * delta_dist_x;
		}
		else
		{
			step_x = 1;
			side_dist_x = (-player_x_fraction + 1.0f) * delta_dist_x;
		}

		if (ray_dir_y < 0.0f)
		{
			step_y = -1;
			side_dist_y = player_y_fraction * delta_dist_y;
		}
		else
		{
			step_y = 1;
			side_dist_y = (-player_y_fraction + 1.0f) * delta_dist_y;
		}

		// Before entering DDA (grid traversing) we must check if cell that the Player is curently in
		// has non-regular wall in it. Because DDA will start from the next cell, this cell won't be detected.
		// If non-regular was is detected the calculations are made and put in the list for further rendering
		// and we can move to the next screen column. If not, the just do the regular DDA.

		what_side_hit = !(side_dist_x < side_dist_y);


		if (LV_map[PL_player.curr_cell_id].cell_type > 0)
		{
			switch (LV_map[PL_player.curr_cell_id].cell_type)
			{
				// WALLS
				case LV_C_WALL_THIN_HORIZONTAL:
					GR_Raycast_Wall_Thin_Horizontal(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
					break;

				case LV_C_WALL_THIN_VERTICAL:
					GR_Raycast_Wall_Thin_Vertical(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
					break;

				case LV_C_WALL_THIN_OBLIQUE:
					GR_Raycast_Wall_Thin_Oblique(ray_x, PL_player.curr_cell_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
					break;

				case LV_C_WALL_BOX:
					GR_Raycast_Wall_Box(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, PL_player.curr_cell_id, &is_solid_hit);
					break;

				case LV_C_WALL_BOX_FOURSIDE:
					GR_Raycast_Wall_Box_Fourside(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, PL_player.curr_cell_id, &is_solid_hit);
					break;

				case LV_C_WALL_BOX_SHORT:
					GR_Raycast_Wall_Box_Short(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, PL_player.curr_cell_id, &is_solid_hit);
					break;

				// DOORS
					
				case LV_C_DOOR_THICK_HORIZONTAL:
				// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further. If doors are open - they are invisible.
					switch (LV_map[PL_player.curr_cell_id].cell_state)
					{
						case LV_S_DOOR_CLOSED:
						case LV_S_DOOR_CLOSED_PROXMITY:
							GR_Raycast_Door_Thick_Horizontal_Closed(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
							break;

						case LV_S_DOOR_OPENING:
						case LV_S_DOOR_CLOSING:
							GR_Raycast_Door_Thick_Horizontal_Moving(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
							break;
					}
					break;
					
				case LV_C_DOOR_THICK_VERTICAL:
				// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further. If doors are open - they are invisible.
					switch (LV_map[PL_player.curr_cell_id].cell_state)
					{
						case LV_S_DOOR_CLOSED:
						case LV_S_DOOR_CLOSED_PROXMITY:
							GR_Raycast_Door_Thick_Vertical_Closed(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
							break;

						case LV_S_DOOR_OPENING:
						case LV_S_DOOR_CLOSING:
							GR_Raycast_Door_Thick_Vertical_Moving(ray_x, PL_player.curr_cell_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
							break;
					}
					break;
					
				case LV_C_DOOR_THIN_OBLIQUE:
				// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further. If doors are open - they are invisible.
					switch (LV_map[PL_player.curr_cell_id].cell_state)
					{
						case LV_S_DOOR_CLOSED:
						case LV_S_DOOR_CLOSED_PROXMITY:
							GR_Raycast_Door_Thin_Oblique_Closed(ray_x, PL_player.curr_cell_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
							break;

						case LV_S_DOOR_OPENING:
						case LV_S_DOOR_CLOSING:
							GR_Raycast_Door_Thin_Oblique_Moving(ray_x, PL_player.curr_cell_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
							break;
					}
					break;
					
				case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
				case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
					GR_Raycast_Door_Box_Fourside_Horizontal(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, PL_player.curr_cell_id, &is_solid_hit);
					break;

				case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
				case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
					GR_Raycast_Door_Box_Fourside_Vertical(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, PL_player.curr_cell_id, &is_solid_hit);
					break;
			}
		}

		// Start DDA.
		while (is_solid_hit == 0)
		{
			// Jump to next map square: in x-direction OR in y-direction...
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				map_x += step_x;
				what_side_hit = 0;
			}
			else
			{
				side_dist_y += delta_dist_y;
				map_y += step_y;
				what_side_hit = 1;
			}

			// Add current cell. Don't add cells containing standard regular walls. They won't be visible.
			GR_VCL_Add_Cell(prew_map_id);

			// ID of map cell that was hit.
			int32 hit_map_id = map_x + (map_y << LV_MAP_LENGTH_BITSHIFT);

			// Remember it for adding to list next time.
			prew_map_id = hit_map_id;

			// If cell that we hit is a wall/solid etc.,
			// lets check the wall type first and perform proper calculations.
			if (LV_map[hit_map_id].cell_type > 0)
			{
				switch (LV_map[hit_map_id].cell_type)
				{
					// WALLS

					// Standard regular walls will always cver the whole cell. So the floor or ceiling will never visible.
					// So we can stop the DDA on that wall.
					case LV_C_WALL_STANDARD:
						is_solid_hit = 1;
						GR_Raycast_Wall_Standard(ray_x, hit_map_id, what_side_hit, map_x, map_y, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div);
						break;

					case LV_C_WALL_FOURSIDE:
						is_solid_hit = 1;
						GR_Raycast_Wall_Foursided(ray_x, hit_map_id, what_side_hit, map_x, map_y, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div);
						break;

					// Non regular walls usually doesn't cover the whole cell, so the floor and ceiling might be visible.
					// If non regular wall (segment) is missed by the ray the DDA must continue.
					case LV_C_WALL_THIN_HORIZONTAL:
						GR_Raycast_Wall_Thin_Horizontal(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
						break;

					case LV_C_WALL_THIN_VERTICAL:
						GR_Raycast_Wall_Thin_Vertical(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
						break;

					case LV_C_WALL_THIN_OBLIQUE:
						GR_Raycast_Wall_Thin_Oblique(ray_x, hit_map_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
						break;

					case LV_C_WALL_BOX:
						GR_Raycast_Wall_Box(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, hit_map_id, &is_solid_hit);
						break;

					case LV_C_WALL_BOX_FOURSIDE:
						GR_Raycast_Wall_Box_Fourside(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, hit_map_id, &is_solid_hit);
						break;

					case LV_C_WALL_BOX_SHORT:
						GR_Raycast_Wall_Box_Short(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, hit_map_id, &is_solid_hit);
						break;

					// DOORS
						
					case LV_C_DOOR_THICK_HORIZONTAL:
					// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further.
						switch (LV_map[hit_map_id].cell_state)
						{
							case LV_S_DOOR_CLOSED:
							case LV_S_DOOR_CLOSED_PROXMITY:
								GR_Raycast_Door_Thick_Horizontal_Closed(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
								break;

							case LV_S_DOOR_OPENING:
							case LV_S_DOOR_CLOSING:
								GR_Raycast_Door_Thick_Horizontal_Moving(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_y__1div, &is_solid_hit);
								break;
						}
						break;
						
					case LV_C_DOOR_THICK_VERTICAL:
					// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further.
						switch (LV_map[hit_map_id].cell_state)
						{
							case LV_S_DOOR_CLOSED:
							case LV_S_DOOR_CLOSED_PROXMITY:
								GR_Raycast_Door_Thick_Vertical_Closed(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
								break;

							case LV_S_DOOR_OPENING:
							case LV_S_DOOR_CLOSING:
								GR_Raycast_Door_Thick_Vertical_Moving(ray_x, hit_map_id, ray_dir_x, ray_dir_y, ray_dir_x__1div, &is_solid_hit);
								break;
						}
						break;
						
					case LV_C_DOOR_THIN_OBLIQUE:
					// If thin doors are closed lets treat them as thin wall, if not, gather informations and let ray go further.
						switch (LV_map[hit_map_id].cell_state)
						{
							case LV_S_DOOR_CLOSED:
							case LV_S_DOOR_CLOSED_PROXMITY:
								GR_Raycast_Door_Thin_Oblique_Closed(ray_x, hit_map_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
								break;

							case LV_S_DOOR_OPENING:
							case LV_S_DOOR_CLOSING:
								GR_Raycast_Door_Thin_Oblique_Moving(ray_x, hit_map_id, what_side_hit, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, &is_solid_hit);
								break;
						}
						break;

					case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
					case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
						GR_Raycast_Door_Box_Fourside_Horizontal(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, hit_map_id, &is_solid_hit);
						break;

					case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
					case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
						GR_Raycast_Door_Box_Fourside_Vertical(ray_x, what_side_hit, step_x, step_y, ray_dir_x, ray_dir_y, ray_dir_x__1div, ray_dir_y__1div, hit_map_id, &is_solid_hit);
						break;
				}
			}
		}
	}
}
void	GR_Walls_Render(void)
{
	// Render first pass - walls and doors with full height.
	int32 ray_x = GR_render_width;

	while (ray_x--)
	{
		// Get values per ray_x, that we calculated earlier.
		int32	wall_draw_height = GR_wall_column_1st_pass[ray_x].wall_draw_height;
		u_int8* lightmap__READY = GR_wall_column_1st_pass[ray_x].lightmap__READY;
		u_int8* texture__READY = GR_wall_column_1st_pass[ray_x].texture__READY;
		u_int8* tex_y_pos = GR_wall_column_1st_pass[ray_x].tex_y_pos;
		u_int8* tex_32lm_y_pos = GR_wall_column_1st_pass[ray_x].tex_32lm_y_pos;
		u_int32* texture_intensity = GR_wall_column_1st_pass[ray_x].texture_intensity;
		u_int32* output_buffer_32__ptr = GR_wall_column_1st_pass[ray_x].output_buffer_32__ptr;

		// Draw a single vertical slice of wall that was hit.
		do
		{
			// At first we are getting intensity value from lightmap pixel (0-127).
			int32 lm_intensity_value = lightmap__READY[*tex_32lm_y_pos];

			// Next, that pixel value should be affected by the distance -
			// so lets use precalculated GR_distance_shading_walls__LUT to get that value.
			// That value is already multiplied by 128 (IO_TEXTURE_MAX_SHADES), so we dont need to multiply it here.
			// Next, move pointer to correct color (intensity) table of texture.
			u_int32* texture_intensity_lm = texture_intensity + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);

			// We can put correct pixel into output buffer.
			*output_buffer_32__ptr = texture_intensity_lm[texture__READY[*tex_y_pos]];
			// *output_buffer_32__ptr = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
			output_buffer_32__ptr += GR_render_width;

			// Move to the next pixel in texture.
			tex_y_pos++;
			tex_32lm_y_pos++;

			wall_draw_height--;

		} while (wall_draw_height > 0);
	}
	
	// Render second pass - walls and doors with different or variable height.
	ray_x = GR_wall_column_2nd_pass_count;

	while (ray_x--)
	{
		// Get values per ray_x, that we calculated earlier.
		int32		wall_draw_height = GR_wall_column_2nd_pass[ray_x].wall_draw_height;
		u_int8*		lightmap__READY = GR_wall_column_2nd_pass[ray_x].lightmap__READY;
		u_int8*		texture__READY = GR_wall_column_2nd_pass[ray_x].texture__READY;
		u_int8*		tex_y_pos = GR_wall_column_2nd_pass[ray_x].tex_y_pos;
		u_int8*		tex_32lm_y_pos = GR_wall_column_2nd_pass[ray_x].tex_32lm_y_pos;
		u_int32*	texture_intensity = GR_wall_column_2nd_pass[ray_x].texture_intensity;
		u_int32*	output_buffer_32__ptr = GR_wall_column_2nd_pass[ray_x].output_buffer_32__ptr;

		// Draw a single vertical slice of wall that was hit.
		do
		{
			// At first we are getting intensity value from lightmap pixel (0-127).
			int32 lm_intensity_value = lightmap__READY[*tex_32lm_y_pos];

			// Next, that pixel value should be affected by the distance -
			// so lets use precalculated GR_distance_shading_walls__LUT to get that value.
			// That value is already multiplied by 128 (IO_TEXTURE_MAX_SHADES), so we dont need to multiply it here.
			// Next, move pointer to correct color (intensity) table of texture.
			u_int32* texture_intensity_lm = texture_intensity + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);

			// We can put correct pixel into output buffer.
			*output_buffer_32__ptr = texture_intensity_lm[texture__READY[*tex_y_pos]];
			// *output_buffer_32__ptr = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
			output_buffer_32__ptr += GR_render_width;

			// Move to the next pixel in texture.
			tex_y_pos++;
			tex_32lm_y_pos++;

			wall_draw_height--;

		} while (wall_draw_height > 0);
	}
}