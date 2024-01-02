#include "Physics2D.hpp"

#include <utility>

using ID = Physics2D::EntityID;

//-----------------------------------------------------------------------

Physics2D::Physics2D()
{
    Instance = this;
}

//-----------------------------------------------------------------------

Physics2D::~Physics2D()
{
    Instance = nullptr;
}

//-----------------------------------------------------------------------

ID Physics2D::Register(Type const type, int const layer, bool const isStatic, OnHit onHit)
{
    auto const id = AllocateID();
    Item * item{};

    if (isStatic == true)
    {
        item = &_staticItemMap[id];
        _isStaticGridDirty = true;
    }
    else 
    {
        item = &_nonStaticItemMap[id];
        _isNonStaticGridDirty = true;
    }

    item->id = id;
    item->layer = layer;
    item->isStatic = isStatic;
    item->type = type;
    item->onHit = std::move(onHit);

    return id;
}

//-----------------------------------------------------------------------

bool Physics2D::UnRegister(EntityID const id)
{
    {
        auto const findResult = _nonStaticItemMap.find(id);
        if (findResult != _nonStaticItemMap.end())
        {
            _isNonStaticGridDirty = true;
            _nonStaticItemMap.erase(findResult);
            return true;
        }
    }
    {
        auto const findResult = _staticItemMap.find(id);
        if (findResult != _staticItemMap.end())
        {
            _isStaticGridDirty = true;
            _staticItemMap.erase(findResult);
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MoveAABB(EntityID const id, glm::vec2 const & min, glm::vec2 const & max)
{
    auto const findResult = _nonStaticItemMap.find(id);
    if (findResult != _nonStaticItemMap.end())
    {
        _isNonStaticGridDirty = true;
        auto & aabb = findResult->second.aabb;
        aabb.min = min;
        aabb.max = max;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MoveSphere(EntityID const id, glm::vec2 const & center, float const radius)
{
    auto const findResult = _nonStaticItemMap.find(id);
    if (findResult != _nonStaticItemMap.end())
    {
        _isNonStaticGridDirty = true;

    	auto& sphere = findResult->second.sphere;
        sphere.center = center;
        sphere.radius = radius;

        auto& aabb = findResult->second.aabb;
        aabb.min.x = center.x - radius;
        aabb.min.y = center.y - radius;
        aabb.max.x = center.x + radius;
        aabb.max.y = center.y + radius;

        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MoveBox(
    EntityID const id, 
    glm::vec2 const& v0, 
	glm::vec2 const& v1, 
	glm::vec2 const& v2, 
	glm::vec2 const& v3
)
{
    auto const findResult = _nonStaticItemMap.find(id);
    if (findResult != _nonStaticItemMap.end())
    {
        _isNonStaticGridDirty = true;

        auto& box = findResult->second.box;
        box.v0 = v0;
        box.v1 = v1;
        box.v2 = v2;
        box.v3 = v3;

        auto& aabb = findResult->second.aabb;
    	AABB2D::Min(v0, v1, aabb.min);
        AABB2D::Min(aabb.min, v2, aabb.min);
        AABB2D::Min(aabb.min, v3, aabb.min);

        AABB2D::Max(v0, v1, aabb.max);
        AABB2D::Max(aabb.max, v2, aabb.max);
        AABB2D::Max(aabb.max, v3, aabb.max);

        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MovePolygon(EntityID const id, std::vector<glm::vec2> const& vertices)
{
    auto const findResult = _nonStaticItemMap.find(id);
    if (findResult != _nonStaticItemMap.end())
    {
        _isNonStaticGridDirty = true;

        auto& polygon = findResult->second.polygon;
        polygon.vertices = vertices;
        
        auto& aabb = findResult->second.aabb;
        AABB2D::Min(polygon.vertices[0], polygon.vertices[1], aabb.min);
        AABB2D::Max(polygon.vertices[0], polygon.vertices[1], aabb.max);

        for (int i = 2; i < static_cast<int>(polygon.vertices.size()); ++i)
        {
            AABB2D::Min(polygon.vertices[i], aabb.min, aabb.min);
            AABB2D::Max(polygon.vertices[i], aabb.max, aabb.max);
        }

        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

void Physics2D::Update()
{
    if (_isNonStaticGridDirty == true)
    {
        for (auto [key, item] : _nonStaticItemMap)
        {
            _nonStaticCellSize += item.aabb.max - item.aabb.min;
        }
        _nonStaticCellSize /= static_cast<double>(_nonStaticItemMap.size());
        _nonStaticCellSize *= 2.0f;

        _nonStaticGrid.clear();
        for (auto [key, item] : _nonStaticItemMap)
        {
            auto const min = item.aabb.min;
            auto const max = item.aabb.max;

            int minX = static_cast<int>(std::floor(min.x / _nonStaticCellSize.x));
            int minY = static_cast<int>(std::floor(min.y / _nonStaticCellSize.y));
            
            int maxX = static_cast<int>(std::ceil(max.x / _nonStaticCellSize.x));
            int maxY = static_cast<int>(std::ceil(max.y / _nonStaticCellSize.y));
            
            for (int x = minX; x <= maxX; ++x)
            {
                for (int y = minY; y <= maxY; ++y)
                {
					_nonStaticGrid[glm::ivec2{ x, y }].emplace_back(item);
                }
            }
        }
    }
    if (_isStaticGridDirty == true)
    {
        for (auto [key, item] : _staticItemMap)
        {
            _staticCellSize += item.aabb.max - item.aabb.min;
        }
        _staticCellSize /= static_cast<double>(_staticItemMap.size());
        _staticCellSize *= 2.0f;

        _staticGrid.clear();
        for (auto [key, item] : _staticItemMap)
        {
            auto const min = item.aabb.min;
            auto const max = item.aabb.max;

            int minX = static_cast<int>(std::floor(min.x / _staticCellSize.x));
            int minY = static_cast<int>(std::floor(min.y / _staticCellSize.y));

            int maxX = static_cast<int>(std::ceil(max.x / _staticCellSize.x));
            int maxY = static_cast<int>(std::ceil(max.y / _staticCellSize.y));

            for (int x = minX; x <= maxX; ++x)
            {
                for (int y = minY; y <= maxY; ++y)
                {
                    _staticGrid[glm::ivec2{ x, y }].emplace_back(item);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------

bool Physics2D::Raycast(
    int layerMask,
    EntityID excludeId,
    glm::vec2 origin, 
	glm::vec2 direction, 
	float maxDistance, 
	OnHit onHit
)
{
}

//-----------------------------------------------------------------------

ID Physics2D::AllocateID()
{
    auto const id = _nextId;
    ++_nextId;
    return id;
}

//-----------------------------------------------------------------------
