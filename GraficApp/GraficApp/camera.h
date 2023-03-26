#pragma once

#include "framework.h"

class Camera {
public:
    Camera();

    void Rotate(float dphi, float dtheta);
    void Zoom(float dr);
    void Move(float di, float dj, float dz); // di - right/left relative to the camera, di - foward/becward relative to the camers, dz - up/down relative to the camera

    XMMATRIX& GetViewMatrix() {
        return viewMatrix_;
    };

    XMFLOAT3& GetPosition() {
        return position_;
    };
private:
    XMMATRIX viewMatrix_;
    XMFLOAT3 focus_;
    XMFLOAT3 position_;
    float r_;
    float theta_;
    float phi_;

    void UpdateViewMatrix();
};