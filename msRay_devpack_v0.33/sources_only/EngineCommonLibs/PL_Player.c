#include "PL_Player.h"
#include "MA_Math.h"

// ---------------------------------------------------
// --- GAME OBJECTS - PUBLIC functions definitions ---
// ---------------------------------------------------
void PL_Player_Init(void)
{
    // Init precalculated z position array.
    float32 tmp_step = 360.0f / PL_Z_LUT_SIZE;
    int32 tmp_index = 0;

    for (float32 f = 0.0f; f < 360.0f; f += tmp_step)
    {
        PL_player.z__LUT[tmp_index] = sinf(f * MA_pi / 180.0f) * PL_Z_MAX;
        tmp_index++;
    }

    // Set pitch speed.
    #if defined _WIN32
        PL_player.pitch_speed = ((float32)IO_prefs.screen_height * 1.5f);
    #elif defined AMIGA
        PL_player.pitch_speed = ((float32)IO_prefs.screen_height * 1.5f) / 10.0f;
    #endif

    // Set max pitch.
    PL_player.pitch_max = IO_prefs.screen_height / 3.0f;
}

void PL_Player_Reset(void)
{
	PL_player.pitch = 0.0f;

	PL_player.z_accumulation = 0.0f;
	PL_player.z = PL_player.z__LUT[(int32)PL_player.z_accumulation];
}
