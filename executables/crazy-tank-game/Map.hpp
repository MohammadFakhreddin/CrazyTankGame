#pragma once

#include "utils/MeshInstance.hpp"
#include "utils/MeshRenderer.hpp"

#include <memory>
#include <vector>

class Map
{
public:
    /*struct Coord {
        int x, y;
    };*/

    explicit Map(
        float mapWidth, float mapHeight,
        int rows, int columns, std::vector<int> const& walls,                     // For now walls are either 1 or zero
        std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
        std::shared_ptr<MFA::RT::GpuTexture> errorTexture
    );
     
    void Render(MFA::RT::CommandRecordState& recordState);

    //glm::vec2 CellPosition(Coord const& c) const;
    //Coord PositionCoord(glm::vec2 const& p) const;

    //int WallAt(Coord const& c) const;
    //bool IsValid(Coord const& c) const;

    //std::vector<glm::vec2> AStar(Coord const& c_from, Coord const& c_to) const;

    //Coord RandomTile(bool avoidWall = true);

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
