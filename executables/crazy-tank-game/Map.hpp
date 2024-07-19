#pragma once

#include "utils/MeshInstance.hpp"
#include "utils/MeshRenderer.hpp"

#include <memory>
#include <vector>

class Map
{
public:
    
    explicit Map(
        float mapWidth, 
        float mapHeight,
        int rows, 
        int columns, 
        std::vector<int> const& walls,                          // For now walls are either 1 or zero
        std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
        std::shared_ptr<MFA::RT::GpuTexture> errorTexture
    );
     
    void Render(MFA::RT::CommandRecordState& recordState);

    [[nodiscard]]
    std::vector<int> const & GetWalls();

    [[nodiscard]]
    int GetRows();

    [[nodiscard]]
    int GetColumns();

    [[nodiscard]]
    glm::vec3 CalcPosition(int row, int column); 

private:

    int const _rows;
    int const _columns;
    std::vector<int> const _walls{};

    float _wallWidth;
    float _wallHeight;

    float _startX;
    float _startY;

    std::unique_ptr<MFA::MeshRenderer> _groundRenderer{};
    std::unique_ptr<MFA::MeshRenderer> _wallRenderer{};

	std::unique_ptr<MFA::MeshInstance> _groundInstance{};
    std::vector<std::unique_ptr<MFA::MeshInstance>> _wallInstances{};

};
