#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vT;
layout(location = 3) in vec3 vB;
layout(location = 4) in vec3 vN;
layout(location = 5) in vec3 vColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D uColorTex;
layout(set = 0, binding = 2) uniform sampler2D uNormalTex;
layout(set = 0, binding = 3) uniform sampler2D uHeightTex;

layout(push_constant) uniform Push {
    mat4 modelOverride;
    uint useOverride;
    uint unlit;
    uint _pad0;
    uint _pad1;
} pc;

// helper: parallax occlusion / ray marching
vec2 parallaxRayMarch(vec2 uv, vec3 viewDirTangent)
{
    // how “tall” the effect is
    float heightScale = 0.05;   // tweak: 0.02–0.1
    // number of layers to march
    int numLayers = 24;         // more = better, slower

    // viewDirTangent.z should be positive for correct stepping;
    // use abs to avoid division by 0-ish
    float layerDepth = 1.0 / float(numLayers);
    float currentLayerDepth = 0.0;

    // amount we move in UV per layer
    // note: viewDirTangent.xy / viewDirTangent.z = perspective correction
    vec2 P = viewDirTangent.xy * heightScale / max(viewDirTangent.z, 0.001);
    vec2 deltaUV = -P / float(numLayers);

    // current sample
    vec2 currentUV = uv;
    float currentHeight = texture(uHeightTex, currentUV).r;

    // move until the sampled height is below the current layer
    while (currentLayerDepth < currentHeight && numLayers > 0)
    {
        currentUV += deltaUV;
        currentHeight = texture(uHeightTex, currentUV).r;
        currentLayerDepth += layerDepth;
        numLayers--;
    }

    // optional: small linear refinement between last two steps
    // we overshot: currentLayerDepth > currentHeight
    // previous step:
    vec2 prevUV = currentUV - deltaUV;
    float afterDepth = currentHeight - currentLayerDepth;
    float beforeHeight = texture(uHeightTex, prevUV).r;
    float beforeDepth = beforeHeight - (currentLayerDepth - layerDepth);

    float weight = beforeDepth / (beforeDepth - afterDepth);
    vec2 finalUV = mix(currentUV, prevUV, clamp(weight, 0.0, 1.0));

    return finalUV;
}

void main()
{
    // build TBN
    vec3 N = normalize(vN);
    vec3 T = normalize(vT);
    vec3 B = normalize(vB);
    mat3 TBN = mat3(T, B, N);

    // world-space view and light
    vec3 V_world = normalize(ubo.eyePos - vWorldPos);
    vec3 L_world = normalize(ubo.lightPos - vWorldPos);

    // convert view to tangent for parallax
    vec3 V_tangent = normalize(transpose(TBN) * V_world);

    // 1) PARALLAX: shift UV **before** sampling color/normal
    vec2 displacedUV = parallaxRayMarch(vUV, V_tangent);

    // safety: if we marched outside the texture, clamp
    displacedUV = clamp(displacedUV, 0.0, 1.0);

    // 2) sample color from displaced UV
    vec3 albedo = texture(uColorTex, displacedUV).rgb;

    // 3) sample normal from displaced UV
    vec3 nrmSample = texture(uNormalTex, displacedUV).rgb;
    vec3 nrmTangent = nrmSample * 2.0 - 1.0;

    // optional extra bumpiness
    float normalStrength = 2.0;
    nrmTangent.xy *= normalStrength;
    nrmTangent = normalize(nrmTangent);

    // tangent ? world
    vec3 nrmWorld = normalize(TBN * nrmTangent);

    // lighting
    vec3 H = normalize(L_world + V_world);
    float NdotL = max(dot(nrmWorld, L_world), 0.0);

    float spec = 0.0;
    if (NdotL > 0.0) {
        float shininess = 32.0;
        spec = pow(max(dot(nrmWorld, H), 0.0), shininess);
    }

    // small base light so faces not hit by the light are still visible
    vec3 ambient = albedo * 0.2;  // try 0.2–0.3

    vec3 diffuse  = albedo * NdotL;
    vec3 specular = vec3(0.15) * spec;

    // now include ambient
    vec3 color    = ambient + diffuse + specular;

    if (pc.unlit == 1u) {
        color = albedo;
    }

    outColor = vec4(color, 1.0);

}
