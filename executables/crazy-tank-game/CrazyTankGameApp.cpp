#include "CrazyTankGameApp.hpp"

#include "ImportTexture.hpp"
#include "BedrockMath.hpp"
#include "Layers.hpp"
#include "Tank.hpp"

#include <omp.h>

using namespace MFA;

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::CrazyTankGameApp()
{
	// TODO: Move to multiple functions
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

	swapChainResource = std::make_shared<SwapChainRenderResource>();
	depthResource = std::make_shared<DepthRenderResource>();
	msaaResource = std::make_shared<MSSAA_RenderResource>();
	displayRenderPass = std::make_shared<DisplayRenderPass>(
		swapChainResource,
		depthResource,
		msaaResource
	);

	ui = std::make_shared<UI>(displayRenderPass);
    ui->UpdateSignal.Register([this]()->void { DebugUI(Time::DeltaTimeSec()); });

	device->ResizeEventSignal2.Register([this]()->void {
		OnResize();
	});

	defaultSampler = RB::CreateSampler(LogicalDevice::Instance->GetVkDevice(), {});

	cameraBuffer = RB::CreateHostVisibleUniformBuffer(
		device->GetVkDevice(),
		device->GetPhysicalDevice(),
		sizeof(ShadingPipeline::ViewProjection),
		device->GetMaxFramePerFlight()
	);

	MFA_ASSERT(cameraBuffer != nullptr);
	cameraBufferTracker = std::make_shared<HostVisibleBufferTracker>(cameraBuffer);

	lightSourceBuffer = RB::CreateHostVisibleUniformBuffer(
		device->GetVkDevice(),
		device->GetPhysicalDevice(),
		sizeof(ShadingPipeline::LightSource),
		device->GetMaxFramePerFlight()
	);
	MFA_ASSERT(lightSourceBuffer != nullptr);
	// TODO: Move this to the proper place
	ShadingPipeline::LightSource lightSourceData{};
	lightSourceData.dir = {-1, 0, 0};
	lightSourceData.color = {1, 1, 1};
	lightSourceBufferTracker = std::make_shared<HostVisibleBufferTracker>(lightSourceBuffer, Alias{lightSourceData});

	shadingPipeline = std::make_shared<ShadingPipeline>(
        displayRenderPass,
        cameraBuffer,
		defaultSampler,
		lightSourceBuffer,
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

	PrepareInGameText();

	physics2D = std::make_unique<Physics2D>(pointRenderer, lineRenderer);
	// TODO: Map needs to generated randomly every time with a limited number of walls
	// TODO: Write a map generator
	// TODO: We need something for the spawn points
	std::vector<int> walls{
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	};

	map = std::make_unique<Map>(40, 40, 20, 20, walls, shadingPipeline, errorTexture);

	tankRenderer = std::make_unique<MeshRenderer>(
		shadingPipeline,
		Importer::GLTF_Model(Path::Instance->Get("models/enemy_tank.glb")),
		errorTexture,
		true,
		glm::vec4{37.0f / 255.0f, 150.0f / 255.0f, 190.0f / 255.0f, 1.0f}
	);

	{// Player params
		playerTankParams = std::make_unique<Tank::Params>();
		playerTank = std::make_unique<Tank>(*tankRenderer, playerTankParams);
		playerTank->Transform().SetLocalScale(glm::vec3{0.25f, 0.25f, 0.25f});
	}

	{
		bulletParams = std::make_shared<Bullet::Params>();
		bulletRenderer = std::make_unique<MFA::MeshRenderer>(
			shadingPipeline,
			MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/test/cube.glb")),
			errorTexture,
			true,
			glm::vec4{ 0.0f, 0.25f, 0.0f, 1.0f }
		);
	}

	MFA_ASSERT(playerTank != nullptr);
	MFA_ASSERT(cameraBufferTracker != nullptr);
	gameCamera = std::make_unique<FollowCamera>(playerTank->Transform(), cameraBufferTracker);

	{// Debug camera
		debugCamera = std::make_unique<MFA::ArcballCamera>(glm::vec3{}, -Math::ForwardVec3);
		debugCamera->SetfovDeg(40.0f);
		debugCamera->SetLocalPosition(glm::vec3{0.0f, 90.0f, 0.0f});
		debugCamera->SetfarPlane(1000.0f);
		debugCamera->SetnearPlane(0.010f);
	}

	// TODO: We need a spawn position for the enemies
}

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::~CrazyTankGameApp()
{
	physics2D.reset();
	lineRenderer.reset();
	linePipeline.reset();
	pointRenderer.reset();
	pointPipeline.reset();
	tankRenderer.reset();
	bulletRenderer.reset();
	map.reset();
	errorTexture.reset();
	shadingPipeline.reset();
	defaultSampler.reset();
	cameraBufferTracker.reset();
	lightSourceBufferTracker.reset();
	debugCamera.reset();
	gameCamera.reset();
	cameraBuffer.reset();
	lightSourceBuffer.reset();
	ui.reset();
	textData.reset();
	fontSampler.reset();
	fontRenderer.reset();
	displayRenderPass.reset();
	msaaResource.reset();
	depthResource.reset();
	swapChainResource.reset();
	device.reset();
	path.reset();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Run()
{
	SDL_GL_SetSwapInterval(0);
	SDL_Event e;
	
	time = Time::Instantiate(120, 30);
	
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
		Update(Time::DeltaTimeSec());

		auto recordState = device->AcquireRecordState(swapChainResource->GetSwapChainImages().swapChain);
		if (recordState.isValid == true)
		{
			Render(recordState);
		}

		time->Update();
	}

	time.reset();

	device->DeviceWaitIdle();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::Update(float deltaTimeSec)
{
	if (useDebugCamera == true)
	{
		debugCamera->Update(deltaTimeSec);
		if (debugCamera->IsDirty())
		{
			cameraBufferTracker->SetData(Alias{debugCamera->ViewProjection()});
		}
	}
	else
	{
		gameCamera->Update(deltaTimeSec);
	}

	ui->Update();

	UpdateInGameText(deltaTimeSec);

	physics2D->Update();

	for (auto & bullet : bullets)
	{
		bullet->Update(Time::DeltaTimeSec());
	}

	for (int i = bullets.size() - 1; i >= 0; --i)
	{
		if (bullets[i]->IsAlive() == false)
		{
			bullets.erase(bullets.begin() + i);
		}
	}

	{// Player movement
		glm::vec2 direction = inputAxis;
		auto const magnitude = glm::length(inputAxis);
		if (magnitude > glm::epsilon<float>())
		{
			direction /= magnitude;
			playerTank->Move(direction, deltaTimeSec);
		}
	}

	// TODO: Check if tanks are alive or not!

	// Player shoot
	if (inputA == true)
	{
		auto bullet = playerTank->Shoot(bulletParams);
		if (bullet != nullptr)
		{
			bullets.emplace_back(std::move(bullet));
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

	lightSourceBufferTracker->Update(recordState);

	textData->vertexData->Update(recordState);

	displayRenderPass->Begin(recordState);

	if (renderPlayer == true)
	{
		tankRenderer->Render(recordState,{playerTank->MeshInstance()});
	}

	for (auto & bullet : bullets)
	{
		bulletRenderer->Render(recordState, {bullet->Transform().GlobalTransform()});
	}

	if (renderMap == true)
	{
		map->Render(recordState);
	}

	if (renderPhysics == true)
	{
		Physics2D::Instance->Render(recordState);
	}

	fontRenderer->Draw(recordState,*textData);

	ui->Render(recordState, Time::DeltaTimeSec());

	displayRenderPass->End(recordState);
	device->EndCommandBuffer(recordState);

	device->SubmitQueues(recordState);
	device->Present(recordState, swapChainResource->GetSwapChainImages().swapChain);
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::DebugUI(float deltaTimeSec)
{
	ui->BeginWindow("Window");
	ImGui::Text("Framerate: %f", 1.0f / deltaTimeSec);
	ImGui::Checkbox("DEBUG render physics", &renderPhysics);
	ImGui::Checkbox("DEBUG render map", &renderMap);
	ImGui::Checkbox("DEBUG render player",&renderPlayer);
	if (ImGui::TreeNode("Debug player params"))
	{
		ImGui::InputFloat("Move speed", &playerTankParams->moveSpeed);
		ImGui::InputFloat("Rotation speed", &playerTankParams->rotationSpeed);
		ImGui::InputFloat2("Half collider extent", reinterpret_cast<float *>(&playerTankParams->halfColliderExtent));
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Debug bullet params"))
	{
		ImGui::InputFloat("Move speed", &bulletParams->moveSpeed);
		ImGui::InputFloat("Radius", &bulletParams->radius);
		ImGui::InputFloat("Friendly fire delay", &bulletParams->friendlyFireDelay);
		ImGui::InputFloat("Life time", &bulletParams->lifeTime);
		ImGui::TreePop();
	}
	if (ImGui::Checkbox("Use debug camera", &useDebugCamera))
	{
		if (useDebugCamera == true)
		{
			cameraBufferTracker->SetData(Alias{debugCamera->ViewProjection()});
		}
		else
		{
			gameCamera->NotifyEnabled();
		}
	}
	if (ImGui::TreeNode("Debug camera params"))
	{
		debugCamera->Debug_UI();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Game Camera params"))
	{
		gameCamera->DebugUI();
		ImGui::TreePop();
	}
	ui->EndWindow();
}

//------------------------------------------------------------------------------------------------------

static float ProcessJoystickAxis(Sint16 joyAxisValue)
{
	constexpr Sint16 JOYSTICK_DEADZONE = 8000;
	return joyAxisValue < -JOYSTICK_DEADZONE ? -1.0 : joyAxisValue > JOYSTICK_DEADZONE ? 1.0 : 0.0;
}

//------------------------------------------------------------------------------------------------------
// TODO: Input system for this game
void CrazyTankGameApp::OnSDL_Event(SDL_Event* event)
{
	if (UI::Instance != nullptr && UI::Instance->HasFocus() == true)
	{
		return;
	}

	{// Keyboard
		if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
		{
			auto const modifier = event->type == SDL_KEYDOWN ? 1.0f : -1.0f;
			if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_RIGHT)
			{
				if (event->key.keysym.sym == SDLK_LEFT)
				{
					inputAxis.x -= modifier;
				}
				else if (event->key.keysym.sym == SDLK_RIGHT)
				{
					inputAxis.x += modifier;
				}
				inputAxis.x = std::clamp(inputAxis.x, -1.0f, 1.0f);
			}
			else if (event->key.keysym.sym == SDLK_UP || event->key.keysym.sym == SDLK_DOWN)
			{
				if (event->key.keysym.sym == SDLK_UP)
				{
					inputAxis.y += modifier;
				}
				else if (event->key.keysym.sym == SDLK_DOWN)
				{
					inputAxis.y -= modifier;
				}
				inputAxis.y = std::clamp(inputAxis.y, -1.0f, 1.0f);
			}
			else if(event->key.keysym.sym == SDLK_z)
			{
				inputA = modifier > 0;
			}
			else if (event->key.keysym.sym == SDLK_x)
			{
				inputB = modifier > 0;
			}
                        else if (event->key.keysym.sym == SDLK_F5) {
                            shadingPipeline->reload();
                        }

		}
	}

	{// Joystick
		if (event->type == SDL_JOYAXISMOTION) 
		{
			if (event->jaxis.axis == 0) 
			{
				inputAxis.x = ProcessJoystickAxis(event->jaxis.value);
			}
			else if (event->jaxis.axis == 1) 
			{
				inputAxis.y = ProcessJoystickAxis(event->jaxis.value);
			}
		}

		if (event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP) 
		{
			auto const modifier = (event->type == SDL_JOYBUTTONDOWN) ? 1.0f : -1.0f;
			if (event->jbutton.button == 1 /* BUTTON A */) 
			{
				inputA = modifier > 0.0f;
			}
			else if (event->jbutton.button == 2 /* BUTTON B */) 
			{
				inputB = modifier > 0.0f;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::OnResize()
{
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::PrepareInGameText()
{
	RB::CreateSamplerParams params{};
	params.magFilter = VK_FILTER_LINEAR;
	params.minFilter = VK_FILTER_LINEAR;
	params.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	params.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	params.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	params.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	params.mipLodBias = 0.0f;
	params.compareOp = VK_COMPARE_OP_NEVER;
	params.minLod = 0.0f;
	params.maxLod = 1.0f;
	params.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	fontSampler = RB::CreateSampler(
		device->GetVkDevice(),
		params
	);
	MFA_ASSERT(fontSampler != nullptr);

	auto pipeline = std::make_shared<TextOverlayPipeline>(displayRenderPass, fontSampler);
	fontRenderer = std::make_unique<ConsolasFontRenderer>(pipeline);
	textData = fontRenderer->AllocateTextData();
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::UpdateInGameText(float deltaTimeSec)
{
	passedTime += deltaTimeSec;
	fontRenderer->ResetText(*textData);
	fontRenderer->AddText(
		*textData,
		"Passed time : " + std::to_string(passedTime),
		0.0f,
		0.0f,
		ConsolasFontRenderer::AddTextParams{.textAlign = ConsolasFontRenderer::TextAlign::Left}
	);
}

//------------------------------------------------------------------------------------------------------
