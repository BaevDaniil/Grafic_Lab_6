Texture2D sourceTexture : register(t0);

SamplerState Sampler : register(s0);

cbuffer PostEffectConstantBuffer : register(b0) {
    int4 params;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET{
    float3 color = sourceTexture.Sample(Sampler, input.tex).xyz;
    if (params.x > 0) {
        color.x = 1.0f - color.x;
        color.y = 1.0f - color.y;
        color.z = 1.0f - color.z;
    }
    return float4(color, 1.0);
}