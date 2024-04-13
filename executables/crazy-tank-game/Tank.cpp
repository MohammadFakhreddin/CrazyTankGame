#include "Tank.hpp"

#include "BedrockAssert.hpp"
#include "Layers.hpp"
#include "utils/MeshInstance.hpp"

using namespace MFA;

//==================================================================

Tank::Tank(
	MeshRenderer const & meshRenderer, 
	Transform const & transform,
	std::shared_ptr<Params> params
)
	: _meshInstance(std::make_unique<MFA::MeshInstance>(meshRenderer))
	, _params(std::move(params))
{

	_meshInstance->SetTransform(transform);
	_transform = &_meshInstance->GetTransform();

	_physicsId = Physics2D::Instance->Register(
		Physics2D::Type::AABB,
		Layer::TankLayer,
		Layer::TankLayer | Layer::WallLayer,
		[this](auto layer)->void {OnHit(layer);}
	);

	{
		auto const position2d = _transform->Getposition().xz();
	auto const canMove = Physics2D::Instance->MoveAABB(
			_physicsId,
			position2d - _params->halfColliderExtent,
			position2d + _params->halfColliderExtent,
			false
		);
		MFA_ASSERT(canMove == true);
	}
}

//==================================================================

void Tank::OnHit(Physics2D::Layer layer)
{
	MFA_LOG_INFO("Tank is hit");
}

//==================================================================

bool Tank::Move(glm::vec2 const & direction, float const deltaTimeSec)
{
	bool success = false;

	auto const & oldPosition = _transform->Getposition();
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
			_transform->Setposition(newPosition);
		}
	}
	{// Rotation
		auto const targetQuaternion = glm::quatLookAt(glm::normalize(moveVector),Math::UpVec3);
		auto const maxDegreesDelta = deltaTimeSec * _params->rotationSpeed;
		_transform->SetQuaternion(
			Math::RotateTowards(
				_transform->Getrotation().GetQuaternion(),
				targetQuaternion,maxDegreesDelta
			)
		);
	}

	return success;
}

//==================================================================

MeshInstance* Tank::MeshInstance()
{
	return _meshInstance.get();
}

//==================================================================
