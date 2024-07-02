#include "PathFinder.hpp"

#include "BedrockAssert.hpp"

#include <gtx/norm.hpp>

//-------------------------------------------------------------------------------------------------

PathFinder::NodeId PathFinder::AddNode(Position const & position)
{
	int const id = _nextNodeId;

	auto node = std::make_unique<Node>();
	node->id = id;
	node->position = position;
	node->neighbors = {};

	_nextNodeId++;

	MFA_ASSERT(_nodesMap.contains(id) == false);
	_nodesMap[id] = std::move(node);

	return id;
}

//-------------------------------------------------------------------------------------------------

bool PathFinder::AddEdge(NodeId const node1, NodeId const node2)
{
	auto const findCell1Result = _nodesMap.find(node1);
	if (findCell1Result == _nodesMap.end())
	{
		return false;
	}

	auto const findCell2Result = _nodesMap.find(node2);
	if (findCell2Result == _nodesMap.end())
	{
		return false;
	}

	auto const & cell1 = findCell1Result->second;
	auto const & cell2 = findCell2Result->second;

	MFA_ASSERT(std::count(cell1->neighbors.begin(), cell1->neighbors.end(), node1) == 0);
	cell1->neighbors.emplace_back(node2);

	MFA_ASSERT(std::count(cell2->neighbors.begin(), cell2->neighbors.end(), node2) == 0);
	cell2->neighbors.emplace_back(node1);

	return true;
}

//-------------------------------------------------------------------------------------------------

PathFinder::NodeId PathFinder::FindNearestNode(Position const & myPosition)//, NodeId previousNode)
{
	NodeId closestNode = InvalidNode;
	double minDist2 = std::numeric_limits<double>::max();

	for (auto const & [id, node] : _nodesMap)
	{
		double const distance2 = glm::length2(node->position - myPosition);
		if (distance2 < minDist2)
		{
			minDist2 = distance2;
			closestNode = id;
		}
	}

	return closestNode;
}

//-------------------------------------------------------------------------------------------------

PathFinder::Position PathFinder::NodePosition(NodeId const nodeId)
{
	MFA_ASSERT(nodeId != InvalidNode);
	Position position {};
	if (auto const findResult = _nodesMap.find(nodeId); findResult != _nodesMap.end())
	{
		position = findResult->second->position;
	}
	return position;
}

//-------------------------------------------------------------------------------------------------

PathFinder::NodeId PathFinder::FindNextNode(NodeId startNode, NodeId targetNode)
{
	// TODO
}

//-------------------------------------------------------------------------------------------------

std::vector<PathFinder::NodeId> PathFinder::FindPath(NodeId startNode, NodeId targetNode)
{
	// TODO
}

//-------------------------------------------------------------------------------------------------
