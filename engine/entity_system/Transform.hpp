#pragma once

#include "BedrockRotation.hpp"
#include "BedrockCommon.hpp"

#include <set>

namespace MFA
{
    class Transform
    {

    public:

        explicit Transform();

        void SetEulerAngles(glm::vec3 const & eulerAngles);

        void SetLocalQuaternion(glm::quat const & quaternion);

        [[nodiscard]]
        glm::mat4 const& LocalTransform();

    	[[nodiscard]]
    	glm::mat4 const& GlobalTransform();

        [[nodiscard]]
        Transform * Parent() const;

        std::set<Transform *> const & Children();

        void SetParent(Transform * parent);

    	void AddChild(Transform * child);

        void RemoveChild(Transform * child);
        // TODO: Cache when use without change
        [[nodiscard]]
        glm::vec3 Forward();

        [[nodiscard]]
        glm::vec3 Right();

        [[nodiscard]]
        glm::vec3 Up();

        [[nodiscard]]
        glm::vec3 & GlobalPosition();

        [[nodiscard]]
        Rotation & GlobalRotation();

    private:

        void SetLocalTransformDirty();

        void SetGlobalTransformDirty();

        // Local position
        MFA_VARIABLE3(LocalPosition, glm::vec3, glm::vec3(0.0f, 0.0f, 0.0f), SetLocalTransformDirty, m)
        // Local rotation
    	MFA_VARIABLE3(LocalRotation, Rotation, Rotation(glm::identity<glm::quat>()), SetLocalTransformDirty, m)
        // Local scale
    	MFA_VARIABLE3(LocalScale, glm::vec3, glm::vec3(1.0f, 1.0f, 1.0f), SetLocalTransformDirty, m)
        // Local extra transform
    	MFA_VARIABLE3(LocalExtraTransform, glm::mat4, glm::identity<glm::mat4>(), SetLocalTransformDirty, m)
        
        bool mIsLocalTransformDirty = true;
        bool mGlobalTransformDirty = true;

        // Note: Do not use this variable directly use the function instead
        glm::mat4 mLocalTransform{};
        // Note: Do not use this variable directly use the function instead
        glm::mat4 mGlobalTransform{};

        std::set<Transform *> mChildren{};
        Transform * mParent = nullptr;

        glm::vec3 mGlobalPosition{};
        bool mGlobalPositionDirty = true;

        Rotation mGlobalRotation{};
        bool mGlobalRotationDirty = true;

        glm::vec3 mForward{};
        bool mForwardDirty = true;

        glm::vec3 mRight{};
        bool mRightDirty = true;

        glm::vec3 mUp{};
        bool mUpDirty = true;

    };
}