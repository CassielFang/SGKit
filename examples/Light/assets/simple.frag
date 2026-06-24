#version 330 core

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
};
struct Light {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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
    vec3 specular = u_Light.specular * spec * u_Material.specular;

    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
