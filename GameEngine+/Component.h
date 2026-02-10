#pragma once
#include <typeindex>
#include <map>
#include <memory>
#include <type_traits>

/// <summary>
/// Base class for all entity components
/// </summary>
class Component
{
public:
    virtual ~Component() = default;
};
