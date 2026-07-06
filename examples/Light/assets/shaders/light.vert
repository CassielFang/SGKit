#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirectionalLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct PointLight
{
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;
uniform vec3 u_cameraPos;
uniform Material u_Material;

uniform DirectionalLight u_DirectionalLight;
uniform PointLight       u_PointLights[4];
uniform SpotLight        u_SpotLights[4];
uniform int u_dLightCount, u_pLightCount, u_sLightCount;

out vec2 texCoord;
out vec3 worldPos;
out vec3 normal;

void main()
{
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    worldPos = vec3(u_Model * vec4(a_Position, 1.0));
    texCoord = a_TexCoord;
    normal = mat3(u_Model) * a_Normal;
}
