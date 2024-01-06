#include "Map.hpp"

#include "BedrockPath.hpp"
#include "Layers.hpp"
#include "Physics2D.hpp"

#include <map>
#include <queue>

//-----------------------------------------------------------------------

Map::Map(
    float mapWidth, float mapHeight,
    int rows, int columns, std::vector<int> const& walls,
    std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
    std::shared_ptr<MFA::RT::GpuTexture> errorTexture
) : _rows(rows), _columns(columns), _walls(walls)
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
        _wallWidth = mapWidth / static_cast<float>(rows);
        _wallHeight = mapHeight / static_cast<float>(columns);
        float const halfWallWidth = _wallWidth * 0.5f;
        float const halfWallHeight = _wallHeight * 0.5f;

        float startX = -mapWidth * 0.5f + halfWallWidth;
        float startY = -mapHeight * 0.5f + halfWallHeight;
        for (int j = 0; j < rows; ++j)
        {
	        for (int i = 0; i < columns; ++i)
	        {
		        if (walls[j * columns + i] > 0)
		        {
                    _wallInstances.emplace_back(*_wallRenderer);
                    auto & wallInstance = _wallInstances.back();
                    auto & transform = wallInstance.GetTransform();
                    transform.Setscale(glm::vec3{ halfWallWidth, 0.5f, halfWallHeight });
                    transform.Setposition(glm::vec3{ static_cast<float>(j) * _wallWidth + startX, 0.3f, static_cast<float>(i) * _wallHeight + startY });
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

glm::vec2 Map::CellPosition(Coord const& c) const {
    return { 
        -0.5f * static_cast<float>(_rows) * _wallWidth + (static_cast<float>(c.x) + 0.5f) * _wallWidth,
        -0.5f * static_cast<float>(_columns) * _wallHeight + (static_cast<float>(c.y) + 0.5f) * _wallHeight
    };
}

Map::Coord Map::PositionCoord(glm::vec2 const& p) const {
    return {
        int((p.x + 0.5f * static_cast<float>(_rows) * _wallWidth) / _wallWidth),
        int((p.y + 0.5f * static_cast<float>(_columns) * _wallHeight) / _wallHeight)
    };
}

int Map::WallAt(Coord const& c) const {
    return _walls[c.x * _columns + c.y];
}

bool Map::IsValid(Coord const& c) const { 
    return c.x >= 0 && c.x < _rows && c.y >= 0 && c.y < _columns; 
}

std::vector<glm::vec2> Map::AStar(Coord const& c_from, Coord const& c_to) const {
    auto dist = [](Coord const& c_a, Coord const& c_b) -> float { return sqrtf(static_cast<float>((c_a.x - c_b.x) * (c_a.x - c_b.x) + (c_a.y - c_b.y) * (c_a.y - c_b.y))); };

    auto coord_cmp_less = [](Coord const& lhs, Coord const& rhs) -> bool { return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x); };
    std::map<Coord, Coord, decltype(coord_cmp_less)> parent(coord_cmp_less);
    std::map<Coord, float, decltype(coord_cmp_less)> g_score(coord_cmp_less); g_score[c_from] = 0.f;
    std::map<Coord, float, decltype(coord_cmp_less)> f_score(coord_cmp_less); f_score[c_from] = dist(c_from, c_to);

    auto f_cmp_greater = [&](Coord const& lhs, Coord const& rhs) -> bool { return f_score[lhs] > f_score[rhs]; };
    std::priority_queue<Coord, std::vector<Coord>, decltype(f_cmp_greater)> open_queue(f_cmp_greater); open_queue.push(c_from);

    while (!open_queue.empty()) {
        Coord c_current = open_queue.top();
        open_queue.pop();
        if (c_current.x == c_to.x && c_current.y == c_to.y) {
            break;
        }
        for (const Coord& c_n : {
                Coord{ c_current.x - 1, c_current.y }, Coord{ c_current.x + 1, c_current.y },
                Coord{ c_current.x, c_current.y - 1 }, Coord{ c_current.x, c_current.y + 1 }
            }) {
            if (IsValid(c_n) && WallAt(c_n) == 0) {
                float tentative_gs = g_score[c_current] + dist(c_current, c_n);
                if (!g_score.contains(c_n) || tentative_gs < g_score[c_n]) {
                    parent[c_n] = c_current;
                    g_score[c_n] = tentative_gs;
                    f_score[c_n] = tentative_gs + dist(c_n, c_to);
                    open_queue.push(c_n);
                }
            }
        }
    }
    Coord c_trace = c_to;
    std::vector<glm::vec2> path;
    while (parent.contains(c_trace)) {
        path.push_back(CellPosition(c_trace));
        c_trace = parent[c_trace];
    }
    //if (c_trace.x == c_from.x && c_trace.y == c_from.y) {
    //    path.push_back(CellPosition(c_from.x, c_from.y));
    //}
    //std::reverse(path.begin(), path.end());
    return path;
}

Map::Coord Map::RandomTile(bool avoidWall) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<> row_dist(0, _rows - 1);
    std::uniform_int_distribution<> col_dist(0, _columns - 1);
    Coord result{ row_dist(rng), col_dist(rng) };
    while (avoidWall && WallAt(result) > 0) {
        result = { row_dist(rng), col_dist(rng) };
    }
    return result;
}

//-----------------------------------------------------------------------
