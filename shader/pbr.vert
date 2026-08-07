#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// Per-instance
uniform mat4 u_Model;
uniform bool u_Instanced = false;
uniform mat4 u_ModelMatrices[100];

// Frame data (shared UBO binding=0)
struct PointLightData  { vec4 pos, amb, diff, spec, atten; };
struct SpotLightData   { vec4 pos, dir, amb, diff, spec, attenCut, outerCutPad; };
layout(std140) uniform FrameBlock {
    mat4 viewProjection;
    vec4 cameraPos;
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
out vec4 fragPosLightSpace;

void main()
{
    mat4 model = u_Instanced ? u_ModelMatrices[gl_InstanceID] : u_Model;
    vec4 worldVertex = model * vec4(a_Position, 1.0);
    gl_Position = viewProjection * worldVertex;
    worldPos    = worldVertex.xyz;
    texCoord    = a_TexCoord;
    normal      = mat3(model) * a_Normal;
    fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
}
