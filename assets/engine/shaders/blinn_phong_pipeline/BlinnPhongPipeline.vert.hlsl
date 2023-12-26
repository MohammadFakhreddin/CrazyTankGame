struct VSIn {
    float3 position : POSITION0;
    float3 normal : NORMAL0;
};

struct VSOut {
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;
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

VSOut main(VSIn input) {
    VSOut output;

    float4x4 mvpMatrix = mul(vpMatrix.viewProjection, pushConsts.model);
    output.position = mul(mvpMatrix, float4(input.position, 1.0));

    output.worldPosition = mul(pushConsts.model, float4(input.position, 1.0));
    output.worldNormal = normalize((pushConsts.model, float4(input.normal, 0.0)));

    return output;
}