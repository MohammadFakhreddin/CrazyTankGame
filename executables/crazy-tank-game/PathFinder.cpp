#include "PathFinder.hpp"

#include "BedrockAssert.hpp"

#include <gtx/norm.hpp>

// Instead of paths we can store distance fields! I could have done the same in our old game.

//-------------------------------------------------------------------------------------------------

PathFinder::NodeId PathFinder::AddNode(Position const & position)
{
	int const id = _nodesMap.size();

	auto node = std::make_unique<Node>();
	node->id = id;
	node->position = position;
	node->neighbors = {};
	_nodesMap.emplace_back(std::move(node));

	return id;
}

//-------------------------------------------------------------------------------------------------

bool PathFinder::AddEdge(NodeId const node1, NodeId const node2, float distance)
{
	if (node1 < 0 || node1 >= _nodesMap.size() ||
		node2 < 0 || node2 >= _nodesMap.size() ||
		node1 == node2)
	{
		MFA_ASSERT(false);
		return false;
	}

	auto const & cell1 = _nodesMap[node1];
	auto const & cell2 = _nodesMap[node2];

#ifdef MFA_DEBUG
	for (auto const & [node, distance] : cell1->neighbors)
	{
		if (node == node2)
		{
			MFA_ASSERT(false);
		}
	}
	for (auto const & [node, distance] : cell2->neighbors)
	{
		if (node == node1)
		{
			MFA_ASSERT(false);
		}
	}
#endif

	cell1->neighbors.emplace_back(std::pair{node2, distance});
	cell2->neighbors.emplace_back(std::pair{node1, distance});

	return true;
}

//-------------------------------------------------------------------------------------------------

PathFinder::NodeId PathFinder::FindNearestNode(Position const & myPosition)//, NodeId previousNode)
{
	NodeId closestNode = InvalidNode;
	double minDist2 = std::numeric_limits<double>::max();

	for (auto const & node : _nodesMap)
	{
		double const distance2 = glm::length2(node->position - myPosition);
		if (distance2 < minDist2)
		{
			minDist2 = distance2;
			closestNode = node->id; 
		}
	}

	return closestNode;
}

//-------------------------------------------------------------------------------------------------

PathFinder::Position PathFinder::NodePosition(NodeId const nodeId)
{
	if (nodeId < 0 || nodeId >= _nodesMap.size())
	{
		MFA_ASSERT(false);
		return glm::zero<Position>();
	}
	return _nodesMap[nodeId]->position;
}

//-------------------------------------------------------------------------------------------------

void PathFinder::CachePaths()
{
	_distanceFields.resize(_nodesMap.size());
	// TODO: This part can be parallelized
	for (auto const & myNode : _nodesMap)
	{
		auto & myDistanceField = _distanceFields[myNode->id];
		myDistanceField.resize(_nodesMap.size());
		for (auto & item : myDistanceField)
		{
			item = -1.0f;
		}

		myDistanceField[myNode->id] = 0.0f;
		
		// We use traversed nodes to avoid infinite loop. Some nodes might not get filled that can be natural.
		std::set<NodeId> traversedNodes{myNode->id};
		{
			std::set<NodeId> selectedNodes{};
			for (auto const & [otherNodeId, distance] : myNode->neighbors)
			{
				myDistanceField[otherNodeId] = distance;
				selectedNodes.emplace(otherNodeId);
			}
			traversedNodes = selectedNodes;

			do
			{
				std::set<NodeId> nextNodes{};
				for (auto const & selectedNodeId : selectedNodes)
				{
					auto const distanceSoFar = myDistanceField[selectedNodeId];
					auto const & selectedNode = _nodesMap[selectedNodeId];
					for (auto const & [otherNodeId, distance] : selectedNode->neighbors)
					{
						auto const newDistance = distance + distanceSoFar;
						auto & currentDistance = myDistanceField[otherNodeId];
						if (currentDistance > newDistance || currentDistance == -1.0f)
						{
							currentDistance = newDistance;
						}
						if (traversedNodes.contains(otherNodeId) == false)
						{
							nextNodes.emplace(otherNodeId);
							traversedNodes.emplace(otherNodeId);
						}
					}
				}
				selectedNodes = nextNodes;
			} while (selectedNodes.empty() == false);
		}
		
	}
}

//-------------------------------------------------------------------------------------------------

std::tuple<bool, PathFinder::NodeId> PathFinder::FindNextNode(NodeId startNodeID, NodeId targetNodeID)
{
	if (startNodeID == targetNodeID)
	{
		return std::tuple {true, targetNodeID};
	}

	auto const & distanceField = _distanceFields[targetNodeID];	
	auto const myDistance = distanceField[startNodeID];
	
	// No path exists
	if (myDistance < 0.0f)
	{
		return std::tuple {false, InvalidNode};
	}
	
	// We are the target node
	if (myDistance < glm::epsilon<float>())
	{
		return std::tuple {true, targetNodeID};
	}

	auto const & startNode = _nodesMap[startNodeID];
	if (startNode->neighbors.empty())
	{
		MFA_ASSERT(false);
		return std::tuple {false, InvalidNode};
	}

	NodeId bestNode = startNode->neighbors[0].first;
	auto bestDistance = distanceField[bestNode];
	
	for (int i = 1; i < startNode->neighbors.size(); ++i)
	{
		auto const & [neighbour, edgeLength] = startNode->neighbors[i];
		auto distance = distanceField[neighbour];

		if (distance >= 0.0f && (distance < bestDistance || bestDistance < 0.0f))
		{
			bestNode = neighbour;
			bestDistance = distance;
		}
	}

	return std::tuple {bestDistance >= 0.0f, bestNode};
}

//-------------------------------------------------------------------------------------------------
