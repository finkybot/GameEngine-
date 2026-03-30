// CStatic.h - marker component to flag static (non-moving) entities such as tiles
#pragma once
#include "Component.h"

class CStatic : public Component
{
public:
    CStatic() = default;
    ~CStatic() override = default;
};
