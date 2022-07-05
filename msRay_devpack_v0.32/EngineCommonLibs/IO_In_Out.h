#ifndef __IO_IN_OUT_H__
#define __IO_IN_OUT_H__


// ----------------------------------------------------------------
// --- Only for Microsoft Visual Studio to not giving warnings. ---
// ----------------------------------------------------------------
#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
// ----------------------------------------------------------------


// ---------------------------------------------------------------------------------------------------------
// --- _IO_BYTE_ALIGN_ - this makro is for AMIGA GCC compiler, so it aligning bytes in structures itself ---
// ---------------------------------------------------------------------------------------------------------

#if defined _WIN32
	#define _IO_BYTE_ALIGN_
#endif

#if defined AMIGA
	#define _IO_BYTE_ALIGN_ __attribute__((aligned(4)))
#endif

// ---------------------------------------------------
// --- MY TYPEDEFS - for more control over porting ---
// ---------------------------------------------------

#if defined _WIN32
	typedef char				int8;		// 8 bits	- 1 byte	<-127, 128>
	typedef unsigned char		u_int8;		// 8 bits	- 1 byte	<0, 255>
	typedef short				int16;		// 16 bits	- 2 bytes
	typedef	unsigned short		u_int16;	// 16 bits	- 2 bytes
	typedef int					int32;		// 32 bits	- 4 bytes
	typedef unsigned int		u_int32;	// 32 bits	- 4 bytes	<0, 4 294 967 295>
	typedef float				float32;	// 32 bits	- 4 bytes
	typedef double				float64;
#endif

#if defined AMIGA
	#include <proto/exec.h>

	typedef BYTE				int8;		
	typedef UBYTE				u_int8;		
	typedef WORD				int16;		
	typedef	UWORD				u_int16;	
	typedef LONG				int32;		
	typedef ULONG				u_int32;	
	typedef float				float32;
	typedef double				float64;
#endif


// -------------------------------
// --- DIFFERENT PIXEL FORMATS ---
// -------------------------------

#define IO_PIXFMT_RGB16			5
#define IO_PIXFMT_BGR16			6
#define IO_PIXFMT_RGB16PC		7
#define IO_PIXFMT_BGR16PC		8
#define IO_PIXFMT_RGB24			9
#define IO_PIXFMT_BGR24			10
#define IO_PIXFMT_ARGB32		11
#define IO_PIXFMT_BGRA32		12
#define IO_PIXFMT_RGBA32		13


// ---------------------------------
// --- KEYCODES - WINDOWS ----------
// ---------------------------------

#if defined _WIN32
	#define IO_KEYCODE_ESC		0x1B

	#define IO_KEYCODE_W		0x57
	#define IO_KEYCODE_S		0x53
	#define IO_KEYCODE_A		0x41
	#define IO_KEYCODE_D		0x44

	#define IO_KEYCODE_UP		0x26
	#define IO_KEYCODE_DOWN		0x28
	#define IO_KEYCODE_LEFT		0x25
	#define IO_KEYCODE_RIGHT	0x27
#endif


// -------------------------------
// --- KEYCODES - AMIGA ----------
// -------------------------------

#if defined AMIGA
	#define IO_KEYCODE_ESC		0x45

	#define IO_KEYCODE_W		0x11
	#define IO_KEYCODE_S		0x21
	#define IO_KEYCODE_A		0x20
	#define IO_KEYCODE_D		0x22

	#define IO_KEYCODE_UP		0x4C
	#define IO_KEYCODE_DOWN		0x4F
	#define IO_KEYCODE_LEFT		0x4D
	#define IO_KEYCODE_RIGHT	0x4E
#endif


// --------------------------
// --- Directories ----------
// --------------------------

#if defined _WIN32
	#if defined _DEBUG
		#define IO_LEVELS_DIRECTORY			"..\\..\\TheGameOutput\\data\\levels\\"
		#define IO_WALL_TEXTURES_DIRECTORY	"..\\..\\TheGameOutput\\data\\wall_textures\\" 
		#define IO_FLAT_TEXTURES_DIRECTORY	"..\\..\\TheGameOutput\\data\\flat_textures\\" 
		#define IO_LIGHTMAPS_DIRECTORY		"..\\..\\TheGameOutput\\data\\lightmaps\\" 
	#else
		#define IO_LEVELS_DIRECTORY			"data\\levels\\"
		#define IO_WALL_TEXTURES_DIRECTORY	"data\\wall_textures\\" 
		#define IO_FLAT_TEXTURES_DIRECTORY	"data\\flat_textures\\" 
		#define IO_LIGHTMAPS_DIRECTORY		"data\\lightmaps\\" 
	#endif
#endif

#if defined AMIGA
	#define IO_LEVELS_DIRECTORY			"data/levels/"
	#define IO_WALL_TEXTURES_DIRECTORY	"data/wall_textures/"
	#define IO_FLAT_TEXTURES_DIRECTORY	"data/flat_textures/"
	#define IO_LIGHTMAPS_DIRECTORY		"data/lightmaps/"
#endif

// ------------------------
// --- File extensions ----
// ------------------------

#define IO_LEVEL_FILE_EXTENSION				".lv"
#define IO_WALL_TEXTURE_FILE_EXTENSION		".tw"
#define IO_FLAT_TEXTURE_FILE_EXTENSION		".tf"
#define IO_LIGHTMAP_FILE_EXTENSION			".lm"

// --------------------------------
// --- Engine States constants ----
// --------------------------------

// Different Engine States maked as 'enum' to be auto-numerated. 
// EN_STATE_GAMEPLAY is the main game rendering state and should be equal to 0
// so we can use fast test against 0.
enum { EN_STATE_GAMEPLAY_RUN = 0, EN_STATE_GAMEPLAY_BEGIN, EN_STATE_GAMEPLAY_CLEANUP, EN_STATE_END };

// --------------------------
// --- Textures constants ---
// --------------------------

#define IO_TEXTURE_MAX_COLORS				128
#define IO_TEXTURE_MAX_SHADES				128

#define IO_TEXTURE_COLOR_DATA_BYTE_SIZE		(IO_TEXTURE_MAX_COLORS * IO_TEXTURE_MAX_SHADES * 4)						// (IO_TEXTURE_MAX_COLORS * IO_TEXTURE_MAX_SHADES * 4 bytes)
#define IO_TEXTURE_IMAGE_DATA_BYTE_SIZE		(256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4)	// (256*256 + 128*128 + 64*64 + 32*32 + 16*16 + 8*8 + 4*4) * 1 byte

// --------------------------
// --- Lightmap constants ---
// --------------------------

#define IO_LIGHTMAP_MAX_SHADES				IO_TEXTURE_MAX_SHADES
#define IO_LIGHTMAP_SIZE					32										// single lightmap size: 32x32 px
#define IO_LIGHTMAP_BYTE_SIZE				(IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE)	// single lightmap byte size: 1024 bytes

// --------------------------
// --- Some other helpers ---
// --------------------------

#define IO_MAX_STRING_PATH_FILENAME		256
#define IO_MAX_STRING_TITLE				32
#define IO_MAX_STRING_TEX_FILENAME		16

// ----------------------------------------------------------
// --- INPUT STRUCTURE - holds global keys and mouse states, etc. ---
// ----------------------------------------------------------

typedef struct
{
	int8	keys[128];
	int16	mouse_x;
	int16	mouse_y;
	int8	mouse_dx;
	int8	mouse_dy;
	int8	mouse_left;
	int8	mouse_right;
} sIO_Input;

extern sIO_Input	IO_input;

// --------------------------------------------------------------
// --- PREFERENCES STRUCTURE - holds some global informations ---
// --------------------------------------------------------------

typedef struct
{
	float32		delta_time;

	int32		screen_frame_buffer_size;
	u_int32*	output_buffer_32;			// holds pointer to 32 bit output buffer

	int16		screen_width;
	int16		screen_height;
	int16		screen_width_center;
	int16		screen_height_center;

	int8		screen_pixel_format;
	int8		screen_bytes_per_pixel;
	int8		ch1, ch2, ch3;				// channels, will be used to display correct colors (by shifting) in different screen pixel formats	in 32 mode		

	int8		engine_state;
} sIO_Prefs;

extern sIO_Prefs	IO_prefs;

#endif
