#version 460
in vec4 FragPos;
uniform vec3 lightPos;
uniform float farPlane;
void main()
{
    // Store world distance / farPlane (linear), so shader.frag can multiply it back up.
    gl_FragDepth = length(FragPos.xyz - lightPos) / farPlane;
}
