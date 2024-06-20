#include "FollowCamera.hpp"

#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"

#include <imgui.h>

using namespace MFA;

//---------------------------------------------------------------------

FollowCamera::FollowCamera(
    MFA::Transform & target, 
    std::shared_ptr<MFA::HostVisibleBufferTracker> cameraBufferTracker
)
    : FollowCamera(target, cameraBufferTracker, Params{})
{}

//---------------------------------------------------------------------

FollowCamera::FollowCamera(
    Transform & target,
    std::shared_ptr<HostVisibleBufferTracker> cameraBufferTracker,
    Params params
)
    : _target(target)
    , _cameraBufferTracker(std::move(cameraBufferTracker))
    , _params(std::move(params))
{
    _observerCamera = std::make_unique<ObserverCamera>();
    _observerCamera->SetfovDeg(40.0f);
    _observerCamera->SetLocalRotation(_params.rotation);
    _observerCamera->SetfarPlane(params.farPlane);
	_observerCamera->SetnearPlane(params.nearPlane);
    UpdatePosition(0.0f, true);
}

//---------------------------------------------------------------------

FollowCamera::~FollowCamera() = default;

//---------------------------------------------------------------------

void FollowCamera::Update(float deltaTime)
{
    UpdatePosition(deltaTime, false);
    if (_observerCamera->IsDirty() == true)
    {
        _cameraBufferTracker->SetData(MFA::Alias{_observerCamera->ViewProjection()});
    }
}

//---------------------------------------------------------------------

void FollowCamera::DebugUI()
{
    _observerCamera->Debug_UI();
    ImGui::InputFloat("Distance to target", &_params.distanceToTarget);
    ImGui::InputFloat("Move speed", &_params.moveSpeed);
}

//---------------------------------------------------------------------

void FollowCamera::UpdatePosition(float const deltaTimeSec, bool const resetPosition)
{
    // TODO: It needs to move like a spring
    auto const targetPosition = _target.GlobalPosition() + _observerCamera->Forward() * _params.distanceToTarget;
    auto position = _observerCamera->LocalPosition();
    if (resetPosition == true)
    {
        position = targetPosition;   
    }
    else
    {
        position = Math::MoveTowards(position, targetPosition, deltaTimeSec * _params.moveSpeed);
    }
	_observerCamera->SetLocalPosition(position);
}

//---------------------------------------------------------------------
