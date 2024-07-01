#include "AABB2D.hpp"

#include <algorithm>

//-----------------------------------------------------------------------

bool AABB2D::Overlap(AABB2D const& other) const
{
    //https://stackoverflow.com/questions/20925818/algorithm-to-check-if-two-boxes-overlap
    return	IsOverlapping1D(min.x, max.x, other.min.x, other.max.x) &&
            IsOverlapping1D(min.y, max.y, other.min.y, other.max.y);
}

//-----------------------------------------------------------------------

bool AABB2D::Overlap(glm::vec2 const& position) const
{
    return position.x >= min.x && position.x <= max.x &&
        position.y >= min.y && position.y <= max.y;
}

//-----------------------------------------------------------------------

void AABB2D::Min(glm::vec2 const& a, glm::vec2 const& b, glm::vec2& outMin)
{
    outMin.x = std::min(a.x, b.x);
    outMin.y = std::min(a.y, b.y);
}

//-----------------------------------------------------------------------

void AABB2D::Max(glm::vec2 const& a, glm::vec2 const& b, glm::vec2& outMax)
{
    outMax.x = std::max(a.x, b.x);
    outMax.y = std::max(a.y, b.y);
}

//-----------------------------------------------------------------------

bool AABB2D::IsOverlapping1D(
    double const& min1, 
    double const& max1, 
	double const& min2, 
	double const& max2
)
{
    return max1 >= min2 && max2 >= min1;
}

//-----------------------------------------------------------------------
