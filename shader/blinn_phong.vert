#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_Model;

struct PointLightData { vec4 pos, amb, diff, spec, atten; };
struct SpotLightData  { vec4 pos, dir, amb, diff, spec, attenCut, outerCutPad; };
layout(std140) uniform FrameBlock {
    mat4 viewProjection;
    vec4 cameraPos;
    vec4 cameraForward;
    vec4 dirDirection;
    vec4 dirAmbient;
    vec4 dirDiffuse;
    vec4 dirSpecular;
    PointLightData pointLights[4];
    SpotLightData  spotLights[4];
    ivec4 lightCounts;
    mat4 lightSpaceMatrix;
};

out vec3 worldPos;
out vec3 normal;
out vec2 texCoord;

void main()
{
    vec4 worldVertex = u_Model * vec4(a_Position, 1.0);
    gl_Position = viewProjection * worldVertex;
    worldPos    = worldVertex.xyz;
    texCoord    = a_TexCoord;
    normal      = mat3(u_Model) * a_Normal;
}
