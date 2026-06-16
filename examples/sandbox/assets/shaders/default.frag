#version 330 core

in vec2 v_TexCoord;

uniform sampler2D u_DiffuseTexture;

out vec4 fragColor;

void main()
{
    fragColor = texture(u_DiffuseTexture, v_TexCoord);
}
