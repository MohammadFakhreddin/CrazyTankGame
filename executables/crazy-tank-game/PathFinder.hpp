#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <set>

#include <glm/glm.hpp>

// TODO: You can write unit tests to make sure it is working right.
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
        std::vector<std::pair<NodeId, float>> neighbors{};
    };

    // We use position for heuristics
    [[nodiscard]]
    NodeId AddNode(Position const & position);

    // In the current implementation the edges do not have any cost on their own
    bool AddEdge(NodeId node1, NodeId node2, float distance);

    [[nodiscard]]
    NodeId FindNearestNode(Position const & myPosition);//, NodeId previousNode = -1);

    [[nodiscard]]
    Position NodePosition(NodeId nodeId);

    // You need to call this after modifying the paths
    void CachePaths();

    [[nodiscard]]
    std::tuple<bool, NodeId> FindNextNode(NodeId startNode, NodeId targetNode);

/*
    [[nodiscard]]
    std::tuple<bool, NodeId> FindNextNode(NodeId startNode, NodeId targetNode, std::set<NodeId> & blockedNodes);

    [[nodiscard]]
    std::vector<NodeId> FindPath(NodeId startNode, NodeId targetNode);
*/
private:

    [[nodiscard]]
    float CalculateHeuristicDistance(
        Position const & start, 
        Position const & neighbor, 
        Position const & target
    );

    std::vector<std::unique_ptr<Node>> _nodesMap{};

    // Technically this is a nxn matrix
    // First key: target node, Second key: start node, Third key: distance
    std::vector<std::vector<float>> _distanceFields{};

};
