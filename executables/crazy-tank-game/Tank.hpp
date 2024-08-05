#pragma once

#include "Physics2D.hpp"
#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"
#include "Bullet.hpp"

#include <glm/glm.hpp>

class Tank {

public:

	struct Params
	{
		float moveSpeed = 5.0f;
		float rotationSpeed = 10.0f;
		glm::vec2 halfColliderExtent{0.5, 0.5};
		float shootCooldown = 0.25f;
	};

	explicit Tank(
		MFA::MeshRenderer const & meshRenderer,
		std::shared_ptr<Params> params
	);

	void OnHit(Physics2D::Layer layer);

	bool Move(glm::vec2 const & direction, float deltaTimeSec);

	[[nodiscard]]
	std::unique_ptr<Bullet> Shoot(std::shared_ptr<Bullet::Params> params);

	[[nodiscard]]
	MFA::MeshInstance * MeshInstance() const;

	[[nodiscard]]
	MFA::Transform & Transform();

	[[nodiscard]]
	bool IsAlive() const;

private:

	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
	MFA::Transform * _transform = nullptr;
	std::shared_ptr<Params> _params{};
	Physics2D::EntityID _physicsId{};
	MFA::Transform * _shootTransform = nullptr;

	float _shootCooldownEndTime = -1000.0f;

	bool _isAlive = true;

};
