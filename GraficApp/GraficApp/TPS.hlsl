#include "LightCalc.hlsli"

struct PS_INPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float4 worldPos : POSITION;
};

float4 main(PS_INPUT input) : SV_TARGET{
#ifdef USE_LIGHTS
    return float4(CalculateColor(input.color.xyz, float3(1, 0, 0), input.worldPos.xyz, 0.0, true), 0.5f);
#else
    return float4(input.color.xyz, 0.5f);
#endif // !USE_LIGHTS
}