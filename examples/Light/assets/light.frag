#version 330 core

struct Material {
    sampler2D texture;

    vec3 ambient;
    vec3 diffuse;
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
    fragColor = vec4(u_Light.ambient + u_Light.diffuse + u_Light.specular, 1.0);
}
