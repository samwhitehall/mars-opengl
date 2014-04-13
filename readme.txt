==PROGRAM OPERATION==
* standard controls (as defined in the spec.)
    - <ESC>/Q: quit
    - P: move to screenshot location
    - T: begin automated tour
    - <LEFT>/<RIGHT>: rotation on horizontal plane
* a "free roam" mode was developed (mostly for testing)
    - F: activate free roam mode
    - <LEFT>/<RIGHT>: rotation on horizontal pane
    - W/A/S/D: move on horizontal plane
    - <UP>/<DOWN>: alter camera altitude

"E" for earthquake (mars quake?) animation.

Instead of "P" for screenshot location, this is the default view when loading,
and is the start & end of the tour.

==FILE LIST==
* main.cpp - "high level" functions, such as those for handling models (in terms
                of material/position), cameras and lights.
* utils.cpp - collection of lower level facilities, such as loading files,
                buffering, compiling shaders etc.
* utils.h - contains definitions of program structs

* vert.glsl - basic vertex shader
* frag.glsl - basic fragment shader, with ambient & diffuse per pixel lighting,
		& texture interpolation.

* terrain.obj - Wavefront OBJ file with the basic terrain mesh

==BUILD INSTRUCTIONS==
This was built and tested on Mac OS X 10.8 (Mountain Lion). It has also been tested
on the Linux lab machines. Uses GLEW, glfw and glm (maths library, not the other one).

==PROGRAM FUNCTIONALITY==
The main program features (camera and light) are held in structs. See utils.h and
comments in here for more details. Most of the low level OpenGL code, such as .obj
loading, buffer binding, etc. is specified in functions in utils.cpp and the code
in main.cpp is mostly for making high level changes to these structs (e.g. moving
the camera).

There was supposed to be multiple objects & animation in the middle, but annoyingly I only 
importing multiple meshes early-on in the project, and seem to have introduced a bug
somewhere that corrupts the appearance of non-identical meshes (but two objects with 
the same .obj file imported separately are fine). I think this is either an issue with 
my code for handling low-level OpenGL operations (e.g. I'm writing into the same buffer
each time) or possibly handling some transformations in the wrong space, distorting the
vertex positions.

As a last-minute measure, to show that there is /some/ animation capability (albeit
without multiple objects), press 'E' for 'earthquake mode'.

==ACKNOWLEDGEMENTS==
Much use was made of the Wikibooks "Modern OpenGL Programming Guide"

Textures originally from mb3d.co.uk, which are free for non-commercial purposes.
Some have been modified.
