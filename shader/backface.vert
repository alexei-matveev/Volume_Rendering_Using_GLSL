// for raycasting
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
LAYOUT_LOCATION(1) in vec3 VerClr;

out vec3 Color;

uniform mat4 MVP;


void main()
{
    Color = VerClr;
    gl_Position = MVP * vec4(VerPos, 1.0);
}
