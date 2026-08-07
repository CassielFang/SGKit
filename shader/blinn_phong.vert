#version 330 core

// SGKit Blinn-Phong Vertex Shader
//
// Vertex layout: location 0=position(3)  1=normal(3)  2=texCoord(2)

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;

out vec3 worldPos;
out vec3 normal;
out vec2 texCoord;

void main()
{
    vec4 worldVertex = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldVertex;
    worldPos    = worldVertex.xyz;
    texCoord    = a_TexCoord;
    normal      = mat3(u_Model) * a_Normal;
}
