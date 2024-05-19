#include "Physics2D.hpp"

#include "BedrockAssert.hpp"

#include <set>
#include <utility>

using ID = Physics2D::EntityID;

//-----------------------------------------------------------------------

Physics2D::Physics2D(
    std::shared_ptr<MFA::PointRenderer> pointRenderer, 
    std::shared_ptr<MFA::LineRenderer> lineRenderer
)
{
    _pointRenderer = std::move(pointRenderer);
    _lineRenderer = std::move(lineRenderer);
	Instance = this;
}

//-----------------------------------------------------------------------

Physics2D::~Physics2D()
{
    Instance = nullptr;
}

//-----------------------------------------------------------------------

ID Physics2D::Register(
    Type const type, 
    Layer const layer,
    Layer const layerMask,
	OnHit onHit
)
{
    auto const id = AllocateID();

	auto& item = _itemMap[id];
    item.id = id;
    item.type = type;
	item.layer = layer;
    item.layerMask = layerMask;
    item.onHit = std::move(onHit);

    _isMapDirty = true;

    return id;
}

//-----------------------------------------------------------------------

bool Physics2D::UnRegister(EntityID const id)
{
    auto const findResult = _itemMap.find(id);
    if (findResult != _itemMap.end())
    {
        _itemMap.erase(findResult);
        _isMapDirty = true;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MoveAABB(
    EntityID const id, 
    glm::vec2 const & min, 
	glm::vec2 const & max,
    bool const checkForCollision
)
{
    auto const findResult = _itemMap.find(id);
    if (findResult != _itemMap.end())
    {
        Entity item = findResult->second;

        MFA_ASSERT(item.type == Type::AABB);

        auto const & v0 = min;
        auto const & v2 = max;
        auto const v1 = glm::vec2{ v2.x, v0.y };
        auto const v3 = glm::vec2{ v0.x, v2.y };

        auto& aabb = item.aabb;
        aabb.min = min;
        aabb.max = max;

        auto& box = item.box;
        box.v0 = v0;
        box.v2 = v2;
        box.v1 = v1;
        box.v3 = v3;
        box.center = (v0 + v1 + v2 + v3) * 0.25f;

        if (checkForCollision == true)
        {
            bool const hasCollision = CheckForAABB_Collision(item);
            if (hasCollision == true)
            {
                return false;
            }
        }

        findResult->second = item;

    	return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::MoveSphere(
    EntityID const id, 
    glm::vec2 const & center, 
	float const radius,
    bool checkForCollision
)
{
    auto const findResult = _itemMap.find(id);
    if (findResult != _itemMap.end())
    {
        Entity item = findResult->second;

        MFA_ASSERT(item.type == Type::Sphere);

        auto& sphere = item.sphere;
        sphere.center = center;
        sphere.radius = radius;

        auto& aabb = item.aabb;
        aabb.min.x = center.x - radius;
        aabb.min.y = center.y - radius;
        aabb.max.x = center.x + radius;
        aabb.max.y = center.y + radius;

        if (checkForCollision == true)
        {
            bool const hasCollision = CheckForSphereCollision(item);
            if (hasCollision == true)
            {
                return false;
            }
        }

        findResult->second = item;

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
	glm::vec2 const& v3,
    bool checkForCollision
)
{
    auto const findResult = _itemMap.find(id);
    if (findResult != _itemMap.end())
    {
        Entity item = findResult->second;

        MFA_ASSERT(item.type == Type::Box);

        auto& box = item.box;
        box.v0 = v0;
        box.v1 = v1;
        box.v2 = v2;
        box.v3 = v3;

        box.center = (box.v0 + box.v1 + box.v2 + box.v3) * 0.25f;

        auto& aabb = item.aabb;
        AABB2D::Min(v0, v1, aabb.min);
        AABB2D::Min(aabb.min, v2, aabb.min);
        AABB2D::Min(aabb.min, v3, aabb.min);

        AABB2D::Max(v0, v1, aabb.max);
        AABB2D::Max(aabb.max, v2, aabb.max);
        AABB2D::Max(aabb.max, v3, aabb.max);

        if (checkForCollision == true)
        {
            bool const hasCollision = CheckForBoxCollision(item);
            if (hasCollision == true)
            {
                return false;
            }
        }

        findResult->second = item;

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
    if (_isMapDirty == true)
    {
        _isMapDirty = false;

        _itemList.clear();
        for (auto & [key, value] : _itemMap)
        {
            _itemList.emplace_back(&value);
        }
    }
	/*
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
    */
}

//-----------------------------------------------------------------------

void Physics2D::Render(MFA::RT::CommandRecordState& recordState)
{
    if (_isMapDirty) { Update(); }
    for (auto * item : _itemList)
    {
	    switch (item->type)
	    {
	    case Type::AABB:
	    {
            auto const& box = item->box;

            glm::vec4 color{ 0.0f, 1.0f, 0.0f, 1.0f };

            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v0.x, 0.0f, box.v0.y },
                glm::vec3{ box.v1.x, 0.0f, box.v1.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v1.x, 0.0f, box.v1.y },
                glm::vec3{ box.v2.x, 0.0f, box.v2.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v2.x, 0.0f, box.v2.y },
                glm::vec3{ box.v3.x, 0.0f, box.v3.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v3.x, 0.0f, box.v3.y },
                glm::vec3{ box.v0.x, 0.0f, box.v0.y },
                color
            );
            break;
	    }
        case Type::Box:
	    {
            auto const& box = item->box;

            glm::vec4 color{ 0.0f, 0.0f, 1.0f, 1.0f };

            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v0.x, 0.0f, box.v0.y },
                glm::vec3{ box.v1.x, 0.0f, box.v1.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v1.x, 0.0f, box.v1.y },
                glm::vec3{ box.v2.x, 0.0f, box.v2.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v2.x, 0.0f, box.v2.y },
                glm::vec3{ box.v3.x, 0.0f, box.v3.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ box.v3.x, 0.0f, box.v3.y },
                glm::vec3{ box.v0.x, 0.0f, box.v0.y },
                color
            );
            break;
	    }
        case Type::Sphere:
        {
            auto const& sphere = item->sphere;
            auto const v0 = glm::vec2{ sphere.center.x - sphere.radius, sphere.center.y };
            auto const v2 = glm::vec2{ sphere.center.x + sphere.radius, sphere.center.y };
            auto const v1 = glm::vec2{ sphere.center.x, sphere.center.y + sphere.radius };
            auto const v3 = glm::vec2{ sphere.center.x, sphere.center.y - sphere.radius };

            glm::vec4 color{ 1.0f, 0.0f, 0.0f, 1.0f };

            _lineRenderer->Draw(
                recordState,
                glm::vec3{ v0.x, 0.0f, v0.y },
                glm::vec3{ v1.x, 0.0f, v1.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ v1.x, 0.0f, v1.y },
                glm::vec3{ v2.x, 0.0f, v2.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ v2.x, 0.0f, v2.y },
                glm::vec3{ v3.x, 0.0f, v3.y },
                color
            );
            _lineRenderer->Draw(
                recordState,
                glm::vec3{ v3.x, 0.0f, v3.y },
                glm::vec3{ v0.x, 0.0f, v0.y },
                color
            );
            break;
	    }
        default:
            MFA_LOG_ERROR("Shape not implemented");
	    }
    }
}

//-----------------------------------------------------------------------

bool Physics2D::Raycast(
    Layer const layerMask,
    std::set<EntityID> const & excludeIds,
    Ray const& ray,
    float maxDistance,
    HitInfo& outHitInfo
)
{
    auto const startPos = ray.origin - ray.direction * 1e-5f;
    auto const endPos = ray.origin + ray.direction * (maxDistance + 1e-5f);
    maxDistance += 2e-5f;

    glm::vec2 max{};
	AABB2D::Max(startPos, endPos, max);

    glm::vec2 min{};
    AABB2D::Min(startPos, endPos, min);

    AABB2D const aabb{ .min = min, .max = max };
    
/*
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
    */
    
    bool hit = false;
    
    for (auto * item : _itemList)
    {
	    if (excludeIds.contains(item->id) == false && (item->layer & layerMask) > 0)
	    {
            if (item->aabb.Overlap(aabb) == true)
            {
                float time{};
                glm::vec2 normal{};
                bool hasCollision = false;

                switch (item->type)
                {
	                case Type::Sphere:
	                {
	                    hasCollision = RaySphereIntersection(
	                        ray,
	                        maxDistance,
	                        item->sphere,
	                        time,
	                        normal
	                    );

	                    break;
	                }
					case Type::AABB:
	                case Type::Box:
	                {
	                    hasCollision = RayBoxIntersection(
							ray,
	                        maxDistance,
	                        item->box,
	                        time,
	                        normal
	                    );
	                    break;
	                }
	                default:
	                {
	                    MFA_LOG_ERROR("Item type not handled");
	                }
                }

                if (hasCollision == true && (hit == false || time < outHitInfo.hitTime))
                {
                    hit = true;
                    outHitInfo.layer = item->layer;
                    outHitInfo.onHit = item->onHit;
                    outHitInfo.hitNormal = normal;
                    outHitInfo.hitTime = time;
                    outHitInfo.entityId = item->id;
                }
		    }
	    }
    }
    
    if (hit == true)
    {
        outHitInfo.hitPoint = ray.origin + (outHitInfo.hitTime * maxDistance * ray.direction);
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

glm::vec2 Physics2D::OrthogonalDirection(glm::vec2 const& v0, glm::vec2 const& v1, glm::vec2 const & center)
{
    auto const direction = v1 - v0;
    glm::vec3 const normal3 = glm::cross(
        glm::vec3 {direction.x, 0.0f, direction.y},
        glm::vec3 {0.0f, 1.0f, 0.0f}
    );
    MFA_ASSERT((std::abs(normal3.y) - glm::epsilon<float>() < 0));
    auto normal2 = glm::vec2{normal3.x, normal3.z};
    if (glm::dot(normal2, v0 - center) < 0)
    {
        normal2 = -normal2;
    }
    return glm::normalize(normal2);
}

//-----------------------------------------------------------------------

bool Physics2D::RaySphereIntersection(
    Ray const& ray,
    float const rayMaxDistance,
    Sphere const& sphere,
    float& outTime,
    glm::vec2& outNormal
)
{
    glm::vec2 const& P = ray.origin;
    glm::vec2 const D = ray.direction * rayMaxDistance;
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

    auto t = 1.1f;

    if (t0 > 0.0f)
    {
        t = std::min(t0, t);
    }
    if (t1 > 0.0f)
    {
        t = std::min(t1, t);
    }

	outTime = t;
    outNormal = ray.origin + (t * rayMaxDistance) - F;
    return t >= 0.0f && t <= 1.0f;
}

//-----------------------------------------------------------------------

bool Physics2D::RayBoxIntersection(
    Ray const& ray,
	float const rayMaxDistance,
	Box const& box, 
    float& outTime,
    glm::vec2& outNormal
)
{
    bool hasCollision = false;

    outTime = rayMaxDistance + 1.0f;

    {
        float distance{};
        glm::vec2 const normal = OrthogonalDirection(box.v1, box.v0, box.center);
        bool const lineHasCollision = RayLineIntersection(
            ray,
            rayMaxDistance,
            box.v0,
            box.v1,
            normal,
            distance
        );
        if (lineHasCollision == true)
        {
            outTime = distance;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float time{};
        glm::vec2 const normal = OrthogonalDirection(box.v2, box.v1, box.center);
        bool const lineHasCollision = RayLineIntersection(
            ray,
            rayMaxDistance,
            box.v1,
            box.v2,
            normal,
            time
        );
        if (lineHasCollision == true && time < outTime)
        {
            outTime = time;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float time{};
        glm::vec2 const normal = OrthogonalDirection(box.v3, box.v2, box.center);
        bool const lineHasCollision = RayLineIntersection(
            ray,
            rayMaxDistance,
            box.v2,
            box.v3,
            normal,
            time
        );
        if (lineHasCollision == true && time < outTime)
        {
            outTime = time;
            outNormal = normal;
            hasCollision = true;
        }
    }
    {
        float time{};
        glm::vec2 const normal = OrthogonalDirection(box.v0, box.v3, box.center);
        bool const lineHasCollision = RayLineIntersection(
            ray,
            rayMaxDistance,
            box.v3,
            box.v0,
            normal,
            time
        );
        if (lineHasCollision == true && time < outTime)
        {
            outTime = time;
            outNormal = normal;
            hasCollision = true;
        }
    }

    return hasCollision;
}

//-----------------------------------------------------------------------

bool Physics2D::RayLineIntersection(
    Ray const& ray,
    float rayMaxDistance,
    glm::vec2 const& lineV0,
    glm::vec2 const& lineV1,
    glm::vec2 const& lineNormal,
    float& outTime
)
{
    auto const & n = lineNormal;

	glm::vec2 const& D = ray.direction * rayMaxDistance;
    glm::vec2 const& P = ray.origin;
    glm::vec2 const& Q0 = lineV0;

    float const DdotN = glm::dot(D, n);
    if (DdotN >= 0.0f)
    {
        return false;
    }

    float const t1 = glm::dot((Q0 - P), n) / DdotN;
    outTime = t1;

    if (t1 >= 0.0f && t1 <= 1.0f)
    {
        auto const Q = (outTime * D) + ray.origin;
        auto const lineDiff = lineV1 - lineV0;
        auto const lineDiff2 = glm::dot(lineDiff, lineDiff);
        if (lineDiff2 == 0.0f)
        {
            MFA_ASSERT(false);
            return false;
        }

        float const t2 = glm::dot(Q - lineV0, lineDiff) / lineDiff2;
        auto const hasCollision = t2 <= 1.0f + 1e-5f && t2 >= -1e-5f;
        return hasCollision;
    }

    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::BoxAABB_Collision(Box const& box, Entity const& aabbEntity)
{
    if (aabbEntity.aabb.Overlap(box.v0) ||
        aabbEntity.aabb.Overlap(box.v1) ||
        aabbEntity.aabb.Overlap(box.v2) ||
        aabbEntity.aabb.Overlap(box.v3) ||
        IsInsideBox(box, aabbEntity.box.v0) ||
        IsInsideBox(box, aabbEntity.box.v1) ||
        IsInsideBox(box, aabbEntity.box.v2) ||
        IsInsideBox(box, aabbEntity.box.v3))
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::SphereBoxCollision(Sphere const& sphere, Box const& box)
{
    if (IsInsideSphere(sphere, box.v0) ||
        IsInsideSphere(sphere, box.v1) ||
        IsInsideSphere(sphere, box.v2) ||
        IsInsideSphere(sphere, box.v3))
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::BoxBoxCollision(Box const& box0, Box const& box1)
{
    if (IsInsideBox(box0, box1.v0) ||
        IsInsideBox(box0, box1.v1) ||
        IsInsideBox(box0, box1.v2) ||
        IsInsideBox(box0, box1.v3) ||
        IsInsideBox(box1, box0.v0) ||
        IsInsideBox(box1, box0.v1) ||
        IsInsideBox(box1, box0.v2) ||
        IsInsideBox(box1, box0.v3))
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::SphereSphere_Collision(Sphere const& sphere0, Sphere const& sphere1)
{
    auto const dist2 = glm::length2(sphere0.center - sphere1.center);

	auto const maxDist = sphere0.radius + sphere1.radius;
    auto const maxDist2 = maxDist * maxDist;

	if (dist2 < maxDist2)
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::IsInsideSphere(Sphere const& sphere, glm::vec2 const& position)
{
    auto const sqrDist = glm::length2(position - sphere.center);
    if (sqrDist <= sphere.radius * sphere.radius)
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::IsInsideBox(Box const& box, glm::vec2 const& position)
{
    {
        glm::vec2 const normal = OrthogonalDirection(box.v1, box.v0, box.center);
        auto const dot = glm::dot(position - box.v0, normal);
        if (dot > 0)
        {
            return false;
        }
    }
    {
        glm::vec2 const normal = OrthogonalDirection(box.v2, box.v1, box.center);
        auto const dot = glm::dot(position - box.v1, normal);
        if (dot > 0)
        {
            return false;
        }
    }
    {
        glm::vec2 const normal = OrthogonalDirection(box.v3, box.v2, box.center);
        auto const dot = glm::dot(position - box.v2, normal);
        if (dot > 0)
        {
            return false;
        }
    }
    {
        glm::vec2 const normal = OrthogonalDirection(box.v0, box.v3, box.center);
        auto const dot = glm::dot(position - box.v3, normal);
        if (dot > 0)
        {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------

bool Physics2D::CheckForAABB_Collision(Entity const & item) const
{
    AABB2D const & aabb = item.aabb;
    for (auto const * other : _itemList)
    {
        if (other->id != item.id && (other->layer & item.layerMask) > 0)
        {
            if (other->aabb.Overlap(aabb) == true)
            {
                switch (other->type)
                {
	                case Type::AABB:
	                case Type::Box:
	                {
                        if (BoxAABB_Collision(other->box, item) == true)
                        {
                            return true;
                        }
		                break;
	                }
		            case Type::Sphere:
		            {
                        if (SphereBoxCollision(other->sphere, item.box) == true)
                        {
                            return true;
                        }
			            break;
		            }
	                default:
	                    MFA_LOG_ERROR("Unhandled collidetr type");
	            }
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::CheckForSphereCollision(Entity const& item) const
{
    for (auto const* other : _itemList)
    {
        if (other->id != item.id && (other->layer & item.layerMask) > 0)
        {
            if (other->aabb.Overlap(item.aabb) == true)
            {
                switch (other->type)
                {
                case Type::AABB:
                case Type::Box:
                {
                    if (SphereBoxCollision(item.sphere, other->box) == true)
                    {
                        return true;
                    }
                    break;
                }
                case Type::Sphere:
                {
                    if (SphereSphere_Collision(other->sphere, item.sphere) == true)
                    {
                        return true;
                    }
                    break;
                }
                default:
                    MFA_LOG_ERROR("Unhandled collidetr type");
                }
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------

bool Physics2D::CheckForBoxCollision(Entity const& item) const
{
    for (auto const* other : _itemList)
    {
        if (other->id != item.id && (other->layer & item.layerMask) > 0)
        {
            if (other->aabb.Overlap(item.aabb) == true)
            {
                switch (other->type)
                {
                case Type::AABB:
                {
                    if (BoxAABB_Collision(item.box, *other) == true)
                    {
                        return true;
                    }
                }
                case Type::Box:
                {
                    if (BoxBoxCollision(item.box, other->box) == true)
                    {
                        return true;
                    }
                    break;
                }
                case Type::Sphere:
                {
                    if (SphereBoxCollision(other->sphere, item.box) == true)
                    {
                        return true;
                    }
                    break;
                }
                default:
                    MFA_LOG_ERROR("Unhandled collidetr type");
                }
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------
