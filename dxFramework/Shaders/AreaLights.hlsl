#ifndef _PS_AREALIGHT_HLSL_
#define _PS_AREALIGHT_HLSL_

#include "LightSettings.hlsl"
#include "Constants.hlsl"

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
    
    if (cosBeta * cosBeta > 1.0 / h2)
    {
        viewFactor = saturate(cosBeta) / h2;
    }
    else
    {
        float x = sqrt(h2 - 1.0);
        float y = -x * cosBeta / sinBeta;
        float sinBetaSqrt1y2 = sinBeta * sqrt(1.0 - y * y);
        viewFactor = (cosBeta * acos(y) - x * sinBetaSqrt1y2) / (PI * h2) + atan(sinBetaSqrt1y2 / x) / PI;
    }
    return viewFactor * diffuseColor;
}

#endif // _PS_AREALIGHT_HLSL_