# msRay

PROJECT OVERVIEW

This is my AMIGA / WINDOWS Raycaster project. 
The main engine files has been written in C without using any external libraries, so the engine is OS independent. It can be easly adapted to OS that uses 32 bit RGBA buffer rendering.

**Please note**, that at the beginning the project was based on great [Lode Vandevenne raycasting tutorial.](https://lodev.org/cgtutor/index.html "Lode Vandevenne raycasting tutorial") If You are interested in raycasting, you should take a look on this tutorial. It is well explained and also contains sources in C. If you wish to understand my code its suggested that you take a look at that tutorial first.

However, as my project evolved, I added many optimalizations and tested different ways to render floor and ceiling trying to get as much performance I could. 

------------

**The devpacks contains:**
- engine source files
- AMIGA OS framework for RTG and HAM8 (as Visual Code project)
- WINDOWS framework (as Visual Studio 2019 project)
- Level Editor (for Windows, as Visual Studio 2019 project)
- executables for Amiga OS and Windows (with demo level)

**The goals:**
- The project was mainly targeted to Amigas with **32-bit RTG** and faster CPUs like m68060 or "modernish" cards like V1200, IceDrake, Firebird, Emu68, PIStrom, PiAmiga, Warp1260 etc.
- One of the main goals was to create a fully 32 bit raycaster renderer, that would be able to run in decent FPS.
- Also, the full 32bit ARGB render was converted into native **Amiga AGA HAM8** mode, that was able to display thousands of colors: 
For this "chunky to planar" conversions I used kalms-c2p routines collection: https://github.com/Kalmalyzer/kalms-c2p
- **The goals were achieved**. The engine with some advanced features (like lightmaps) run very smooth on V1200 turbo card in 384x216 screen and other "modernish" cards like PIStorm, Emu68, Warp1260. But it also run very decent on older Amiga cards like CyberStorm Mk2 68060 50 Mhz.

**Results:**
- Check out this YT playlist for videos of final results and progress: https://www.youtube.com/playlist?list=PLXyg79wsI88GWKknk59lt5cFlR7HUVYR7
- Creating simple level with lightmaps: https://youtu.be/q8jkLqvP4ZQ
- Windows/Amiga builds comparision: https://youtu.be/iEUx0xVAae0
- HAM8 mode result: https://youtu.be/wv3OYbADkJQ
- RTG result: https://youtu.be/v5oOLcYEvoM

**Other:**
- You can find here many interesting things, examples, source codes and ideas if you are interested in Amiga RTG coding, raycasting coding, raycasting algorithms, ReAction GUI windows, etc.

------------


**msRay_devpack_v0.36 features:**
- free head movement by mouse moving
- non regular walls
- horizontal and vertical doors
- walls/doors collisions
- mip mapping walls and flats texturing (from 128px to 32px)
- baked lighting
- 'glowing pixels' (not affected by shadows)
- info panel (auto scaled to screen width)
- 32bit RTG and HAM8 output
- level editor


------------
