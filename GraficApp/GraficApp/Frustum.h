#pragma once

#include "framework.h"

class Frustum {
public:
    Frustum(float screenDepth);

    void ConstructFrustum(XMMATRIX viewMatrix, XMMATRIX projectionMatrix);
    bool CheckRectangle(float maxWidth, float maxHeight, float maxDepth, float minWidth, float minHeight, float minDepth);

    ~Frustum() = default;
private:
    float screenDepth_;
    float planes_[6][4] = { {0}, {0}, {0}, {0}, {0}, {0} };
};
