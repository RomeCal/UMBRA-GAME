#version 460
layout (location = 0) in vec2 pos;      // pixels, origin bottom-left
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 col;
out vec2 UV;
out vec4 Col;
uniform mat4 projection;                 // glm::ortho(0, width, 0, height, -1, 1)
void main()
{
    UV = uv;
    Col = col;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
