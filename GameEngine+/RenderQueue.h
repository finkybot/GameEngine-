/////////////////////////////////
// RenderQueue.h - Manages drawable batching and depth-sorted rendering
// Systems enqueue drawables with depth info; the queue renders in order, batching where possible
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
/////////////////////////////////



/////////////////////////////////
// DrawRequest - A single drawable request with depth and rendering state info
struct DrawRequest {
	sf::Drawable* drawable = nullptr;
	std::shared_ptr<sf::Texture> texture;
	sf::BlendMode blendMode = sf::BlendAlpha;
	int depth = 0; // higher depth renders on top
	sf::RenderStates* states = nullptr; // optional custom render state

	DrawRequest() = default;
	DrawRequest(sf::Drawable* d, int z) : drawable(d), depth(z) {}
	DrawRequest(sf::Drawable* d, int z, const std::shared_ptr<sf::Texture>& tex)
		: drawable(d), texture(tex), depth(z) {}
	DrawRequest(sf::Drawable* d, int z, const std::shared_ptr<sf::Texture>& tex, sf::BlendMode blend)
		: drawable(d), texture(tex), blendMode(blend), depth(z) {}

	bool operator<(const DrawRequest& other) const {
		return depth < other.depth; // sort ascending (draw low-depth first)
	}
};
/////////////////////////////////



/////////////////////////////////
// RenderQueue - Collects drawable requests and renders them in sorted order
class RenderQueue {
public:
	RenderQueue() = default;
	~RenderQueue() = default;

	/////////////////////////////////
	// Enqueue a drawable request with depth info
	void Enqueue(sf::Drawable* drawable, int depth) {
		m_queue.emplace_back(drawable, depth);
	}

	void Enqueue(sf::Drawable* drawable, int depth, const std::shared_ptr<sf::Texture>& tex) {
		m_queue.emplace_back(drawable, depth, tex);
	}

	void Enqueue(sf::Drawable* drawable, int depth, const std::shared_ptr<sf::Texture>& tex, sf::BlendMode blend) {
		m_queue.emplace_back(drawable, depth, tex, blend);
	}

	void Enqueue(const DrawRequest& request) {
		m_queue.push_back(request);
	}
	/////////////////////////////////



	/////////////////////////////////
	// Flush the queue: sort by depth and render all drawables to the window
	void Flush(sf::RenderWindow& window) {
		// Sort by depth (ascending so lower depths render first)
		std::sort(m_queue.begin(), m_queue.end());

		for (const auto& req : m_queue) {
			if (!req.drawable) continue;

			sf::RenderStates states;
			if (req.texture) states.texture = req.texture.get();
			states.blendMode = req.blendMode;

			window.draw(*req.drawable, states);
		}

		m_queue.clear();
	}
	/////////////////////////////////



	/////////////////////////////////
	// Clear queue without rendering (e.g., on error or scene change)
	void Clear() {
		m_queue.clear();
	}

	size_t Size() const { return m_queue.size(); }
	/////////////////////////////////



private:
	std::vector<DrawRequest> m_queue;
};
/////////////////////////////////
