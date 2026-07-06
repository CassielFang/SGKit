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

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 worldPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 worldPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(u_cameraPos - worldPos);

    vec3 result = vec3(0.0, 0.0, 0.0);
    if (u_dLightCount > 0)
    {
        result += CalcDirectionalLight(u_DirectionalLight, norm, viewDir);
    }
    for (int i = 0; i < u_pLightCount && i < 4; ++i)
    {
        result += CalcPointLight(u_PointLights[i], norm, worldPos, viewDir);
    }
    for (int i = 0; i < u_sLightCount && i < 4; ++i)
    {
        result +=  CalcSpotLight(u_SpotLights[i], norm, worldPos, viewDir);
    }

    fragColor = vec4(result, 1.0);
}

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 ambient = light.ambient * vec3(texture(u_Material.diffuse, texCoord));
    
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(u_Material.diffuse, texCoord));
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(u_Material.specular, texCoord));

    return (ambient + diffuse + specular);
}
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 worldPos, vec3 viewDir)
{
    vec3 ambient = light.ambient * vec3(texture(u_Material.diffuse, texCoord));

    vec3 lightDir = normalize(light.position - worldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(u_Material.diffuse, texCoord));

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(u_Material.specular, texCoord));

    float distance = length(light.position - worldPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    return (ambient + diffuse + specular) * attenuation;
}
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 worldPos, vec3 viewDir)
{
    vec3 ambient = light.ambient * vec3(texture(u_Material.diffuse, texCoord));

    vec3 lightDir = normalize(light.position - worldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(u_Material.diffuse, texCoord));

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(u_Material.specular, texCoord));

    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;

    float distance = length(light.position - worldPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    return (ambient + diffuse + specular) * attenuation;
}
