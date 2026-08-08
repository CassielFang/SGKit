#version 330 core

// SGKit PBR Fragment Shader  (Cook-Torrance metallic-roughness)

// -- Frame UBO  (binding=0, shared across all shaders)
struct PointLightData { vec4 pos, amb, diff, spec, atten; };
struct SpotLightData  { vec4 pos, dir, amb, diff, spec, attenCut, outerCutPad; };
layout(std140) uniform FrameBlock {
    mat4 viewProjection;
    vec4 cameraPos;
    vec4 cameraForward;
    vec4 dirDirection;
    vec4 dirAmbient;
    vec4 dirDiffuse;
    vec4 dirSpecular;
    PointLightData pointLights[4];
    SpotLightData  spotLights[4];
    ivec4 lightCounts;
    mat4 lightSpaceMatrix;
};

// Debug: 0=normal, 1=albedo, 2=normal, 3=metallic, 4=roughness, 5=ao, 6=NaN(red)
uniform int u_DebugMode = 0;

// Per-instance
uniform mat4 u_Model;

// -- PBR Material  (slots: 0=albedo  1=metallic  2=roughness  3=normal  4=ao  5=emissive)
uniform sampler2D u_PBR_albedo;
uniform sampler2D u_PBR_metallic;
uniform sampler2D u_PBR_roughness;
uniform sampler2D u_PBR_normal;
uniform sampler2D u_PBR_ao;
uniform sampler2D u_PBR_emissive;
uniform vec3      u_PBR_emissiveFactor = vec3(0.0);
uniform float u_PBR_metallicFactor  = 1.0;
uniform float u_PBR_roughnessFactor = 1.0;
uniform float u_PBR_alphaFactor     = 1.0;
uniform float u_PBR_alphaCutoff     = 0.5;
uniform float u_PBR_normalScale     = 1.0;
uniform float u_PBR_aoStrength      = 1.0;
uniform int   u_PBR_combinedMR = 0;

// -- CSM shadows
uniform sampler2D u_ShadowMap;
uniform mat4      u_CSM_LightMatrices[3];
uniform float     u_CSM_Splits[3];
uniform float     u_CSM_TexelSize;
uniform float     u_CSM_AtlasTexelX;
uniform float     u_CSM_CascadeCount;
uniform bool      u_ShadowsEnabled = false;

// -- Point-light shadows
uniform int        u_PointShadows;
uniform samplerCube u_PointShadowMap[4];
uniform float      u_PointShadowFar[4];
uniform int        u_PointShadowEnabled[4];

// -- IBL
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D   u_BRDFLUT;
uniform bool        u_IBLEnabled = false;

in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;
in vec4 v_Tangent;
out vec4 fragColor;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

// -- Normal mapping
vec3 GetWorldNormal()
{
    vec3 texN = texture(u_PBR_normal, texCoord).xyz * 2.0 - 1.0;
    texN.xy *= u_PBR_normalScale;
    vec3 N = length(normal) > 0.0001 ? normalize(normal) : vec3(0, 1, 0);

    vec3 T, B;

    // Use pre-computed tangent if available, otherwise derivative fallback
    if (length(v_Tangent.xyz) > 0.0001)
    {
        T = normalize(v_Tangent.xyz);
        B = normalize(cross(N, T)) * v_Tangent.w;  // w = handedness (±1)
    }
    else
    {
        vec3 Q1 = dFdx(worldPos), Q2 = dFdy(worldPos);
        vec2 st1 = dFdx(texCoord), st2 = dFdy(texCoord);

        vec3 Traw = Q1 * st2.y - Q2 * st1.y;
        float lenT = length(Traw);
        T = lenT > 0.0001 ? normalize(Traw)
            : (abs(N.x) > 0.9 ? normalize(cross(N, vec3(0,1,0)))
                              : normalize(cross(N, vec3(1,0,0))));

        vec3 Braw = cross(N, T);
        float lenB = length(Braw);
        B = lenB > 0.0001 ? normalize(Braw)
            : (abs(N.x) > 0.9 ? normalize(cross(N, vec3(0,0,1)))
                              : normalize(cross(N, vec3(0,1,0))));
    }

    return normalize(mat3(T, B, N) * texN);
}

// -- BRDF
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness, a2 = a*a;
    float Nh = max(dot(N,H),0.0), Nh2 = Nh*Nh;
    return a2 / (PI * (Nh2*(a2-1.0)+1.0) * (Nh2*(a2-1.0)+1.0));
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness+1.0, k = (r*r)/8.0;
    return NdotV / (NdotV*(1.0-k)+k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N,L),0.0),roughness)
         * GeometrySchlickGGX(max(dot(N,V),0.0),roughness);
}
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0-F0) * pow(clamp(1.0-cosTheta,0.0,1.0), 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0-roughness),F0)-F0) * pow(clamp(1.0-cosTheta,0.0,1.0), 5.0);
}

vec3 CalcPBRRadiance(vec3 L, vec3 radiance, vec3 N, vec3 V,
                     vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 H = normalize(V+L);
    float D = DistributionGGX(N,H,roughness);
    float G = GeometrySmith(N,V,L,roughness);
    vec3  F = FresnelSchlick(max(dot(H,V),0.0), F0);
    vec3 specular = (D*G*F) / (4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.0001);
    vec3 kS = F;
    vec3 kD = (1.0-kS)*(1.0-metallic);
    return (kD*albedo/PI + specular) * radiance * max(dot(N,L),0.0);
}

// -- Radiance from UBO arrays
vec3 RadianceDir(vec3 N, vec3 V, vec3 albedo, float m, float r, vec3 F0)
{
    return CalcPBRRadiance(normalize(-dirDirection.xyz), dirDiffuse.rgb,
                           N,V,albedo,m,r,F0);
}
vec3 RadiancePoint(int i, vec3 N, vec3 V, vec3 P, vec3 albedo, float m, float r, vec3 F0)
{
    vec3 lp = pointLights[i].pos.xyz;
    vec3 L = normalize(lp-P);
    float d = length(lp-P);
    float atten = 1.0/(pointLights[i].atten.x + pointLights[i].atten.y*d
                                         + pointLights[i].atten.z*d*d);
    return CalcPBRRadiance(L, pointLights[i].diff.rgb*atten, N,V,albedo,m,r,F0);
}
vec3 RadianceSpot(int i, vec3 N, vec3 V, vec3 P, vec3 albedo, float m, float r, vec3 F0)
{
    vec3 lp = spotLights[i].pos.xyz;
    vec3 L = normalize(lp-P);
    float theta = dot(L, normalize(-spotLights[i].dir.xyz));
    float eps = spotLights[i].attenCut.w - spotLights[i].outerCutPad.x;
    float intensity = clamp((theta-spotLights[i].outerCutPad.x)/eps, 0.0,1.0);
    float d = length(lp-P);
    float atten = 1.0/(spotLights[i].attenCut.x + spotLights[i].attenCut.y*d
                                                + spotLights[i].attenCut.z*d*d);
    return CalcPBRRadiance(L, spotLights[i].diff.rgb*atten*intensity, N,V,albedo,m,r,F0);
}

// -- CSM directional shadow
float ShadowCalculation(vec3 worldP, vec3 N, vec3 lightDir)
{
    if (!u_ShadowsEnabled) return 0.0;

    // Select cascade by view-space depth along camera forward axis
    float viewZ = abs(dot(worldP - cameraPos.xyz, cameraForward.xyz));
    int cascade = 0;
    for (int c = 0; c < int(u_CSM_CascadeCount) - 1; ++c)
        if (viewZ > u_CSM_Splits[c]) cascade = c + 1;

    vec4 fragLS = u_CSM_LightMatrices[cascade] * vec4(worldP, 1.0);
    if (abs(fragLS.w) < 0.0001) return 0.0;
    vec3 proj = fragLS.xyz / fragLS.w;
    proj = proj * 0.5 + 0.5;
    // NaN/Inf guard + bounds check (NaN only true for x!=x)
    if (proj.z > 1.0 || proj.z != proj.z ||
        proj.x != proj.x || proj.y != proj.y ||
        proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 0.0;

    // Atlas UV: cascade-local X mapped to atlas strip
    proj.x = (proj.x + float(cascade)) / u_CSM_CascadeCount;

    float bias = max(0.008 * (1.0 - dot(N, lightDir)), 0.002);

    float shadow = 0.0;
    // Atlas X texels are 1/N narrower (N cascades packed horizontally)
    vec2 ts = vec2(u_CSM_AtlasTexelX, u_CSM_TexelSize);
    for (int x = -2; x <= 2; ++x)
        for (int y = -2; y <= 2; ++y)
            shadow += (proj.z - bias > texture(u_ShadowMap, proj.xy + vec2(x, y) * ts).r) ? 1.0 : 0.0;
    return shadow / 25.0;
}

// -- Omnidirectional point-light shadow (Poisson-disk PCF)
float PointShadowCalculation(int idx, vec3 worldP, vec3 N)
{
    if (u_PointShadows == 0 || u_PointShadowEnabled[idx] == 0) return 0.0;

    vec3 fragToLight = worldP - pointLights[idx].pos.xyz;
    float currentDepth = length(fragToLight);
    float farPlane = u_PointShadowFar[idx];
    if (currentDepth > farPlane) return 0.0;   // beyond shadow range

    const vec3 disk[20] = vec3[](
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );

    float bias = max(0.15 * (1.0 - dot(N, normalize(-fragToLight))), 0.05);
    float viewDist = length(cameraPos.xyz - worldP);
    float diskRadius = (1.0 + (viewDist / farPlane)) / 40.0;

    float shadow = 0.0;
    for (int s = 0; s < 20; ++s)
    {
        float closestDepth = texture(u_PointShadowMap[idx],
                                     fragToLight + disk[s] * diskRadius).r;
        closestDepth *= farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 20.0;
}

void main()
{
    vec3 albedo    = pow(texture(u_PBR_albedo, texCoord).rgb, vec3(2.2));
    vec4 mrSample  = texture(u_PBR_metallic, texCoord);
    float metallic  = ((u_PBR_combinedMR!=0) ? mrSample.b : mrSample.r) * u_PBR_metallicFactor;
    float roughness = ((u_PBR_combinedMR!=0) ? mrSample.g : texture(u_PBR_roughness,texCoord).r)
                      * u_PBR_roughnessFactor;
    float ao = 1.0 - (1.0 - texture(u_PBR_ao, texCoord).r) * u_PBR_aoStrength;
    vec3 N = GetWorldNormal();
    vec3 V = normalize(cameraPos.xyz - worldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    if (lightCounts.x > 0) {
        float shadow = ShadowCalculation(worldPos, N, normalize(-dirDirection.xyz));
        Lo += (1.0-shadow) * RadianceDir(N,V,albedo,metallic,roughness,F0);
    }
    for (int i=0; i<lightCounts.y && i<4; ++i)
    {
        float ptShadow = PointShadowCalculation(i, worldPos, N);
        Lo += (1.0 - ptShadow) * RadiancePoint(i, N,V,worldPos,albedo,metallic,roughness,F0);
    }
    for (int i=0; i<lightCounts.z && i<4; ++i)
        Lo += RadianceSpot(i, N,V,worldPos,albedo,metallic,roughness,F0);

    // Ambient
    vec3 ambient = u_IBLEnabled
        ? vec3(0.03) * albedo * ao  // TODO: proper IBL
        : vec3(0.03) * albedo * ao;

    vec3 emissiveCol = pow(texture(u_PBR_emissive, texCoord).rgb, vec3(2.2)) * u_PBR_emissiveFactor;
    vec3 color = ambient + Lo + emissiveCol;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    float alpha = texture(u_PBR_albedo, texCoord).a * u_PBR_alphaFactor;
    if (alpha < u_PBR_alphaCutoff) discard;

    // -- Debug overlay
    if      (u_DebugMode == 1) color = albedo;
    else if (u_DebugMode == 2) color = N * 0.5 + 0.5;
    else if (u_DebugMode == 3) color = vec3(metallic);
    else if (u_DebugMode == 4) color = vec3(roughness);
    else if (u_DebugMode == 5) color = vec3(ao);
    else if (u_DebugMode == 6 && (isnan(color.r) || isnan(color.g) || isnan(color.b))) color = vec3(1,0,0);
    else if (u_DebugMode == 7) color = vec3(1.0 - ShadowCalculation(worldPos, N, normalize(-dirDirection.xyz)));
    else if (u_DebugMode == 8) color = ambient;
    else if (u_DebugMode == 9) color = Lo;

    if (color.r != color.r || color.g != color.g || color.b != color.b)
        color = vec3(0.0);
    fragColor = vec4(color, alpha);
}
