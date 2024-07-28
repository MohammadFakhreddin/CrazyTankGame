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
#include "LogicalDevice.hpp"
#include "Tank.hpp"
#include "Time.hpp"
#include "FollowCamera.hpp"
#include "camera/ArcballCamera.hpp"

#include <memory>
#include <thread>

#include "utils/ConsolasFontRenderer.hpp"

class CrazyTankGameApp
{
public:

    using ShadingPipeline = MFA::FlatShadingPipeline;
    
    explicit CrazyTankGameApp();

    ~CrazyTankGameApp();

    void Run();

private:

    void Update(float deltaTimeSec);

    void Render(MFA::RT::CommandRecordState& recordState);

    void DebugUI(float deltaTimeSec);

    void OnSDL_Event(SDL_Event* event);

    void OnResize();

    void PrepareInGameText();

    void UpdateInGameText(float deltaTimeSec);

    // Render parameters
	std::unique_ptr<MFA::Path> path{};
	std::unique_ptr<MFA::LogicalDevice> device{};
	std::shared_ptr<MFA::UI> ui{};
    std::unique_ptr<MFA::Time> time{};
	std::shared_ptr<MFA::SwapChainRenderResource> swapChainResource{};
	std::shared_ptr<MFA::DepthRenderResource> depthResource{};
    // TODO: We might have to disable this
	std::shared_ptr<MFA::MSSAA_RenderResource> msaaResource{};
	std::shared_ptr<MFA::DisplayRenderPass> displayRenderPass{};

    // TODO: We can have a fixed camera and a debug camera
    std::unique_ptr<FollowCamera> gameCamera{};
    std::unique_ptr<MFA::ArcballCamera> debugCamera{};
	
    std::shared_ptr<MFA::RT::BufferGroup> cameraBuffer{};
	std::shared_ptr<MFA::HostVisibleBufferTracker> cameraBufferTracker{};

    std::shared_ptr<MFA::RT::BufferGroup> lightSourceBuffer{};
	std::shared_ptr<MFA::HostVisibleBufferTracker> lightSourceBufferTracker{};

    std::shared_ptr<ShadingPipeline> shadingPipeline{};

    std::shared_ptr<MFA::RT::SamplerGroup> defaultSampler{};
    std::shared_ptr<MFA::RT::GpuTexture> errorTexture{};

    std::shared_ptr<MFA::PointPipeline> pointPipeline{};
    std::shared_ptr<MFA::PointRenderer> pointRenderer{};

    std::shared_ptr<MFA::LinePipeline> linePipeline{};
    std::shared_ptr<MFA::LineRenderer> lineRenderer{};


    glm::vec2 inputAxis{};
    bool inputA{};
    bool inputB{};

    std::unique_ptr<Physics2D> physics2D{};

    bool renderPhysics = false;
    bool renderMap = true;
    bool renderPlayer = true;
    bool useDebugCamera = false;

    std::unique_ptr<Map> map{};
    std::unique_ptr<MFA::MeshRenderer> tankRenderer{};
    std::shared_ptr<Tank::Params> playerTankParams{};
	std::unique_ptr<Tank> playerTank{};

    // TODO: Some kind of memory pool is needed
    std::unique_ptr<MFA::MeshRenderer> bulletRenderer{};
    std::shared_ptr<Bullet::Params> bulletParams{};
    std::vector<std::unique_ptr<Bullet>> bullets{};

    float passedTime = 0.0f;

    std::unique_ptr<MFA::ConsolasFontRenderer> fontRenderer{};
    std::shared_ptr<MFA::RT::SamplerGroup> fontSampler{};
    std::unique_ptr<MFA::ConsolasFontRenderer::TextData> textData{};

};
