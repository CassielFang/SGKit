#version 330 core

// SGKit — Equirectangular to Cubemap fragment shader
// Converts a direction vector to spherical coordinates and samples
// an equirectangular HDR map.

in vec3 v_LocalPos;

uniform sampler2D u_EquirectMap;

out vec4 fragColor;

const vec2 k_InvAtan = vec2(0.1591, 0.3183);  // 1/(2π), 1/π

vec2 DirToUV(vec3 dir)
{
    vec3 d = normalize(dir);
    float u = 0.5 + atan(d.z, d.x) * k_InvAtan.x;
    float v = 0.5 + asin(d.y)          * k_InvAtan.y;
    return vec2(u, v);
}

void main()
{
    vec2 uv = DirToUV(v_LocalPos);
    fragColor = vec4(texture(u_EquirectMap, uv).rgb, 1.0);
}
