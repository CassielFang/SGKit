#version 330 core

// SGKit — Normal-debug fragment shader

in vec3 g_Color;
out vec4 fragColor;

void main()
{
    fragColor = vec4(g_Color, 1.0);
}
