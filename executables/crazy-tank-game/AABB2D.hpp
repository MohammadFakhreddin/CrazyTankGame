#pragma once

#include <glm.hpp>

class AABB2D
{
public:
      
    glm::dvec2 min{};
    glm::dvec2 max{};

    bool Overlap(AABB2D const& other);

    static void Min(glm::dvec2 const& a, glm::dvec2 const& b, glm::dvec2& outMin) ;

    static void Max(glm::dvec2 const& a, glm::dvec2 const& b, glm::dvec2& outMax) ;

private:

    static bool IsOverlapping1D(double const & min1, double const & max1, double const & min2, double const & max2);
};