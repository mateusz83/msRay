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

// Raycaster Engine - engine libraries include
#include "../../EngineCommonLibs/EN_Engine_Main.h"

// ------------------------------------------
// --- AMIGA Framework - global variables ---
// ------------------------------------------
static const char 	*FRM_version =	"$VER: Version 0.36";

#define FRM_WINDOW_NAME             "msRay (v0.36), Amiga Framework."
#define FRM_CONSOLE_INFO            "CON:30/35/480/340/"FRM_WINDOW_NAME

enum {FRM_MODE_FULLSCREEN_32BIT, FRM_MODE_HAM };

struct Library* CyberGfxBase        = NULL;
struct Screen*  FRM_screen          = NULL;
struct Window*  FRM_window          = NULL;

ULONG               FRM_window_signal;
struct MsgPort*     FRM_window_user_port;
struct RastPort*    FRM_window_rastport;

// Pointer to my output console.
BPTR FRM_console;

// Input parameters for the Engine.
UBYTE FRM_requested_display_mode, FRM_requested_buffers, FRM_requested_bits_per_pixel; 
ULONG FRM_requested_mode_id, FRM_requested_pixel_format;
UWORD FRM_requested_width, FRM_requested_height;

ULONG                   FRM_HAM8_bitplane_size;
UBYTE*                  FRM_Chip_Buffer[]           = { NULL, NULL };
struct ScreenBuffer*    FRM_mbuf_screen_buffer[]    = { NULL, NULL, NULL };

// Helper string for buffers number.
UBYTE*  FRM_buffering_strings[] = { "", "single-buffering", "double-buffering", "triple-buffering" };

// Empty mouse pointer
UWORD   FRM_empty_mouse_pointer[] = {0,0,0,0,0,0};

// ------------------------------------------------
// --- AMIGA Framework - functions declarations ---
// ------------------------------------------------
UBYTE   FRM_Init(int _argc, char **_argv);
void    FRM_Run(void);
void    FRM_Cleanup(void);

void    FRM_Console_Init();
void    FRM_Console_Write(char* _input, ...);
void    FRM_Console_Cleanup();

UBYTE   FRM_Init_Mode_Fullscreen_32bit();
void    FRM_Run_Mode_Fullscreen_32bit_Single_Buffer(void);
void    FRM_Run_Mode_Fullscreen_32bit_Double_Buffer(void);
void    FRM_Run_Mode_Fullscreen_32bit_Triple_Buffer(void);

UBYTE   FRM_Init_Mode_HAM8(void);
void    FRM_Run_Mode_HAM8_Single_Buffering(void);
void    FRM_Run_Mode_HAM8_Double_Buffering(void);

UBYTE   FRM_Init_Screen();
UBYTE   FRM_Init_Window();

// m68k asm functions declarations. Chunky-To-Planar for HAM8 (by Kalms).
extern void c2p_4rgb888_4rgb666h8_040_init(int chunkyx __asm("d0"), int chunkyy __asm("d1"), int scroffsx __asm("d2"),  int scroffsy __asm("d3"),int rowlen  __asm("d4"), int bplsize __asm("d5"), int chunkylen __asm("d6"));
extern void c2p_4rgb888_4rgb666h8_040(void* c2pscreen __asm("a0"), void* bitplanes __asm("a1"));

// ----------------------------------------------
// --- AMIGA Framework - main() - entry point ---
// ----------------------------------------------
int main(int _argc, char **_argv)
{
    // Init the Amiga Framework (and also Raycaster Engine) with parameters selected by user from Starter GUI.
	if (!FRM_Init(_argc, _argv))
	{   
		// If Framework initialisation went wrong, cleanup the resources and end application.
        Delay(200);
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

UBYTE FRM_Init(int _argc, char **_argv)
{
    // Init my output console.
    FRM_Console_Init();

    // Write out some init info.
    FRM_Console_Write("-------------------------------------------\n");
    FRM_Console_Write(FRM_WINDOW_NAME);
    FRM_Console_Write("\n-------------------------------------------\n\n");

    // Check number of input arguments, first argument is the application name.
    if (_argc < 6)
    {
        FRM_Console_Write("\nWrong number of input arguments:\n(1) display mode (0-1)\n(2) display mode id\n(3) screen width\n(4) screen height\n(5) number of buffers (1-3)\n\n");
        return 0;
    }

    // Get all input arguments.
    FRM_requested_display_mode  = strtol(_argv[1], NULL, 0);
    FRM_requested_mode_id       = strtol(_argv[2], NULL, 0);
    FRM_requested_width         = strtol(_argv[3], NULL, 0);
    FRM_requested_height        = strtol(_argv[4], NULL, 0);
    FRM_requested_buffers       = strtol(_argv[5], NULL, 0);

    // Depending on selected display mode,
    // we will have different elements to initialize.
    switch(FRM_requested_display_mode)
    {
        case FRM_MODE_FULLSCREEN_32BIT:
            return FRM_Init_Mode_Fullscreen_32bit();
            break;

        case FRM_MODE_HAM:
            return FRM_Init_Mode_HAM8();
            break;

        default:
            return 0;
    }

    return 1;
}

void FRM_Run()
{
    switch(FRM_requested_display_mode)
    {
        case FRM_MODE_FULLSCREEN_32BIT:
            switch(FRM_requested_buffers)
            {
                case 1:
                    FRM_Run_Mode_Fullscreen_32bit_Single_Buffer();
                    break;

                case 2:
                    FRM_Run_Mode_Fullscreen_32bit_Double_Buffer();
                    break;
            
                case 3:
                    FRM_Run_Mode_Fullscreen_32bit_Triple_Buffer();
                    break;
            }
            break;

        case FRM_MODE_HAM:
            switch(FRM_requested_buffers)
            {
                case 1:
                    FRM_Run_Mode_HAM8_Single_Buffering();
                    break;

                case 2:
                    FRM_Run_Mode_HAM8_Double_Buffering();
                    break;
            }
            break;
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

    // Free multi screen buffers.
    if (FRM_mbuf_screen_buffer[0]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[0]);
    if (FRM_mbuf_screen_buffer[1]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[1]);
    if (FRM_mbuf_screen_buffer[2]) FreeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[2]);

    // Free CHIP MEMORY buffers.
    if (FRM_Chip_Buffer[0]) FreeMem(FRM_Chip_Buffer[0], FRM_HAM8_bitplane_size * 8);
    if (FRM_Chip_Buffer[1]) FreeMem(FRM_Chip_Buffer[1], FRM_HAM8_bitplane_size * 8);

    // Close CGX library.
	if (CyberGfxBase) CloseLibrary(CyberGfxBase);
}


void FRM_Console_Init()
{
    FRM_console = Open(FRM_CONSOLE_INFO, MODE_NEWFILE);
}

void FRM_Console_Write(char* _input, ...)
{
    char output[256];
    memset(output, 0, sizeof(output));

    va_list args;   
    va_start(args, _input);
    vsprintf(output, _input, args);
    va_end(args);

   if (FRM_console)     Write(FRM_console, output, strlen(output));
   else                 printf(output);
}

void FRM_Console_Cleanup()
{
    if (FRM_console) 
        Close(FRM_console);
}


UBYTE FRM_Init_Mode_Fullscreen_32bit()
{
    char c_out_bytes[64];

    // --- 01 --- 
    // Init RTG libraries.
    FRM_Console_Write("Opening cybergraphics.library: ");
    CyberGfxBase = (struct Library*)OpenLibrary("cybergraphics.library", 41L);

	if (CyberGfxBase == NULL)
	{
		FRM_Console_Write("[FAILED]");
        return 0;
	}
    FRM_Console_Write("[OK]");


    // --- 02 ---
    // Validate display mode ID and get rest of the indo from mode ID.
    FRM_Console_Write("\nRequested screen mode: ");

	if (!IsCyberModeID(FRM_requested_mode_id))
	{
	    FRM_Console_Write("[FAILED]");
        return 0;
	}
	
    FRM_requested_width = GetCyberIDAttr(CYBRIDATTR_WIDTH, FRM_requested_mode_id);
    FRM_requested_height = GetCyberIDAttr(CYBRIDATTR_HEIGHT, FRM_requested_mode_id);
    FRM_requested_pixel_format = GetCyberIDAttr(CYBRIDATTR_PIXFMT, FRM_requested_mode_id);    
    FRM_requested_bits_per_pixel = GetCyberIDAttr(CYBRMATTR_DEPTH, FRM_requested_mode_id);    

    if ( (FRM_requested_pixel_format == PIXFMT_ARGB32) || (FRM_requested_pixel_format == PIXFMT_BGRA32) || (FRM_requested_pixel_format == PIXFMT_RGBA32) )
        FRM_requested_bits_per_pixel = 32;

    FRM_Console_Write("%d x %d x %d\n", FRM_requested_width, FRM_requested_height, (int)FRM_requested_bits_per_pixel);


    // --- 03 ---
    // Init screen.
    if (!FRM_Init_Screen())
        return 0;


    // --- 04---
    // Init window.
    if (!FRM_Init_Window())
        return 0;


    // --- 05 ---
    // Init screen buffers for multi buffering.
    ULONG buffer_mem_size = FRM_requested_width * FRM_requested_height * (FRM_requested_bits_per_pixel / 8);

    FRM_Console_Write("\n\nAllocating memory for %s...", FRM_buffering_strings[FRM_requested_buffers]);

    // Alloc requested number of buffers.
    // The first buffer takes bitmap from screen bitmap.
    FRM_Console_Write("\nAllocating FAST RAM for buffer [1]:");
    FRM_mbuf_screen_buffer[0] = AllocScreenBuffer(FRM_screen, NULL, SB_SCREEN_BITMAP);

    if (!FRM_mbuf_screen_buffer[0])
    {
        FRM_Console_Write("[FAILED]");
	    return 0;
    }

    memset(c_out_bytes, 0, sizeof(c_out_bytes));
    MA_Add_Number_Spaces( buffer_mem_size, c_out_bytes);
    FRM_Console_Write("%s Bytes", c_out_bytes);

    // Allocate rest of the buffer if needed.
    for (ULONG i = 1; i < FRM_requested_buffers; i++)
    {
        FRM_Console_Write("\nAllocating FAST RAM for buffer [%d]:", i + 1);
        FRM_mbuf_screen_buffer[i] = AllocScreenBuffer(FRM_screen, NULL, 0);

        if (!FRM_mbuf_screen_buffer[i])
        {
            FRM_Console_Write("[FAILED]");
	        return 0;
        }

        // Something is wrong with bitamp allocating when AllocScreenBuffer parameter is '0' (for 2 and 3 buffer).
        // So, to override it, first - free existing bitmap in screen buffer structure, and then Allocate two new bitmaps
        // with the same parameters as screen bitmap. Then assign the pointers.
        FreeBitMap(FRM_mbuf_screen_buffer[i]->sb_BitMap);
        struct BitMap* tmp_bitmap = AllocBitMap(FRM_requested_width, FRM_requested_height, FRM_requested_bits_per_pixel, BMF_DISPLAYABLE, &FRM_screen->BitMap );
        if ( tmp_bitmap == NULL)
        {
            FRM_Console_Write("[FAILED]");
            return 0;
        }
	    FRM_mbuf_screen_buffer[i]->sb_BitMap = tmp_bitmap;

        memset(c_out_bytes, 0, sizeof(c_out_bytes));
        MA_Add_Number_Spaces( buffer_mem_size, c_out_bytes);
        FRM_Console_Write("%s Bytes", c_out_bytes);
    }

    // Clear buffers with black color.
    ULONG*  base_address;

    for (int i = 0; i < FRM_requested_buffers; i++)
    {
        APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[i]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&base_address, TAG_DONE);
        if (handle) memset(base_address, 0, buffer_mem_size);
        UnLockBitMap(handle);
    }

    // --- 06 ---
    // ENGINE: init engine
    FRM_Console_Write("\n\nPreparing engine... ");

    LONG engine_mem_allocated = EN_Init(FRM_requested_buffers, FRM_requested_width, FRM_requested_height, FRM_requested_pixel_format);

    if (!engine_mem_allocated)
    {
        FRM_Console_Write("[FAILED]");
        return 0;
    }

    FRM_Console_Write("[OK]");
    FRM_Console_Write("\nFAST RAM allocated for engine LUTs: ");

    memset(c_out_bytes, 0, sizeof(c_out_bytes));
    MA_Add_Number_Spaces(engine_mem_allocated, c_out_bytes);
	FRM_Console_Write("%s Bytes", c_out_bytes);

    // Total FAST RAM info.
    FRM_Console_Write("\n\nTotal FAST RAM allocated so far: ");

    memset(c_out_bytes, 0, sizeof(c_out_bytes));

    MA_Add_Number_Spaces( (buffer_mem_size * FRM_requested_buffers) + engine_mem_allocated, c_out_bytes);
	FRM_Console_Write("%s Bytes", c_out_bytes);

    // Finishing.
    FRM_Console_Write("\n\nStarting...\n");

    // Make short delay.
    Delay(50);

    // Hide mouse pointer
    SetPointer( FRM_window, FRM_empty_mouse_pointer, 1, 1, 0, 0 );

    // Get out screen to front.
    ScreenToFront(FRM_screen);

    return 1;
}

void FRM_Run_Mode_Fullscreen_32bit_Single_Buffer(void)
{
    // for FPS counter
	int fps = 0;
    LONG frames = 0;
    LONG elapsed_time = 0;
    LONG curr_time = clock();
    LONG start_time = curr_time;
    char fps_string[24];

    // Signal mask for window.
    ULONG signal_mask = ( 1 << FRM_window_user_port->mp_SigBit );

    // Enter main loop.
    BYTE FRM_main_loop = 1;

    while (FRM_main_loop)
    {
        IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

        // Enter Window Message Loop - handle messages and events.
	    if(SetSignal(0L, signal_mask) & signal_mask)
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

        // -- Single Buffering routine --

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

                    APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[0]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&IO_prefs.output_buffer_32, TAG_DONE);
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

        // -- End Single Buffering routine
    }
}

void FRM_Run_Mode_Fullscreen_32bit_Double_Buffer(void)
{
    // for FPS counter
	int fps = 0;
    LONG frames = 0;
    LONG elapsed_time = 0;
    LONG curr_time = clock();
    LONG start_time = curr_time;
    char fps_string[24];

    // Set the screen flag for double buffer.
    FRM_screen->RastPort.Flags = DBUFFER;

    // Set the screen to buffer 0.
    FRM_screen->RastPort.BitMap          = FRM_mbuf_screen_buffer[0]->sb_BitMap;
    FRM_screen->ViewPort.RasInfo->BitMap = FRM_mbuf_screen_buffer[0]->sb_BitMap;
    RethinkDisplay();

    // Next buffer.
    ULONG mbuf_nextswap = 1;

    // Signal mask for window.
    ULONG signal_mask = ( 1 << FRM_window_user_port->mp_SigBit );

    // Enter main loop.
    BYTE FRM_main_loop = 1;

    while (FRM_main_loop)
    {
        IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

        if(SetSignal(0L, signal_mask) & signal_mask)
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

            APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[mbuf_nextswap]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&IO_prefs.output_buffer_32, TAG_DONE);
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

    
        // Swap Buffers with sync.
        FRM_screen->RastPort.BitMap          = FRM_mbuf_screen_buffer[mbuf_nextswap]->sb_BitMap;
        FRM_screen->ViewPort.RasInfo->BitMap = FRM_mbuf_screen_buffer[mbuf_nextswap]->sb_BitMap;
 
        RethinkDisplay();
                        
        mbuf_nextswap ^= 1;
    }
}

void FRM_Run_Mode_Fullscreen_32bit_Triple_Buffer(void)
{
    // for FPS counter
	int fps = 0;
    LONG frames = 0;
    LONG elapsed_time = 0;
    LONG curr_time = clock();
    LONG start_time = curr_time;
    char fps_string[24];

    // Set the screen to buffer 0;
    ChangeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[0]);

    // Next buffer.
    ULONG mbuf_nextswap = 1;

    // Signal mask for window.
    ULONG signal_mask = ( 1 << FRM_window_user_port->mp_SigBit );

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

            APTR handle = LockBitMapTags(FRM_mbuf_screen_buffer[mbuf_nextswap]->sb_BitMap, LBMI_BASEADDRESS, (ULONG)&IO_prefs.output_buffer_32, TAG_DONE);
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


        // Change buffers without sync.
        if (ChangeScreenBuffer(FRM_screen, FRM_mbuf_screen_buffer[mbuf_nextswap] ) )
	    {
            // For triple buffer we can use:
             switch(mbuf_nextswap)
             {
                 case 0: mbuf_nextswap = 1; break;
                 case 1: mbuf_nextswap = 2; break;
                 case 2: mbuf_nextswap = 0; break;
             }
        }
        else
	        WaitTOF();
    }
}


UBYTE FRM_Init_Mode_HAM8(void)
{
    char c_out_bytes[64];

    // --- 01 ---
    // Right now FRM_requested_width and FRM_requested_height holds desired size of the 32bit frame buffer that will be rendered,
    // not the actual screen size. Lets save these values.
    const int chunkyx = FRM_requested_width; 
    const int chunkyy = FRM_requested_height;

    // Get real, full info from display mode id.    
    struct DimensionInfo dn_Dimensioninfo;
    GetDisplayInfoData(NULL, (UBYTE*)&dn_Dimensioninfo, sizeof(struct DimensionInfo),  DTAG_DIMS,  FRM_requested_mode_id);

    FRM_requested_width = dn_Dimensioninfo.Nominal.MaxX + 1;
    FRM_requested_height = dn_Dimensioninfo.Nominal.MaxY + 1;
    FRM_requested_bits_per_pixel = dn_Dimensioninfo.MaxDepth;

    // Add HAM flag to requested display mode id.
    FRM_requested_mode_id |= HAM_KEY;

    // Write out info about selected mode.
    FRM_Console_Write("Requested screen mode: %d x %d x %d bit (HAM8)\n", FRM_requested_width, FRM_requested_height, FRM_requested_bits_per_pixel);
    FRM_Console_Write("Rendered frame size: %d x %d x 32 bit\n", chunkyx, chunkyy);


    // --- 02 ---
    // Init screen.
    if (!FRM_Init_Screen())
        return 0;


    // --- 03 ---
    // Init window.
    if (!FRM_Init_Window())
        return 0;


    // --- 04 ---
    // Allocate screen buffers.
    FRM_Console_Write("\n\nAllocating memory for %s...", FRM_buffering_strings[FRM_requested_buffers]);
        
    // We need frame buffers in CHIP MEMORY.
    FRM_HAM8_bitplane_size = FRM_requested_width * FRM_requested_height / 8;
    ULONG chip_buf_size = FRM_HAM8_bitplane_size * 8;

    for (ULONG i = 0; i < FRM_requested_buffers; i++)
    {
        FRM_Console_Write("\nAllocating CHIP RAM for buffer [%d]:", i + 1);

        FRM_Chip_Buffer[i] = (UBYTE*)AllocMem(chip_buf_size, MEMF_CHIP | MEMF_CLEAR);

        if (FRM_Chip_Buffer[i] == NULL)
        {
            FRM_Console_Write("[FAILED]");
	        return 0;
        }
      
        memset(c_out_bytes, 0, sizeof(c_out_bytes));
        MA_Add_Number_Spaces(chip_buf_size, c_out_bytes);
        FRM_Console_Write("%s Bytes", c_out_bytes);
    }

    // Link first chip buffer to screen bitmap.
    FRM_screen->BitMap.Planes[0] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 0;
    FRM_screen->BitMap.Planes[1] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 1;
    FRM_screen->BitMap.Planes[2] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 2;
    FRM_screen->BitMap.Planes[3] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 3;
    FRM_screen->BitMap.Planes[4] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 4;
    FRM_screen->BitMap.Planes[5] = (PLANEPTR)FRM_Chip_Buffer[0] + FRM_HAM8_bitplane_size * 5;

    memset(FRM_screen->BitMap.Planes[6], 0x77, FRM_HAM8_bitplane_size);
    memset(FRM_screen->BitMap.Planes[7], 0xcc, FRM_HAM8_bitplane_size);

    MakeScreen(FRM_screen);
    RethinkDisplay();


    // --- 05 ---
    // Lets init HAM8 c2p routine.
    const int scroffsx = 0;
    const int scroffsy = 0;
    const int rowlen = FRM_requested_width / 8;
    const int chunkylen = chunkyx * 4;

    c2p_4rgb888_4rgb666h8_040_init(chunkyx, chunkyy, scroffsx, scroffsy, rowlen, FRM_HAM8_bitplane_size, chunkylen);

    // Set overscan border color to black.
    SetRGB32(&FRM_screen->ViewPort, 0, 0, 0, 0);


    // --- 06 ---
    // ENGINE: init engine
    FRM_Console_Write("\n\nPreparing engine: ");

    LONG engine_mem_allocated = EN_Init(FRM_requested_buffers, chunkyx, chunkyy, IO_PIXFMT_ARGB32);

    if (!engine_mem_allocated)
    {
        FRM_Console_Write("[FAILED]");
        return 0;
    }
    FRM_Console_Write("[OK]");

    // ENGINE: alloc memory in FAST RAM for 32bit frame buffer.
    FRM_Console_Write("\nAllocating FAST RAM for frame buffer: ");

    ULONG render_frame_mem_size = chunkyx * chunkyy * 4;
    IO_prefs.output_buffer_32 = (ULONG*)AllocMem(render_frame_mem_size, MEMF_FAST | MEMF_CLEAR);

    if (IO_prefs.output_buffer_32 == NULL)
    {
        FRM_Console_Write("[FAILED]");
	    return 0;
    }
    else
    {
        memset(c_out_bytes, 0, sizeof(c_out_bytes));
        MA_Add_Number_Spaces( render_frame_mem_size, c_out_bytes);
        FRM_Console_Write( "%s Bytes", c_out_bytes);
    }

    // ENGINE: return amount of allocated memory so far..
    FRM_Console_Write("\nFAST RAM allocated for engine LUTs: ");

    memset(c_out_bytes, 0, sizeof(c_out_bytes));
    MA_Add_Number_Spaces(engine_mem_allocated, c_out_bytes);
	FRM_Console_Write("%s Bytes", c_out_bytes);

    // TOTAL CHIP MEMORY INFO..
    FRM_Console_Write("\n\nTotal CHIP RAM allocated so far: ");

    memset(c_out_bytes, 0, sizeof(c_out_bytes));
    MA_Add_Number_Spaces( (chip_buf_size * FRM_requested_buffers) + (FRM_HAM8_bitplane_size * FRM_requested_bits_per_pixel), c_out_bytes);
	FRM_Console_Write( "%s Bytes", c_out_bytes);

    // TOTAL FAST MEMORY INFO..
    FRM_Console_Write("\nTotal FAST RAM allocated so far: ");

    memset(c_out_bytes, 0, sizeof(c_out_bytes));
    MA_Add_Number_Spaces( render_frame_mem_size + engine_mem_allocated, c_out_bytes);
	FRM_Console_Write( "%s Bytes", c_out_bytes);
   
    // Finishing.
    FRM_Console_Write("\n\nStarting...\n");

    // Make short delay.
    Delay(50);

    // Hide mouse pointer
    SetPointer( FRM_window, FRM_empty_mouse_pointer, 1, 1, 0, 0 );

    // Get out screen to front.
    ScreenToFront(FRM_screen);

    return 1;
}

void FRM_Run_Mode_HAM8_Single_Buffering(void)
{
    // Signal mask for window.
    ULONG signal_mask = ( 1 << FRM_window_user_port->mp_SigBit );

    // Enter main loop.
    BYTE FRM_main_loop = 1;

    while (FRM_main_loop)
    {
        IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

        // Enter Window Message Loop - handle messages and events.
	    if(SetSignal(0L, signal_mask) & signal_mask)
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

        // --- DRAW HERE ---

            EN_Run(&FRM_main_loop);       
                    
            c2p_4rgb888_4rgb666h8_040(IO_prefs.output_buffer_32, FRM_Chip_Buffer[0]);

        // -----------------
    }
}

void FRM_Run_Mode_HAM8_Double_Buffering(void)
{
    // Signal mask for window.
    ULONG signal_mask = ( 1 << FRM_window_user_port->mp_SigBit );

    // Enter main loop.
    BYTE FRM_main_loop = 1;

    int buffer_counter = 0;

    while (FRM_main_loop)
    {
        IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

        // Enter Window Message Loop - handle messages and events.
	    if(SetSignal(0L, signal_mask) & signal_mask)
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

        // --- DRAW HERE ---
            
            EN_Run(&FRM_main_loop);       
                    
            c2p_4rgb888_4rgb666h8_040(IO_prefs.output_buffer_32, FRM_Chip_Buffer[buffer_counter]);

            // Link ready buffer to screen bitmap.
            FRM_screen->BitMap.Planes[0] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 0;
            FRM_screen->BitMap.Planes[1] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 1;
            FRM_screen->BitMap.Planes[2] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 2;
            FRM_screen->BitMap.Planes[3] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 3;
            FRM_screen->BitMap.Planes[4] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 4;
            FRM_screen->BitMap.Planes[5] = (PLANEPTR)FRM_Chip_Buffer[buffer_counter] + FRM_HAM8_bitplane_size * 5;
            RethinkDisplay();

            // Switch buffer.
            buffer_counter ^= 1;

        // -----------------
    }
}


UBYTE FRM_Init_Screen()
{
    FRM_Console_Write("\nOpening screen: ");

    // Opening Fullscreen.
    switch(FRM_requested_display_mode)
    {
        // For 32bit fullscreen - is better to provide only display_mode_id without additional parameters.
        case FRM_MODE_FULLSCREEN_32BIT:
        {
            FRM_screen = OpenScreenTags(	NULL,
                                            SA_DisplayID, FRM_requested_mode_id,						
                                            SA_Type, CUSTOMSCREEN,
                                            SA_Quiet, TRUE,
                                            SA_Behind, TRUE,
                                            SA_ShowTitle, FALSE,
                                            SA_Draggable, FALSE,
                                            SA_Exclusive, TRUE,
                                            SA_AutoScroll, FALSE,
                                            TAG_END);
        }
        break;

        // For HAM fullscreen - SA_Depth must be also provided.
        case FRM_MODE_HAM:
        {
            FRM_screen = OpenScreenTags(	NULL,
                                            SA_DisplayID, FRM_requested_mode_id,	
                                            SA_Depth, FRM_requested_bits_per_pixel, 							
                                            SA_Type, CUSTOMSCREEN,
                                            SA_Quiet, TRUE,
                                            SA_Behind, TRUE,
                                            SA_ShowTitle, FALSE,
                                            SA_Draggable, FALSE,
                                            SA_Exclusive, TRUE,
                                            SA_AutoScroll, FALSE,
                                            TAG_END);
            break;
        }
    }

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
