#version 330 core

// SGKit PBR Vertex Shader  (Cook-Torrance metallic-roughness + optional IBL)
//
// Vertex layout matches SGKit convention:  loc 0=position  1=normal  2=texCoord.
// Light struct definitions are in the fragment shader (they aren't needed here).

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;
uniform mat4 u_LightSpaceMatrix;

out vec3 worldPos;
out vec3 normal;
out vec2 texCoord;
out vec4 fragPosLightSpace;

void main()
{
    vec4 worldVertex = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldVertex;
    worldPos    = worldVertex.xyz;
    texCoord    = a_TexCoord;
    normal      = mat3(u_Model) * a_Normal;
    fragPosLightSpace = u_LightSpaceMatrix * vec4(worldPos, 1.0);
}
