#pragma once

#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"
#include "Physics2D.hpp"

#include <glm/glm.hpp>

class Bullet
{
public:

	struct Params
	{
		float moveSpeed = 20.0f;
		float radius = 0.25f;
	};

	explicit Bullet(std::shared_ptr<Params> params);

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
	MFA::Transform _transform {};
	Physics2D::EntityID _physicsId{};

	bool _isAlive = true;

};