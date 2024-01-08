#pragma once

#include <glm.hpp>

class AABB2D
{
public:
      
    glm::vec2 min{};
    glm::vec2 max{};

    [[nodiscard]]
    bool Overlap(AABB2D const& other) const;

    [[nodiscard]]
    bool Overlap(glm::vec2 const& position) const;

    static void Min(glm::vec2 const& a, glm::vec2 const& b, glm::vec2& outMin) ;

    static void Max(glm::vec2 const& a, glm::vec2 const& b, glm::vec2& outMax) ;

private:

    static bool IsOverlapping1D(double const & min1, double const & max1, double const & min2, double const & max2);
};