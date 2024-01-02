#pragma once

#include "Physics2D.hpp"

namespace Layer
{
    static constexpr Physics2D::Layer WallLayer = 1 << 1;
    static constexpr Physics2D::Layer TankLayer = 1 << 2;
    static constexpr Physics2D::Layer ShellLayer = 1 << 3;
};