#pragma once

#include "BufferTracker.hpp"
#include "BedrockPath.hpp"
#include "UI.hpp"
#include "pipeline/FlatShadingPipeline.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "utils/MeshRenderer.hpp"
#include "utils/MeshInstance.hpp"
#include "Map.hpp"
#include "pipeline/PointPipeline.hpp"
#include "utils/LineRenderer.hpp"
#include "utils/PointRenderer.hpp"
#include "Physics2D.hpp"

#include <memory>
#include <thread>

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

    std::shared_ptr<MFA::PointPipeline> pointPipeline{};
    std::shared_ptr<MFA::PointRenderer> pointRenderer{};

    std::shared_ptr<MFA::LinePipeline> linePipeline{};
    std::shared_ptr<MFA::LineRenderer> lineRenderer{};

    // Temporary
    std::unique_ptr<MFA::MeshRenderer> tankRenderer{};


    glm::vec2 inputAxis{};
    bool inputA{};
    bool inputB{};
    
    float playerSpeed = 10.0f;
    
	float playerAngularSpeed = glm::pi<float>();
    float tankHeadAngularSpeed = glm::pi<float>();

    float playerAngle = 0.f;
    float tankHeadAngle = 0.f;

    std::unique_ptr<MFA::MeshInstance> playerInstance{};

    using Node = MFA::Asset::GLTF::Node;
    Node * tankHead{};

    glm::vec2 colliderDimension{5.0f, 6.0f};
    glm::vec2 colliderCenter{0.0f, 0.0f };
    Physics2D::EntityID playerColliderId{};

    std::vector<glm::vec4> tankCollider{};

    std::unique_ptr<Map> map{};

    std::unique_ptr<Physics2D> physics2D{};

    bool renderPhysics = false;
    bool renderMap = true;
    bool renderPlayerCollider = false;
};
