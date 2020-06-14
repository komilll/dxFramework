#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl>

SamplerState baseSampler : register(s0);
Texture2D shadowMapTexture : register(t0);

float4 ShadowCalculation(float3 normal, float4 lightPos, float3 pointToLight)
{
    const float bias = 0.001f;
    float2 projectTexCoord;
    projectTexCoord.x = lightPos.x / lightPos.w / 2.0f + 0.5f;
    projectTexCoord.y = -lightPos.y / lightPos.w / 2.0f + 0.5f;
    
    if (saturate(projectTexCoord.x) == projectTexCoord.x && saturate(projectTexCoord.y) == projectTexCoord.y)
    {
        //return shadowMapTexture.Sample(baseSampler, projectTexCoord.xy).r;
        float depthValue = shadowMapTexture.Sample(baseSampler, projectTexCoord.xy).r;
        float lightDepthValue = lightPos.z / lightPos.w;
        lightDepthValue -= bias;

        return depthValue;
        if (lightDepthValue < depthValue)
        {
            //float lightIntensity = saturate(dot(normal, pointToLight));
            //if (lightIntensity > 0.0f)
            return 0.0;
        }
    }
    
    return 1.0;
}

float4 main(PixelInputType input) : SV_TARGET
{
    return float4(input.instanceColor, 1);
	const float3 L = normalize(input.pointToLight.xyz);
	const float3 N = normalize(input.normal);
	const float intensity = input.pointToLight.w;	
	const float NdotL = dot(L, N);
	const float3 diffuse = NdotL;
	const float3 H = normalize(L + input.viewDir.xyz);
	const float3 specular = pow(max(dot(N, H), 0.0f), 32.0f) * 0.1f;
	
    //return shadowMapTexture.Sample(baseSampler, input.uv).r;
    //return ShadowCalculation(input.normal, input.lightPos, input.pointToLight.xyz);
    
	return float4(saturate((diffuse + specular) * intensity * g_directionalLightColor.xyz), 1.0f);	
}