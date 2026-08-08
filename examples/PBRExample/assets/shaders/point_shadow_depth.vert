#version 330 core

// SGKit — Point-light shadow depth vertex shader
// Transforms geometry to world space; geometry shader projects to 6 faces.

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Model;

void main()
{
    gl_Position = u_Model * vec4(a_Position, 1.0);
}
