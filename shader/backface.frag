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

in vec3 Color;
LAYOUT_LOCATION(0) out vec4 FragColor;


void main()
{
    FragColor = vec4(Color, 1.0);
}
