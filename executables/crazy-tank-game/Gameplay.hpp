#pragma once

#include "BufferTracker.hpp"
#include "BedrockPath.hpp"
#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"

#include "Physics2D.hpp"
#include "Layers.hpp"
#include "Map.hpp"

#include <list>
#include <iostream>

struct TankEntity {
	int hitCount = 0;
	Physics2D::EntityID physicsId{};
	
	TankEntity() {}

	TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos = {}, float initBA = 0.f, float initScale = 0.16f);

	bool Move(glm::vec2 fPos, float bAngl, bool checkForCollision);

	MFA::MeshInstance* GetMI() const { return _meshInstance.get(); }

	glm::vec2 BaseDir() const { return { -sinf(_angle), -cosf(_angle) }; }

	glm::vec3 ShootPos() const { return _meshInstance->GetTransform().GetMatrix() * glm::vec4(_shootPos, 1.f); }

	float AimAt(const glm::vec2 aim_dir) { return fmodf(glm::half_pi<float>() + atan2f(-aim_dir.y, aim_dir.x), glm::two_pi<float>()); }

	float GetAngle() { return _angle; }

	glm::vec2 GetFlatPos() { return _flatPosition; }

	void OnHit(Physics2D::Layer layer);

private:

	void UpdateMI() { _meshInstance->GetTransform() = GetTransform(_flatPosition, _angle, _scale); }

	static MFA::Transform GetTransform(glm::vec2 fPos, float bAngl, float scl);

	glm::vec2 _flatPosition{};
	glm::vec2 _flatColliderDimension{ 0.75f, 0.75f };
	float _angle = 0.f, _scale = 1.f;

	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
	glm::vec3 _shootPos{};
	float _radius = 0.25f;
	//std::vector<glm::vec4> _collider{};
};

struct BulletEntity {
	glm::vec3 position{};
	float angle = 0.f, scale = 1.f;
	float lifetimer = 4.f;
	int bounceLeft = 2;
	Physics2D::EntityID physicsId{};

	bool isHit = false;

	BulletEntity() {}

	BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec3& initPos = {}, float initBA = 0.f, float initScale = 0.1f);

	void UpdateMI() { _meshInstance->GetTransform() = GetTransform(position, angle, scale); }

	static MFA::Transform GetTransform(glm::vec3 pos, float bAngl, float scl);

	MFA::MeshInstance* GetMI() const { return _meshInstance.get(); }

	glm::vec3 BaseDir() const { return { sinf(angle), 0.f, cosf(angle) }; }

	void Reflect(glm::vec2 const& wallNormal) {
		glm::vec2 in_dir{ sinf(angle), cosf(angle) };
		glm::vec2 out_dir = in_dir - 2.f * glm::dot(in_dir, wallNormal) * wallNormal;
		angle = AimAt(out_dir);
	}

	float AimAt(const glm::vec2 aim_dir) { return fmodf(glm::half_pi<float>() + atan2f(-aim_dir.y, aim_dir.x), glm::two_pi<float>()); }

private:
	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
	void OnHit(Physics2D::Layer layer) { isHit = true; }
};

struct GameInstance {
	static constexpr float PLAYER_SPEED = 2.0f;
	static constexpr float TEST_BULLET_SPEED = 4.0f;
	static constexpr float PLAYER_TURN_SPEED = glm::half_pi<float>();
	TankEntity player;
	Map map;

	std::list<BulletEntity> player_bullets;

	struct TankAI {
		enum class TankAiState {
			MOVING,
			AT_NODE
		};

		TankEntity entity;
		std::vector<glm::vec2> path_queue;
		TankAiState state = TankAiState::AT_NODE;
	};
	std::list<TankAI> simple_tank_enemies;

	GameInstance(std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
		std::shared_ptr<MFA::RT::GpuTexture> errorTexture,
		int rows, int columns, std::vector<int> const& walls
	);

	std::list<TankAI>::iterator AddTankEnemy(const glm::vec2& pos);

	void reset();

	void Update(float delta, const glm::vec2& joystickInp, bool inputA, bool inputB);

	void Render(MFA::RT::CommandRecordState& recordState);

	void RenderNoMap(MFA::RT::CommandRecordState& recordState);

private:
	bool _inputA = false, _inputB = false;
	std::unique_ptr<MFA::MeshRenderer> _pTankRenderer, _eTankRenderer;
	std::unique_ptr<MFA::MeshRenderer> _pBulletRenderer;
};