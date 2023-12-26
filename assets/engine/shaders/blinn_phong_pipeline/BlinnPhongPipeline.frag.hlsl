#include "../ColorUtils.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;
};

struct PSOut {
    float4 color : SV_Target0;
};

struct ViewProjectionBuffer {
    float4x4 viewProjection;
    float4 cameraPosition;
};

ConstantBuffer <ViewProjectionBuffer> vpMatrix: register(b0, space0);


struct PushConsts
{    
    float4x4 model;
    float4 materialColor;
    float4 lightPosition;
    float4 lightColor;
};

[[vk::push_constant]]
cbuffer {
    PushConsts pushConsts;
};

PSOut main(PSIn input) {
    PSOut output;

    float4 materialColor = pushConsts.materialColor.rgba;
    float3 lightColor = pushConsts.lightColor.rgb;
    
    float3 cameraPosition = -vpMatrix.cameraPosition.xyz;

    float3 fragmentPosition = input.worldPosition;
    float3 fragmentNormal = normalize(input.worldNormal);

    float3 lightPosition = pushConsts.lightPosition;

    float3 lightDirection = normalize(lightPosition - fragmentPosition);
    float3 viewDirection = normalize(cameraPosition - fragmentPosition);
    float3 halfwayDirection = normalize(lightDirection + viewDirection);

	float3 ambientColor = lightColor.rgb * 0.05f;

    float diffuseFactor = max(dot(fragmentNormal, lightDirection), 0.0);
    float3 diffuseColor = diffuseFactor * lightColor * 0.25;

    float3 reflectDir = reflect(-lightDirection, fragmentNormal);
    float3 specularFactor = float3(0.0, 0.0, 0.0);
    
    // Blinn
    // specularFactor = pow(max(dot(fragmentNormal, halfwayDirection), 0.0), 16.0);
    // blinn-phong
    specularFactor = pow(max(dot(viewDirection, reflectDir), 0.0), 8.0);
    
    float3 specularColor = specularFactor * lightColor;

    float3 color = (ambientColor + diffuseColor + specularColor) * materialColor.rgb;

    // exposure tone mapping
    // color = ApplyExposureToneMapping(color);
    // Gamma correct
    color = ApplyGammaCorrection(color); 

    output.color = float4(color, materialColor.a);
    return output;
}