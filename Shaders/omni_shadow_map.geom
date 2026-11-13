#version 460
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 lightMatrices[6];
out vec4 FragPos;

void main()
{
    // One incoming triangle becomes six: one per cube face, routed with gl_Layer.
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;
        for (int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = lightMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
