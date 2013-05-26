//
// FIXME:  The  #version  directive  must  occur in  a  shader  before
// anything else, except for comments and white space.
//
#if __VERSION__ >= 400
#version 400
#define LAYOUT_LOCATION(n) layout(location = n)
#else
#version 130
#define LAYOUT_LOCATION(n) /* layout(location = n) */
#endif

LAYOUT_LOCATION(0) in vec3 VerPos;
/* Have to use this variable! Or it will be very hard to debug for AMD
   video card */
LAYOUT_LOCATION(1) in vec3 VerClr;


out vec3 EntryPoint;
out vec4 ExitPointCoord;

uniform mat4 MVP;

void main()
{
    EntryPoint = VerClr;
    gl_Position = MVP * vec4(VerPos,1.0);
    // ExitPointCoord 输入到fragment shader 的过程中经过rasterization， interpolation, assembly primitive
    ExitPointCoord = gl_Position;
}
