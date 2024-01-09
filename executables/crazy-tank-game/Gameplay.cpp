#include "Gameplay.hpp"

TankEntity::TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos, float initBA, float initScale) {
	_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

	_flatPosition = initPos;
	_baseAngle = initBA;
	_scale = initScale;

	_shootPos = _meshInstance->FindNode("Shoot")->transform.Getposition();

	/*_collider.emplace_back(glm::vec4{ _flatColliderDimension.x, 0.0f, _flatColliderDimension.y, 1.0f });
	_collider.emplace_back(glm::vec4{ -_flatColliderDimension.x, 0.0f, _flatColliderDimension.y, 1.0f });
	_collider.emplace_back(glm::vec4{ -_flatColliderDimension.x, 0.0f, -_flatColliderDimension.y, 1.0f });
	_collider.emplace_back(glm::vec4{ _flatColliderDimension.x, 0.0f, -_flatColliderDimension.y, 1.0f });
	MFA_ASSERT(_collider.size() == 4);*/

	_physicsId = Physics2D::Instance->Register(
		Physics2D::Type::AABB,
		Layer::TankLayer,
		Layer::WallLayer | Layer::TankLayer,
		[this] (auto layer) { OnHit(layer); }
	);

	UpdateMI();
}

bool TankEntity::Move(glm::vec2 fPos, float bAngl, bool checkForCollision) {
	Physics2D::HitInfo hitInfo{
		.hitTime = 1000.0f
	};

	auto transform = GetTransform(fPos, bAngl, _scale);
	auto const & matrix = transform.GetMatrix();

	/*std::vector<glm::vec2> points{};
	for (auto& p : _collider)
	{
		auto const p_to = matrix * p;
		points.emplace_back(glm::vec2{ p_to.x, p_to.z });
	}*/
	glm::vec2 max = fPos + _flatColliderDimension * 0.5f;
	glm::vec2 min = fPos - _flatColliderDimension * 0.5f;

	bool success = Physics2D::Instance->MoveAABB(
		_physicsId,
		min,
		max,
		checkForCollision
	);

	if (success == true)
	{
		_meshInstance->SetTransform(transform);
		_flatPosition = fPos;
		_baseAngle = bAngl;
	}

	return success;
}

MFA::Transform TankEntity::GetTransform(glm::vec2 fPos, float bAngl, float scl) {
	MFA::Transform transform{};
	transform.Setposition({ fPos.x, 0.f, fPos.y });
	transform.Setscale(glm::vec3{ scl });
	transform.SetQuaternion(glm::angleAxis(bAngl, glm::vec3{ 0.f, 1.f, 0.f }));
	return transform;
}

void TankEntity::OnHit(Physics2D::Layer layer)
{
	// TODO: Handle bullet hit here
}

BulletEntity::BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec3& initPos, float initBA, float initScale) {
	_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

	position = initPos;
	baseAngle = initBA;
	scale = initScale;

	physicsId = Physics2D::Instance->Register(
		Physics2D::Type::Sphere,
		Layer::ShellLayer,
		Layer::WallLayer | Layer::TankLayer | Layer::ShellLayer,
		nullptr
	);

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

	//AddTankEnemy(map.CellPosition(map.RandomTile()));
	//AddTankEnemy(map.CellPosition(map.RandomTile()));

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

	{
		auto const player_new_pos = player.GetFPos() + player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;
		auto const player_new_angle = fmodf(player.GetBAngle() + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
		player.Move(player_new_pos, player_new_angle, true);
		/*if (!player.CheckCollision(player_new_pos, player_new_angle, player._scale)) {
			player._flatPosition = player_new_pos;
			player._baseAngle = player_new_angle;
		}
		player.UpdateMI();*/

		if (!inputA && _inputA) {
			player_bullets.emplace_back(*_pBulletRenderer, player.ShootPos(), player.GetBAngle(), 0.1f);
		}

		_inputA = inputA;
		_inputB = inputB;
	}
	{
		// TODO: Enemy should handle collision too
		for (TankAI& e : simple_tank_enemies) {
			auto enemy_pos = e.entity.GetFPos();
			auto enemy_angle = e.entity.GetBAngle();
			switch (e.state)
			{
				case TankAI::TankAiState::MOVING:
				{
					float delta_dist = glm::dot(-e.entity.BaseDir(), e.path_queue.back() - enemy_pos);
					if (delta_dist > 1e-3f) {
						enemy_pos -= e.entity.BaseDir() * PLAYER_SPEED * delta;
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
						e.path_queue = map.AStar(map.PositionCoord(enemy_pos), new_goal);
					}
					float goal_angle = e.entity.AimAt(e.path_queue.back() - enemy_pos);
					float delta_angle = goal_angle - enemy_angle;
					delta_angle = delta_angle > glm::pi<float>() ? delta_angle - glm::two_pi<float>() : delta_angle;
					if (fabsf(delta_angle) > 1e-2f) {
						float angular_vel = delta_angle > 0.f ? 1.f : -1.f;
						enemy_angle = fmodf(enemy_angle + angular_vel * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
					}
					else {
						e.state = TankAI::TankAiState::MOVING;
					}
				}
				break;
				default: break;
			}
			e.entity.Move(enemy_pos, enemy_angle, true);
			//e.entity.UpdateMI();
		}
	}

	std::vector<std::list<BulletEntity>::iterator> pbToRemove;
	std::vector<std::list<TankAI>::iterator> steToRemove;
	for (std::list<BulletEntity>::iterator pbit = player_bullets.begin(); pbit != player_bullets.end(); ++pbit) {
		glm::vec3 displacement = pbit->BaseDir() * TEST_BULLET_SPEED * delta;
		Physics2D::HitInfo wallHitInfo{};
		bool hit = Physics2D::Instance->Raycast(
			Layer::WallLayer | Layer::TankLayer, 
			pbit->physicsId, 
			Physics2D::Ray{ glm::vec2{ pbit->position.x, pbit->position.z }, glm::vec2{ pbit->BaseDir().x, pbit->BaseDir().z } },
			TEST_BULLET_SPEED* delta,
			wallHitInfo);
		if (hit == true) {
			if ((wallHitInfo.layer & Layer::TankLayer) > 0)
			{
				if (wallHitInfo.onHit != nullptr)
				{
					wallHitInfo.onHit(Layer::ShellLayer);
				}
			}
			else
			{
				--pbit->hitLeft;
				if (pbit->hitLeft >= 0) pbit->Reflect(wallHitInfo.hitNormal);
				else pbToRemove.emplace_back(pbit);
			}
		}
		pbit->position += displacement;
		pbit->lifetimer -= delta;
		if (pbit->lifetimer <= 0) {
			pbToRemove.emplace_back(pbit);
		}
		pbit->UpdateMI();
	}
	for (std::list<BulletEntity>::iterator it : pbToRemove) {
		player_bullets.erase(it);
	}
	for (std::list<TankAI>::iterator it : steToRemove) {
		simple_tank_enemies.erase(it);
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