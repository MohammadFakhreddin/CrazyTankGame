#pragma once

#include "BufferTracker.hpp"
#include "BedrockPath.hpp"
#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"

#include <list>

struct TankEntity {
    glm::vec2 flatPosition{};
    float baseAngle = 0.f, headAngle = 0.f, scale = 1.f;

    TankEntity() {}

    TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos = {}, float initBA = 0.f, float initHA = 0.f, float initScale = 0.1f) {
        _meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);
        _headNode = _meshInstance->FindNode("Head");
        _meshInstance->FindNode("Tip");
        MFA_ASSERT(_headNode != nullptr);

        flatPosition = initPos;
        baseAngle = initBA;
        headAngle = initHA;
        scale = initScale;
        UpdateMI();
    }

    void UpdateMI() {
        auto& transform = _meshInstance->GetTransform();
        transform.Setposition({ flatPosition.x, 0.f, flatPosition.y });
        transform.Setscale(glm::vec3{ scale });
        transform.SetQuaternion(glm::angleAxis(baseAngle, glm::vec3{ 0.f, 1.f, 0.f }));
        _headNode->transform.SetQuaternion(glm::angleAxis(headAngle, glm::vec3{ 0.f, 1.f, 0.f }));
    }

    MFA::MeshInstance* GetMI() const {
        return _meshInstance.get();
    }

    glm::vec2 BaseDir() const {
        return { sinf(baseAngle), cosf(baseAngle) };
    }

    glm::vec2 HeadDir() const {
        return { sinf(baseAngle + headAngle), cosf(baseAngle + headAngle) };
    }

    void AimAt(const glm::vec2 aim_dir) {
        headAngle = fmodf(-glm::half_pi<float>() - atan2f(aim_dir.y, aim_dir.x) - baseAngle, glm::two_pi<float>());
    }

private:
    std::unique_ptr<MFA::MeshInstance> _meshInstance{};
    MFA::Asset::GLTF::Node* _headNode = nullptr;
};

struct BulletEntity {
    glm::vec2 flatPosition{};
    float baseAngle = 0.f, scale = 1.f;
    float lifetimer = 4.f;

    BulletEntity() {}

    BulletEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos = {}, float initBA = 0.f, float initScale = 0.1f) {
        _meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);

        flatPosition = initPos;
        baseAngle = initBA;
        scale = initScale;
        UpdateMI();
    }

    void UpdateMI() {
        auto& transform = _meshInstance->GetTransform();
        transform.Setposition({ flatPosition.x, 0.f, flatPosition.y });
        transform.Setscale(glm::vec3{ scale });
        transform.SetQuaternion(glm::angleAxis(baseAngle, glm::vec3{ 0.f, 1.f, 0.f }));
    }

    MFA::MeshInstance* GetMI() const {
        return _meshInstance.get();
    }

    glm::vec2 BaseDir() const {
        return { -sinf(baseAngle), -cosf(baseAngle) };
    }

private:
    std::unique_ptr<MFA::MeshInstance> _meshInstance{};
};

struct GameInstance {
    static constexpr float PLAYER_SPEED = 4.0f;
    static constexpr float PLAYER_TURN_SPEED = glm::quarter_pi<float>();
    static constexpr float PLAYER_HEAD_TURN_SPEED = glm::half_pi<float>();
    TankEntity player;
    std::list<BulletEntity> test_player_bullets;

    std::list<TankEntity> test_enemies;

    GameInstance(std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
        std::shared_ptr<MFA::RT::GpuTexture> errorTexture) {
        _pTankRenderer = std::make_unique<MFA::MeshRenderer>(
            pipeline,
            MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/test/tank_2.glb")),
            errorTexture,
            true,
            glm::vec4{ 0.0f, 0.25f, 0.0f, 1.0f }
        );
        player = TankEntity(*_pTankRenderer);

        _pBulletRenderer = std::make_unique<MFA::MeshRenderer>(
            pipeline,
            MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/test/cube.glb")),
            errorTexture,
            true,
            glm::vec4{ 0.0f, 0.25f, 0.0f, 1.0f }
        );
    }

    void add_test_enemies() {
        float new_x = float(rand() % 1500) * 1e-2f - 7.5f;
        float new_y = float(rand() % 1500) * 1e-2f - 7.5f;
        test_enemies.emplace_back(*_pTankRenderer, glm::vec2{ new_x, new_y });
    }

    void reset() {
        _pTankRenderer.reset();
    }

    void Update(float delta, const glm::vec2& joystickInp, bool inputA, bool inputB) {
        player.flatPosition += player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;

        if (!inputA) {
            player.baseAngle = fmodf(player.baseAngle + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
        }
        else {
            player.headAngle = fmodf(player.headAngle + joystickInp.x * PLAYER_HEAD_TURN_SPEED * delta, glm::two_pi<float>());
        }

        if (!inputA && _inputA) {
            test_player_bullets.emplace_back(*_pBulletRenderer, player.flatPosition, player.baseAngle + player.headAngle, 0.1f);
        }

        player.UpdateMI();

        for (TankEntity& e : test_enemies) {
            e.AimAt(player.flatPosition - e.flatPosition);
            e.UpdateMI();
        }

        _inputA = inputA;
        _inputB = inputB;

        std::vector<std::list<BulletEntity>::iterator> toRemove;
        for (std::list<BulletEntity>::iterator it = test_player_bullets.begin(); it != test_player_bullets.end(); ++it) {
            it->flatPosition += it->BaseDir() * 10.f * delta;
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
    }

private:
    bool _inputA = false, _inputB = false;
    std::unique_ptr<MFA::MeshRenderer> _pTankRenderer;
    std::unique_ptr<MFA::MeshRenderer> _pBulletRenderer;
};