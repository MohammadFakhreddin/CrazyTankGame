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
	cameraBufferTracker = std::make_shared<CameraBufferTracker>(cameraBuffer);

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

	PrepareInGameText();

	physics2D = std::make_unique<Physics2D>(pointRenderer, lineRenderer);
	

	InitMap();

	auto const tankModel = Importer::GLTF_Model(Path::Instance->Get("models/enemy_tank.glb"));

	playerTankRenderer = std::make_unique<MeshRenderer>(
		shadingPipeline,
		tankModel,
		errorTexture,
		true,
		glm::vec4{37.0f / 255.0f, 150.0f / 255.0f, 190.0f / 255.0f, 1.0f}
	);
	
	{// Player params
		playerTankParams = std::make_unique<Tank::Params>();
		playerTank = std::make_unique<Tank>(*playerTankRenderer, playerTankParams);
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
	gameCamera = std::make_unique<FollowCamera>(playerTank->Transform(),cameraBufferTracker);

	{// Debug camera
		debugCamera = std::make_unique<MFA::ArcballCamera>(glm::vec3{}, -Math::ForwardVec3);
		debugCamera->SetfovDeg(40.0f);
		debugCamera->SetLocalPosition(glm::vec3{0.0f, 90.0f, 0.0f});
		debugCamera->SetfarPlane(1000.0f);
		debugCamera->SetnearPlane(0.010f);
	}

	InitPathFinder();

	// TODO: We need a spawn position for the enemies
	enemyTankRenderer = std::make_unique<MeshRenderer>(
		shadingPipeline,
		tankModel,
		errorTexture,
		true,
		glm::vec4{252.0f / 255.0f, 69.0f / 255.0f, 3.0f / 255.0f, 1.0f}
	);

	// TODO: Write a player and enemy behaviour class
	{
		// TODO: Use spawn positions
		auto const node0Position = pathFinder->NodePosition(0);
		enemyTankParams = std::make_unique<Tank::Params>();
		enemyTanks.emplace_back(std::make_unique<Tank>(*enemyTankRenderer, enemyTankParams));
		auto & enemyTransform = enemyTanks.back()->Transform();
		enemyTransform.SetLocalScale(glm::vec3{0.25f, 0.25f, 0.25f});
		enemyTransform.SetLocalPosition(node0Position);
	}

}

//------------------------------------------------------------------------------------------------------

CrazyTankGameApp::~CrazyTankGameApp()
{
	physics2D.reset();
	lineRenderer.reset();
	linePipeline.reset();
	pointRenderer.reset();
	pointPipeline.reset();
	enemyTankRenderer.reset();
	playerTankRenderer.reset();
	bulletRenderer.reset();
	map.reset();
	errorTexture.reset();
	shadingPipeline.reset();
	defaultSampler.reset();
	cameraBufferTracker.reset();
	debugCamera.reset();
	gameCamera.reset();
	cameraBuffer.reset();
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

	UpdateBullets(deltaTimeSec);

	UpdatePlayer(deltaTimeSec);

	UpdateEnemies(deltaTimeSec);
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

	textData->vertexData->Update(recordState);

	displayRenderPass->Begin(recordState);
	
	// Rendering player tank
	playerTankRenderer->Render(recordState, {playerTank->MeshInstance()});
	
	{// Rendering enemy tank
		std::vector<MeshInstance *> instances{};
		for (auto & enemyTank : enemyTanks)
		{	
			instances.emplace_back(enemyTank->MeshInstance());
		}
		enemyTankRenderer->Render(recordState, instances);
	}

	{// Rendering bullets
		std::vector<glm::mat4> bulletTransforms{};
		for (auto & bullet : bullets)
		{
			bulletTransforms.emplace_back(bullet->Transform().GlobalTransform());
		}
		bulletRenderer->Render(recordState, bulletTransforms);
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

void CrazyTankGameApp::InitMap()
{
	// TODO: Map needs to generated randomly every time with a limited number of walls
	// TODO: Write a map generator
	// TODO: We need something for the spawn points and for the enemies
	std::vector<int> walls{
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,
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
		1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	};
	int rows = 20;
	int columns = 20;

	map = std::make_unique<Map>(40, 40, 20, 20, walls, shadingPipeline, errorTexture);

}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::InitPathFinder()
{
	auto const rows = map->GetRows();
	auto const columns = map->GetColumns();
	auto const & walls = map->GetWalls();

	pathFinder = std::make_unique<PathFinder>();
	
	std::string idMap = "";
	std::vector<PathFinder::NodeId> nodes{};
	for (int j = 0; j < rows; ++j)
	{
		for (int i = 0; i < columns; ++i)
		{
			if (walls[j * columns + i] != 1)
			{
				auto const position = map->CalcPosition(j, i);
				nodes.emplace_back(pathFinder->AddNode(position));
				idMap += "=" + std::to_string(nodes.back()) + "=";
			}
			else
			{
				nodes.emplace_back(PathFinder::InvalidNode);
				idMap += "=" + std::to_string(nodes.back()) + "=";
			}
		}
		idMap += "\n";
	}
	MFA_LOG_INFO("Id map is\n%s", idMap.c_str());

	for (int j = 0; j < rows; ++j)
	{
		for (int i = 0; i < columns; ++i)
		{
			auto myIdx = j * columns + i;
			if (walls[myIdx] != 1)
			{
				auto myID = nodes[myIdx]; 

				auto rightIdx = (j * columns) + i + 1;
				if (walls[rightIdx] != 1)
				{
					auto const addResult = pathFinder->AddEdge(myID, nodes[rightIdx]);
					MFA_ASSERT(addResult == true);
				}

				auto forwardIdx = ((j + 1) * columns) + i;
				if (walls[forwardIdx] != 1)
				{
					auto const addResult = pathFinder->AddEdge(myID, nodes[forwardIdx]);
					MFA_ASSERT(addResult == true);
				}

				auto forwardRightIdx = ((j + 1) * columns) + i + 1;
				if (walls[rightIdx] != 1 && walls[forwardIdx] != 1 && walls[forwardRightIdx] != 1)
				{
					auto const addResult = pathFinder->AddEdge(myID, nodes[forwardRightIdx]);
					MFA_ASSERT(addResult == true);
				}
			}
		}
	}

	pathFinder->CachePaths();
	
	// auto currentNode = 0;
	// auto targetNode = 40;

	// MFA_LOG_INFO("Trying to find a path from %d to %d", currentNode, targetNode);
	// std::string path = std::to_string(currentNode);
	// do
	// {
	// 	auto const [findResult, node] = pathFinder->FindNextNode(currentNode, targetNode);
	// 	MFA_ASSERT(findResult == true);
	// 	currentNode = node;
	// 	path += "/" + std::to_string(currentNode);
	// }
	// while (currentNode != targetNode);
	// MFA_LOG_INFO("The path is %s", path.c_str());
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::UpdateBullets(float deltaTimeSec)
{
	for (auto & bullet : bullets)
	{
		bullet->Update(deltaTimeSec);
	}

	for (int i = bullets.size() - 1; i >= 0; --i)
	{
		if (bullets[i]->IsAlive() == false)
		{
			bullets.erase(bullets.begin() + i);
		}
	}
}

//------------------------------------------------------------------------------------------------------

void CrazyTankGameApp::UpdatePlayer(float deltaTimeSec)
{
	// Player behaviour code, TODO: Move it to player behaviour
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

void CrazyTankGameApp::UpdateEnemies(float deltaTimeSec)
{
	auto const playerNode = pathFinder->FindNearestNode(playerTank->Transform().GlobalPosition());
	for (auto & enemyTank : enemyTanks)
	{
		auto const enemyNode = pathFinder->FindNearestNode(enemyTank->Transform().GlobalPosition());
		MFA_ASSERT(enemyNode >= 0);
		auto const [success, nextNode] = pathFinder->FindNextNode(enemyNode, playerNode);
		MFA_ASSERT(success == true && nextNode >= 0);

		auto const & currentPosition = enemyTank->Transform().GlobalPosition();
		auto const nextNodePosition = pathFinder->NodePosition(nextNode);
		auto const vector = nextNodePosition - currentPosition;
		auto const magnitude = glm::length(vector);
		if (magnitude > glm::epsilon<float>())
		{
			auto const direction = vector / magnitude;
			enemyTank->Move(glm::vec2 {direction.x, direction.z}, deltaTimeSec);
		}
	}

	for (int i = enemyTanks.size() - 1; i >= 0; --i)
	{
		if (enemyTanks[i]->IsAlive() == false)
		{
			enemyTanks.erase(enemyTanks.begin() + i);
		}
	}
}

//------------------------------------------------------------------------------------------------------
