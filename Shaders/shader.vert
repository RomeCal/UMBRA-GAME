#version 460
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 norm;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 DirectionalLightSpacePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;               // transpose(inverse(mat3(model))), computed once per object on the CPU
uniform mat4 directionalLightTransform;
uniform vec2 uvScale;

void main()
{
    vec4 world = model * vec4(pos, 1.0);
    gl_Position = projection * view * world;
    DirectionalLightSpacePos = directionalLightTransform * world;
    TexCoord = tex * uvScale;
    Normal = normalMatrix * norm;
    FragPos = world.xyz;
}
