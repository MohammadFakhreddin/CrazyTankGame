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
	glm::vec2 flatPosition{};
	glm::vec2 flatColliderDimension{ 2.f, 2.f };
	float baseAngle = 0.f, scale = 1.f;

	TankEntity() {}

	TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos = {}, float initBA = 0.f, float initScale = 0.16f) {
		_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

		flatPosition = initPos;
		baseAngle = initBA;
		scale = initScale;

		_shootPos = scale * _meshInstance->FindNode("Shoot")->transform.Getposition();

		_collider.emplace_back(glm::vec4{ flatColliderDimension.x, 0.0f, flatColliderDimension.y, 1.0f });
		_collider.emplace_back(glm::vec4{ -flatColliderDimension.x, 0.0f, flatColliderDimension.y, 1.0f });
		_collider.emplace_back(glm::vec4{ flatColliderDimension.x, 0.0f, -flatColliderDimension.y, 1.0f });
		_collider.emplace_back(glm::vec4{ -flatColliderDimension.x, 0.0f, -flatColliderDimension.y, 1.0f });

		UpdateMI();
	}

	void UpdateMI() {
		_meshInstance->GetTransform() = GetTransform(flatPosition, baseAngle, scale);
	}

	bool CheckCollision(glm::vec2 fPos, float bAngl, float scl) {
		Physics2D::HitInfo hitInfo{
			.hitTime = 1000.0f
		};

		for (auto& p : _collider)
		{
			auto const p_from = _meshInstance->GetTransform().GetMatrix() * p;
			auto const p_to = GetTransform(fPos, bAngl, scl).GetMatrix() * p;
			auto const vector = p_to - p_from;
			auto const length = glm::length(vector);
			if (length > 0.0f)
			{
				auto const direction = vector / length;
				Physics2D::HitInfo myHitInfo{};
				auto const hit = Physics2D::Instance->Raycast(
					Layer::WallLayer,
					-1,
					glm::vec2{ p_from.x, p_from.z },
					glm::vec2{ direction.x, direction.z },
					length,
					myHitInfo
				);
				if (hit)
				{
					if (myHitInfo.hitTime < hitInfo.hitTime)
					{
						hitInfo = myHitInfo;
					}
					return true;
				}
			}
		}
		return false;
	}

	static MFA::Transform GetTransform(glm::vec2 fPos, float bAngl, float scl) {
		MFA::Transform transform{};
		transform.Setposition({ fPos.x, 0.f, fPos.y });
		transform.Setscale(glm::vec3{ scl });
		transform.SetQuaternion(glm::angleAxis(bAngl, glm::vec3{ 0.f, 1.f, 0.f }));
		return transform;
	}

	MFA::MeshInstance* GetMI() const {
		return _meshInstance.get();
	}

	glm::vec2 BaseDir() const {
		return { -sinf(baseAngle), -cosf(baseAngle) };
	}

	glm::vec3 ShootPos() const {
		return glm::vec3{ flatPosition.x, 0.f, flatPosition.y } + glm::rotate(glm::angleAxis(baseAngle, glm::vec3{ 0.0f, 1.f, 0.f }), _shootPos);
	}

	void AimAt(const glm::vec2 aim_dir) {
		baseAngle = fmodf(glm::half_pi<float>() + atan2f(-aim_dir.y, aim_dir.x), glm::two_pi<float>());
	}

private:
	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
	glm::vec3 _shootPos{};

	std::vector<glm::vec4> _collider{};
};

struct BulletEntity {
	glm::vec3 position{};
	float baseAngle = 0.f, scale = 1.f;
	float lifetimer = 4.f;

	BulletEntity() {}

	BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec3& initPos = {}, float initBA = 0.f, float initScale = 0.1f) {
		_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

		position = initPos;
		baseAngle = initBA;
		scale = initScale;
		UpdateMI();
	}

	void UpdateMI() {
		_meshInstance->GetTransform() = GetTransform(position, baseAngle, scale);
	}

	static MFA::Transform GetTransform(glm::vec3 pos, float bAngl, float scl) {
		MFA::Transform transform{};
		transform.Setposition(pos);
		transform.Setscale(glm::vec3{ scl });
		transform.SetQuaternion(glm::angleAxis(bAngl, glm::vec3{ 0.f, 1.f, 0.f }));
		return transform;
	}

	MFA::MeshInstance* GetMI() const {
		return _meshInstance.get();
	}

	glm::vec3 BaseDir() const {
		return { sinf(baseAngle), 0.f, cosf(baseAngle) };
	}

private:
	std::unique_ptr<MFA::MeshInstance> _meshInstance{};
};

struct GameInstance {
	static constexpr float PLAYER_SPEED = 4.0f;
	static constexpr float TEST_BULLET_SPEED = 10.0f;
	static constexpr float PLAYER_TURN_SPEED = glm::pi<float>();
	TankEntity player;
	Map map;

	std::list<BulletEntity> test_player_bullets;

	std::list<TankEntity> test_enemies;

	GameInstance(std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
		std::shared_ptr<MFA::RT::GpuTexture> errorTexture,
		int rows, int columns, std::vector<int> const& walls
	) : map(float(rows), float(columns), rows, columns, walls, pipeline, errorTexture) {
		_pTankRenderer = std::make_unique<MFA::MeshRenderer>(
			pipeline,
			MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/player_tank.glb")),
			errorTexture,
			true,
			glm::vec4{ 0.0f, 0.25f, 0.0f, 1.0f }
		);

		_eTankRenderer = std::make_unique<MFA::MeshRenderer>(
			pipeline,
			MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/enemy_tank.glb")),
			errorTexture,
			true,
			glm::vec4{ 0.25f, 0.0f, 0.0f, 1.0f }
		);

		_pBulletRenderer = std::make_unique<MFA::MeshRenderer>(
			pipeline,
			MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/test/cube.glb")),
			errorTexture,
			true,
			glm::vec4{ 0.0f, 0.25f, 0.0f, 1.0f }
		);

		player = TankEntity(*_pTankRenderer);

		map.AStar(1, 1, 8, 8);
	}

	void reset() {
		_pTankRenderer.reset();
	}

	void Update(float delta, const glm::vec2& joystickInp, bool inputA, bool inputB) {
		glm::vec2 player_new_pos = player.flatPosition + player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;
		float player_new_angle = fmodf(player.baseAngle + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());

		if (!inputA && _inputA) {
			test_player_bullets.emplace_back(*_pBulletRenderer, player.ShootPos(), player.baseAngle, 0.1f);
		}

		for (TankEntity& e : test_enemies) {
			e.AimAt(player.flatPosition - e.flatPosition);
			e.UpdateMI();
		}

		if (!player.CheckCollision(player_new_pos, player_new_angle, player.scale)) {
			player.flatPosition = player_new_pos;
			player.baseAngle = player_new_angle;
		}
		player.UpdateMI();

		_inputA = inputA;
		_inputB = inputB;

		std::vector<std::list<BulletEntity>::iterator> toRemove;
		for (std::list<BulletEntity>::iterator it = test_player_bullets.begin(); it != test_player_bullets.end(); ++it) {
			it->position += it->BaseDir() * TEST_BULLET_SPEED * delta;
			it->lifetimer -= delta;
			if (it->lifetimer <= 0) {
				toRemove.emplace_back(it);
			}
			it->UpdateMI();
		}
		for (std::list<BulletEntity>::iterator it : toRemove) {
			test_player_bullets.erase(it);
		}
	}

	void Render(MFA::RT::CommandRecordState& recordState) {
		_pTankRenderer->Render(recordState, { player.GetMI() });

		for (const BulletEntity& b : test_player_bullets) {
			_pBulletRenderer->Render(recordState, { b.GetMI() });
		}

		for (const TankEntity& e : test_enemies) {
			_pTankRenderer->Render(recordState, { e.GetMI() });
		}

		map.Render(recordState);
	}

	void RenderNoMap(MFA::RT::CommandRecordState& recordState) {
		_pTankRenderer->Render(recordState, { player.GetMI() });

		for (const BulletEntity& b : test_player_bullets) {
			_pBulletRenderer->Render(recordState, { b.GetMI() });
		}

		for (const TankEntity& e : test_enemies) {
			_pTankRenderer->Render(recordState, { e.GetMI() });
		}
	}

private:
	bool _inputA = false, _inputB = false;
	std::unique_ptr<MFA::MeshRenderer> _pTankRenderer, _eTankRenderer;
	std::unique_ptr<MFA::MeshRenderer> _pBulletRenderer;
};