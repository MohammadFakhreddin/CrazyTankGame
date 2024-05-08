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
		Layer::Bullet,
		Layer::Empty,
		[this](auto layer)->void {OnHit(layer);}
	);
}

//=============================================================================

Bullet::~Bullet()
{
	Die();
}

//=============================================================================

void Bullet::Update(float deltaTimeSec)
{
	if (_isAlive == false)
	{
		return;
	}

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
			Layer::Wall | Layer::Tank | Layer::Bullet,
			_physicsId,
			Physics2D::Ray {currPos.xz(), moveDir.xz()},
			moveMag,
			hitInfo
		);

		hitWall = false;
		if (isHit == true)
		{
			if (hitInfo.layer == Layer::Wall)
			{
				auto const epsilon = 1e-5f;
				// TODO: 1e-5 must be base on the bullet radius
				auto time = glm::max(hitInfo.hitTime - epsilon, 0.0f);

				currPos.x = hitInfo.hitPoint.x;
				currPos.z = hitInfo.hitPoint.y;
				currPos -= epsilon * moveDir;

				moveMag = (1.0f - time) * moveMag;
				auto const wallNormal = glm::vec3 {hitInfo.hitNormal.x, 0.0f, hitInfo.hitNormal.y};
				moveDir = glm::normalize(glm::reflect(moveDir, wallNormal));
				newPos = currPos + (moveDir * moveMag);
				hitWall = true;
			}
			else if (hitInfo.layer == Layer::Tank)
			{

			}
			else if (hitInfo.layer == Layer::Bullet)
			{
				hitInfo.onHit(Layer::Bullet);
				OnHit(Layer::Bullet);
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

    _transform.SetLocalPosition(newPos);
    
}

//=============================================================================

MFA::Transform & Bullet::Transform()
{
    return _transform;
}

//=============================================================================

bool Bullet::IsAlive() const
{
	return _isAlive;
}

//=============================================================================

void Bullet::OnHit(Physics2D::Layer layer)
{
	if (layer == Layer::Bullet)
	{
		Die();
	}
}

//=============================================================================

void Bullet::Die()
{
	if (_isAlive == false)
	{
		return;
	}
	_isAlive = false;
	if (Physics2D::Instance != nullptr)
	{
		Physics2D::Instance->UnRegister(_physicsId);
	}
}

//=============================================================================
