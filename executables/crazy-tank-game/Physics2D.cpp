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
    EntityID id, 
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

bool Physics2D::MovePolygon(EntityID id, std::vector<glm::vec2> const& vertices)
{
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

void Physics2D::Update()
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
