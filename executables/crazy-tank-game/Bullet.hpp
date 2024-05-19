#pragma once

#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"
#include "Physics2D.hpp"

#include <glm/glm.hpp>

class Bullet
{
public:
	// TODO: Support pooling
	struct Params
	{
		float moveSpeed = 20.0f;			// Bullet move speed
		float radius = 0.25f;				// Bullet radius
		float friendlyFireDelay = 0.5f;		// Bullet won't hit the owner before this duration 
	};

	explicit Bullet(
		std::shared_ptr<Params> param, 
		Physics2D::EntityID ownerId
	);

	~Bullet();

    void Update(float deltaTimeSec);

	[[nodiscard]]
	MFA::Transform & Transform();

	[[nodiscard]]
	bool IsAlive() const;

private:

	void OnHit(Physics2D::Layer layer);

	void Die();

    std::shared_ptr<Params> _params{};
	Physics2D::EntityID _ownerId{};
	float _noFriendlyFireRemainingTime = 0.0f;

	MFA::Transform _transform {};
	Physics2D::EntityID _physicsId{};

	bool _isAlive = true;

};