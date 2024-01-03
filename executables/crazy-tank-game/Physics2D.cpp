#include "Physics2D.hpp"

#include "BedrockAssert.hpp"

#include <set>
#include <utility>
#include <gtx/norm.inl>

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
        _isStaticGridDirty = true;
        auto & newItem = _staticItemMap[id];
        item = &newItem;
    }
    else 
    {
        _isNonStaticGridDirty = true;
        auto & newItem = _nonStaticItemMap[id];
        item = &newItem;
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

//bool Physics2D::MovePolygon(EntityID const id, std::vector<glm::vec2> const& vertices)
//{
//    auto const findResult = _nonStaticItemMap.find(id);
//    if (findResult != _nonStaticItemMap.end())
//    {
//        _isNonStaticGridDirty = true;
//
//        auto& polygon = findResult->second.polygon;
//        polygon.vertices = vertices;
//        
//        auto& aabb = findResult->second.aabb;
//        AABB2D::Min(polygon.vertices[0], polygon.vertices[1], aabb.min);
//        AABB2D::Max(polygon.vertices[0], polygon.vertices[1], aabb.max);
//
//        for (int i = 2; i < static_cast<int>(polygon.vertices.size()); ++i)
//        {
//            AABB2D::Min(polygon.vertices[i], aabb.min, aabb.min);
//            AABB2D::Max(polygon.vertices[i], aabb.max, aabb.max);
//        }
//
//        return true;
//    }
//    return false;
//}

//-----------------------------------------------------------------------

void Physics2D::Update()
{
    if (_isNonStaticGridDirty == true)
    {
        for (auto const & [key, item] : _nonStaticItemMap)
        {
            _nonStaticCellSize += item.aabb.max - item.aabb.min;
        }
        _nonStaticCellSize /= static_cast<double>(_nonStaticItemMap.size());
        _nonStaticCellSize *= 2.0f;

        _nonStaticGrid.clear();
        for (auto & [key, item] : _nonStaticItemMap)
        {
            auto const min = item.aabb.min;
            auto const max = item.aabb.max;

            int const minX = static_cast<int>(std::floor(min.x / _nonStaticCellSize.x));
            int const minY = static_cast<int>(std::floor(min.y / _nonStaticCellSize.y));
            
            int const maxX = static_cast<int>(std::ceil(max.x / _nonStaticCellSize.x));
            int const maxY = static_cast<int>(std::ceil(max.y / _nonStaticCellSize.y));
            
            for (int x = minX; x <= maxX; ++x)
            {
                for (int y = minY; y <= maxY; ++y)
                {
					_nonStaticGrid[glm::ivec2{ x, y }].emplace_back(&item);
                }
            }
        }
    }
    if (_isStaticGridDirty == true)
    {
        for (auto const & [key, item] : _staticItemMap)
        {
            _staticCellSize += item.aabb.max - item.aabb.min;
        }
        _staticCellSize /= static_cast<double>(_staticItemMap.size());
        _staticCellSize *= 2.0f;

        _staticGrid.clear();
        for (auto & [key, item] : _staticItemMap)
        {
            auto const min = item.aabb.min;
            auto const max = item.aabb.max;

            int const minX = static_cast<int>(std::floor(min.x / _staticCellSize.x));
            int const minY = static_cast<int>(std::floor(min.y / _staticCellSize.y));

            int const maxX = static_cast<int>(std::ceil(max.x / _staticCellSize.x));
            int const maxY = static_cast<int>(std::ceil(max.y / _staticCellSize.y));

            for (int x = minX; x <= maxX; ++x)
            {
                for (int y = minY; y <= maxY; ++y)
                {
                    _staticGrid[glm::ivec2{ x, y }].emplace_back(&item);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------

bool Physics2D::Raycast(
    int layerMask,
    EntityID excludeId,
    glm::vec2 const & origin, 
	glm::vec2 const & direction, 
	float maxDistance, 
    HitInfo& outHitInfo
)
{
    auto const startPos = origin;
    auto const endPos = origin + direction * maxDistance;

    glm::vec2 max{};
	AABB2D::Max(startPos, endPos, max);

    glm::vec2 min{};
    AABB2D::Min(startPos, endPos, min);

    AABB2D aabb{ .min = min, .max = max };

    std::set<Item*> set{};

    {// Static
        int const minX = static_cast<int>(std::floor(min.x / _staticCellSize.x));
        int const minY = static_cast<int>(std::floor(min.y / _staticCellSize.y));

        int const maxX = static_cast<int>(std::ceil(max.x / _staticCellSize.x));
        int const maxY = static_cast<int>(std::ceil(max.y / _staticCellSize.y));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                auto & items =  _staticGrid[glm::ivec2{ x, y }];
                set.insert(items.begin(), items.end());
            }
        }
    }

    {// Non static
        int const minX = static_cast<int>(std::floor(min.x / _nonStaticCellSize.x));
        int const minY = static_cast<int>(std::floor(min.y / _nonStaticCellSize.y));

        int const maxX = static_cast<int>(std::ceil(max.x / _nonStaticCellSize.x));
        int const maxY = static_cast<int>(std::ceil(max.y / _nonStaticCellSize.y));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                auto& items = _nonStaticGrid[glm::ivec2{ x, y }];
                set.insert(items.begin(), items.end());
            }
        }
    }

    bool hit = false;
    outHitInfo.hitDistance = std::numeric_limits<float>().max();

    for (auto * item : set)
    {
	    if ((item->layer & layerMask) > 0 && item->aabb.Overlap(aabb) == true)
	    {
		    switch (item->type)
		    {
				case Type::Sphere:
				{
                    float distance{};
                    glm::vec2 normal{};
                    bool const hasCollision = RaySphereIntersection(
                        origin,
                        direction,
                        maxDistance,
                        item->sphere,
                        distance,
                        normal
                    );
                    if (hasCollision == true && distance < outHitInfo.hitDistance)
                    {
                        hit = true;
                        outHitInfo.layer = item->layer;
                        outHitInfo.onHit = item->onHit;
                        outHitInfo.hitNormal = normal;
                        outHitInfo.hitDistance = distance;
                    }
					break;	
				}
				case Type::Box:
                {
                    float distance{};
                    glm::vec2 normal{};
                    bool const hasCollision = RayBoxIntersection(
                        origin,
                        direction,
                        maxDistance,
                        item->box,
                        distance,
                        normal
                    );
                    if (hasCollision == true && distance < outHitInfo.hitDistance)
                    {
                        hit = true;
                        outHitInfo.layer = item->layer;
                        outHitInfo.onHit = item->onHit;
                        outHitInfo.hitNormal = normal;
                        outHitInfo.hitDistance = distance;
                    }
                    break;
                }
				default:
				{
                    MFA_LOG_ERROR("Item type not handled");
				}
		    }
	    }
    }

    if (hit == true)
    {
        outHitInfo.hitPoint = origin + direction * outHitInfo.hitDistance;
    }

    return hit;
}

//-----------------------------------------------------------------------

ID Physics2D::AllocateID()
{
    auto const id = _nextId;
    ++_nextId;
    return id;
}

//-----------------------------------------------------------------------

glm::vec2 Physics2D::OrthogonalDirection(glm::vec2 const& direction)
{
    glm::vec3 const normal3 = glm::cross(
        glm::vec3 {direction.x, 0.0f, direction.y},
        glm::vec3 {0.0f, 1.0f, 0.0f}
    );
    MFA_ASSERT((std::abs(normal3.y) - glm::epsilon<float>() < 0));
    auto const normal2 = glm::vec2{normal3.x, normal3.z};
    return normal2;
}

//-----------------------------------------------------------------------

bool Physics2D::RaySphereIntersection(
    glm::vec2 const& rayOrigin,
    glm::vec2 const& rayDirection,
    float const rayMaxDistance,
    Sphere const& sphere,
    float& outDistance,
    glm::vec2& outNormal
)
{
    glm::vec2 const& P = rayOrigin;
    glm::vec2 const D = rayDirection * rayMaxDistance;
    glm::vec2 const& F = sphere.center;
    float const & r = sphere.radius;

    float const A = glm::dot(D, D);
    if (std::abs(A) < glm::epsilon<float>())
    {
        return false;
    }

	glm::vec2 const PMinF = P - F;
	float const B = 2.0f * glm::dot(D, PMinF);
    float const C = glm::dot(PMinF, PMinF) - (r * r);

    float const B2Min4AC = (B * B) - (4.0f * A * C);
    if (B2Min4AC < 0.0f)
    {
        return false;
    }

	auto const TwoA = 2.0f * A;

    auto const t0 = (-B + B2Min4AC) / TwoA;
    auto const t1 = (-B - B2Min4AC) / TwoA;

    auto t = -1.0f;

    if (t0 > 0.0f)
    {
        t = std::min(t0, t);
    }
    if (t1 > 0.0f)
    {
        t = std::min(t1, t);
    }

	outDistance = t * rayMaxDistance;
    return t >= 0.0f && t <= 1.0f;
}

//-----------------------------------------------------------------------

bool Physics2D::RayBoxIntersection(
    glm::vec2 const& rayOrigin, 
    glm::vec2 const& rayDirection, 
	float rayMaxDistance,
	Box const& box, 
    float& outDistance,
    glm::vec2& outNormal
)
{
    bool hasCollision = false;

    outDistance = rayMaxDistance + 1.0f;

    {
        float distance{};
        glm::vec2 normal{};
        bool const lineHasCollision = RayLineIntersection(
            rayOrigin,
            rayDirection,
            rayMaxDistance,
            box.v0,
            box.v1,
            distance,
			normal  
        );
        if (lineHasCollision == true)
        {
            outDistance = distance;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float distance{};
        glm::vec2 normal{};
        bool const lineHasCollision = RayLineIntersection(
            rayOrigin,
            rayDirection,
            rayMaxDistance,
            box.v1,
            box.v2,
            distance,
            normal
        );
        if (lineHasCollision == true && distance < outDistance)
        {
            outDistance = distance;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float distance{};
        glm::vec2 normal{};
        bool const lineHasCollision = RayLineIntersection(
            rayOrigin,
            rayDirection,
            rayMaxDistance,
            box.v2,
            box.v3,
            distance,
            normal
        );
        if (lineHasCollision == true && distance < outDistance)
        {
            outDistance = distance;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float distance{};
        glm::vec2 normal{};
        bool const lineHasCollision = RayLineIntersection(
            rayOrigin,
            rayDirection,
            rayMaxDistance,
            box.v3,
            box.v0,
            distance,
            normal
        );
        if (lineHasCollision == true && distance < outDistance)
        {
            outDistance = distance;
            outNormal = normal;
            hasCollision = true;
        }
    }

    return hasCollision;
}

//-----------------------------------------------------------------------

bool Physics2D::RayLineIntersection(
    glm::vec2 const& rayOrigin, 
    glm::vec2 const& rayDirection, 
    float rayMaxDistance,
	glm::vec2 const& lineV0, 
    glm::vec2 const& lineV1,
    float& outDistance,
    glm::vec2& outNormal
)
{
    glm::vec2 const n = OrthogonalDirection(lineV1 - lineV0);
    glm::vec2 const & D = rayDirection * rayMaxDistance;
    glm::vec2 const& P = rayOrigin;
    glm::vec2 const& Q0 = lineV0;

    float const DdotN = glm::dot(D, n);
    if (DdotN < glm::epsilon<float>())
    {
        return false;
    }

    float const t1 = glm::dot((Q0 - P), n) / DdotN;
    
    if (t1 >= 0.0f && t1 < 1.0f)
    {
        outDistance = t1 * rayMaxDistance;
        auto const Q = outDistance * rayDirection + rayOrigin;
        auto const lineDiff = lineV1 - lineV0;
        auto const lineDiff2 = glm::dot(lineDiff, lineDiff);
        if (lineDiff2 < glm::epsilon<float>())
        {
            return false;
        }

        float const t2 = glm::dot(Q - lineV0, lineDiff) / lineDiff2;
        return t2 <= 1.0f + glm::epsilon<float>() && t2 >= -glm::epsilon<float>();
    }

    return false;
}

//-----------------------------------------------------------------------
