#version 330 core

// SGKit Shadow Map Vertex Shader  (directional light depth-only pass)
// Renders geometry from the light's perspective into a depth texture.

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}
