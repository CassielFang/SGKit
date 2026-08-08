#version 330 core

// SGKit — Point-light shadow depth geometry shader
// Renders each triangle into all 6 cubemap faces via gl_Layer.

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 u_ShadowMatrices[6];
uniform vec3 u_LightPos;

out vec4 g_WorldPos;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;
        for (int i = 0; i < 3; ++i)
        {
            vec4 worldPos = gl_in[i].gl_Position;
            g_WorldPos    = worldPos;
            vec4 relPos   = vec4(worldPos.xyz - u_LightPos, 1.0);
            gl_Position   = u_ShadowMatrices[face] * relPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
