cbuffer UberBuffer : register(b7)
{
    float4 g_unlitColor;
    float4 g_directionalLightColor;
    float3 g_directionalLightDirection;
    float g_uberPad1;
    
	float3 g_viewerPosition;
	float g_uberPad2;
    
    float3 g_directionalLightPosition;
    float g_uberPad3;
    
    matrix g_lightViewMatrix;
    matrix g_lightProjMatrix;
};