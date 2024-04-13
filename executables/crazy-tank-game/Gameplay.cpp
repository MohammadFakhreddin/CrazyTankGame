#include "Gameplay.hpp"

BulletEntity::BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec3& initPos, float initBA, float initScale) {
	_meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

	position = initPos;
	angle = initBA;
	scale = initScale;

	physicsId = Physics2D::Instance->Register(
		Physics2D::Type::Sphere,
		Layer::ShellLayer,
		Layer::WallLayer | Layer::TankLayer | Layer::ShellLayer,
		[this](auto layer) { OnHit(layer); }
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
	player.physicsId = Physics2D::Instance->Register(
		Physics2D::Type::AABB,
		Layer::TankLayer,
		Layer::WallLayer | Layer::TankLayer,
		[&](auto layer) { player.OnHit(layer); }
	);

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
	auto ret = std::prev(simple_tank_enemies.end());
	ret->entity.physicsId = Physics2D::Instance->Register(
		Physics2D::Type::AABB,
		Layer::TankLayer,
		Layer::WallLayer | Layer::TankLayer,
		[ret](auto layer) { ret->entity.OnHit(layer); }
	);
	return ret;
}

void GameInstance::reset() {
	_pTankRenderer.reset();
	_eTankRenderer.reset();
	_pBulletRenderer.reset();
}

void GameInstance::Update(float delta, const glm::vec2& joystickInp, bool inputA, bool inputB) {

	{
		auto const player_new_pos = player.GetFlatPos() + player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;
		auto const player_new_angle = fmodf(player.GetAngle() + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
		player.Move(player_new_pos, player_new_angle, true);
		/*if (!player.CheckCollision(player_new_pos, player_new_angle, player._scale)) {
			player._flatPosition = player_new_pos;
			player._baseAngle = player_new_angle;
		}
		player.UpdateMI();*/

		if (!inputA && _inputA) {
			player_bullets.emplace_back(*_pBulletRenderer, player.ShootPos(), player.GetAngle(), 0.1f);
		}

		_inputA = inputA;
		_inputB = inputB;
	}
	std::vector<std::list<TankAI>::iterator> steToRemove;
	{
		// TODO: Enemy should handle collision too
		for (std::list<TankAI>::iterator e_it = simple_tank_enemies.begin(); e_it != simple_tank_enemies.end(); ++e_it) {
			if (e_it->entity.hitCount > 0) {
				steToRemove.push_back(e_it);
			}

			auto enemy_pos = e_it->entity.GetFlatPos();
			auto enemy_angle = e_it->entity.GetAngle();
			switch (e_it->state)
			{
				case TankAI::TankAiState::MOVING:
				{
					float delta_dist = glm::dot(-e_it->entity.BaseDir(), e_it->path_queue.back() - enemy_pos);
					if (delta_dist > 1e-3f) {
						enemy_pos -= e_it->entity.BaseDir() * PLAYER_SPEED * delta;
					}
					else {
						e_it->path_queue.pop_back();
						e_it->state = TankAI::TankAiState::AT_NODE;
					}
				}
				break;
				case TankAI::TankAiState::AT_NODE:
				{
					while (e_it->path_queue.empty()) {
						auto new_goal = map.RandomTile();
						e_it->path_queue = map.AStar(map.PositionCoord(enemy_pos), new_goal);
					}
					float goal_angle = e_it->entity.AimAt(e_it->path_queue.back() - enemy_pos);
					float delta_angle = goal_angle - enemy_angle;
					delta_angle = delta_angle > glm::pi<float>() ? delta_angle - glm::two_pi<float>() : delta_angle;
					if (fabsf(delta_angle) > 1e-2f) {
						float angular_vel = delta_angle > 0.f ? 1.f : -1.f;
						enemy_angle = fmodf(enemy_angle + angular_vel * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
					}
					else {
						e_it->state = TankAI::TankAiState::MOVING;
					}
				}
				break;
				default: break;
			}
			e_it->entity.Move(enemy_pos, enemy_angle, true);
			//e.entity.UpdateMI();
		}
	}

	std::vector<std::list<BulletEntity>::iterator> bulletToRemove;
	for (std::list<BulletEntity>::iterator b_it = player_bullets.begin(); b_it != player_bullets.end(); ++b_it) {
		if (b_it->isHit) {
			bulletToRemove.emplace_back(b_it);
		}
		glm::vec3 displacement = b_it->BaseDir() * TEST_BULLET_SPEED * delta;
		Physics2D::HitInfo hit_info{};
		bool hit = Physics2D::Instance->Raycast(
			Layer::WallLayer | Layer::TankLayer, 
			b_it->physicsId, 
			Physics2D::Ray{ glm::vec2{ b_it->position.x, b_it->position.z }, glm::vec2{ b_it->BaseDir().x, b_it->BaseDir().z } },
			TEST_BULLET_SPEED* delta,
			hit_info);
		if (hit == true) {
			if (hit_info.layer & (Layer::TankLayer | Layer::ShellLayer))
			{
				if (hit_info.onHit != nullptr)
				{
					hit_info.onHit(Layer::ShellLayer);
					bulletToRemove.emplace_back(b_it);
				}
			}
			else
			{
				--b_it->bounceLeft;
				if (b_it->bounceLeft >= 0) b_it->Reflect(hit_info.hitNormal);
				else bulletToRemove.emplace_back(b_it);
			}
		}
		b_it->position += displacement;
		Physics2D::Instance->MoveSphere(b_it->physicsId, glm::vec2{ b_it->position.x, b_it->position.z }, 0.1f, false);
		b_it->lifetimer -= delta;
		if (b_it->lifetimer <= 0) {
			bulletToRemove.emplace_back(b_it);
		}
		b_it->UpdateMI();
	}
	for (std::list<BulletEntity>::iterator it : bulletToRemove) {
		Physics2D::Instance->UnRegister(it->physicsId);
		player_bullets.erase(it);
	}
	for (std::list<TankAI>::iterator it : steToRemove) {
		Physics2D::Instance->UnRegister(it->entity.physicsId);
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