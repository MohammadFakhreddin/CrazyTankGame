#pragma once

#include "Physics2D.hpp"
#include "utils/MeshRenderer.hpp"
#include <glm/glm.hpp>

class Tank {

public:

	struct Params
	{
		float moveSpeed = 10.0f;
		float rotationSpeed = 10.0f;

		glm::vec2 halfColliderExtent{0.5, 0.5};
	};

	explicit Tank(
		MFA::MeshRenderer const & meshRenderer,
		MFA::Transform const & transform,
		std::shared_ptr<Params> params
	);

	void OnHit(Physics2D::Layer layer);

	bool Move(glm::vec2 const & direction,float deltaTimeSec);

	[[nodiscard]]
	MFA::MeshInstance * MeshInstance();

private:

	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
	MFA::Transform* _transform = nullptr;
	std::shared_ptr<Params> _params{};
	Physics2D::EntityID _physicsId{};

};
