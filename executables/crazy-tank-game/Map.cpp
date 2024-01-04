#include "Map.hpp"

#include "BedrockPath.hpp"
#include "Layers.hpp"
#include "Physics2D.hpp"

//-----------------------------------------------------------------------

Map::Map(
    float mapWidth, float mapHeight,
    int rows, int columns, int * walls,
    std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
    std::shared_ptr<MFA::RT::GpuTexture> errorTexture
)
{
    auto cubeCpuModel = MFA::Importer::GLTF_Model(MFA::Path::Instance->Get("models/test/cube.glb"));
    _groundRenderer = std::make_unique<MFA::MeshRenderer>(
        pipeline, 
        cubeCpuModel, 
        errorTexture, 
        true, 
        glm::vec4{ 1.0f, 1.0f, 1.0f, 0.5f }
    );
    
    {
        _groundInstance = MFA::MeshInstance(*_groundRenderer);
        auto& transform = _groundInstance.GetTransform();
        transform.Setscale(glm::vec3{ mapWidth * 0.5f, 0.1f, mapHeight * 0.5f });
        transform.Setposition(glm::vec3{ 0.0f, -0.3f, 0.0f });
    }

    _wallRenderer = std::make_unique<MFA::MeshRenderer>(
        std::move(pipeline),
        cubeCpuModel,
        std::move(errorTexture),
        true,
        glm::vec4{ 0.0f, 0.0f, 0.5f, 1.0f }
    );

    {
        float const wallWidth = mapWidth / static_cast<float>(rows);
        float const wallHeight = mapHeight / static_cast<float>(columns);
        float const halfWallWidth = wallWidth * 0.5f;
        float const halfWallHeight = wallHeight * 0.5f;

        float startX = -mapWidth * 0.5f + halfWallWidth;
        float startY = -mapHeight * 0.5f + halfWallHeight;
        for (int x = 0; x < rows; ++x)
        {
	        for (int y = 0; y < columns; ++y)
	        {
		        if (walls[x * columns + y] > 0)
		        {
                    _wallInstances.emplace_back(*_wallRenderer);
                    auto & wallInstance = _wallInstances.back();
                    auto & transform = wallInstance.GetTransform();
                    transform.Setscale(glm::vec3{ halfWallWidth, 0.5f, halfWallHeight });
                    transform.Setposition(glm::vec3{ static_cast<float>(x) * wallWidth + startX, 0.3f, static_cast<float>(y) * wallHeight + startY });
                    auto colliderId = Physics2D::Instance->Register(
                        Physics2D::Type::Box, 
                        Layer::WallLayer, 
                        true, 
                        nullptr
                    );
                    auto const v0 = transform.GetMatrix() * glm::vec4{ -1.0f, 0.0f, -1.0f, 1.0f };
                    auto const v1 = transform.GetMatrix() * glm::vec4{ -1.0f, 0.0f, 1.0f, 1.0f };
                    auto const v2 = transform.GetMatrix() * glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f };
                    auto const v3 = transform.GetMatrix() * glm::vec4{ 1.0f, 0.0f, -1.0f, 1.0f };
                    Physics2D::Instance->MoveBox(
                        colliderId, 
                        glm::vec2{ v0.x, v0.z }, 
                        glm::vec2{ v1.x, v1.z }, 
                        glm::vec2{ v2.x, v2.z }, 
                        glm::vec2{ v3.x, v3.z }
                    );
		        }
	        }
        }
    }
}

//-----------------------------------------------------------------------

void Map::Render(MFA::RT::CommandRecordState& recordState)
{
    _groundRenderer->Render(recordState, { &_groundInstance });

    std::vector<MFA::MeshInstance*> wallInstances{};
    for (auto & instance : _wallInstances)
    {
        wallInstances.emplace_back(&instance);
    }
    _wallRenderer->Render(recordState, wallInstances);
}

//-----------------------------------------------------------------------
