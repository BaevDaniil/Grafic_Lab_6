#include "camera.h"

Camera::Camera() {
    focus_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
    r_ = 5.0f;
    theta_ = 0;
    phi_ = 0;
    position_ = XMFLOAT3(focus_.x - cosf(theta_) * cosf(phi_) * r_,
        focus_.y - sinf(theta_) * r_,
        focus_.z - cosf(theta_) * sinf(phi_) * r_);

    UpdateViewMatrix();
}

void Camera::Rotate(float dphi, float dtheta) {
    phi_ -= dphi;
    theta_ -= dtheta;
    theta_ = min(max(theta_, -XM_PIDIV2), XM_PIDIV2);
    focus_ = XMFLOAT3(cosf(theta_) * cosf(phi_) * r_ + position_.x,
        sinf(theta_) * r_ + position_.y,
        cosf(theta_) * sinf(phi_) * r_ + position_.z);

    UpdateViewMatrix();
}

void Camera::Zoom(float dr) {
    r_ += dr;
    if (r_ < 2.0f) {
        r_ = 2.0f;
    }
    if (r_ > 5.0f) {
        r_ = 5.0f;
    }
    position_ = XMFLOAT3(focus_.x - cosf(theta_) * cosf(phi_) * r_,
        focus_.y - sinf(theta_) * r_,
        focus_.z - cosf(theta_) * sinf(phi_) * r_);

    UpdateViewMatrix();
}

void Camera::Move(float di, float dj, float dz) {
    float upTheta = theta_ + XM_PIDIV2;
    XMFLOAT3 up = XMFLOAT3(cosf(upTheta) * cosf(phi_) * dz, sinf(upTheta) * dz, cosf(upTheta) * sinf(phi_) * dz);
    XMFLOAT3 forward = XMFLOAT3(cosf(theta_) * cosf(phi_) * dj, sinf(theta_) * dj, cosf(theta_) * sinf(phi_) * dj);
    focus_ = XMFLOAT3(focus_.x + forward.x + up.x, focus_.y + forward.y + up.y, focus_.z + forward.z + up.z);
    position_ = XMFLOAT3(position_.x + forward.x + up.x, position_.y + forward.y + up.y, position_.z + forward.z + up.z);

    float rightPhi = phi_ + XM_PIDIV2;
    XMFLOAT3 right = XMFLOAT3(cosf(rightPhi) * di, 0, sinf(rightPhi) * di);
    focus_ = XMFLOAT3(focus_.x + right.x, focus_.y + right.y, focus_.z + right.z);
    position_ = XMFLOAT3(position_.x + right.x, position_.y + right.y, position_.z + right.z);

    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix() {
    float upTheta = theta_ + XM_PIDIV2;
    XMFLOAT3 up = XMFLOAT3(cosf(upTheta) * cosf(phi_), sinf(upTheta), cosf(upTheta) * sinf(phi_));

    viewMatrix_ = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(position_.x, position_.y, position_.z, 0.0f),
        DirectX::XMVectorSet(focus_.x, focus_.y, focus_.z, 0.0f),
        DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
    );
}