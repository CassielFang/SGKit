#version 330 core

// SGKit — Point-light shadow depth fragment shader
// Writes linear distance (world units) mapped to [0,1] via far_plane.

in vec4 g_WorldPos;

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main()
{
    float dist = length(g_WorldPos.xyz - u_LightPos) / u_FarPlane;
    gl_FragDepth = dist;
}
