#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// Material
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;
uniform int hasDiffuseMap;
uniform int hasNormalMap;
uniform int hasRoughnessMap;
uniform int hasMetallicMap;
uniform int hasAoMap;

// Lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 viewPos;
uniform vec3 ambientColor;

// Shadow
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Flashlight (Spotlight)
uniform int useFlashlight;
uniform vec3 flashPos;
uniform vec3 flashDir;
uniform vec3 flashColor;
uniform float flashIntensity;
uniform float flashInnerCone;
uniform float flashOuterCone;

const float PI = 3.14159265359;

// Cook-Torrance BRDF
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float rough) {
    float r = (rough + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, rough);
    float ggx1 = GeometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(vec3 fragPos, vec3 N, vec3 lightDir) {
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    // Bias based on normal and light direction
    float bias = max(0.005 * (1.0 - dot(N, normalize(-lightDir))), 0.0005);
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    if (projCoords.z > 1.0) shadow = 0.0;
    return shadow;
}

void main() {
    // Material properties
    vec3 albedoVal = hasDiffuseMap == 1 ? pow(texture(diffuseMap, TexCoords).rgb, vec3(2.2)) : albedo;
    float metallicVal = hasMetallicMap == 1 ? texture(metallicMap, TexCoords).r : metallic;
    float roughnessVal = hasRoughnessMap == 1 ? texture(roughnessMap, TexCoords).r : roughness;
    float aoVal = hasAoMap == 1 ? texture(aoMap, TexCoords).r : ao;

    // Normal mapping
    vec3 N;
    if (hasNormalMap == 1) {
        vec3 normalMapVal = texture(normalMap, TexCoords).rgb;
        normalMapVal = normalMapVal * 2.0 - 1.0;
        N = normalize(TBN * normalMapVal);
    } else {
        N = normalize(Normal);
    }

    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    // Reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedoVal, metallicVal);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughnessVal);
    float G = GeometrySmith(N, V, L, roughnessVal);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallicVal;

    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = lightColor * lightIntensity * NdotL;

    // Shadow
    float shadow = ShadowCalculation(FragPos, N, lightDir);

    vec3 Lo = (kD * albedoVal / PI + specular) * radiance * (1.0 - shadow);

    // Ambient
    vec3 ambient = ambientColor * albedoVal * aoVal;

    vec3 color = ambient + Lo;

    // Flashlight contribution
    if (useFlashlight == 1) {
        vec3 flashToFrag = normalize(FragPos - flashPos);
        float theta = dot(flashToFrag, normalize(-flashDir));
        float epsilon = flashInnerCone - flashOuterCone;
        float spotIntensity = clamp((theta - flashOuterCone) / epsilon, 0.0, 1.0);

        // Distance attenuation
        float dist = length(FragPos - flashPos);
        float attenuation = flashIntensity / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        vec3 fL = normalize(-flashToFrag);
        vec3 fH = normalize(V + fL);

        float fNDF = DistributionGGX(N, fH, roughnessVal);
        float fG = GeometrySmith(N, V, fL, roughnessVal);
        vec3 fF = fresnelSchlick(max(dot(fH, V), 0.0), F0);

        vec3 fNum = fNDF * fG * fF;
        float fDenom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, fL), 0.0) + 0.0001;
        vec3 fSpec = fNum / fDenom;

        vec3 fkS = fF;
        vec3 fkD = (vec3(1.0) - fkS) * (1.0 - metallicVal);

        float fNdotL = max(dot(N, fL), 0.0);
        vec3 fRadiance = flashColor * attenuation * fNdotL * spotIntensity;
        vec3 fLo = (fkD * albedoVal / PI + fSpec) * fRadiance;

        color += fLo;
    }

    // HDR tone mapping (Reinhard)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
