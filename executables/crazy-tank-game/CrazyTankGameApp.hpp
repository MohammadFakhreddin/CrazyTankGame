#pragma once

#include "BufferTracker.hpp"
#include "BedrockPath.hpp"
#include "UI.hpp"
#include "pipeline/FlatShadingPipeline.hpp"
#include "camera/PerspectiveCamera.hpp"

#include <memory>

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
};
