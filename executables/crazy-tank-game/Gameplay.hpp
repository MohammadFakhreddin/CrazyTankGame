#pragma once

#include "utils/MeshInstance.hpp"

#include <list>

struct TankEntity {
    glm::vec2 flatPosition;
    float baseAngle, headAngle, scale;

    TankEntity(MFA::MeshRenderer const& meshRenderer, const glm::vec2& initPos = {}, float initBA = 0.f, float initHA = 0.f, float initScale = 0.1f) {
        _meshInstance = std::make_unique<MFA::MeshInstance>(meshRenderer);
        _headNode = _meshInstance->FindNode("Head");
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
    std::unique_ptr<MFA::MeshInstance> _meshInstance;
    MFA::Asset::GLTF::Node* _headNode;
};

struct GameLogic {
    static constexpr float PLAYER_SPEED = 10.0f;
    static constexpr float PLAYER_TURN_SPEED = glm::pi<float>();
    static constexpr float PLAYER_HEAD_TURN_SPEED = glm::pi<float>();
    TankEntity player;

    std::list<TankEntity> test_enemies;

    GameLogic(std::unique_ptr<MFA::MeshRenderer> pTankRenderer) : player(*pTankRenderer) {
        _pTankRenderer = std::move(pTankRenderer);
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
        float const inputMagnitude = glm::length(joystickInp);

        if (inputMagnitude > glm::epsilon<float>()) {
            player.baseAngle = fmodf(player.baseAngle + joystickInp.x * PLAYER_TURN_SPEED * delta, glm::two_pi<float>());
            player.flatPosition += player.BaseDir() * joystickInp.y * PLAYER_SPEED * delta;
        }

        if (inputA) {
            player.headAngle += delta * PLAYER_HEAD_TURN_SPEED;
        }

        if (inputB) {
            player.headAngle -= delta * PLAYER_HEAD_TURN_SPEED;
        }

        player.UpdateMI();

        for (TankEntity& e : test_enemies) {
            e.AimAt(player.flatPosition - e.flatPosition);
            e.UpdateMI();
        }

        _inputA = inputA;
        _inputB = inputB;
    }

    void Render(MFA::RT::CommandRecordState& recordState) {
        _pTankRenderer->Render(recordState, { player.GetMI() });
        for (const TankEntity& e : test_enemies) {
            _pTankRenderer->Render(recordState, { e.GetMI() });
        }
    }

private:
    bool _inputA = false, _inputB = false;
    std::unique_ptr<MFA::MeshRenderer> _pTankRenderer;
};