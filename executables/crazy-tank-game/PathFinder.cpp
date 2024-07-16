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

bool PathFinder::AddEdge(NodeId const node1, NodeId const node2)
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
			for (auto const & otherNodeId : myNode->neighbors)
			{
				myDistanceField[otherNodeId] = glm::length(myNode->position - _nodesMap[otherNodeId]->position);
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
					for (auto const & otherNodeId : selectedNode->neighbors)
					{
						auto const newDistance = glm::length(selectedNode->position - _nodesMap[otherNodeId]->position) + distanceSoFar;
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
		return std::tuple {false, targetNodeID};
	}

	auto const & startNode = _nodesMap[startNodeID];
	if (startNode->neighbors.empty())
	{
		MFA_ASSERT(false);
		return std::tuple {false, InvalidNode};
	}

	NodeId bestNode = startNode->neighbors[0];
	auto bestDistance = distanceField[bestNode];
	
	for (int i = 1; i < startNode->neighbors.size(); ++i)
	{
		auto const neighbour = startNode->neighbors[i];
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

/*
std::tuple<bool, PathFinder::NodeId> PathFinder::FindNextNode(
	NodeId startNodeID, 
	NodeId targetNodeID, 
	std::set<NodeId> & blockedNodes
)
{
	if (startNodeID == targetNodeID)
	{
		return std::tuple {true, targetNodeID};
	}

	auto const & startNode = _nodesMap[startNodeID];
	auto const & targetNode = _nodesMap[targetNodeID];	

	blockedNodes.emplace(startNodeID);

	std::vector<std::pair<Node *, float>> sortedNeighbors{};

	for (auto const & neighbourID : targetNode->neighbors)
	{
		if (neighbourID == targetNodeID)
		{
			return neighbourID;
		}
		
		auto const & neighbour = _nodesMap[neighbourID];
		if (blockedNodes.contains(neighbourID) == false)
		{
			auto const score = CalculateHeuristicDistance(
				startNode->position, 
				neighbour->position, 
				targetNode->position
			);
			
			bool inserted = false;
			for (int i = 0; i < sortedNeighbors.size(); ++i)
			{
				if (sortedNeighbors[i].second > score)
				{
					sortedNeighbors.insert(sortedNeighbors.begin() + i, std::pair{neighbour.get(), score});
					inserted = true;
					break;
				}
			}
			if (inserted == false)
			{
				sortedNeighbors.emplace_back(std::pair{neighbour.get(), score});
			}
		}
	}

	for (int i = 0; i < sortedNeighbors.size(); ++i)
	{
		FindNextNode	
	}

	blockedNodes.erase(startNodeID);
}

//-------------------------------------------------------------------------------------------------

std::vector<PathFinder::NodeId> PathFinder::FindPath(NodeId startNodeID, NodeId targetNodeID)
{
	// TODO
}

//-------------------------------------------------------------------------------------------------

float PathFinder::CalculateHeuristicDistance(
	Position const & start, 
	Position const & neighbor, 
	Position const & target
)
{
	return glm::length(target - neighbor) + glm::length(neighbor - start);
}

//-------------------------------------------------------------------------------------------------
*/