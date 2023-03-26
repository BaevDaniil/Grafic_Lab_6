#include "SceneMatrixBuffer.hlsli"

cbuffer WorldMatrixBuffer : register (b0) {
    float4x4 worldMatrix;
    float4 color : COLOR;
};

struct VS_INPUT {
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float4 worldPos : POSITION;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;

    output.worldPos = mul(worldMatrix, input.position);
    output.position = mul(viewProjectionMatrix, output.worldPos);
    output.color = color;

    return output;
}