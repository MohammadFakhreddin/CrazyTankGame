#pragma once

#include "Physics2D.hpp"

namespace Layer
{
    static constexpr Physics2D::Layer Empty = 0;
    static constexpr Physics2D::Layer Wall = 1 << 1;
    static constexpr Physics2D::Layer Tank = 1 << 2;
    static constexpr Physics2D::Layer Bullet = 1 << 3;
};