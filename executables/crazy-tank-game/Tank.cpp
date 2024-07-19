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
	bool success = false;

	auto const & oldPosition = _transform->GetLocalPosition();
	auto const moveVector = glm::vec3{direction.x, 0.0, direction.y} * deltaTimeSec * _params->moveSpeed;

	{// Position
		auto const newPosition = oldPosition + moveVector;
		auto const position2d = newPosition.xz();
		success = Physics2D::Instance->MoveAABB(
			_physicsId,
			position2d - _params->halfColliderExtent,
			position2d + _params->halfColliderExtent,
			true
		);

		if(success == true)
		{
			_transform->SetLocalPosition(newPosition);
		}
	}

	{// Rotation
		auto const targetQuaternion = glm::quatLookAt(glm::normalize(moveVector), Math::UpVec3);

		auto const maxDegreesDelta = deltaTimeSec * _params->rotationSpeed;
		_transform->SetLocalQuaternion(
			Math::RotateTowards(
				_transform->GetLocalRotation().GetQuaternion(),
				targetQuaternion,
				maxDegreesDelta
			)
		);
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
	bullet->Transform().SetLocalRotation(_shootTransform->GlobalRotation());
	bullet->Transform().SetLocalScale(glm::one<glm::vec3>() * 0.25f);
	bullet->Transform().SetLocalPosition(_shootTransform->GlobalPosition() - _shootTransform->Forward() * 1.0f);
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
