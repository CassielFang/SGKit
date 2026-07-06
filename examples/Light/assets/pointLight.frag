#version 330 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
struct Light {
    vec3 position;
    
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
uniform Light u_Light;

in vec2 texCoord;
in vec3 worldPos;
in vec3 normal;
out vec4 fragColor;

void main()
{
    // 环境光
    vec3 ambient = u_Light.ambient * vec3(texture(u_Material.diffuse, texCoord));

    // 漫反射
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(u_Light.position - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = u_Light.diffuse * diff * vec3(texture(u_Material.diffuse, texCoord));

    // 镜面光
    vec3 viewDir = normalize(u_cameraPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.shininess);
    vec3 specular = u_Light.specular * spec * vec3(texture(u_Material.specular, texCoord));

    // 衰减
    float distance = length(u_Light.position - worldPos);
    float attenuation = 1.0f / (u_Light.constant + u_Light.linear * distance + u_Light.quadratic * distance * distance);

    fragColor = vec4((ambient + diffuse + specular) * attenuation, 1.0);
}
