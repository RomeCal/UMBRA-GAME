#version 460
layout (location = 0) in vec3 pos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;            // translation removed on the CPU
void main()
{
    TexCoords = pos;
    vec4 p = projection * view * vec4(pos, 1.0);
    gl_Position = p.xyww;     // depth = 1.0 everywhere, so the sky is always behind the level
}
