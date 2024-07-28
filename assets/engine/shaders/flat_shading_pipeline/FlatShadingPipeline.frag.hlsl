#include "../ColorUtils.hlsl"

struct PSIn {
    float4 position : SV_POSITION;
    float2 baseColorUV : TEXCOORD0;
    float3 worldNormal : NORMAL;
};

struct PSOut {
    float4 color : SV_Target0;
};

struct Material
{
    float4 color;
    int hasBaseColorTexture;
    int placeholder0;
    int placeholder1;
    int placeholder2;
};

struct LightSource
{
    float3 dir;
    float placeholder0;
    float3 color;
    float placeholder1;
};

ConstantBuffer <LightSource> lightSource : register(b1, space0);

sampler textureSampler : register(s2, space0);

ConstantBuffer <Material> material : register(b0, space1);

Texture2D baseColorTexture : register(t1, space1);

PSOut main(PSIn input) {
    PSOut output;

    float3 color;
    float alpha;
    if (material.hasBaseColorTexture == 0)
    {
        color = material.color.rgb;
        alpha = material.color.a;
    }
    else
    {
        color = baseColorTexture.Sample(textureSampler, input.baseColorUV);
        alpha = 1.0;
    }
    
    float ambient = 0.25f;

    float dotProd = dot(normalize(-lightSource.dir), normalize(input.worldNormal));
    float3 dirLight = max(dotProd, 0.0f) * color;
    float3 color2 = dirLight + ambient * color;
    // color = ambient * color;

    // color2 = ApplyExposureToneMapping(color2);
    // Gamma correct
    color2 = ApplyGammaCorrection(color2); 

    output.color = float4(color2, alpha);
    return output;
}