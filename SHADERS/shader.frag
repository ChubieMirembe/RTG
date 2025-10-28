#version 460

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    vec3 eyePos; float _pad0;
    vec3 light1Pos; float _pad1;
    vec3 light1Col; float _pad2;
    vec3 light2Pos; float _pad3;
    vec3 light2Col; float _pad4;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 Ka;    // ambient
    vec4 Kd;    // diffuse
    vec4 Ks;    // specular; Ks.w = shininess
} pc;

vec3 phongLight(vec3 N, vec3 V, vec3 P, vec3 Lpos, vec3 Lcol,
                vec3 Ka, vec3 Kd, vec3 Ks, float shininess)
{
    vec3 L = normalize(Lpos - P);
    vec3 R = reflect(-L, N);

    float NdotL = max(dot(N, L), 0.0);
    float RdotV = max(dot(R, V), 0.0);

    vec3 ambient  = Ka * Lcol;
    vec3 diffuse  = Kd * Lcol * NdotL;
    vec3 specular = Ks * Lcol * pow(RdotV, shininess);

    return ambient + diffuse + specular;
}

void main() {
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(ubo.eyePos - vWorldPos);

    vec3 Ka = pc.Ka.rgb;
    vec3 Kd = pc.Kd.rgb;
    vec3 Ks = pc.Ks.rgb;
    float shininess = max(pc.Ks.w, 1.0);

    vec3 c1 = phongLight(N, V, vWorldPos, ubo.light1Pos, ubo.light1Col, Ka, Kd, Ks, shininess);
    vec3 c2 = phongLight(N, V, vWorldPos, ubo.light2Pos, ubo.light2Col, Ka, Kd, Ks, shininess);

    vec3 color = c1 + c2;
    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
