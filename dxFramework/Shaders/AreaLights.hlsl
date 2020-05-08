#ifndef _PS_AREALIGHT_HLSL_
#define _PS_AREALIGHT_HLSL_

#include "LightSettings.hlsl"
#include "Constants.hlsl"
#include <PS_BRDF_Helper.hlsl>

float3 DiffuseSphereLight_ViewFactor(float3 positionWS, float3 diffuseColor, float3 N, Light areaLight)
{
    float3 delta = areaLight.positionWS - positionWS;
    float squareDist = dot(delta, delta);
    float dist = sqrt(squareDist);
    float3 L = delta / dist; //Normalized light direction
    float cosBeta = dot(N, L); //N will always be normalized already
    
    float sinBeta = sqrt(1.0 - cosBeta * cosBeta);
    float h = dist / areaLight.radius;
    float h2 = h * h;
    
    float3 viewFactor = 0;
    
    if (cosBeta > 1.0 / h)
    {
        viewFactor = saturate(cosBeta) / h2;
    }
    else
    {
        float x = sqrt(h2 - 1.0);
        float y = -x * cosBeta / sinBeta;
        float sinBetaSqrt1y2 = sinBeta * sqrt(1.0 - y * y);
        viewFactor = (cosBeta * acos(y) - x * sinBetaSqrt1y2) / (PI * h2) + atan(sinBetaSqrt1y2 / x) / PI;
        viewFactor = max(0, viewFactor);
    }
    return viewFactor * diffuseColor;
}

//https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf#page=15&zoom=auto,-274,344
float3 SpecularSphereLight_KarisMRP(float3 positionWS, float3 specularColor, float roughness, float3 N, float3 V, Light areaLight)
{
    const float NoV = dot(N, V);
    
    const float3 delta = areaLight.positionWS - positionWS; //Shading point to center of the light
    float3 L = delta;
    const float3 R = 2.0 * N * NoV - V; //Reflection vector - not normalized
    const float3 centerToRay = dot(L, R) * R - L;
    const float3 closestPoint = L + centerToRay * saturate(areaLight.radius / length(centerToRay));
    
    const float3 L2 = normalize(closestPoint);
    const float3 H = normalize(L2 + V);
    const float NoH = dot(N, H);
    const float NoL = dot(N, L2);
    const float VoH = dot(V, H);
    
    const float D = Specular_D_GGX(roughness, NoH);
    const float G = max(0, Specular_G_GGX(roughness, NoV) * Specular_G_GGX(roughness, NoL));
    const float F = Specular_F_Schlick(specularColor, VoH);
    
    const float area = (areaLight.radius * areaLight.radius * 4 * PI);
    
    float distanceSquared = dot(delta, delta);
    float intensity = 1.0 / distanceSquared;
    
    return intensity * (D * F * G) * NoL;
}

#endif // _PS_AREALIGHT_HLSL_