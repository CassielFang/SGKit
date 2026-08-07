#version 330 core

// SGKit Blinn-Phong Fragment Shader

struct PointLightData { vec4 pos, amb, diff, spec, atten; };
struct SpotLightData  { vec4 pos, dir, amb, diff, spec, attenCut, outerCutPad; };
layout(std140) uniform FrameBlock {
    mat4 viewProjection;
    vec4 cameraPos;
    vec4 dirDirection;
    vec4 dirAmbient;
    vec4 dirDiffuse;
    vec4 dirSpecular;
    PointLightData pointLights[4];
    SpotLightData  spotLights[4];
    ivec4 lightCounts;
    mat4 lightSpaceMatrix;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};
uniform Material u_Material;
uniform sampler2D u_ShadowMap;
uniform mat4      u_CSM_LightMatrices[3];
uniform float     u_CSM_Splits[3];
uniform float     u_CSM_TexelSize;
uniform float     u_CSM_CascadeCount;
uniform bool      u_ShadowsEnabled = false;

in vec3 worldPos;
in vec3 normal;
in vec4 fragPosLightSpace;
in vec2 texCoord;
out vec4 fragColor;

float ShadowCalculation(vec3 worldP, vec3 N, vec3 lightDir)
{
    if (!u_ShadowsEnabled) return 0.0;
    float viewZ = length(worldP - cameraPos.xyz);
    int cascade = 0;
    for (int c = 0; c < int(u_CSM_CascadeCount)-1; ++c)
        if (viewZ > u_CSM_Splits[c]) cascade = c+1;
    vec4 fragLS = u_CSM_LightMatrices[cascade] * vec4(worldP, 1.0);
    vec3 proj = fragLS.xyz / fragLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    proj.x = (proj.x + float(cascade)) / u_CSM_CascadeCount;
    float bias = max(0.08*(1.0-dot(N,lightDir)), 0.01);
    float shadow = 0.0;
    vec2 ts = vec2(u_CSM_TexelSize);
    for (int x=-1; x<=1; ++x)
        for (int y=-1; y<=1; ++y)
            shadow += (proj.z-bias > texture(u_ShadowMap, proj.xy+vec2(x,y)*ts).r) ? 1.0 : 0.0;
    return shadow/9.0;
}

vec3 CalcDir(vec3 N, vec3 V)
{
    vec3 L = normalize(-dirDirection.xyz);
    vec3 H = normalize(L+V);
    vec3 albedo = pow(texture(u_Material.diffuse,texCoord).rgb, vec3(2.2));
    vec3 specC  = texture(u_Material.specular,texCoord).rgb;
    float spec  = pow(max(dot(N,H),0.0), u_Material.shininess);
    return dirAmbient.rgb*albedo + (dirDiffuse.rgb*albedo*max(dot(N,L),0.0))
                                 + (dirSpecular.rgb*specC*spec);
}

vec3 CalcPoint(int i, vec3 N, vec3 P, vec3 V)
{
    vec3 lp = pointLights[i].pos.xyz;
    vec3 L = normalize(lp-P);
    vec3 H = normalize(L+V);
    vec3 albedo = pow(texture(u_Material.diffuse,texCoord).rgb, vec3(2.2));
    vec3 specC  = texture(u_Material.specular,texCoord).rgb;
    float spec  = pow(max(dot(N,H),0.0), u_Material.shininess);
    float d = length(lp-P);
    float atten = 1.0/(pointLights[i].atten.x+pointLights[i].atten.y*d
                                        +pointLights[i].atten.z*d*d);
    return (pointLights[i].amb.rgb*albedo
          + pointLights[i].diff.rgb*albedo*max(dot(N,L),0.0)
          + pointLights[i].spec.rgb*specC*spec) * atten;
}

vec3 CalcSpot(int i, vec3 N, vec3 P, vec3 V)
{
    vec3 lp = spotLights[i].pos.xyz;
    vec3 L = normalize(lp-P);
    vec3 H = normalize(L+V);
    vec3 albedo = pow(texture(u_Material.diffuse,texCoord).rgb, vec3(2.2));
    vec3 specC  = texture(u_Material.specular,texCoord).rgb;
    float spec  = pow(max(dot(N,H),0.0), u_Material.shininess);
    float theta = dot(L, normalize(-spotLights[i].dir.xyz));
    float eps = spotLights[i].attenCut.w - spotLights[i].outerCutPad.x;
    float intensity = clamp((theta-spotLights[i].outerCutPad.x)/eps, 0.0,1.0);
    float d = length(lp-P);
    float atten = 1.0/(spotLights[i].attenCut.x+spotLights[i].attenCut.y*d
                                              +spotLights[i].attenCut.z*d*d);
    return (spotLights[i].amb.rgb*albedo
          + spotLights[i].diff.rgb*albedo*max(dot(N,L),0.0)
          + spotLights[i].spec.rgb*specC*spec) * atten * intensity;
}

void main()
{
    vec3 N = normalize(normal);
    vec3 V = normalize(cameraPos.xyz - worldPos);
    vec3 color = vec3(0.0);

    if (lightCounts.x > 0) {
        float shadow = ShadowCalculation(worldPos, N, normalize(-dirDirection.xyz));
        color += (1.0-shadow) * CalcDir(N, V);
    }
    for (int i=0; i<lightCounts.y && i<4; ++i) color += CalcPoint(i, N, worldPos, V);
    for (int i=0; i<lightCounts.z && i<4; ++i) color += CalcSpot(i, N, worldPos, V);

    color = pow(color, vec3(1.0/2.2));
    fragColor = vec4(color, 1.0);
}
