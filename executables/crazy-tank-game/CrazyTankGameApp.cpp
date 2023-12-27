#include "CrazyTankGameApp.hpp"

#include "camera/ObserverCamera.hpp"
#include "camera/ArcballCamera.hpp"
#include "ImportTexture.hpp"
#include "BedrockMath.hpp"

#include <omp.h>

using namespace MFA;

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::CrazyTankGameApp()
{
    MFA_LOG_DEBUG("Loading...");
    omp_set_num_threads(static_cast<int>(static_cast<float>(std::thread::hardware_concurrency()) * 0.8f));
	MFA_LOG_INFO("Number of available workers are: %d", omp_get_max_threads());

    path = Path::Instantiate();

    LogicalDevice::InitParams params
	{
		.windowWidth = 800,
		.windowHeight = 800,
		.resizable = true,
		.fullScreen = false,
		.applicationName = "CrazyTankGame"
	};

    device = LogicalDevice::Instantiate(params);
	assert(device->IsValid() == true);

    device->SDL_EventSignal.Register([&](SDL_Event* event)->void
		{
			OnSDL_Event(event);
		}
	);

    camera = std::make_unique<ArcballCamera>();

	swapChainResource = std::make_shared<SwapChainRenderResource>();
	depthResource = std::make_shared<DepthRenderResource>();
	msaaResource = std::make_shared<MSSAA_RenderResource>();
	displayRenderPass = std::make_shared<DisplayRenderPass>(
		swapChainResource,
		depthResource,
		msaaResource
	);

	ui = std::make_shared<UI>(displayRenderPass);
    ui->UpdateSignal.Register([this]()->void { OnUI(_deltaTimeSec); });

	cameraBuffer = RB::CreateHostVisibleUniformBuffer(
		device->GetVkDevice(),
		device->GetPhysicalDevice(),
		sizeof(ShadingPipeline::ViewProjection),
		device->GetMaxFramePerFlight()
	);

	camera->Setposition({ 0.0f, -1.0f, 15.0f });

	cameraBufferTracker = std::make_shared<CameraBufferTracker>(
		cameraBuffer,
		ShadingPipeline::ViewProjection{
			.matrix = camera->GetViewProjection()
		}
	);

	device->ResizeEventSignal2.Register([this]()->void {
		OnResize();
	});

	defaultSampler = RB::CreateSampler(LogicalDevice::Instance->GetVkDevice(), {});

    shadingPipeline = std::make_shared<ShadingPipeline>(
        displayRenderPass,
        cameraBuffer,
		defaultSampler,
        ShadingPipeline::Params{
			.cullModeFlags = VK_CULL_MODE_BACK_BIT,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		}
    );

    {// Error texture
		auto const* device = LogicalDevice::Instance;

		auto const commandBuffer = RB::BeginSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool()
		);

		auto const cpuTexture = Importer::ErrorTexture();

		auto [gpuTexture, stagingBuffer] = RB::CreateTexture(
			*cpuTexture,
			LogicalDevice::Instance->GetVkDevice(),
			LogicalDevice::Instance->GetPhysicalDevice(),
			commandBuffer
		);

		RB::EndAndSubmitSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool(),
			device->GetGraphicQueue(),
			commandBuffer
		);

		errorTexture = gpuTexture;
    }

	{// Tank model
		auto tankCpuModel = Importer::GLTF_Model(Path::Instance->Get("models/test/tank_1.glb"));
		tankRenderer = std::make_unique<MeshRenderer>(
			shadingPipeline,
			tankCpuModel,
			errorTexture
		);
	}
}

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::~CrazyTankGameApp()
{
	tankRenderer.reset();
	errorTexture.reset();
	shadingPipeline.reset();
	defaultSampler.reset();
	cameraBufferTracker.reset();
	camera.reset();
	cameraBuffer.reset();
	ui.reset();
	displayRenderPass.reset();
	msaaResource.reset();
	depthResource.reset();
	swapChainResource.reset();
	camera.reset();
	device.reset();
	path.reset();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Run()
{
	const uint32_t MinDeltaTimeMs = 1000 / 120;

	SDL_GL_SetSwapInterval(0);
	SDL_Event e;
	_deltaTimeMs = MinDeltaTimeMs;
	_deltaTimeSec = static_cast<float>(MinDeltaTimeMs) / 1000.0f;
	uint32_t startTime = SDL_GetTicks();

	bool shouldQuit = false;

	while (shouldQuit == false)
	{
		//Handle events
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				shouldQuit = true;
			}
		}

		device->Update();

		Update(_deltaTimeSec);

		auto recordState = device->AcquireRecordState(swapChainResource->GetSwapChainImages().swapChain);
		if (recordState.isValid == true)
		{
			Render(recordState);
		}

		_deltaTimeMs = SDL_GetTicks() - startTime;
		if (MinDeltaTimeMs > _deltaTimeMs)
		{
			SDL_Delay(MinDeltaTimeMs - _deltaTimeMs);
		}

		_deltaTimeMs = SDL_GetTicks() - startTime;
		_deltaTimeSec = static_cast<float>(_deltaTimeMs) / 1000.0f;
		startTime = SDL_GetTicks();
	}

	device->DeviceWaitIdle();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Update(float deltaTimeSec)
{
	camera->Update(_deltaTimeSec);
	if (camera->IsDirty())
	{
		cameraBufferTracker->SetData(
			ShadingPipeline::ViewProjection{
				.matrix = camera->GetViewProjection()
			}
		);
	}

	ui->Update();

	{// Player matrix
		auto const inputMagnitude = glm::length(inputAxis);

		if (inputMagnitude > glm::epsilon<float>())
		{
			auto inputVector = inputAxis / inputMagnitude;
			playerPosition += inputVector * playerSpeed * deltaTimeSec;
		}

		auto const translateMatrix = Math::Translate(playerPosition);
		auto const rotationMatrix = glm::toMat4(playerRotation);
		auto const scaleMatrix = Math::Scale(playerScale);
		playerMatrix = translateMatrix * rotationMatrix * scaleMatrix;
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Render(MFA::RT::CommandRecordState& recordState)
{
	device->BeginCommandBuffer(
		recordState,
		RT::CommandBufferType::Compute
	);
	device->EndCommandBuffer(recordState);

	device->BeginCommandBuffer(
		recordState,
		RT::CommandBufferType::Graphic
	);

	cameraBufferTracker->Update(recordState);

	displayRenderPass->Begin(recordState);

	tankRenderer->Render(recordState, {playerMatrix});

	ui->Render(recordState, _deltaTimeSec);

	displayRenderPass->End(recordState);
	device->EndCommandBuffer(recordState);

	device->SubmitQueues(recordState);
	device->Present(recordState, swapChainResource->GetSwapChainImages().swapChain);
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::OnUI(float deltaTimeSec)
{
	ui->BeginWindow("Window");
	ImGui::Text("Framerate: %f", 1.0f / deltaTimeSec);
	ui->EndWindow();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::OnSDL_Event(SDL_Event* event)
{
	if (UI::Instance != nullptr && UI::Instance->HasFocus() == true)
	{
		return;
	}
	
	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
	{
		auto modifier = event->type == SDL_KEYDOWN ? 1.0f : -1.0f;
		
		if (event->key.keysym.sym == SDLK_UP)
		{
			inputAxis.z += -1.0f * modifier;
		}
		else if (event->key.keysym.sym == SDLK_DOWN)
		{
			inputAxis.z += +1.0f * modifier;
		}
		else if (event->key.keysym.sym == SDLK_RIGHT)
		{
			inputAxis.x += -1.0f * modifier;
		}
		else if (event->key.keysym.sym == SDLK_LEFT)
		{
			inputAxis.x += 1.0f * modifier;
		}

		inputAxis.x = std::clamp(inputAxis.x, -1.0f, 1.0f);
		inputAxis.z = std::clamp(inputAxis.z, -1.0f, 1.0f);
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::OnResize()
{
}

//------------------------------------------------------------------------------------------------------
