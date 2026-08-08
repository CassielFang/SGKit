#version 330 core

// SGKit — Normal-debug vertex shader
// Passes world-space position + normal through to the geometry shader.

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_Model;

out vec3 v_WorldPos;
out vec3 v_Normal;

void main()
{
    vec4 world = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = world.xyz;
    v_Normal   = mat3(u_Model) * a_Normal;
}
