#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl>

SamplerState baseSampler : register(s0);
Texture2D shadowMapTexture : register(t0);
Texture2D texture1 : register(t1);
Texture2D texture2 : register(t2);
Texture2D texture3 : register(t3);
Texture2D texture4 : register(t4);

float GetCoeffPCF(float2 projectTexCoord)
{
    float width;
    float height;
    shadowMapTexture.GetDimensions(width, height);
                
    const float xOffset = 1.0 / width;
    const float yOffset = 1.0 / height;
    float factor = 0;
                
    //return shadowMapTexture.Sample(baseSampler, projectTexCoord.xy).r;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float2 offsets = float2((float) x * xOffset, (float) y * yOffset);
            float2 uvCoords = float2(projectTexCoord.xy + offsets);
            factor += shadowMapTexture.Sample(baseSampler, uvCoords.xy).r;
        }
    }
    
    return factor / 9.0;
}

float ShadowCalculation(float3 normal, float4 lightPos, float3 pointToLight)
{
    static float2 poissonDisk[4] =
    {
        float2(-0.94201624, -0.39906216),
        float2(0.94558609, -0.76890725),
        float2(-0.094184101, -0.92938870),
        float2(0.34495938, 0.29387760)
    };
    
    //float visibility = 1.0f;

    const float bias = 0.0001f;
    float3 projectTexCoord;
    projectTexCoord.x = lightPos.x / lightPos.w / 2.0f + 0.5f;
    projectTexCoord.y = -lightPos.y / lightPos.w / 2.0f + 0.5f;
    projectTexCoord.z = lightPos.z / lightPos.w / 2.0f + 0.5f;
    
    if (saturate(projectTexCoord.x) == projectTexCoord.x && saturate(projectTexCoord.y) == projectTexCoord.y)
    {
        //for (int i = 0; i < 4; i++)
        {
            float depthValue = GetCoeffPCF(projectTexCoord.xy/* + poissonDisk[i] / 700.0*/);
            float lightDepthValue = lightPos.z / lightPos.w;
            lightDepthValue -= bias;

            if (lightDepthValue > depthValue)
            {
                float percentage = (1.0 - lightDepthValue) / (1.0 - depthValue);
                //visibility -= saturate(percentage - 0.5);
                return saturate(percentage - 0.5);
            }
        }
    }
    
    //return saturate(visibility);
    return 1.0f;
}

float4 main(PixelInputType input) : SV_TARGET
{
    // INSTANCE TESTING //

    //switch (input.instanceIndex)
    //{
    //    case 0:
    //        return texture1.Sample(baseSampler, input.uv);
    //    case 1:
    //        return texture2.Sample(baseSampler, input.uv);
    //    case 2:
    //        return texture3.Sample(baseSampler, input.uv);
    //    //case 3:
    //    //    return texture4.Sample(baseSampler, input.uv);
    //}
    //return float4(input.instanceColor, 1);
    
    //////////////////////
	const float3 L = normalize(input.pointToLight.xyz);
	const float3 N = normalize(input.normal);
	const float intensity = input.pointToLight.w;	
	const float NdotL = dot(L, N);
	const float3 diffuse = NdotL;
	const float3 H = normalize(L + input.viewDir.xyz);
	const float3 specular = pow(max(dot(N, H), 0.0f), 32.0f) * 0.1f;
	
    const float shadow = ShadowCalculation(input.normal, input.lightPos, input.pointToLight.xyz);
    
    return float4(saturate((diffuse + specular) * intensity * g_directionalLightColor.xyz * shadow), 1.0f);
}