#pragma once

#include "utils/MeshInstance.hpp"

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