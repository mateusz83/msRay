#include "GR_Flats.h"
#include "GR_Projection_Plane.h"
#include "GR_Visible_Cells_List.h"
#include "GR_Walls.h"

#include "LV_Level.h"
#include "PL_Player.h"
#include "MA_Math.h"

// -------------------------------------------------------------------
// --- GAME RENDER - FLATS - PRIVATE globals, constants, variables ---
// -------------------------------------------------------------------
#define GR_MAX_POLY_POINTS			12
#define GR_TEST_NEAR_CLIP			100.0f
#define GR_FLATS_Z_OFFSET			2

typedef struct
{
	int32	flat_x_step__fp, flat_y_step__fp;
	int32	flat_x0__fp, flat_y0__fp;
	int32	ray_y__mul__GR_render_width;
}_IO_BYTE_ALIGN_ sGR_Flat_Row;

static sGR_Flat_Row* GR_flat_row;
static int16* GR_x_buffer[2];

// -----------------------------------------------------------
// --- GAME RENDER - FLATS - PRIVATE functions definitions ---
// -----------------------------------------------------------
static int32	GR_Init_Once_X_Buffer(void)
{
	int32 mem_allocated = 0;

	// -- MALLOC --
	int32 mem_size = GR_render_height * sizeof(int16);

	for (int32 i = 0; i < 2; i++)
	{
		GR_x_buffer[i] = (int16*)malloc(mem_size);
		if (GR_x_buffer[i] == NULL) return 0;

		// counting memory...
		mem_allocated += mem_size;
	}

	return mem_allocated;
}
static void		GR_Cleanup_X_Buffer(void)
{
	free(GR_x_buffer[0]);
	free(GR_x_buffer[1]);
}

static void		GR_Segment_To_Buffer(const int8 _buff_id, int16 x0, int16 y0, const int16 x1, const int16 y1)
{
	int16* fv_buf_x__READY = &GR_x_buffer[_buff_id][0];

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

				err += dx;
				y0++;

				if (y0 == y1)
				{
					fv_buf_x__READY[y0] = x0;
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

				err += dx;
				y0++;

				if (y0 == y1)
				{
					fv_buf_x__READY[y0] = x0;
					break;
				}
			}
		}
	}
}
static void		GR_Process_Segment(int16 _x0, int16 _y0, int16 _x1, int16 _y1)
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
		GR_Segment_To_Buffer(0, _x0, _y0, _x1, _y1);
	}
	else if (direction > 0)
	{
		GR_Segment_To_Buffer(1, _x0, _y0, _x1, _y1);
	}
}

static void		GR_Line_To_Screen_Top_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (int32)((_y0 * _x1 - _x0 * _y1) / (_y0 - _y1));
	*_iy = 0;
}
static void		GR_Line_To_Screen_Left_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = 0;
	*_iy = (_x0 * _y1 - _y0 * _x1) / (_x0 - _x1);
}
static void		GR_Line_To_Screen_Bottom_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (GR_render_height__sub1 * (_x1 - _x0) + (_x0 * _y1 - _y0 * _x1)) / (_y1 - _y0);
	*_iy = GR_render_height__sub1;
}
static void		GR_Line_To_Screen_Right_Intersection(int32 _x0, int32 _y0, int32 _x1, int32 _y1, int32* _ix, int32* _iy)
{
	*_ix = (GR_render_width__sub1 * (_x1 - _x0)) / (_x1 - _x0);
	*_iy = (GR_render_width__sub1 * (_y1 - _y0) - (_x0 * _y1 - _y0 * _x1)) / (_x1 - _x0);
}
static void		GR_Line_To_Line_Intersection(float32 x1, float32 y1, float32 x2, float32 y2, float32 x3, float32 y3, float32 x4, float32 y4, float32* x, float32* y)
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

static void		GR_Clip_To_Screen_Top(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_MAX_POLY_POINTS][2] = { 0 };
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
			GR_Line_To_Screen_Top_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
			GR_Line_To_Screen_Top_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
static void		GR_Clip_To_Screen_Left(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_MAX_POLY_POINTS][2] = { 0 };
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
			GR_Line_To_Screen_Left_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
			GR_Line_To_Screen_Left_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
static void		GR_Clip_To_Screen_Bottom(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_MAX_POLY_POINTS][2] = { 0 };
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
			GR_Line_To_Screen_Bottom_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
			GR_Line_To_Screen_Bottom_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
static void		GR_Clip_To_Screen_Right(int32 poly_points[][2], int8* poly_size)
{
	int32	new_points[GR_MAX_POLY_POINTS][2] = { 0 };
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
			GR_Line_To_Screen_Right_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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
			GR_Line_To_Screen_Right_Intersection(ix, iy, kx, ky, &new_x, &new_y);

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

static void		GR_Clip_Poly_To_Screen(int32 poly_points[][2], int8* _poly_size, int16* _y_min, int16* _y_max)
{
	GR_Clip_To_Screen_Left(poly_points, _poly_size);
	GR_Clip_To_Screen_Bottom(poly_points, _poly_size);
	GR_Clip_To_Screen_Right(poly_points, _poly_size);
	GR_Clip_To_Screen_Top(poly_points, _poly_size);

	int8 get_poly_size = (*_poly_size);

	if (get_poly_size < 2) return;

	get_poly_size--;

	// Find y-min and y-max values.
	int32	last_y_min = poly_points[0][1],
		last_y_max = poly_points[0][1];

	// Process all segments from the list.
	for (int32 i = 0; i < get_poly_size; i++)
	{
		GR_Process_Segment(poly_points[i][0], poly_points[i][1], poly_points[i + 1][0], poly_points[i + 1][1]);

		last_y_min = MA_MIN_2(last_y_min, poly_points[i][1]);
		last_y_max = MA_MAX_2(last_y_max, poly_points[i][1]);
	}

	// Process the the last segment.
	GR_Process_Segment(poly_points[get_poly_size][0], poly_points[get_poly_size][1], poly_points[0][0], poly_points[0][1]);

	last_y_min = MA_MIN_2(last_y_min, poly_points[get_poly_size][1]);
	last_y_max = MA_MAX_2(last_y_max, poly_points[get_poly_size][1]);

	*_y_min = last_y_min;
	*_y_max = last_y_max;
}
static void		GR_Clip_Poly_To_Line(float32 _poly_points[][2], int8* _poly_size, float32 _clipping_line[][2])
{
	float32	new_points[GR_MAX_POLY_POINTS][2] = { 0 };
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
			GR_Line_To_Line_Intersection(x1, y1, x2, y2, ix, iy, kx, ky, &new_x, &new_y);

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
			GR_Line_To_Line_Intersection(x1, y1, x2, y2, ix, iy, kx, ky, &new_x, &new_y);

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

static void		GR_Render_Flat_Horizontal_Stripe(	int32 _draw_length, u_int8 _tex_size_bitshift, u_int8 _tex_size_bitshift__fp, int32 _flat_x__fp, int32 _flat_y__fp, int32 _flat_step_x__fp, int32 _flat_step_y__fp,
													u_int8* _lightmap__PTR, int8* _tex__PTR, int32* _tex_colors__PTR, u_int32* _output_buffer_32__PTR)
{
	// This function will put two same pixels at once. This is for optimalization.
	// The result is not very different as puting one pixel at once - and the performance is higher.

	// But first, lets test if draw_length is even. If so, decrement draw_length by 1
	// and put the first pixel.
	if ((_draw_length & 1) == 0)
	{
		_draw_length--;

		int32 lm_x = _flat_x__fp >> 13;
		int32 lm_y = _flat_y__fp >> 13;

		int32 lm_intensity_value = _lightmap__PTR[lm_x + (lm_y << 5)];

		int32 tex_x = _flat_x__fp >> _tex_size_bitshift__fp;
		int32 tex_y = _flat_y__fp >> _tex_size_bitshift__fp;

		int8 texture_pixel_index = _tex__PTR[tex_x + (tex_y << _tex_size_bitshift)];

		int32* texture_intensity_lm = _tex_colors__PTR + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		_output_buffer_32__PTR++;

		_flat_x__fp += _flat_step_x__fp;
		_flat_y__fp += _flat_step_y__fp;
	}

	// This loop will put the rest of pixels
	// - duplicating them and putting two at once.
	int32 flat_step_x__fp__mul2 = _flat_step_x__fp + _flat_step_x__fp;
	int32 flat_step_y__fp__mul2 = _flat_step_y__fp + _flat_step_y__fp;

	while (_draw_length >= 0)
	{
		//  -- Step 01. --
		//  Lets find current x,y coords in lightmap bitmap. The lightmap bitmap is 32 x 32 px.	

			// BEFORE OPTIMALIZATION
			// int32 lm_tx = (int32)((floor_x__fp << 5) >> 18) & (IO_LIGHTMAP_SIZE - 1);
			// int32 lm_ty = (int32)((floor_y__fp << 5) >> 18) & (IO_LIGHTMAP_SIZE - 1);

			// AFTER OPTIMALIZATION: 
		int32 lm_x = _flat_x__fp >> 13;
		int32 lm_y = _flat_y__fp >> 13;


		//  -- Step 02. --
		//  Now we can get intensity valie from ligtmap - its 8-bit value from 0 to 127.

		int32 lm_intensity_value = _lightmap__PTR[lm_x + (lm_y << 5)];


		//  -- Step 03. --
		//  Lets find current x,y coords in texture bitmap.		

			// BEFORE OPTIMALIZATION
			// int32 tx = (int32)((floor_x__fp << 8) >> 18) & 255;
			// int32 ty = (int32)((floor_y__fp << 8) >> 18) & 255;

			// AFTER OPTIMALIZATION: 
		int32 tex_x = _flat_x__fp >> _tex_size_bitshift__fp;
		int32 tex_y = _flat_y__fp >> _tex_size_bitshift__fp;


		//  -- Step 04. --
		//  Now we can get index to color table from texture bitmap.

		int8 texture_pixel_index = _tex__PTR[tex_x + (tex_y << _tex_size_bitshift)];


		//  -- Step 05. --
		//  We would like also to use distance shading.
		//	Lets take our intensity value from lighmap (from 02) and use it in precalculated array "distance_shading_flats__LUT__READY" 
		//	to get lightmap intensity value affected by distance.
		//  With that final lightmap intensity value we can move pointer to correct color table for that texture

		int32* texture_intensity_lm = _tex_colors__PTR + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);


		//  -- Step 06. --
		//  Use texture index (from 04) to get RGBA pixel from correct color table. Put the pixel in output_buffer.

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		//*_output_buffer_32__PTR = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
		_output_buffer_32__PTR++;

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		//*_output_buffer_32__PTR = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
		_output_buffer_32__PTR++;


		//  -- Step 07. --
		// Increment by two steps.
		_flat_x__fp += flat_step_x__fp__mul2;
		_flat_y__fp += flat_step_y__fp__mul2;

		// Increment loop by two steps.
		_draw_length -= 2;
	}
}

static void		GR_Render_Flat_Horizontal_Stripe__32x32(int32 _draw_length, int32 _flat_x__fp, int32 _flat_y__fp, int32 _flat_x_step__fp, int32 _flat_y_step__fp, 
														u_int8* _lightmap__PTR,  int8* _tex__PTR, int32* _tex_colors__PTR, u_int32* _output_buffer_32__PTR)
{
	// This is optimized variant of:
	// void GR_Render_Flat_Horizontal_Stripe(...)
	// to be used when mip-map level for texture is 32x32 (the same as lightmap is).

	// This function will put two same pixels at once. This is for optimalization.
	// The result is not very different as puting one pixel at once - and the performance is higher.

	// But first, lets test if draw_length is even. If so, decrement draw_length by 1
	// and put the first pixel.

	if ((_draw_length & 1) == 0)
	{
		_draw_length--;

		int32 x = _flat_x__fp >> 13;
		int32 y = _flat_y__fp >> 13;

		int32 index = x + (y << 5);

		int32 lm_intensity_value = _lightmap__PTR[index];
		u_int8 texture_pixel_index = _tex__PTR[index];
		u_int32* texture_intensity_lm = _tex_colors__PTR + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		_output_buffer_32__PTR++;

		_flat_x__fp += _flat_x_step__fp;
		_flat_y__fp += _flat_y_step__fp;
	}

	int32 flat_x_step__fp__mul2 = _flat_x_step__fp + _flat_x_step__fp;
	int32 flat_y_step__fp__mul2 = _flat_y_step__fp + _flat_y_step__fp;

	while (_draw_length >= 0)
	{
		// Lightmap and text ture coords for 32x32 bitmap.
		int32 x = _flat_x__fp >> 13;
		int32 y = _flat_y__fp >> 13;

		// Calculate index.
		int32 index = x + (y << 5);

		int32 lm_intensity_value = _lightmap__PTR[index];

		// Use the same index for texture mip-map 32x32
		u_int8 texture_pixel_index = _tex__PTR[index];

		u_int32* texture_intensity_lm = _tex_colors__PTR + (lm_intensity_value << IO_TEXTURE_MAX_COLORS_BITSHIFT);

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		// *_output_buffer_32__PTR = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
		_output_buffer_32__PTR++;

		*_output_buffer_32__PTR = texture_intensity_lm[texture_pixel_index];
		// *_output_buffer_32__PTR = (lm_intensity_value << 24) | (lm_intensity_value << 16) | (lm_intensity_value << 8) | (lm_intensity_value);
		_output_buffer_32__PTR++;

		// Update incrementations.

		_flat_x__fp += flat_x_step__fp__mul2;
		_flat_y__fp += flat_y_step__fp__mul2;

		_draw_length -= 2;
	}
}


static void		GR_Render_Single_Flat(int8 _flat, int16 _vc_cell_id, int32 _vc_cell_x__fp, int32 _vc_cell_y__fp, int16 _y_min, int16 _y_max)
{
	// Prepare pointers to each mip-map of current texture.
	int8* texture_128__PTR = LV_flat_textures__PTR + (LV_map[_vc_cell_id].flat_texture_id[_flat] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
	int8* texture_64__PTR = LV_flat_textures__PTR + (LV_map[_vc_cell_id].flat_texture_id[_flat] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE) + (128 * 128);
	int8* texture_32__PTR = LV_flat_textures__PTR + (LV_map[_vc_cell_id].flat_texture_id[_flat] * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE) + (128 * 128 + 64 * 64);

	// The "texture_colors" pointer, points to the color table of current texture.
	// But each texture doesn't have ony one color table but multiple color tables
	// with fading intensity.

	int32* texture_colors__PTR = LV_flat_textures_colors__PTR + (LV_map[_vc_cell_id].flat_texture_id[_flat] * IO_TEXTURE_COLOR_DATA_BYTE_SIZE);

	// Pointer to the lightmap bitmap.

	u_int8* lightmap__PTR = LV_lightmaps__PTR + (LV_map[_vc_cell_id].flat_lightmap_id[_flat] * IO_LIGHTMAP_BYTE_SIZE);

	int8 mip_map_level[2] = { 0, 0 };

	// To render single polygon lets go from its top to bottom.

	for (int32 ray_y = _y_min; ray_y <= _y_max; ray_y++)
	{
		// The length of the current horizontal stripe - skip if it is smaller than 2.
		int32 draw_length = GR_x_buffer[1][ray_y] - GR_x_buffer[0][ray_y];

		if (draw_length < 2)
			continue;

		// Lets take care of the left-most ray that is hitting starting of the line
		int32 flat_x0__fp = GR_flat_row[ray_y].flat_x0__fp + GR_flat_row[ray_y].flat_x_step__fp * GR_x_buffer[0][ray_y];
		int32 flat_y0__fp = GR_flat_row[ray_y].flat_y0__fp + GR_flat_row[ray_y].flat_y_step__fp * GR_x_buffer[0][ray_y];

		int32 flat_x0_int__fp = MA_FP18_FLOOR(flat_x0__fp);
		int32 flat_y0_int__fp = MA_FP18_FLOOR(flat_y0__fp);

		int32 flat_x0_frac__fp = MA_FP18_FRACTION(flat_x0__fp);
		int32 flat_y0_frac__fp = MA_FP18_FRACTION(flat_y0__fp);

		if (flat_x0_int__fp != _vc_cell_x__fp)
		{
			if (flat_x0_frac__fp > MA_FP18_05)		flat_x0_frac__fp = 0;
			else									flat_x0_frac__fp = MA_FP18_0999;
		}

		if (flat_y0_int__fp != _vc_cell_y__fp)
		{
			if (flat_y0_frac__fp > MA_FP18_05)		flat_y0_frac__fp = 0;
			else									flat_y0_frac__fp = MA_FP18_0999;
		}

		// Lets take care of the right-most ray that is hitting ending of the line
		int32 flat_x1__fp = GR_flat_row[ray_y].flat_x0__fp + GR_flat_row[ray_y].flat_x_step__fp * (GR_x_buffer[1][ray_y]);
		int32 flat_y1__fp = GR_flat_row[ray_y].flat_y0__fp + GR_flat_row[ray_y].flat_y_step__fp * (GR_x_buffer[1][ray_y]);

		int32 flat_x1_int__fp = MA_FP18_FLOOR(flat_x1__fp);
		int32 flat_y1_int__fp = MA_FP18_FLOOR(flat_y1__fp);

		int32 flat_x1_frac__fp = MA_FP18_FRACTION(flat_x1__fp);
		int32 flat_y1_frac__fp = MA_FP18_FRACTION(flat_y1__fp);

		if (flat_x1_int__fp != _vc_cell_x__fp)
		{
			if (flat_x1_frac__fp > MA_FP18_05)		flat_x1_frac__fp = 0;
			else									flat_x1_frac__fp = MA_FP18_0999;
		}

		if (flat_y1_int__fp != _vc_cell_y__fp)
		{
			if (flat_y1_frac__fp > MA_FP18_05)		flat_y1_frac__fp = 0;
			else									flat_y1_frac__fp = MA_FP18_0999;
		}

		// Now we can caltulcate new, fine-tuned step sizes
		int32 flat_x_new_step__fp = (flat_x1_frac__fp - flat_x0_frac__fp) / draw_length;
		int32 flat_y_new_step__fp = (flat_y1_frac__fp - flat_y0_frac__fp) / draw_length;

		// Find the position in the screen buffer, where the current horizontal line begins.
		// This is optimized way to move thru the screen buffer. In the inner loop we will only +1 to the buffer,
		// move to the next pixel.
		int32 draw_start_in_buffer = GR_x_buffer[0][ray_y] + GR_flat_row[ray_y].ray_y__mul__GR_render_width;
		u_int32* output_buffer_32__PTR = IO_prefs.output_buffer_32 + draw_start_in_buffer;

		// Select mip-map level of texture depending in the "draw_lenght" of horizontal stripe
		// and render that single stripe.

		if ((draw_length > 190) || mip_map_level[0])
		{
			// Use 128x128 texture size.
			mip_map_level[0] = 1;

			GR_Render_Flat_Horizontal_Stripe(draw_length, 7, 11, flat_x0_frac__fp, flat_y0_frac__fp, flat_x_new_step__fp, flat_y_new_step__fp,
				lightmap__PTR, texture_128__PTR, texture_colors__PTR, output_buffer_32__PTR);
		}
		else if ((draw_length > 75) || mip_map_level[1])
		{
			// Use 64x64 texture size.
			mip_map_level[1] = 1;

			GR_Render_Flat_Horizontal_Stripe(draw_length, 6, 12, flat_x0_frac__fp, flat_y0_frac__fp, flat_x_new_step__fp, flat_y_new_step__fp,
				lightmap__PTR, texture_64__PTR, texture_colors__PTR, output_buffer_32__PTR);
		}
		else
		{
			// Use 32x32 texture size.
			GR_Render_Flat_Horizontal_Stripe__32x32(draw_length, flat_x0_frac__fp, flat_y0_frac__fp, flat_x_new_step__fp, flat_y_new_step__fp,
				lightmap__PTR, texture_32__PTR, texture_colors__PTR, output_buffer_32__PTR);
		}
	}
}


// ----------------------------------------------------------
// --- GAME RENDER - FLATS - PUBLIC functions definitions ---
// ----------------------------------------------------------
int32	GR_Flats_Init_Once(void)
{
	int32 mem_size;
	int32 memory_allocated = 0;

	mem_size = sizeof(sGR_Flat_Row) * GR_render_height;
	GR_flat_row = (sGR_Flat_Row*)malloc(mem_size);
	if (GR_flat_row == NULL) return 0;
	memory_allocated += mem_size;

	// Init the X-Buffer that will hold rasterized edges of flats.
	mem_size = GR_Init_Once_X_Buffer();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;

	return memory_allocated;
}
void	GR_Flats_Cleanup(void)
{
	GR_Cleanup_X_Buffer();
	free(GR_flat_row);
}

void	GR_Flats_Cache_Calculations(void)
{
	// Find horizont position.
	// Adding 0.5f for rounding.
	GR_horizont = (int16)(GR_render_height__div2 + PL_player.pitch + 0.5f);

	// Ray direction for leftmost ray (x = 0).
	float32 ray_dir_x0 = GR_pp_dx - GR_pp_nsize_x__div2;
	float32 ray_dir_y0 = GR_pp_dy - GR_pp_nsize_y__div2;

	// Precalculated and optimized helper values.
	float32 rdx1__sub__rdx0__div__width = ((GR_pp_nsize_x__div2 + GR_pp_nsize_x__div2) * GR_render_width__1div);
	float32 rdy1__sub__rdy0__div__width = ((GR_pp_nsize_y__div2 + GR_pp_nsize_y__div2) * GR_render_width__1div);

	// Update projection plane Z position.
	float32 pp_z__floor = GR_render_height__div2 + PL_player.z;

	for (int32 ray_y = GR_wall_end_min; ray_y < GR_render_height; ray_y++)
	{
		int32 ray_y_pos = (int32)(ray_y - GR_horizont);
		float32 straight_distance_to_point = fabsf(pp_z__floor / ray_y_pos);

		// Adding 0.5f for rounding.
		GR_flat_row[ray_y].flat_x_step__fp = (int32)((straight_distance_to_point * rdx1__sub__rdx0__div__width) * MA_FP18_DEC + 0.5f);
		GR_flat_row[ray_y].flat_y_step__fp = (int32)((straight_distance_to_point * rdy1__sub__rdy0__div__width) * MA_FP18_DEC + 0.5f);

		// Adding 0.5f for rounding.
		GR_flat_row[ray_y].flat_x0__fp = (int32)((PL_player.x + (straight_distance_to_point * ray_dir_x0)) * MA_FP18_DEC + 0.5f);
		GR_flat_row[ray_y].flat_y0__fp = (int32)((PL_player.y + (straight_distance_to_point * ray_dir_y0)) * MA_FP18_DEC + 0.5f);

		GR_flat_row[ray_y].ray_y__mul__GR_render_width = ray_y * GR_render_width;
	}

	float32 pp_z__ceil = GR_render_height__div2 - PL_player.z;

	for (int32 ray_y = GR_wall_start_max + 1; ray_y--; )
	{
		int32 ray_y_pos = (int32)(ray_y - GR_horizont);
		float32 straight_distance_to_point = fabsf(pp_z__ceil / ray_y_pos);

		// Adding 0.5f for rounding.
		GR_flat_row[ray_y].flat_x_step__fp = (int32)((straight_distance_to_point * rdx1__sub__rdx0__div__width) * MA_FP18_DEC + 0.5f);
		GR_flat_row[ray_y].flat_y_step__fp = (int32)((straight_distance_to_point * rdy1__sub__rdy0__div__width) * MA_FP18_DEC + 0.5f);

		// Adding 0.5f for rounding.
		GR_flat_row[ray_y].flat_x0__fp = (int32)((PL_player.x + (straight_distance_to_point * ray_dir_x0)) * MA_FP18_DEC + 0.5f);
		GR_flat_row[ray_y].flat_y0__fp = (int32)((PL_player.y + (straight_distance_to_point * ray_dir_y0)) * MA_FP18_DEC + 0.5f);

		GR_flat_row[ray_y].ray_y__mul__GR_render_width = ray_y * GR_render_width;
	}
}
void	GR_Flats_Render(void)
{
	//  -- Step 01 a. --
	//  Prepare some precalculated helpers.
	//	That values will be used to project map cells vertices on screen.

	float32 pp_inv_det = 1.0f / (GR_pp_nsize_x__div2 * GR_pp_dy - GR_pp_dx * GR_pp_nsize_y__div2);

	float32 ppid_dx = pp_inv_det * GR_pp_dx;
	float32 ppid_dy = pp_inv_det * GR_pp_dy;
	float32 ppid_nx = pp_inv_det * GR_pp_nsize_x__div2;
	float32 ppid_ny = pp_inv_det * GR_pp_nsize_y__div2;


	//  -- Step 01 b. --
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


	//  -- Step 01 c. --
	//  Prepare other helpers and variables.

		// Reset the visible cells list size.
	int32 vc_list_size = GR_vcl_size;


	//  -- Step 02. --
	//  Lets start our main loop. Lets go thru all visible cells from the list.

	do
	{
		//  -- Step 03. --
		//  Setup some values before entering another loop.

		// Update list counter.
		vc_list_size--;

		// Get ID and x,y of current visible cell from the VC list.
		int16 vc_cell_id = GR_vcl_list[vc_list_size];
		int8 vc_cell_y = vc_cell_id >> LV_MAP_LENGTH_BITSHIFT;
		int8 vc_cell_x = vc_cell_id - (vc_cell_y << LV_MAP_LENGTH_BITSHIFT);

		// We also need that values as fixed-point-18, they will be use later on.
		int32 vc_cell_x__fp = vc_cell_x << MA_FP18_BITSHIFT;
		int32 vc_cell_y__fp = vc_cell_y << MA_FP18_BITSHIFT;

		//if (vc_cell_id != 2080) continue;

		// Get coordinates of that cell relative to Player position.
		float32 cell_x0 = vc_cell_x - PL_player.x;
		float32 cell_y0 = vc_cell_y - PL_player.y;
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


		//  -- Step 04. --
		//  Lets project cell vertices on the screen and make clipping if neccesery.
		//  The polygons that will be created on the screen can be divided into a few cases.
		//  We can then suggest optimal algorithm for each case.

			// Lets test if any of projected points will be behind the Player.

		if (transform_top_left_y__1div > 0.0f && transform_top_left_y__1div < GR_TEST_NEAR_CLIP &&
			transform_top_right_y__1div		> 0.0f && transform_top_right_y__1div < GR_TEST_NEAR_CLIP &&
			transform_bottom_left_y__1div	> 0.0f && transform_bottom_left_y__1div < GR_TEST_NEAR_CLIP &&
			transform_bottom_right_y__1div	> 0.0f && transform_bottom_right_y__1div < GR_TEST_NEAR_CLIP)
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
			// Because wall rendering and flats rendering doesn't share the same math
			// there can be small gaps between wall and flats common edges. To reduce/avoid that lets offset
			// ceil z position a bit down and floor position a bit up.
			int32 s_00__x = (int32)(GR_render_width__div2 * (1.0f + transform_top_left_x * transform_top_left_y__1div) + 0.5f);
			int32 s_00__y__ceil = (int32)(-wall_height__top_left__div2 + vertical_move__top_left + 0.5f) + GR_FLATS_Z_OFFSET;
			int32 s_00__y__floor = (int32)(wall_height__top_left__div2 + vertical_move__top_left + 0.5f) - GR_FLATS_Z_OFFSET;

			int32 s_01__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x + ppid_dy) * transform_bottom_left_y__1div) + 0.5f);
			int32 s_01__y__ceil = (int32)(-wall_height__top_right__div2 + vertical_move__top_right + 0.5f) + GR_FLATS_Z_OFFSET;
			int32 s_01__y__floor = (int32)(wall_height__top_right__div2 + vertical_move__top_right + 0.5f) - GR_FLATS_Z_OFFSET;

			int32 s_10__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x - ppid_dx) * transform_top_right_y__1div) + 0.5f);
			int32 s_10__y__ceil = (int32)(-wall_height__bottom_left__div2 + vertical_move__bottom_left + 0.5f) + GR_FLATS_Z_OFFSET;
			int32 s_10__y__floor = (int32)(wall_height__bottom_left__div2 + vertical_move__bottom_left + 0.5f) - GR_FLATS_Z_OFFSET;

			int32 s_11__x = (int32)(GR_render_width__div2 * (1.0f + (transform_top_left_x + ppid_dy - ppid_dx) * transform_bottom_right_y__1div) + 0.5f);
			int32 s_11__y__ceil = (int32)(-wall_height__bottom_right__div2 + vertical_move__bottom_right + 0.5f) + GR_FLATS_Z_OFFSET;
			int32 s_11__y__floor = (int32)(wall_height__bottom_right__div2 + vertical_move__bottom_right + 0.5f) - GR_FLATS_Z_OFFSET;


			// Make a few visibility tests of the all points against the screen area.

			int8 test_x_points_in_screen = (s_00__x >= 0 && s_00__x < GR_render_width&&
				s_01__x >= 0 && s_01__x < GR_render_width&&
				s_10__x >= 0 && s_10__x < GR_render_width&&
				s_11__x >= 0 && s_11__x < GR_render_width);

			int8 test_y_floor_points_in_screen = (s_00__y__floor > GR_horizont && s_00__y__floor < GR_render_height&&
				s_01__y__floor > GR_horizont && s_01__y__floor < GR_render_height&&
				s_10__y__floor > GR_horizont && s_10__y__floor < GR_render_height&&
				s_11__y__floor > GR_horizont && s_11__y__floor < GR_render_height);

			int8 test_y_ceil_points_in_screen = (s_00__y__ceil > 0 && s_00__y__ceil < GR_horizont&&
				s_01__y__ceil > 0 && s_01__y__ceil < GR_horizont&&
				s_10__y__ceil > 0 && s_10__y__ceil < GR_horizont&&
				s_11__y__ceil > 0 && s_11__y__ceil < GR_horizont);


			// Check the FLOOR points againts the screen area.

			if (test_x_points_in_screen && test_y_floor_points_in_screen)
			{
				// CASE 01-A FLOOR.

				// If all points are in the screen area, the whole current polygon is visible,
				// so we can use different, faster approach and algorithm.

				// Lets rasterize all edges into buffer.
				// Note, the edges direction for floor/ceil are different and that is important.

				// Segment 00-01.
				GR_Process_Segment(s_00__x, s_00__y__floor, s_01__x, s_01__y__floor);

				// Segment 01-11.
				GR_Process_Segment(s_01__x, s_01__y__floor, s_11__x, s_11__y__floor);

				// Segment 11-10.
				GR_Process_Segment(s_11__x, s_11__y__floor, s_10__x, s_10__y__floor);

				// Segment 10-00.
				GR_Process_Segment(s_10__x, s_10__y__floor, s_00__x, s_00__y__floor);

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

				int32 floor_poly_points[GR_MAX_POLY_POINTS][2] = { {s_00__x, s_00__y__floor},
																		{s_01__x, s_01__y__floor},
																		{s_11__x, s_11__y__floor},
																		{s_10__x, s_10__y__floor} };

				GR_Clip_Poly_To_Screen(floor_poly_points, &floor_poly_size, &floor_y_min, &floor_y_max);
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
				GR_Process_Segment(s_00__x, s_00__y__ceil, s_10__x, s_10__y__ceil);

				// Segment 01-11.
				GR_Process_Segment(s_10__x, s_10__y__ceil, s_11__x, s_11__y__ceil);

				// Segment 11-10.
				GR_Process_Segment(s_11__x, s_11__y__ceil, s_01__x, s_01__y__ceil);

				// Segment 10-00.
				GR_Process_Segment(s_01__x, s_01__y__ceil, s_00__x, s_00__y__ceil);

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

				int32 ceil_poly_points[GR_MAX_POLY_POINTS][2] = { {s_00__x, s_00__y__ceil},
																		{s_10__x, s_10__y__ceil},
																		{s_11__x, s_11__y__ceil},
																		{s_01__x, s_01__y__ceil} };

				GR_Clip_Poly_To_Screen(ceil_poly_points, &ceil_poly_size, &ceil_y_min, &ceil_y_max);
			}
		}
		else
		{
			// CASE 02.
			// If not all points are in the safe zone - lets use different algorithm.
			// First we must clip the cell edges by near clipping line.
			// Then the cell vertices can be projected on screen and the screen clipping can be done.

			// The polygon created from current cell coordinates.
			float32 cell_poly_points[GR_MAX_POLY_POINTS][2] = { {cell_x0, cell_y0}, {cell_x1, cell_y0}, {cell_x1, cell_y1}, {cell_x0, cell_y1} };

			// Initial size of that cell poly is 4.
			int8 cell_poly_size = 4;

			// Lets clip the cell poly by our "near clipping line".
			GR_Clip_Poly_To_Line(cell_poly_points, &cell_poly_size, near_clipping_line);

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
				int32 screen_projected_points[GR_MAX_POLY_POINTS][3] = { 0 };

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
					screen_projected_points[i][1] = (int32)(wall_height__div2 + vertical_move) - GR_FLATS_Z_OFFSET;
					screen_projected_points[i][2] = (int32)(-wall_height__div2 + vertical_move) + GR_FLATS_Z_OFFSET;
				}

				// Lets setup edges of our new "near clipped" floor poligon.
				// The edges order is important.

				int32 floor_screen_clipped_points[GR_MAX_POLY_POINTS][2] = { { screen_projected_points[0][0], screen_projected_points[0][1]	},
																				{ screen_projected_points[1][0], screen_projected_points[1][1]	},
																				{ screen_projected_points[2][0], screen_projected_points[2][1]	},
																				{ screen_projected_points[3][0], screen_projected_points[3][1]	},
																				{ screen_projected_points[4][0], screen_projected_points[4][1]	} };

				// Perform the screen clipping.
				GR_Clip_Poly_To_Screen(floor_screen_clipped_points, &floor_poly_size, &floor_y_min, &floor_y_max);


				// Now lets setup our "near clipped" ceil poligon.
				// The edges order is important.
				// For some reason I could't make this work well as in floor case. I have to split it depend on ceil_poly_size.

				int32 ceil_screen_clipped_points[GR_MAX_POLY_POINTS][2] = { {screen_projected_points[0][0], screen_projected_points[0][2]} };

				for (int32 i = 1; i < ceil_poly_size; i++)
				{
					ceil_screen_clipped_points[i][0] = screen_projected_points[ceil_poly_size - i][0];
					ceil_screen_clipped_points[i][1] = screen_projected_points[ceil_poly_size - i][2];
				}

				// Perform the screen clipping.
				GR_Clip_Poly_To_Screen(ceil_screen_clipped_points, &ceil_poly_size, &ceil_y_min, &ceil_y_max);
			}
		}

		//  -- Step 05. --
		//  After points are projected, segments clipped and rasterized into x0 and x1 buffers,
		//  the current floor and ceil polygons can be rendered.

			// Render the FLOOR if the final floor poly has more than 2 segments.
		if (floor_poly_size > 2)
		{
			// The floor y_min shouldn't be higher than GR_wall_end_min - it will be overdrawn.
			int16 floor_min = MA_MAX_2(floor_y_min, GR_wall_end_min);

			GR_Render_Single_Flat(0, vc_cell_id, vc_cell_x__fp, vc_cell_y__fp, floor_min, floor_y_max);
		}

		// Render the CEILING if the final ceil poly has more than 2 segments.
		if (ceil_poly_size > 2)
		{
			// The ceil y_max shouldn't be lower than GR_wall_start_max - it will be overdrawn.
			int32 ceil_max = MA_MIN_2(ceil_y_max, GR_wall_start_max);

			GR_Render_Single_Flat(1, vc_cell_id, vc_cell_x__fp, vc_cell_y__fp, ceil_y_min, ceil_max);
		}

	} while (vc_list_size);
}


