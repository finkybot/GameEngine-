// CLayer.h - Render layer component
#pragma once
#include "Component.h"

// Small render-layer component to tag entities with a rendering layer.
// This keeps rendering concerns in data components and allows systems to
// change layers without touching entity internals.
class CLayer : public Component {
public:
    enum class Layer { Background = 0, Mid = 1, Foreground = 2, Overlay = 3 };
    Layer m_layer = Layer::Mid;

    CLayer() = default;
    explicit CLayer(Layer layer) : m_layer(layer) {}
};
