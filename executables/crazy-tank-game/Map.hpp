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
        int rows, int columns, int * walls,                     // For now walls are either 1 or zero
        std::shared_ptr<MFA::FlatShadingPipeline> pipeline,
        std::shared_ptr<MFA::RT::GpuTexture> errorTexture
    );

    void Render(MFA::RT::CommandRecordState& recordState);

private:

    std::unique_ptr<MFA::MeshRenderer> _groundRenderer{};
    std::unique_ptr<MFA::MeshRenderer> _wallRenderer{};

	MFA::MeshInstance _groundInstance{};
    std::vector<MFA::MeshInstance> _wallInstances{};

};
