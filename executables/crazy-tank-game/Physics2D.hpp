#pragma once

#include "AABB2D.hpp"

#include <functional>
#include <glm/vec2.hpp>

//enum class Layers : uint32_t // We can have up to 32 layers
//{
//    Invalid = 1 << 0,
//    Wall = 1 << 1,
//    Tank = 1 << 2,
//    Shell = 1 << 3
//};

class Physics2D
{
public:

    using EntityID = uint32_t;

    using Layer = uint32_t;

    using OnHit = std::function<void(int)>;

    enum class Type
    {
        Invalid,
        AABB,
        Sphere,
        Polygon
    };

    explicit Physics2D();

    ~Physics2D();

    [[nodiscard]]
    EntityID Register(Type type, int layer, bool isStatic, OnHit onHit);

    bool UnRegister(EntityID id);

    bool MoveAABB(EntityID id, glm::vec2 const & min, glm::vec2 const & max);

    bool MoveSphere(EntityID id, glm::vec2 const & center, float radius);

    bool MoveBox(
        EntityID id, 
        glm::vec2 const& v0, 
		glm::vec2 const& v1, 
		glm::vec2 const& v2, 
		glm::vec2 const& v3
    );

    bool MovePolygon(EntityID id, std::vector<glm::vec2> const& vertices);

    struct HitInfo
    {
        int layer{};
        glm::vec2 hitPoint {};
        glm::vec2 hitNormal {};
        float hitDistance{};
        OnHit onHit{};
    };
    [[nodiscard]]
    bool Raycast(
        int layerMask,
        EntityID excludeId,
        glm::vec2 origin,
        glm::vec2 direction,
        float maxDistance,
        OnHit onHit
    );

    // TODO: Sphere cast

    // TODO: Raycast for moving polygons

    void Update();

    inline static Physics2D * Instance = nullptr;

private:

    [[nodiscard]]
    EntityID AllocateID();

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

    struct Polygon
    {
        std::vector<glm::vec2> vertices;
    };

    struct Item
    {
        EntityID id{};
        Type type = Type::Invalid;
        bool isStatic = true;
        AABB2D aabb{};
        OnHit onHit{};
        Layer layer {};
    	union 
        {
            Sphere sphere;
            Box box{};
            Polygon polygon;
        };
    };

    bool _isStaticGridDirty = false;
    bool _isNonStaticGridDirty = false;

    std::unordered_map<int, Item> _staticItemMap{};
    std::unordered_map<int, Item> _nonStaticItemMap{};

    std::vector<Item&> _staticItems{};
    std::vector<Item&> _nonStaticItems{};

    std::unordered_map<glm::ivec2, std::vector<Item&>> _nonStaticGrid{};
    std::unordered_map<glm::ivec2, std::vector<Item&>> _staticGrid{};

    EntityID _nextId{};
};
