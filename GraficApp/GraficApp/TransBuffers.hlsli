#pragma once

#include "Constant.h"
#include "LightCalc.hlsli"

cbuffer WorldMatrixBuffer : register (b0) {
    float4x4 worldMatrix;
    float4 color;
};

cbuffer SceneMatrixBuffer : register (b1) {
    float4x4 viewProjectionMatrix;
};