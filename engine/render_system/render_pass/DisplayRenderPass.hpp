#pragma once

#include "render_resource/DepthRenderResource.hpp"
#include "RenderPass.hpp"
#include "render_resource/MSAA_RenderResource.hpp"
#include "render_resource/SwapChainRenderResource.hpp"

#include <memory>

namespace MFA
{
	class DisplayRenderPass final : public RenderPass
    {
    public:

        explicit DisplayRenderPass(
            std::shared_ptr<SwapChainRenderResource> swapChain,
            std::shared_ptr<DepthRenderResource> depth,
            std::shared_ptr<MSSAA_RenderResource> msaa
        );

        ~DisplayRenderPass() override;

        DisplayRenderPass(DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass(DisplayRenderPass &&) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass const &) noexcept = delete;
        DisplayRenderPass & operator = (DisplayRenderPass &&) noexcept = delete;

        [[nodiscard]]
        VkRenderPass GetVkRenderPass() override;
        
        void Begin(RT::CommandRecordState & recordState, std::optional<std::array<float, 4>> backgroundColor = {});

        void End(RT::CommandRecordState & recordState);
        
        void NotifyDepthImageLayoutIsSet();
        
    private:


        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(RT::CommandRecordState const & recordState) const;

        [[nodiscard]]
        VkFramebuffer GetFrameBuffer(uint32_t imageIndex) const;

        void CreateFrameBuffers();

        void CreateRenderPass();
        
        void CreatePresentToDrawBarrier();

        void ClearDepthBufferIfNeeded(RT::CommandRecordState const & recordState);
        
        void UsePresentToDrawBarrier(RT::CommandRecordState const & recordState);

        void OnResize() override;

        std::shared_ptr<RT::RenderPass> mRenderPass{};
        
        VkImageMemoryBarrier mPresentToDrawBarrier {};

        bool mIsDepthImageUndefined = true;

        std::shared_ptr<SwapChainRenderResource> mSwapChain{};
        std::shared_ptr<DepthRenderResource> mDepth {};
        std::shared_ptr<MSSAA_RenderResource> mMSAA {};

        std::vector<std::shared_ptr<RT::FrameBuffer>> mFrameBuffers{};

    };

}
