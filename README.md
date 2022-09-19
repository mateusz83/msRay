# msRay

PROJECT OVERVIEW

This is my AMIGA / WINDOWS Raycaster project. 
The main engine files written in C without using any external libraries, so the engine is OS independent. It can be easly adapted to OS that uses 32 bit RGBA buffer rendering.

**Please note**, that at the beginning the project was based on great [Lode Vandevenne raycasting tutorial.](https://lodev.org/cgtutor/index.html "Lode Vandevenne raycasting tutorial") If You are interested in raycasting, you should take a look on this tutorial. It is well explained and also contains sources in C.

As my project evolved, I added many optimalizations and tested different ways to render floor and ceiling trying to get as much performance I could. If you wish to understand my code its suggested that you take a look at  [Lode Vandevenne raycasting tutorial.](https://lodev.org/cgtutor/index.html "Lode Vandevenne raycasting tutorial") first.

------------



The devpacks contains:
- engine source files in C
- AMIGA OS framework for RTG
- WINDOWS framework
- level editor (Windows)
- executables for Amiga OS and Windows (with demo level)

The goals:

 - The project was mainly targeted to Amigas with **32-bit RTG** and faster CPUs, so "modernish" cards like V1200,  IceDrake, Firebird, Emu68, PIStrom, PiAmiga, Warp1260 etc. are recommended.
 - One of the main goals was to create a fully 32 bit raycaster renderer, that would be able to run in decent FPS.

The other tests:

 - But during the tests also the **8 bit** version was made and was converted to **Amiga AGA** native display chip thanks to "chunky to planar" conversions: https://youtu.be/keYe7AltI4w
 - Also, in another test the full 32bit ARGB render was converted into **Amiga AGA HAM8** mode,
that was able to display over 10k colors: https://youtu.be/SwnDqj8pNe4

Other:

 - You can find here many interesting things, examples, source codes and ideas if you are interested in Amiga RTG coding, raycasting coding, raycasting algorithms, ReAction GUI windows, etc.
 - Check out the playlist of YT videos for details and progress: https://www.youtube.com/playlist?list=PLXyg79wsI88GWKknk59lt5cFlR7HUVYR7



------------


**History log, progress and features**

*msRay_devpack_v0.34__amiga_ham8:*
- this is a test that takes v0.34 and its fully 32bit ARGB render output (at 320x256) and converts it "on fly" into Amiga 1200/400 AGA Ham8 mode, that is capable to display hundreds of thousands colors.
- only Amiga Framework C sources changed. Adapted to use chunky to planar ham8 convertion routines

*msRay_devpack_v0.34:*
- the previous floor/ceiling rendering algorithm has been corrected and is faster. The lists of potentially visible cells are no longer created in editor and stored in level file, but created "on fly" during wall casting. Thanks to that the list are shorter and ther is less flats overdrawn by walls. Additionally the horizontal pixels are doubled - no big difference in quality but more performance.
- memory usage reduction
- updated editor
- textured walls/floors/ceilings
- mipmaps from 128x128 to 32x32 px (reduced)
- baked lightmaps
- distance shading
- glowing pixels

*msRay_devpack_v0.33:*
- textured walls/floors/ceilings
- new, faster floor/ceiling rendering algorithm (that uses screen projection and Sutherland-Hodgman polygon clipping)
- mipmaps from 256x256 px to 32x32 px
- baked lightmaps
- distance shading
- glowing pixels

*msRay_devpack_v0.32:*
- textured walls/floors/ceilings
- mipmaps from 256x256 px to 4x4 px
- baked lightmaps
- distance shading
- glowing pixels


------------
