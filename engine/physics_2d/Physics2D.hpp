#pragma once

#include "AABB2D.hpp"

#include <functional>
#include <glm/vec2.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <set>

#include "glm/gtx/hash.hpp"
#include "utils/LineRenderer.hpp"
#include "utils/PointRenderer.hpp"

// TODO: Instead of having static and non static, I can only have one instance or we can just use layers as separator
// TODO: Each layer probably should be on a separate map maybe just to do less query or maybe it may not help at all
class Physics2D
{
public:

    using EntityID = uint32_t;

    using Layer = uint32_t;

    using OnHit = std::function<void(Layer)>;

    enum class Type
    {
        Invalid,
        AABB,
        Sphere,
        Box
        //Polygon
    };

    struct Ray
    {
        glm::vec2 origin{};
        glm::vec2 direction{};
    };

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

        glm::vec2 center{};

        glm::vec2 v0v1N{};
        bool v0v1N_dirty = true;
        
        glm::vec2 v1v2N{};
        bool v1v2N_dirty = true;
        
        glm::vec2 v2v3N{};
        bool v2v3N_dirty = true;
    
        glm::vec2 v3v0N{};
        bool v3v0N_dirty = true;
    };

private:

    //struct Polygon
    //{
    //    glm::vec2 vertices;
    //};

    struct Entity
    {
        explicit Entity() {}
        ~Entity() = default;

        EntityID id{};
        Type type = Type::Invalid;
        AABB2D aabb{
            .min = {-1000.0f, -1000.0f},
            .max = {-1000.0f, -1000.0f}
        };
        OnHit onHit{};
        Layer layer{};
        Layer layerMask{};          // Layers that the entity collide with
        union
        {
            Sphere sphere;
            Box box{
                .v0 = {-10000.0f, -10000.0f},
                .v1 = {-10000.0f, -10000.0f},
                .v2 = {-10000.0f, -10000.0f},
                .v3 = {-10000.0f, -10000.0f}
            };
        };
        //Polygon polygon;
    };

public:

    explicit Physics2D(
        std::shared_ptr<MFA::PointRenderer> pointRenderer,
        std::shared_ptr<MFA::LineRenderer> lineRenderer
    );

    ~Physics2D();

    [[nodiscard]]
    EntityID Register(
        Type type, 
        Layer layer,                // Entity layer
        Layer layerMask,            // Layers that the entity collide with
        OnHit onHit
    );

    bool UnRegister(EntityID id);

    // Returns false if collision detected unless the object is static
    bool MoveAABB(
        EntityID id, 
        glm::vec2 const & min, 
		glm::vec2 const & max, 
		bool checkForCollision = true
    );
    
	// Returns false if collision detected unless the object is static
    bool MoveSphere(
        EntityID id, 
        glm::vec2 const & center, 
		float radius, 
		bool checkForCollision = true
    );

	// Returns false if collision detected unless the object is static
    bool MoveBox(
        EntityID id, 
        glm::vec2 const& v0, 
		glm::vec2 const& v1, 
		glm::vec2 const& v2, 
		glm::vec2 const& v3,
        bool checkForCollision = true
    );

    //bool MovePolygon(EntityID id, std::vector<glm::vec2> const& vertices);

    void Update();

    void Render(MFA::RT::CommandRecordState& recordState);

    struct HitInfo
    {
        EntityID entityId{};
        int layer{};
        glm::vec2 hitPoint {};
        glm::vec2 hitNormal {};
        float hitTime{};            // Hit time is between 0 to 1
        OnHit onHit{};
    };
    [[nodiscard]]
    bool Raycast(
        Layer layerMask,
        std::set<EntityID> const & excludeIds,
        Ray const & ray,
        float maxDistance,
        HitInfo& outHitInfo
    );

    //[[nodiscard]]
    //bool HasCollision(
    //    glm::vec2 const& position,
    //    EntityID excludeId,
    //    Layer layerMask
    //) const;
    
    inline static Physics2D * Instance = nullptr;

private:

    [[nodiscard]]
    EntityID AllocateID();

    [[nodiscard]]
    static glm::vec2 OrthogonalDirection(glm::vec2 const& v0, glm::vec2 const& v1, glm::vec2 const& center);

    [[nodiscard]]
    static void OrthogonalDirection(
        bool & inOutIsDirty, 
        glm::vec2 & inOutNormal, 
        glm::vec2 const& v0, 
        glm::vec2 const& v1, 
        glm::vec2 const& center
    );

    [[nodiscard]]
    static bool RaySphereIntersection(
        Ray const & ray,
		float rayMaxDistance, 
		Sphere const& sphere,
        float & outTime,            // Hit time is between 0 to 1
		glm::vec2 & outNormal
    );

    [[nodiscard]]
    static bool RayBoxIntersection(
        Ray const& ray,
        float rayMaxDistance,
        Box & box,
        float & outTime,            // Hit time is between 0 to 1
        glm::vec2 & outNormal
    );

    [[nodiscard]]
    static bool RayLineIntersection(
        Ray const& ray,
        float rayMaxDistance,
        glm::vec2 const& lineV0,
        glm::vec2 const& lineV1,
        glm::vec2 const& lineNormal,
        float& outTime
    );

    [[nodiscard]]
    static bool BoxAABB_Collision(
		Box & box,
        Entity const & aabbEntity
    );

    [[nodiscard]]
    static bool SphereBoxCollision(
		Sphere const & sphere,
        Box & box
    );

    [[nodiscard]]
    static bool BoxBoxCollision(
		Box & box0,
        Box & box1
    );

    [[nodiscard]]
    static bool SphereSphere_Collision(
        Sphere const& sphere0,
        Sphere const& sphere1
    );

    // TODO
  //  static bool SphereAABB_Collision(
		//Sphere const & sphere,
  //      AABB2D const & aabb
  //  );

    [[nodiscard]]
    static bool IsInsideSphere(
        Sphere const & sphere, 
        glm::vec2 const & position
    );

    [[nodiscard]]
    static bool IsInsideBox(
		Box & box,
        glm::vec2 const & position,
        glm::vec2 & outClosestWallNormal
    );

    [[nodiscard]]
    static bool IsInsideBox(
		Box & box,
        glm::vec2 const & position
    );

    [[nodiscard]]
    bool CheckForAABB_Collision(Entity & item) const;

    [[nodiscard]]
    bool CheckForSphereCollision(Entity const& item) const;

    [[nodiscard]]
    bool CheckForBoxCollision(Entity & item) const;

	std::unordered_map<EntityID, Entity> _itemMap{};
    bool _isMapDirty = false;

    std::vector<Entity*> _itemList{};

    EntityID _nextId{};

    std::shared_ptr<MFA::PointRenderer> _pointRenderer{};
    std::shared_ptr<MFA::LineRenderer> _lineRenderer{};
};
