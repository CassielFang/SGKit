#version 330 core

// SGKit — Normal-debug geometry shader
// Emits a short line segment from each vertex along its normal direction.

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

uniform mat4 viewProjection;
uniform float u_NormalLength = 0.3;

in vec3 v_WorldPos[];
in vec3 v_Normal[];

out vec3 g_Color;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        vec3 start  = v_WorldPos[i];
        vec3 end    = start + normalize(v_Normal[i]) * u_NormalLength;

        g_Color     = vec3(1.0, 1.0, 0.0);  // yellow
        gl_Position = viewProjection * vec4(start, 1.0);
        EmitVertex();

        g_Color     = vec3(1.0, 1.0, 0.0);
        gl_Position = viewProjection * vec4(end, 1.0);
        EmitVertex();

        EndPrimitive();
    }
}
