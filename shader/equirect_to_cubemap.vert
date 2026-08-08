#version 330 core

// SGKit — Equirectangular to Cubemap vertex shader
// Renders a unit cube; the vertex position IS the direction vector
// used to sample the equirectangular map in the fragment shader.

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;

out vec3 v_LocalPos;

void main()
{
    v_LocalPos  = a_Position;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
