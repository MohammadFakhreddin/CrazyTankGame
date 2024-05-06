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
		Layer::EmptyLayer,
		[this](auto layer)->void {OnHit(layer);}
	);
}

//=============================================================================

void Bullet::Update(float deltaTimeSec)
{
    auto currPos = _transform.GetLocalPosition();
    auto moveDir = _transform.Forward();// Something is wrong with the forward direction
    auto moveMag = deltaTimeSec * _params->moveSpeed;
    auto vector = moveMag * moveDir;
    auto newPos = currPos + vector;
    
    bool hitWall = true;
    bool destroyBullet = false;
    do
    {
		Physics2D::HitInfo hitInfo {};
		auto const isHit = Physics2D::Instance->Raycast(
			Layer::WallLayer | Layer::TankLayer | Layer::ShellLayer,
			_physicsId,
			Physics2D::Ray {currPos.xz(), moveDir.xz()},
			moveMag,
			hitInfo
		);

		hitWall = false;
		if (isHit == true)
		{
			if (hitInfo.layer == Layer::WallLayer)
			{
				// TODO: 1e-5 must be base on the bullet radius
				auto time = glm::max(hitInfo.hitTime - 1e-5f, 0.0f);

				currPos.x = hitInfo.hitPoint.x;
				currPos.z = hitInfo.hitPoint.y;
				currPos -= 1e-5f * moveDir;

				moveMag = (1.0f - time) * moveMag;
				auto const wallNormal = glm::vec3 {hitInfo.hitNormal.x, 0.0f, hitInfo.hitNormal.y};
				moveDir = glm::normalize(glm::reflect(moveDir, wallNormal));
				newPos = currPos + (moveDir * moveMag);
				hitWall = true;
			}
			else if (hitInfo.layer == Layer::TankLayer)
			{

			}
			else if (hitInfo.layer == Layer::ShellLayer)
			{

			}
			else
			{
				MFA_ASSERT(false);
			}
		}
    }
	while (hitWall == true);

    _transform.SetLocalQuaternion(glm::quatLookAt(moveDir,Math::UpVec3));
    Physics2D::Instance->MoveSphere(
        _physicsId, 
        newPos.xz(), 
        _params->radius, 
        false
    );

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
