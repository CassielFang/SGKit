#version 330 core

// SGKit — Skybox fragment shader

in vec3 v_TexCoord;

uniform samplerCube u_Skybox;

out vec4 fragColor;

void main()
{
    fragColor = texture(u_Skybox, v_TexCoord);
}
