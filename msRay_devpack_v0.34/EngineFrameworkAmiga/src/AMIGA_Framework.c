// AmigaOS includes.
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

// Standard C includes.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Our helper common globals.
#include "AMIGA_Globals.h"

// Raycaster Engine - engine libraries include
#include "../../EngineCommonLibs/EN_Engine_Main.h"

// ------------------------------------------
// --- AMIGA Framework - global variables ---
// ------------------------------------------
#define FRM_CONSOLE_INFO    "CON:30/35/480/230/"GLOBAL_NAME
#define CYBERGFXVERSION     41

struct Library* CyberGfxBase     = NULL;
struct Screen*  FRM_screen      = NULL;
struct Window*  FRM_window      = NULL;

ULONG               FRM_window_signal;
struct MsgPort*     FRM_window_user_port;
struct RastPort*    FRM_window_rastport;

// Pointer to my output console.
BPTR FRM_console;

// Selected screen parameters, displayID...
ULONG FRM_requested_display_id;
UWORD FRM_requested_width;
UWORD FRM_requested_height;
ULONG FRM_requested_pixel_format;

// bits per pixel 24, 32...
ULONG FRM_requested_pixel_depth; 

// Multiple buffering variables.
#define	FRM_MBUF_OK_REDRAW	    1	// Buffer fully detached, ready for redraw
#define FRM_MBUF_OK_SWAPIN	    2	// Buffer redrawn, ready for swap-in

struct MsgPort*         FRM_mbuf_port;
struct ScreenBuffer*    FRM_mbuf_screen_buffer[] = { NULL, NULL, NULL };

ULONG FRM_mbuf_status[3];

// Empty mouse pointer
UWORD FRM_empty_mouse_pointer[] = {0,0,0,0,0,0};

// ------------------------------------------------
// --- AMIGA Framework - functions declarations ---
// ------------------------------------------------
UBYTE   FRM_Init();
void    FRM_Run(void);
void    FRM_Cleanup(void);

UBYTE   FRM_Console_Init();
void    FRM_Console_Write(char*);
void    FRM_Console_Cleanup();

UBYTE   FRM_Validate_CPU_FPU();
UBYTE   FRM_Validate_Input_Parameters();
UBYTE   FRM_Open_Libraries();
UBYTE   FRM_Init_Screen();
UBYTE   FRM_Init_Window();
ULONG   FRM_Init_Screen_Buffers();

// ----------------------------------------------
// --- AMIGA Framework - main() - entry point ---
// ----------------------------------------------
int main(int _argc, char **_argv)
{
    // Get display ID from the 2nd parameter (1st is application name)
    FRM_requested_display_id = strtol(_argv[1], NULL, 0);

    // Init the Amiga Framework (and also Raycaster Engine) with parameters selected by user from Starter GUI.
	if (!FRM_Init())
	{   
		// If Framework initialisation went wrong, cleanup the resources and end application.
        Delay(100);
		FRM_Cleanup();
        FRM_Console_Cleanup();
		return 0;
	}
   
    // If success close console...
    FRM_Console_Cleanup();

	// Run Amiga Framework Message Loop...
	FRM_Run();

	// After ending Framework Message Loop, cleanup all resources and end the application.
	FRM_Cleanup();

	return 0;
}

// -----------------------------------------------
// --- AMIGA Framework - functions DEFINITIONS ---
// -----------------------------------------------
UBYTE FRM_Init()
{
    // Init my output console.
    FRM_Console_Init();
    
    // Write out some init info.
    FRM_Console_Write("-------------------------------------------\n");
    FRM_Console_Write(GLOBAL_FRM_IN_CONSOLE_NAME);
    FRM_Console_Write("\n-------------------------------------------\n\n");

    // Try open libraries
    if (!FRM_Open_Libraries())
        return 0;

    // Validate input parameters - the screen mode ID - from the Starter.
    if (!FRM_Validate_Input_Parameters())
        return 0;

    // Init screen.
    if (!FRM_Init_Screen())
        return 0;

    // Init window.
    if (!FRM_Init_Window())
        return 0;

    // Init screen buffers for multiple buffering.
    ULONG buf_mem_size = FRM_Init_Screen_Buffers();
    if (buf_mem_size == 0)
        return 0;

    FRM_Console_Write("\nMemory allocated for triple-buffering: ");

    char c_out[32], c_bytes[16];
    memset(c_out, 0, sizeof(c_out));
    memset(c_bytes, 0, sizeof(c_bytes));

    MA_Add_Number_Spaces(buf_mem_size, c_bytes);
    sprintf(c_out, "%s Bytes", c_bytes);

	FRM_Console_Write(c_out);

    // Create multi buffer port
    FRM_mbuf_port = CreateMsgPort();

    // ENGINE: init engine
    FRM_Console_Write("\nPreparing engine... ");

    LONG engine_mem_alloc_size = EN_Init(FRM_requested_width, FRM_requested_height, FRM_requested_pixel_format);

    if (!engine_mem_alloc_size)
    {
        FRM_Console_Write("[FAILED]");
        return 0;
    }
    FRM_Console_Write("[OK]");

    // ENGINE: return amount of allocated memory so far..
    FRM_Console_Write("\nMemory allocated for LUTs: ");

    memset(c_out, 0, sizeof(c_out));
    memset(c_bytes, 0, sizeof(c_bytes));

    MA_Add_Number_Spaces(engine_mem_alloc_size, c_bytes);
    sprintf(c_out, "%s Bytes", c_bytes);

	FRM_Console_Write(c_out);

    // ENGINE and SYSTEM: total memory allocated info...
    FRM_Console_Write("\nTotal memory allocated so far: ");

    memset(c_out, 0, sizeof(c_out));
    memset(c_bytes, 0, sizeof(c_bytes));

    MA_Add_Number_Spaces( (buf_mem_size + engine_mem_alloc_size), c_bytes);
    sprintf(c_out, "%s Bytes", c_bytes);

	FRM_Console_Write(c_out);

    // Make short delay.
    Delay(30);

    // Hide mouse pointer
    SetPointer( FRM_window, FRM_empty_mouse_pointer, 1, 1, 0, 0 );

    // Get out screen to front.
    ScreenToFront(FRM_screen);

    return 1;
}

void FRM_Run(void)
{
    // for FPS counter
	int fps = 0;
    LONG frames = 0;
    LONG elapsed_time = 0;
    LONG curr_time = clock();
    LONG start_time = curr_time;
    char fps_string[24];

    // Signal mask for window and multi-buffer.
    ULONG signal_mask = 0;

    // Some helpers for multi buffering routine.
    ULONG mbuf_nextdraw = 1;
    ULONG mbuf_nextswap = 1;

    // Enter main loop.
    BYTE FRM_main_loop = 1;

    while (FRM_main_loop)
    {
        IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

        // Enter Window Message Loop - handle messages and events.
        if(signal_mask & ( 1 << FRM_window_user_port->mp_SigBit ))
	    {
		    struct IntuiMessage	*imsg;

            while( imsg = (struct IntuiMessage *)GetMsg(FRM_window_user_port) )
            {
                switch (imsg->Class)
                {
                    case IDCMP_MOUSEMOVE:
                        IO_input.mouse_dx = imsg->MouseX;
		                IO_input.mouse_dy = imsg->MouseY;
                    break;

                    case IDCMP_RAWKEY:
                        if (imsg->Code & 0x80)
                        {                
                            IO_input.keys[ (u_int8)(imsg->Code & 0x7F) ] = 0;
                        }
                        else
                        {
                            IO_input.keys[ (u_int8)(imsg->Code & 0x7F) ] = 1;
                        }
                    break;
                }
                ReplyMsg((struct Message *)imsg);
            }
	    }

        // -- Multi Buffering routine. --

                // Check for and handle any double-buffering messages. 
                // Note that double-buffering messages are "replied" to us, so we don't want to reply them to anyone.

                if (signal_mask & (1 << FRM_mbuf_port->mp_SigBit))
                {
                    struct Message *mbuf_msg;
                    while(mbuf_msg = GetMsg(FRM_mbuf_port))
                    {
                        // dbi_SafeMessage is followed by an APTR dbi_UserData1, which
                        // contains the buffer number.  This is an easy way to extract it.
                        // The dbi_SafeMessage tells us that it's OK to redraw the in the previous buffer.                
                        ULONG buffer = (ULONG) *((APTR**)(mbuf_msg + 1));

                        // Mark the PREVIOUS buffer as OK to redraw into.
                        
                        // For double buffer we can use:
                        // FRM_mbuf_status[buffer^1] = FRM_MBUF_OK_REDRAW;

                        // For triple buffer we can use:
                        switch(buffer)
                        {
                            case 0: buffer = 2; break;
                            case 1: buffer = 0; break;
                            case 2: buffer = 1; break;
                        }
                        FRM_mbuf_status[buffer] = FRM_MBUF_OK_REDRAW;
                    }
                }

                //  if (FRM_main_loop)
                {
                    ULONG held_off = 0;

                    // 'buf_nextdraw' is the next buffer to draw into. The buffer is ready for drawing when we've received the
                    // dbi_SafeMessage for that buffer. Our routine to handle messaging from the double-buffering functions sets the
                    // OK_REDRAW flag when this message has appeared. Here, we set the OK_SWAPIN flag after we've redrawn
                    // the imagery, since the buffer is ready to be swapped in. We clear the OK_REDRAW flag, since we're done with redrawing
    
                    if (FRM_mbuf_status[mbuf_nextdraw] == FRM_MBUF_OK_REDRAW)
                    {
                        // --- DRAW HERE ---

                                // get fps
                                frames++;
                                curr_time = clock();
                                int32 TM_time_total = (curr_time - start_time) / CLOCKS_PER_SEC;

                                if (TM_time_total - elapsed_time >= 1)
                                {
                                    fps = frames;

                                    // reset
                                    frames = 0;
                                    elapsed_time += 1;
                                }

                                APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[mbuf_nextdraw]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&IO_prefs.output_buffer_32, TAG_DONE);
                                if (handle)			
                                {	        
                                    EN_Run(&FRM_main_loop);
                                }
                                UnLockBitMap(handle);
                                            
                                // Display FPS info.
                                memset(&fps_string, 0, 24);
                                sprintf(fps_string, "%d  %.3f", fps, IO_prefs.delta_time);

                                Move(FRM_window_rastport, 0,8);
                                Text(FRM_window_rastport, (CONST_STRPTR)fps_string, strlen(fps_string));

                        // -----------------

                        FRM_mbuf_status[mbuf_nextdraw] = FRM_MBUF_OK_SWAPIN;

                        // Toggle which the next buffer to draw is.

                        // For double buffer we can use:
                        // mbuf_nextdraw ^= 1;

                        // For triple buffer we can use:
                        switch(mbuf_nextdraw)
                        {
                            case 0: mbuf_nextdraw = 1; break;
                            case 1: mbuf_nextdraw = 2; break;
                            case 2: mbuf_nextdraw = 0; break;
                        }
                    }

                    // Let's make sure that the next frame is rendered before we swap
                    if (FRM_mbuf_status[mbuf_nextswap] == FRM_MBUF_OK_SWAPIN )
                    {
                        FRM_mbuf_screen_buffer[mbuf_nextswap]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = FRM_mbuf_port;

	                    if (ChangeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[mbuf_nextswap] ) )
	                    {
	                        FRM_mbuf_status[mbuf_nextswap] = 0;

	                        // Toggle which the next buffer to swap in is. 

                            // For double buffer we can use:
                            // mbuf_nextswap ^= 1;

                            // For triple buffer we can use:
                            switch(mbuf_nextswap)
                            {
                                case 0: mbuf_nextswap = 1; break;
                                case 1: mbuf_nextswap = 2; break;
                                case 2: mbuf_nextswap = 0; break;
                            }
	                    }
	                    else
	                    {
	                        held_off = 1;
	                    }
                    }
                
                    if (held_off)
                    {
                        // If were held-off at ChangeScreenBuffer() time, then we need to try ChangeScreenBuffer() again, 
                        // without awaiting a signal. We WaitTOF() to avoid busy-looping.
                       // WaitTOF();
                    }
                    else
                    {
                        // If we were not held-off, then we're all done with what we have to do.  We'll have no work to do
                        // until some kind of signal arrives. This will normally be the arrival of the dbi_SafeMessage from the ROM
                        // double-buffering routines, but it might also be an IntuiMessage.
                        signal_mask = Wait( ( 1 << FRM_mbuf_port->mp_SigBit ) | ( 1 << FRM_window_user_port->mp_SigBit ) );
                    }
                }        
        // -----------------------------------
    }
}

void FRM_Cleanup(void)
{
    // Cleanup Engine
    EN_Cleanup();

    // Close standard Window.
    if (FRM_window) CloseWindow(FRM_window);

    // Close screen.
    if (FRM_screen) CloseScreen(FRM_screen);

    // Delete multi buffer port.
    if (FRM_mbuf_port) DeleteMsgPort(FRM_mbuf_port);

    // Free multi screen buffers.
    if (FRM_mbuf_screen_buffer[0]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[0]);
    if (FRM_mbuf_screen_buffer[1]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[1]);
    if (FRM_mbuf_screen_buffer[2]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[2]);

    // Close CGX library.
	if (CyberGfxBase) CloseLibrary(CyberGfxBase);
}


UBYTE FRM_Console_Init()
{
    FRM_console = Open(FRM_CONSOLE_INFO, MODE_NEWFILE);
    
    if (FRM_console)    return 1;
    else                return 0;
}

void FRM_Console_Write(char* _input)
{
    Write(FRM_console, _input, strlen(_input));
}

void FRM_Console_Cleanup()
{
    Close(FRM_console);
}


UBYTE FRM_Validate_Input_Parameters()
{
    // Validate input parameters.

    // Validate Screen mode ID parameter and get its parameters.
    FRM_Console_Write("\nRequested screen mode: ");

	if (!IsCyberModeID(FRM_requested_display_id))
	{
		FRM_Console_Write("[FAILED]");
        return 0;
	}
	
	FRM_requested_width = GetCyberIDAttr(CYBRIDATTR_WIDTH, FRM_requested_display_id);
	FRM_requested_height = GetCyberIDAttr(CYBRIDATTR_HEIGHT, FRM_requested_display_id);
	FRM_requested_pixel_format = GetCyberIDAttr(CYBRIDATTR_PIXFMT, FRM_requested_display_id);    
    FRM_requested_pixel_depth = GetCyberIDAttr(CYBRMATTR_DEPTH, FRM_requested_display_id);    

	if (    (FRM_requested_pixel_format == PIXFMT_ARGB32)	||
			(FRM_requested_pixel_format == PIXFMT_BGRA32)	||
			(FRM_requested_pixel_format == PIXFMT_RGBA32)   )
    {
        FRM_requested_pixel_depth = 32;
    }

    char tmp_buf[32];
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "%d x %d x %d", FRM_requested_width, FRM_requested_height, (int)FRM_requested_pixel_depth);

	FRM_Console_Write(tmp_buf);

	return 1;
}

UBYTE FRM_Open_Libraries()
{
    // try open cybergraphics library
    FRM_Console_Write("Opening cybergraphics.library: ");
    CyberGfxBase = (struct Library*)OpenLibrary("cybergraphics.library", CYBERGFXVERSION);

    // if no RTG system found end the application
	if (CyberGfxBase == NULL)
	{
		FRM_Console_Write("[FAILED]");
        return 0;
	}
    else FRM_Console_Write("[OK]");

    return 1;
}

UBYTE FRM_Init_Screen()
{
    FRM_Console_Write("\nOpening screen: ");

    // opening the fullscreen
    FRM_screen = OpenScreenTags(	NULL,
									SA_DisplayID, FRM_requested_display_id,									
									SA_Type, CUSTOMSCREEN,
									SA_Quiet, TRUE,
                                    SA_Behind, TRUE,
									SA_ShowTitle, FALSE,
									SA_Draggable, FALSE,
									SA_Exclusive, TRUE,
									SA_AutoScroll, FALSE,
									TAG_END);

    if (FRM_screen == NULL)
	{
	    FRM_Console_Write("[FAILED]");
		return 0;
	}

    FRM_Console_Write("[OK]");

	return 1;
}

UBYTE FRM_Init_Window()
{
    FRM_Console_Write("\nOpening window: ");

	// Create full screen window on screen.
	FRM_window = OpenWindowTags(        NULL,
										WA_Left, 0,
										WA_Top, 0,
										WA_Width, FRM_requested_width,
										WA_Height, FRM_requested_height,
										WA_CustomScreen, (ULONG)FRM_screen,
										WA_Backdrop, TRUE,
										WA_Borderless, TRUE,
										WA_DragBar, FALSE,
										WA_Activate, TRUE,
										WA_NoCareRefresh, TRUE,
										WA_ReportMouse, TRUE,
										WA_RMBTrap, TRUE,
										WA_IDCMP, IDCMP_RAWKEY | IDCMP_DELTAMOVE | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS,
										TAG_END); 


    if (!FRM_window) 
    {
        FRM_Console_Write("[FAILED]");
        return 0;
	}

    FRM_Console_Write("[OK]");

    // Get window signal mask.
    FRM_window_user_port = CreateMsgPort();
    FRM_window->UserPort = FRM_window_user_port;

 	FRM_window_signal = ( 1 << FRM_window->UserPort->mp_SigBit );    
    FRM_window_rastport = FRM_window->RPort;

    return 1;
}

ULONG FRM_Init_Screen_Buffers()
{
    FRM_Console_Write("\nAllocating screen buffers: ");

    // The first buffer takes bitmap from screen bitmap.
    FRM_mbuf_screen_buffer[0] = AllocScreenBuffer(FRM_screen, NULL, SB_SCREEN_BITMAP);
    if (!FRM_mbuf_screen_buffer[0])
    {
        FRM_Console_Write("[FAILED]");
	    return 0;
    }

    FRM_mbuf_screen_buffer[1] = AllocScreenBuffer(FRM_screen, NULL, 0);
    if (!FRM_mbuf_screen_buffer[1])
    {
        FRM_Console_Write("[FAILED]");
	    return 0;
    }

    FRM_mbuf_screen_buffer[2] = AllocScreenBuffer(FRM_screen, NULL, 0);
    if (!FRM_mbuf_screen_buffer[2])
    {
        FRM_Console_Write("[FAILED]");
	    return 0;
    }

    // Something is wrong with bitamp allocating when AllocScreenBuffer parameter is '0' (for 1 and 2 buffer).
    // So, to override it, first - free existing bitmap in screen buffer structure, and then Allocate 2 new bitmaps
    // with the same parameters as screen bitmap. Then assign the pointers.
    FreeBitMap(FRM_mbuf_screen_buffer[1]->sb_BitMap);
    FreeBitMap(FRM_mbuf_screen_buffer[2]->sb_BitMap);

    struct BitMap* bitmap_1 = AllocBitMap(FRM_requested_width, FRM_requested_height, FRM_requested_pixel_depth, BMF_DISPLAYABLE, &FRM_screen->BitMap );
	struct BitMap* bitmap_2 = AllocBitMap(FRM_requested_width, FRM_requested_height, FRM_requested_pixel_depth, BMF_DISPLAYABLE, &FRM_screen->BitMap );

	FRM_mbuf_screen_buffer[1]->sb_BitMap = bitmap_1;
    FRM_mbuf_screen_buffer[2]->sb_BitMap = bitmap_2;

    if ( !bitmap_1 || !bitmap_2)
    {
         FRM_Console_Write("[FAILED]");
        return 0;
    }

    // Let's use the UserData to store the buffer number, for easy identification when the message comes back.
    FRM_mbuf_screen_buffer[0]->sb_DBufInfo->dbi_UserData1 = (APTR)0;
    FRM_mbuf_screen_buffer[1]->sb_DBufInfo->dbi_UserData1 = (APTR)1;
    FRM_mbuf_screen_buffer[2]->sb_DBufInfo->dbi_UserData1 = (APTR)2;

    FRM_mbuf_status[0] = FRM_MBUF_OK_REDRAW;
    FRM_mbuf_status[1] = FRM_MBUF_OK_REDRAW;
    FRM_mbuf_status[2] = FRM_MBUF_OK_REDRAW;

    // Clear buffers with black color.
    ULONG*  base_address;
    ULONG   buf_size = FRM_requested_width * FRM_requested_height * (FRM_requested_pixel_depth / 8);

    for (int i = 0; i < 3; i++)
    {
        APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[i]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&base_address, TAG_DONE);
        if (handle) memset(base_address, 0, buf_size);
        UnLockBitMap(handle);
    }

    FRM_Console_Write("[OK]");

    return (buf_size * 3);
}