#pragma once

#include "camera/ObserverCamera.hpp"
#include "BufferTracker.hpp"
#include "Transform.hpp"

#include <memory>

class FollowCamera
{
    
public: 

    struct Params
    {
        float distanceToTarget = 25.0f;
        float moveSpeed = 4.0f;
        MFA::Rotation rotation {glm::vec3{-145.0, 0.0, 0.0}};
        float farPlane = 100.0f;
        float nearPlane = 0.01f; 
    };

    explicit FollowCamera(
        MFA::Transform & target, 
        std::shared_ptr<MFA::HostVisibleBufferTracker> cameraBufferTracker
    );

    explicit FollowCamera(
        MFA::Transform & target, 
        std::shared_ptr<MFA::HostVisibleBufferTracker> cameraBufferTracker,
        Params params
    );

    ~FollowCamera();

    void Update(float deltaTime);

    void DebugUI();

private:

    void UpdatePosition(float deltaTime, bool resetPosition = false);

    MFA::Transform & _target;
    std::unique_ptr<MFA::ObserverCamera> _observerCamera{};
    std::shared_ptr<MFA::HostVisibleBufferTracker> _cameraBufferTracker{};

    Params _params{};

};