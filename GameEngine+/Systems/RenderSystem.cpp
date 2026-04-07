// RenderSystem.cpp
#include "RenderSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include "../CTransform.h"
#include "../CText.h"
#include "../FontManager.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>

void RenderSystem::RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window)
{
    // Backwards-compatible: render shapes then text if configured.
    RenderShapes(entities, window);
    if (m_fontManager) RenderText(entities, window);
}

void RenderSystem::RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window, RenderSystem::RenderMode mode)
{
    // Always render shapes
    RenderShapes(entities, window);
    if (mode == RenderMode::ShapesThenText)
    {
        if (m_fontManager) RenderText(entities, window);
    }
    // ShapesThenTextAfterOverlays intentionally does not render text here so caller can draw overlays first
}

void RenderSystem::RenderEntity(Entity* entity, sf::RenderWindow& window) const
{
    if (auto shape = entity->GetComponent<CShape>())
    {
        auto transform = entity->GetComponent<CTransform>();
        if (transform)
        {
            shape->GetShape().setPosition(sf::Vector2f(transform->m_position.x, transform->m_position.y));
        }
        window.draw(shape->GetShape());
    }
}

void RenderSystem::RenderShapes(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window)
{
    for (const auto& entity : entities)
    {
        if (!entity->IsAlive()) continue;
        RenderEntity(entity.get(), window);
    }
}

void RenderSystem::RenderText(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) const
{
    if (!m_fontManager) return;
    for (const auto& entity : entities)
    {
        if (!entity->IsAlive()) continue;
        RenderTextEntity(entity.get(), window);
    }
}

// RenderTextEntity: isolated text rendering logic.
// - Looks up CText component
// - Acquires the font from FontManager (optional shared_ptr wrapped in std::optional)
// - Builds an sf::Text, applies alignment, position and color, then draws to the window.
void RenderSystem::RenderTextEntity(Entity* entity, sf::RenderWindow& window) const
{
    //Guard clause: if no CText component or if text is not visible, skip rendering.
    if (!entity) return;

    // Get CText component and check visibility; if no text component or if text is not visible, skip rendering, and get out of here.
    auto txt = entity->GetComponent<CText>();
    if (!txt || !txt->visible) return;

    // Resolve font via the configured FontManager
    if (!m_fontManager) return;
    auto fontOpt = m_fontManager->GetFont(txt->fontKey);
    if (!fontOpt.has_value() || !(*fontOpt))
    {
        // Font not found - nothing to draw or fallback behavior could be added here.
        return;
    }
    std::shared_ptr<sf::Font> font = *fontOpt;

    // Build sf::Text
    sf::Text sfTxt(*font);
    sfTxt.setString(txt->text);
    sfTxt.setCharacterSize(txt->charSize);
    sfTxt.setFillColor(txt->color);

    // Determine world position: prefer transform position, fall back to entity centre
    Vec2 worldPos = entity->GetCentrePoint();
    if (auto transform = entity->GetComponent<CTransform>(); transform)
    {
        worldPos = transform->m_position;
    }
    // Apply offset from CText
    sf::Vector2f pos(worldPos.x + txt->offset.x, worldPos.y + txt->offset.y);
    sfTxt.setPosition(pos);

    // Horizontal alignment: measure local bounds and set origin appropriately
    sf::FloatRect bounds = sfTxt.getLocalBounds();
    
    // Set origin based on alignment. SFML 3.0 changed setOrigin to take a Vector2f for the origin point, so we calculate the appropriate origin based on the desired alignment and the text bounds.
    if (txt->align == CText::Align::Center)
    {
        sfTxt.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f));
    }
    else if (txt->align == CText::Align::Right)
    {
        sfTxt.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x, bounds.position.y));
    }
    else
    {
        sfTxt.setOrigin(sf::Vector2f(bounds.position.x, bounds.position.y + bounds.size.y * 0.0f));
    }

    window.draw(sfTxt);
}

