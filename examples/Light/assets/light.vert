#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

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

out vec2 texCoord;
out vec3 worldPos;
out vec3 normal;

void main()
{
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    worldPos = vec3(u_Model * vec4(a_Position, 1.0));
    texCoord = a_TexCoord;
    normal = a_Normal;
}
