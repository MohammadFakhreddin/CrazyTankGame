#include "Gameplay.hpp"

TankEntity::TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos, float initBA, float initScale) {
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

bool TankEntity::CheckCollision(glm::vec2 fPos, float bAngl, float scl) {
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

MFA::Transform TankEntity::GetTransform(glm::vec2 fPos, float bAngl, float scl) {
	MFA::Transform transform{};
	transform.Setposition({ fPos.x, 0.f, fPos.y });
	transform.Setscale(glm::vec3{ scl });
	transform.SetQuaternion(glm::angleAxis(bAngl, glm::vec3{ 0.f, 1.f, 0.f }));
	return transform;
}

BulletEntity::BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec3& initPos, float initBA, float initScale) {
	_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

	position = initPos;
	baseAngle = initBA;
	scale = initScale;
	UpdateMI();
}

GameInstance::GameInstance(std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
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

	AddTankEnemy(map.CellPosition(map.RandomTile()));
	AddTankEnemy(map.CellPosition(map.RandomTile()));

}

MFA::Transform BulletEntity::GetTransform(glm::vec3 pos, float bAngl, float scl) {
	MFA::Transform transform{};
	transform.Setposition(pos);
	transform.Setscale(glm::vec3{ scl });
	transform.SetQuaternion(glm::angleAxis(bAngl, glm::vec3{ 0.f, 1.f, 0.f }));
	return transform;
}

std::list<GameInstance::TankAI>::iterator GameInstance::AddTankEnemy(const glm::vec2& pos) {
	simple_tank_enemies.emplace_back(TankEntity{ *_eTankRenderer, pos }, std::vector<glm::vec2>{});
	return std::prev(simple_tank_enemies.end());
}

void GameInstance::reset() {
	_pTankRenderer.reset();
	_eTankRenderer.reset();
	_pBulletRenderer.reset();
}

void GameInstance::Update(float delta, const glm::vec2& joystickInp, bool inputA, bool inputB) {
	glm::vec2 player_new_pos = player.flatPosition + player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;
	float player_new_angle = fmodf(player.baseAngle + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());

	if (!inputA && _inputA) {
		player_bullets.emplace_back(*_pBulletRenderer, player.ShootPos(), player.baseAngle, 0.1f);
	}

	for (TankAI& e : simple_tank_enemies) {
		switch (e.state)
		{
		case TankAI::TankAiState::MOVING:
		{
			float delta_dist = glm::dot(-e.entity.BaseDir(), e.path_queue.back() - e.entity.flatPosition);
			if (delta_dist > 1e-3f) {
				e.entity.flatPosition -= e.entity.BaseDir() * PLAYER_SPEED * delta;
			}
			else {
				e.path_queue.pop_back();
				e.state = TankAI::TankAiState::AT_NODE;
			}
		}
			break;
		case TankAI::TankAiState::AT_NODE:
		{
			while (e.path_queue.empty()) {
				auto new_goal = map.RandomTile();
				e.path_queue = map.AStar(map.PositionCoord(e.entity.flatPosition), new_goal);
			}
			float goal_angle = e.entity.AimAt(e.path_queue.back() - e.entity.flatPosition);
			float delta_angle = goal_angle - e.entity.baseAngle;
			if (fabsf(delta_angle) > 1e-2f) {
				float angular_vel = delta_angle > 0.f ? 1.f : -1.f;
				e.entity.baseAngle = fmodf(e.entity.baseAngle + angular_vel * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
			}
			else {
				e.state = TankAI::TankAiState::MOVING;
			}
		}
			break;
		default: break;
		}
		e.entity.UpdateMI();
	}

	if (!player.CheckCollision(player_new_pos, player_new_angle, player.scale)) {
		player.flatPosition = player_new_pos;
		player.baseAngle = player_new_angle;
	}
	player.UpdateMI();

	_inputA = inputA;
	_inputB = inputB;

	std::vector<std::list<BulletEntity>::iterator> toRemove;
	for (std::list<BulletEntity>::iterator it = player_bullets.begin(); it != player_bullets.end(); ++it) {
		it->position += it->BaseDir() * TEST_BULLET_SPEED * delta;
		it->lifetimer -= delta;
		if (it->lifetimer <= 0) {
			toRemove.emplace_back(it);
		}
		it->UpdateMI();
	}
	for (std::list<BulletEntity>::iterator it : toRemove) {
		player_bullets.erase(it);
	}
}

void GameInstance::Render(MFA::RT::CommandRecordState& recordState) {
	_pTankRenderer->Render(recordState, { player.GetMI() });

	for (const BulletEntity& b : player_bullets) {
		_pBulletRenderer->Render(recordState, { b.GetMI() });
	}

	for (const TankAI& e : simple_tank_enemies) {
		_eTankRenderer->Render(recordState, { e.entity.GetMI() });
	}

	map.Render(recordState);
}

void GameInstance::RenderNoMap(MFA::RT::CommandRecordState& recordState) {
	_pTankRenderer->Render(recordState, { player.GetMI() });

	for (const BulletEntity& b : player_bullets) {
		_pBulletRenderer->Render(recordState, { b.GetMI() });
	}

	for (const TankAI& e : simple_tank_enemies) {
		_pTankRenderer->Render(recordState, { e.entity.GetMI() });
	}
}