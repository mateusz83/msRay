#include "GR_Projection_Plane.h"
#include "PL_Player.h"

#include <math.h>

// ---------------------------------------------------------------------
// --- GAME RENDER - PROJECTION PLANE - PUBLIC functions definitions ---
// ---------------------------------------------------------------------
void	GR_Projection_Plane_Init_Once(void)
{
	// Calculate info panel height. The hardcoded minimal info panel size is: 320 x 40 px, so:
	GR_info_panel_height = (IO_prefs.screen_width / 320) * GR_INFO_PANEL_HEIGHT_RELATIVE;

	// Get and calculate rendering size.
	GR_render_width = IO_prefs.screen_width;
	GR_render_height = IO_prefs.screen_height - GR_info_panel_height;

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
}
void	GR_Projection_Plane_Reset(void)
{
	// Reset projection plane orientation according to Player starting rotation.
	// 0* (UP), 90* (LEFT), 180* (DOWN), 270* (RIGHT).
	float32 tmp_sinf = sinf(-PL_player.starting_angle_radians);
	float32 tmp_cosf = cosf(-PL_player.starting_angle_radians);

	float32 old_pp_dx = GR_pp_dx;
	GR_pp_dx = GR_pp_dx * tmp_cosf - GR_pp_dy * tmp_sinf;
	GR_pp_dy = old_pp_dx * tmp_sinf + GR_pp_dy * tmp_cosf;

	float32 old_pp_nsize_x = GR_pp_nsize_x__div2;
	GR_pp_nsize_x__div2 = GR_pp_nsize_x__div2 * tmp_cosf - GR_pp_nsize_y__div2 * tmp_sinf;
	GR_pp_nsize_y__div2 = old_pp_nsize_x * tmp_sinf + GR_pp_nsize_y__div2 * tmp_cosf;
}