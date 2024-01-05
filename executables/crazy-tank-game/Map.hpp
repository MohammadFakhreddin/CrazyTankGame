#pragma once

#include "utils/MeshInstance.hpp"
#include "utils/MeshRenderer.hpp"

#include <memory>
#include <vector>

class Map
{
public:

    explicit Map(
        float mapWidth, float mapHeight,
        int rows, int columns, std::vector<int> const& walls,                     // For now walls are either 1 or zero
        std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
        std::shared_ptr<MFA::RT::GpuTexture> errorTexture
    );

    void Render(MFA::RT::CommandRecordState& recordState);

    glm::vec2 CellPosition(int x, int y) const;
    int WallAt(int x, int y) const;

    std::vector<glm::vec2> AStar(int x_from, int y_from, int x_to, int y_to) const;

private:
    int _rows, _columns;
    std::vector<int> _walls;
    float _wallWidth;
    float _wallHeight;

    std::unique_ptr<MFA::MeshRenderer> _groundRenderer{};
    std::unique_ptr<MFA::MeshRenderer> _wallRenderer{};

	MFA::MeshInstance _groundInstance{};
    std::vector<MFA::MeshInstance> _wallInstances{};

};
