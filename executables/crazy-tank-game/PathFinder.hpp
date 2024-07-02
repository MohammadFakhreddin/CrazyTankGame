#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

class PathFinder
{
public:

    using NodeId = int;
    static constexpr NodeId InvalidNode = -1;
    using Position = glm::vec3;

    struct Node
    {
        NodeId id{};
        Position position{};
        std::vector<NodeId> neighbors{};
    };

    // We use position for heuristics
    [[nodiscard]]
    NodeId AddNode(Position const & position);

    bool AddEdge(NodeId node1, NodeId node2);

    [[nodiscard]]
    NodeId FindNearestNode(Position const & myPosition);//, NodeId previousNode = -1);

    [[nodiscard]]
    Position NodePosition(NodeId nodeId);

    [[nodiscard]]
    NodeId FindNextNode(NodeId startNode, NodeId targetNode);

    [[nodiscard]]
    std::vector<NodeId> FindPath(NodeId startNode, NodeId targetNode);

private:

    std::unordered_map<NodeId, std::unique_ptr<Node>> _nodesMap{};

    NodeId _nextNodeId = 0;

};
