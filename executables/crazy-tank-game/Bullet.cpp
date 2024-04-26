#include "Bullet.hpp"

#include "BedrockAssert.hpp"
#include "Layers.hpp"
#include "utils/MeshInstance.hpp"

using namespace MFA;

//=============================================================================

Bullet::Bullet(std::shared_ptr<Params> params)
    : _params(std::move(params))
{
    _physicsId = Physics2D::Instance->Register(
		Physics2D::Type::Sphere,
		Layer::ShellLayer,
		Layer::WallLayer | Layer::TankLayer | Layer::ShellLayer,
		[this](auto layer)->void {OnHit(layer);}
	);
}

//=============================================================================

void Bullet::Update(float deltaTimeSec)
{
    auto vector = deltaTimeSec * _params->moveSpeed * _transform.Forward();
    _transform.SetLocalPosition(_transform.GetLocalPosition() + vector);
}

//=============================================================================

MFA::Transform & Bullet::Transform()
{
    return _transform;
}

//=============================================================================

void Bullet::OnHit(Physics2D::Layer layer)
{

}

//=============================================================================
