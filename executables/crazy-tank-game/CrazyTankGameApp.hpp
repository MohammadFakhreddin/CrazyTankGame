#pragma once

#include "BufferTracker.hpp"
#include "BedrockPath.hpp"
#include "UI.hpp"
#include "pipeline/FlatShadingPipeline.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"
#include "Map.hpp"

#include <memory>
#include <thread>

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

class CrazyTankGameApp
{
public:

    using ShadingPipeline = MFA::FlatShadingPipeline;
    using CameraBufferTracker = MFA::HostVisibleBufferTracker<ShadingPipeline::ViewProjection>;
    
    explicit CrazyTankGameApp();

    ~CrazyTankGameApp();

    void Run();

private:

    void Update(float deltaTimeSec);

    void Render(MFA::RT::CommandRecordState& recordState);

    void OnUI(float deltaTimeSec);

    void OnSDL_Event(SDL_Event* event);

    void OnResize();

    uint32_t _deltaTimeMs{};
    float _deltaTimeSec{};

    // Render parameters
	std::shared_ptr<MFA::Path> path{};
	std::shared_ptr<MFA::LogicalDevice> device{};
	std::shared_ptr<MFA::UI> ui{};
	std::shared_ptr<MFA::SwapChainRenderResource> swapChainResource{};
	std::shared_ptr<MFA::DepthRenderResource> depthResource{};
    // TODO: We might have to disable this
	std::shared_ptr<MFA::MSSAA_RenderResource> msaaResource{};
	std::shared_ptr<MFA::DisplayRenderPass> displayRenderPass{};

    // TODO: We can have a fixed camera and a debug camera
    std::unique_ptr<MFA::PerspectiveCamera> camera{};
	std::shared_ptr<MFA::RT::BufferGroup> cameraBuffer{};
	std::shared_ptr<CameraBufferTracker> cameraBufferTracker{};

    std::shared_ptr<ShadingPipeline> shadingPipeline{};

    std::shared_ptr<MFA::RT::SamplerGroup> defaultSampler{};
    std::shared_ptr<MFA::RT::GpuTexture> errorTexture{};

    // Temporary
    std::unique_ptr<MFA::MeshRenderer> tankRenderer{};

    glm::vec2 inputAxis{};
    bool inputA{};
    bool inputB{};
    
    float playerSpeed = 10.0f;
    
	float playerAngularSpeed = glm::pi<float>();
    float tankHeadAngularSpeed = glm::pi<float>();

    std::unique_ptr<TankEntity> playerTank;
    std::unique_ptr<TankEntity> enemyTank;

    std::unique_ptr<Map> map{};
};
