#include "Tank.hpp"

#include "BedrockAssert.hpp"
#include "Layers.hpp"
#include "Time.hpp"
#include "utils/MeshInstance.hpp"

using namespace MFA;

//==================================================================

Tank::Tank(
	MeshRenderer const & meshRenderer, 
	std::shared_ptr<Params> params
)
	: _meshInstance(std::make_unique<MFA::MeshInstance>(meshRenderer))
	, _params(std::move(params))
{

	_transform = &_meshInstance->GetTransform();

	_physicsId = Physics2D::Instance->Register(
		Physics2D::Type::AABB,
		Layer::Tank,
		Layer::Tank | Layer::Wall,
		[this](auto layer)->void {OnHit(layer);}
	);

	_shootTransform = &_meshInstance->FindNode("Shoot")->transform;

	{
		auto const position2d = _transform->GetLocalPosition().xz();
		auto const canMove = Physics2D::Instance->MoveAABB(
			_physicsId,
			position2d - _params->halfColliderExtent,
			position2d + _params->halfColliderExtent,
			false
		);
		MFA_ASSERT(canMove == true);
	}

	_isAlive == true;
}

//==================================================================

void Tank::OnHit(Physics2D::Layer layer)
{
	MFA_LOG_INFO("Tank is hit");
	if (layer == Layer::Bullet)
	{
		_isAlive = false;
		Physics2D::Instance->UnRegister(_physicsId);
	}
}

//==================================================================

bool Tank::Move(glm::vec2 const & direction, float const deltaTimeSec)
{
	// I probably need to move this part to a more common class like movableObject or rigidbody.
	bool success = false;

	static constexpr float epsilon = 1e-2f;

	// TODO: Move this code to physics and rename the teleport		
	auto const moveVector = glm::vec3{direction.x, 0.0, direction.y} * deltaTimeSec * _params->moveSpeed;
		
	{// Position
		auto const startPos3d = _transform->GetLocalPosition();
		glm::vec2 remMoveVector = moveVector.xz();
		do
		{
			auto const startPos2d = startPos3d.xz();
			auto const startV0 = startPos2d - _params->halfColliderExtent;
			auto const startV2 = startPos2d + _params->halfColliderExtent;
			auto const startV1 = glm::vec2{ startV2.x, startV0.y };
			auto const startV3 = glm::vec2{ startV0.x, startV2.y };

			std::vector<glm::vec2> startPoses{startV0, startV1, startV2, startV3};

			auto const moveMag = glm::length(remMoveVector);

			if (moveMag < epsilon)
			{
				break;
			}

			glm::vec2 const moveDir = remMoveVector / moveMag;

			bool hit = false;
			float timeOfHit = 1.0f;
			glm::vec2 hitNormal{};

			for (int i = 0; i < startPoses.size(); ++i)
			{
				Physics2D::HitInfo hitInfo{};
				auto const localHit = Physics2D::Instance->Raycast(
					Layer::Wall | Layer::Tank,
					{_physicsId},
					Physics2D::Ray {startPoses[i], moveDir},
					moveMag,
					hitInfo
				);
				if (localHit == true)
				{
					if (hit == false || hitInfo.hitTime < timeOfHit)
					{
						timeOfHit = hitInfo.hitTime;
						hitNormal = hitInfo.hitNormal;
					}
					hit = true;	
				}
			}
			
			glm::vec2 appliedMoveVector = timeOfHit * remMoveVector;
			if (hit == true)
			{
				if (glm::length(appliedMoveVector) > epsilon)
				{
					appliedMoveVector -= epsilon * appliedMoveVector;
				}
				else
				{
					appliedMoveVector = {};
				}
			}

			remMoveVector -= appliedMoveVector;
			if (hit == true)
			{
				auto const dot = glm::dot(hitNormal, remMoveVector);
				remMoveVector = remMoveVector - (dot * hitNormal);
			}

			auto const targetPos2d = startPos2d + appliedMoveVector;
			auto const targetV0 = targetPos2d - _params->halfColliderExtent;
			auto const targetV2 = targetPos2d + _params->halfColliderExtent;			

			success = Physics2D::Instance->MoveAABB(
				_physicsId,
				targetV0,
				targetV2,
				true
			);
			if(success == true)
			{
				_transform->SetLocalPosition(glm::vec3 {targetPos2d.x, startPos3d.y, targetPos2d.y});
			}
			else
			{
				break;
			}
		} while (true);
	}

	auto const finalPos3d = _transform->GetLocalPosition();

	{// Rotation
		auto const magnitude = glm::length(moveVector);
		if (magnitude > glm::epsilon<float>())
		{
			auto const direction = moveVector / magnitude;
			
			auto const targetQuaternion = glm::quatLookAt(direction, Math::UpVec3);

			auto const maxDegreesDelta = deltaTimeSec * _params->rotationSpeed;
			_transform->SetLocalQuaternion(
				Math::RotateTowards(
					_transform->GetLocalRotation().GetQuaternion(),
					targetQuaternion,
					maxDegreesDelta
				)
			);
		}
	}

	return success;
}

//==================================================================

std::unique_ptr<Bullet> Tank::Shoot(std::shared_ptr<Bullet::Params> params)
{
	auto const now = Time::NowSec();

	if (_shootCooldownEndTime > now)
	{
		return nullptr;
	}

	auto bullet = std::make_unique<Bullet>(std::move(params), _physicsId);
	auto & transform = bullet->Transform();
	transform.SetLocalRotation(_shootTransform->GlobalRotation());
	transform.SetLocalScale(glm::one<glm::vec3>() * 0.25f);
	transform.SetLocalPosition(_shootTransform->GlobalPosition() - _shootTransform->Forward() * 1.0f);
	_shootCooldownEndTime = Time::NowSec() + _params->shootCooldown;

	return bullet;
}

//==================================================================

MeshInstance* Tank::MeshInstance() const
{
	return _meshInstance.get();
}

//==================================================================

Transform & Tank::Transform()
{
	return *_transform;
}

//==================================================================

bool Tank::IsAlive() const
{
	return _isAlive;
}

//==================================================================
