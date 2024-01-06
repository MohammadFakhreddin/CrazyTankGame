#include "CrazyTankGameApp.hpp"

#include "camera/ObserverCamera.hpp"
#include "camera/ArcballCamera.hpp"
#include "ImportTexture.hpp"
#include "BedrockMath.hpp"

#include <omp.h>

#include "Layers.hpp"

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

	if (SDL_JoystickOpen(0) != nullptr) SDL_JoystickEventState(SDL_ENABLE);

    device->SDL_EventSignal.Register([&](SDL_Event* event)->void
		{
			OnSDL_Event(event);
		}
	);

    auto arcBallCamera = std::make_unique<ArcballCamera>();
	arcBallCamera->SetmaxDistance(30.0f);
	camera = std::move(arcBallCamera);

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

	pointPipeline = std::make_shared<PointPipeline>(displayRenderPass, cameraBuffer, 10000);
	pointRenderer = std::make_shared<PointRenderer>(pointPipeline);

	linePipeline = std::make_shared<LinePipeline>(displayRenderPass, cameraBuffer, 10000);
	lineRenderer = std::make_shared<LineRenderer>(linePipeline);

	physics2D = std::make_unique<Physics2D>(pointRenderer, lineRenderer);

	{// Tank model
		auto tankCpuModel = Importer::GLTF_Model(Path::Instance->Get("models/test/tank_2.glb"));
		tankRenderer = std::make_unique<MeshRenderer>(
			shadingPipeline,
			tankCpuModel,
			errorTexture,
			true,
			glm::vec4 {0.0f, 0.25f, 0.0f, 1.0f}
		);
		playerInstance = std::make_unique<MeshInstance>(*tankRenderer);
		playerInstance->GetTransform().Setscale(glm::vec3{ 0.1f, 0.1f, 0.1f });
		tankHead = playerInstance->FindNode("Head");
		MFA_ASSERT(tankHead != nullptr);
	}

    {// Box collider
		tankCollider.clear();
		tankCollider.emplace_back(glm::vec4{ colliderDimension.x - colliderCenter.x, 0.0f, colliderDimension.y - colliderCenter.y, 1.0f });
		tankCollider.emplace_back(glm::vec4{ -(colliderDimension.x - colliderCenter.x), 0.0f, colliderDimension.y - colliderCenter.y, 1.0f });
		tankCollider.emplace_back(glm::vec4{ colliderDimension.x - colliderCenter.x, 0.0f, -(colliderDimension.y - colliderCenter.y), 1.0f });
		tankCollider.emplace_back(glm::vec4{ -(colliderDimension.x - colliderCenter.x), 0.0f, -(colliderDimension.y - colliderCenter.y), 1.0f });
    }

	{
		std::vector<int> walls{
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
 			1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
			1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
			1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
			1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		};
		map = std::make_unique<Map>(15.0f, 15.0f, 10, 10, walls.data(), shadingPipeline, errorTexture);
	}

}

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::~CrazyTankGameApp()
{
	lineRenderer.reset();
	linePipeline.reset();
	pointRenderer.reset();
	pointPipeline.reset();
	physics2D.reset();
	map.reset();
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

	physics2D->Update();

	{// Player matrix
		float const inputMagnitude = glm::length(inputAxis);

		if (inputMagnitude > glm::epsilon<float>())
		{
			auto& transformRef = playerInstance->GetTransform();
			auto transformCopy = playerInstance->GetTransform();

			float prevPlayerAngle = playerAngle;
			playerAngle = fmodf(playerAngle + inputAxis.x * playerAngularSpeed * deltaTimeSec, glm::two_pi<float>());
			glm::vec3 const playerDirection{ sinf(playerAngle), 0.f, cosf(playerAngle) };

			auto newPosition = transformCopy.Getposition() + playerDirection * inputAxis.y * playerSpeed * deltaTimeSec;
			auto newAngle = glm::angleAxis(playerAngle, glm::vec3{ 0.f, 1.f, 0.f });

			transformCopy.Setposition(newPosition);
			transformCopy.SetQuaternion(newAngle);

			bool hasCollision = false;
			Physics2D::HitInfo hitInfo{
				.hitTime = 1000.0f
			};

			for (auto & point : tankCollider)
			{
				auto const prevPoint = transformRef.GetMatrix() * point;
				auto const nextPoint = transformCopy.GetMatrix() * point;
				auto const vector = nextPoint - prevPoint;
				auto const length = glm::length(vector);
				if (length > 0.0f)
				{
					auto const direction = vector / length;
					Physics2D::HitInfo myHitInfo{};
					auto const hit = physics2D->Raycast(
						Layer::WallLayer,
						-1,
						glm::vec2{ prevPoint.x, prevPoint.z },
						glm::vec2{ direction.x, direction.z },
						length,
						myHitInfo
					);
					if (hit == true)
					{
						if (myHitInfo.hitTime < hitInfo.hitTime || hasCollision == false)
						{
							hitInfo = myHitInfo;
						}
						hasCollision = true;
					}
				}
			}

			if (hasCollision == false)
			{
				//if (hasCollision == true)
				//{
				//	newPosition = glm::mix(transformRef.Getposition(), newPosition, hitInfo.hitTime - 1e-5f);
				//	//newAngle = glm::slerp(transformRef.Getrotation().GetQuaternion(), newAngle, hitInfo.hitTime - 1e-5f);
				//	playerAngle = glm::mix(prevPlayerAngle, playerAngle, hitInfo.hitTime - 1e-5);
				//	newAngle = glm::angleAxis(playerAngle, glm::vec3{ 0.f, 1.f, 0.f });
				//}

				transformRef.Setposition(newPosition);
				transformRef.SetQuaternion(newAngle);
			}
			else
			{
				playerAngle = prevPlayerAngle;
			}
		}

		if (inputA == true)
		{
			tankHeadAngle += deltaTimeSec * tankHeadAngularSpeed;
			tankHead->transform.SetQuaternion(glm::angleAxis(tankHeadAngle, glm::vec3{ 0.f, 1.f, 0.f }));
		}
		if (inputB == true)
		{
			tankHeadAngle -= deltaTimeSec * tankHeadAngularSpeed;
			tankHead->transform.SetQuaternion(glm::angleAxis(tankHeadAngle, glm::vec3{ 0.f, 1.f, 0.f }));
		}
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Render(RT::CommandRecordState& recordState)
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

	tankRenderer->Render(recordState, { playerInstance.get() });

	if (renderMap == true)
	{
		map->Render(recordState);
	}

	if (renderPlayerCollider == true)
	{// Player collider
		for (auto const & point : tankCollider)
		{
			pointRenderer->Draw(recordState, playerInstance->GetTransform().GetMatrix() * point);
		}
	}

	if (renderPhysics == true)
	{
		physics2D->Render(recordState);
	}

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
	ImGui::Checkbox("DEBUG render physics", &renderPhysics);
	ImGui::Checkbox("DEBUG render map", &renderMap);
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
		auto const modifier = event->type == SDL_KEYDOWN ? 1.0f : -1.0f;
		
		if (event->key.keysym.sym == SDLK_UP)
		{
			inputAxis.y -= modifier;
		}
		else if (event->key.keysym.sym == SDLK_DOWN)
		{
			inputAxis.y += modifier;
		}
		else if (event->key.keysym.sym == SDLK_LEFT)
		{
			inputAxis.x -= modifier;
		}
		else if (event->key.keysym.sym == SDLK_RIGHT)
		{
			inputAxis.x += modifier;
		}

		inputAxis.x = std::clamp(inputAxis.x, -1.0f, 1.0f);
		inputAxis.y = std::clamp(inputAxis.y, -1.0f, 1.0f);

		if(event->key.keysym.sym == SDLK_a)
		{
			inputA = modifier > 0;
		}
		if (event->key.keysym.sym == SDLK_z)
		{
			inputB = modifier > 0;
		}
	}

	if (event->type == SDL_JOYAXISMOTION) {
		const auto process_axis = [](Sint16 joyAxisValue) -> float {
			constexpr Sint16 JOYSTICK_DEADZONE = 8000;
			return joyAxisValue < -JOYSTICK_DEADZONE ? -1.0 : joyAxisValue > JOYSTICK_DEADZONE ? 1.0 : 0.0;
			};
		if (event->jaxis.axis == 0) {
			inputAxis.x = process_axis(event->jaxis.value);
		}
		else if (event->jaxis.axis == 1) {
			inputAxis.y = process_axis(event->jaxis.value);
		}
	}

	if (event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP) {
		const bool is_button_released = event->type == SDL_JOYBUTTONUP;
		if (event->jbutton.button == 1 /* BUTTON A */) {
			if (is_button_released) {
				MFA_LOG_DEBUG("A released");
				inputA = false;
			}
			else {
				MFA_LOG_DEBUG("A pressed");
				inputA = true;
			}
		}
		else if (event->jbutton.button == 2 /* BUTTON B */) {
			if (is_button_released) {
				MFA_LOG_DEBUG("B released");
				inputB = false;
			}
			else {
				MFA_LOG_DEBUG("B pressed");
				inputB = true;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::OnResize()
{
}

//------------------------------------------------------------------------------------------------------
