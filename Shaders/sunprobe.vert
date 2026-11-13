#version 460
layout (location = 0) in vec3 pos;
void main()
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);   // the one pixel of a 1x1 framebuffer
}
