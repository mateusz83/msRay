#include "TM_Timer.h"

// -----------------------------------------------------
// --- TIMER - PRIVATE globals, constants, variables ---
// -----------------------------------------------------

// ----------------------------------------
// --- for Windows and MS Visual Studio ---
// ----------------------------------------
#if defined _MSC_VER
    #include <Windows.h>
    #include <profileapi.h>
    static LARGE_INTEGER TM_frequency;

    static LARGE_INTEGER TM_current_time;
    static LARGE_INTEGER TM_prev_time;
#endif

// ----------------------------------------
// --- for Amiga OS -----------------------
// ----------------------------------------
#if defined AMIGA
    #include <proto/timer.h>
    #include <proto/exec.h>

    static struct timerequest* TM_time_requester;

    static struct timeval TM_current_time;
    static struct timeval TM_prev_time;
#endif

// --------------------------------------------
// --- TIMER - PUBLIC functions definitions ---
// --------------------------------------------
int8    TM_Init(void)
{
    // ----------------------------------------
    // --- for Windows and MS Visual Studio ---
    // ----------------------------------------
    #ifdef _MSC_VER
        if (!QueryPerformanceFrequency(&TM_frequency)) return 0;
        if (!QueryPerformanceCounter(&TM_current_time)) return 0;
    #endif

    // ----------------------------------------
    // --- for Amiga OS -----------------------
    // ----------------------------------------
    #if defined AMIGA
        struct MsgPort* mp;

        mp = CreateMsgPort();
        if (mp == NULL)
            return 0;

        TM_time_requester = (struct timerequest*)CreateIORequest(mp, sizeof(struct timerequest));
        if (TM_time_requester == NULL)
        {
            DeleteMsgPort(mp);
            return 0;
        }

        if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest*)TM_time_requester, 0))
            return 0;

        struct Device* TimerBase = TM_time_requester->tr_node.io_Device;
        GetSysTime(&TM_current_time);
    #endif

    TM_prev_time = TM_current_time;

    return 1;
}
void    TM_Cleanup(void)
{
    // ----------------------------------------
    // --- for Windows and MS Visual Studio ---
    // ----------------------------------------
    #if defined _MSC_VER
    #endif

    // ----------------------------------------
    // --- for Amiga OS -----------------------
    // ----------------------------------------
    #if defined AMIGA
        if (TM_time_requester != NULL)
        {
            struct MsgPort* mp = TM_time_requester->tr_node.io_Message.mn_ReplyPort;

            if (mp != NULL)
            {
                CloseDevice((struct IORequest*)TM_time_requester);
                DeleteIORequest((struct IORequest*)TM_time_requester);
                DeleteMsgPort(mp);
            }
        }
    #endif
}
float32 TM_Get_Delta_Time(void)
{
    // ----------------------------------------
    // --- for Windows and MS Visual Studio ---
    // ----------------------------------------
    #if defined _MSC_VER
        QueryPerformanceCounter(&TM_current_time);

        double tm_delta_d = 0.0;
        float32 tm_delta = 0.0f;

        tm_delta_d = (double)(TM_current_time.QuadPart - TM_prev_time.QuadPart);
        tm_delta_d /= TM_frequency.QuadPart;
        tm_delta = (float32)tm_delta_d;
    #endif
    
    // ----------------------------------------
    // --- for Amiga OS -----------------------
    // ----------------------------------------
    #if defined AMIGA   
        struct Device* TimerBase = TM_time_requester->tr_node.io_Device;
        GetSysTime(&TM_current_time);

        float32 tm_delta = 0.0f;
        tm_delta = (float32)(((TM_current_time.tv_secs - TM_prev_time.tv_secs) * 1000000) + (TM_current_time.tv_micro - TM_prev_time.tv_micro));
        tm_delta /= 1000000.0f;
    #endif

    TM_prev_time = TM_current_time;
    return tm_delta;
}