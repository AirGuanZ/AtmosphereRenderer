#pragma once

#include "./common.h"

class Camera
{
public:

    struct UpdateParams
    {
        bool front = false;
        bool left  = false;
        bool right = false;
        bool back  = false;

        bool up   = false;
        bool down = false;

        float cursorRelX = 0;
        float cursorRelY = 0;
    };

    struct FrustumDirections
    {
        Float3 frustumA; // top-left dir
        Float3 frustumB; // top-right dir
        Float3 frustumC; // bottom-left dir
        Float3 frustumD; // bottom-right dir
    };

    Camera();

    void setPosition(const Float3 &position);

    void setDirection(float horiRad, float vertRad);

    void setSpeed(float speed);

    void setViewRotationSpeed(float speed);

    void update(const UpdateParams &params);

    void setPerspective(float fovDeg, float nearZ, float farZ);

    void setWOverH(float wOverH);

    void recalculateMatrics();

    float getNearZ() const;

    float getFarZ() const;

    const Float3 &getPosition() const;

    Float2 getDirection() const;

    const Mat4 &getView() const;

    const Mat4 &getProj() const;

    const Mat4 &getViewProj() const;

    FrustumDirections getFrustumDirections() const;

private:

    Float3 computeDirection() const;

    Float3 pos_;
    float vertRad_;
    float horiRad_;

    float fovDeg_;
    float nearZ_;
    float farZ_;

    float wOverH_;

    float speed_;
    float cursorSpeed_;

    Mat4 view_;
    Mat4 proj_;
    Mat4 viewProj_;
};
