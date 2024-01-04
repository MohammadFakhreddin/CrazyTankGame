#pragma once

#include "AABB2D.hpp"

#include <functional>
#include <glm/vec2.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

// TODO: Instead of having static and non static, I can only have one instance or we can just use layers as separator
// TODO: Each layer probably should be on a separate map maybe just to do less query or maybe it may not help at all
class Physics2D
{
public:

    using EntityID = uint32_t;

    using Layer = uint32_t;

    using OnHit = std::function<void(int)>;

    enum class Type
    {
        Invalid,
        //AABB,
        Sphere,
        Box
        //Polygon
    };

private:

    struct Sphere
    {
        glm::vec2 center;
        float radius;
    };

    struct Box
    {
        glm::vec2 v0{};
        glm::vec2 v1{};
        glm::vec2 v2{};
        glm::vec2 v3{};
    };

    //struct Polygon
    //{
    //    glm::vec2 vertices;
    //};

    struct Item
    {
        explicit Item() {}
        ~Item() = default;

        EntityID id{};
        Type type = Type::Invalid;
        bool isStatic = true;
        AABB2D aabb{};
        OnHit onHit{};
        Layer layer{};
        union
        {
            Sphere sphere;
            Box box{};
        };
        //Polygon polygon;
    };

public:

    explicit Physics2D();

    ~Physics2D();

    [[nodiscard]]
    EntityID Register(Type type, int layer, bool isStatic, OnHit onHit);

    bool UnRegister(EntityID id);

    //bool MoveAABB(EntityID id, glm::vec2 const & min, glm::vec2 const & max);

    bool MoveSphere(EntityID id, glm::vec2 const & center, float radius);

    bool MoveBox(
        EntityID id, 
        glm::vec2 const& v0, 
		glm::vec2 const& v1, 
		glm::vec2 const& v2, 
		glm::vec2 const& v3
    );

    //bool MovePolygon(EntityID id, std::vector<glm::vec2> const& vertices);

    void Update();

    struct HitInfo
    {
        int layer{};
        glm::vec2 hitPoint {};
        glm::vec2 hitNormal {};
        float hitTime{};
        OnHit onHit{};
    };
    [[nodiscard]]
    bool Raycast(
        int layerMask,
        EntityID excludeId,
        glm::vec2 const & origin,
        glm::vec2 const & direction,
        float maxDistance,
        HitInfo& outHitInfo
    );

    // TODO: Sphere cast

    // TODO: Raycast for moving polygons
    
    inline static Physics2D * Instance = nullptr;

private:

    [[nodiscard]]
    EntityID AllocateID();

    [[nodiscard]]
    static glm::vec2 OrthogonalDirection(glm::vec2 const& direction);

    static bool RaySphereIntersection(
        glm::vec2 const& rayOrigin, 
        glm::vec2 const& rayDirection, 
		float rayMaxDistance, 
		Sphere const& sphere,
        float & outTime,
		glm::vec2 & outNormal
    );

    static bool RayBoxIntersection(
        glm::vec2 const& rayOrigin,
        glm::vec2 const& rayDirection,
        float rayMaxDistance,
        Box const& box,
        float & outTime,
        glm::vec2 & outNormal
    );

    static bool RayLineIntersection(
        glm::vec2 const& rayOrigin,
        glm::vec2 const& rayDirection,
        float rayMaxDistance,
        glm::vec2 const& lineV0,
        glm::vec2 const& lineV1,
        float& outTime,
        glm::vec2 & outNormal
    );
    
    bool _isStaticGridDirty = false;
    bool _isNonStaticGridDirty = false;

    std::unordered_map<EntityID, Item> _staticItemMap{};
    std::unordered_map<EntityID, Item> _nonStaticItemMap{};

    glm::vec2 _nonStaticCellSize{};
    std::unordered_map<glm::ivec2, std::vector<Item *>> _nonStaticGrid{};

    glm::vec2 _staticCellSize{};
    std::unordered_map<glm::ivec2, std::vector<Item *>> _staticGrid{};

    EntityID _nextId{};
};
