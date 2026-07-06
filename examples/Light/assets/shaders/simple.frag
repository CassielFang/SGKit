#version 330 core

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

in vec2 texCoord;
in vec3 worldPos;
in vec3 normal;
out vec4 fragColor;

void main()
{
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
