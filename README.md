# msRay

This is my AMIGA/PC Raycaster project. 
The main engine files are OS independent, so it can be easly adapted to OS
that uses RGBA buffer redering.

The project contains:
- engine common files
- AMIGA RTG framework
- Windows framework
- level editor (Windows)
- executables for Amiga and Windows (with demo level)

The project was mainly targeted to Amigas with 32-bit RTG and faster CPUs,
so "modernish" cards like V1200, IceDrake, Firebird, Emu68, PIStrom, PiAmiga, Warp1260 etc. are recommended.

The FPS speed on Amiga 1200 with V1200 card in 320x200 was 30-40 average.

You can find there many interesting things, examples and ideas if you are interested in
Amiga RTG coding, raycasting coding, ReAction GUI windows, etc.

The latest msDevPack 0.32 have some interesting features:
- textured walls/floors/ceilings
- mipmaps from 256x256px to 4x4px
- baked lightmaps
- distance shading
- glowing pixels

check out the video:
https://www.youtube.com/watch?v=__vXgtkBG8c
