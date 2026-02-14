#pragma once
#include "Vec2.h"
#include <array>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>

// Forward declarations.
class Vec2;

struct BoundingBox
{
	Vec2 topLeft{ Vec2::Zero };
	Vec2 bottomRight{ Vec2::Zero };

	BoundingBox() = default;
	BoundingBox(const Vec2& topLeft, const Vec2& bottomRight)
	{
		this->topLeft = topLeft;
		this->bottomRight = bottomRight;
	}

	Vec2 GetCentrePoint() const	noexcept { return Vec2((topLeft.GetX() + bottomRight.GetX()) * 0.5f, (topLeft.GetY() + bottomRight.GetY()) * 0.5f); }

	// Use half-open interval [topLeft, bottomRight) for ContainsPoint to avoid ambiguous membership on boundaries
	bool ContainsPoint(const Vec2& point) const noexcept
	{
		return point.GetX() >= topLeft.GetX() && point.GetX() < bottomRight.GetX()
			&& point.GetY() >= topLeft.GetY() && point.GetY() < bottomRight.GetY();
	}

	// For half-open intervals, boxes that touch at edges do not intersect.
	bool Intersects(const BoundingBox& otherRect) const noexcept
	{
		return !(otherRect.bottomRight.GetX() <= topLeft.GetX() ||
				otherRect.topLeft.GetX() >= bottomRight.GetX() ||
				otherRect.bottomRight.GetY() <= topLeft.GetY() ||
				otherRect.topLeft.GetY() >= bottomRight.GetY());
	}

	Vec2 GetTopLeftPoint() const noexcept { return topLeft; }
	Vec2 GetBottomRightPoint() const noexcept { return bottomRight; }

	float GetHeight() const noexcept { return bottomRight.GetY() - topLeft.GetY(); }
	float GetWidth() const noexcept { return bottomRight.GetX() - topLeft.GetX(); }
};

/// <summary>
/// DynamicQuadTree Template class.
/// Stores raw pointers (borrowed references) to objects owned elsewhere.
/// </summary>
/// <typeparam name="T">Template class object (game objects)</typeparam>
template<typename T>
class QuadTree
{
private:
	BoundingBox						m_boundary;
	unsigned int					m_capacity;
	std::vector<T*>					m_objects;
	bool							m_isSubDivided = false;
	std::array<std::unique_ptr<QuadTree<T>>, 4> m_childTreePtrs{{ nullptr, nullptr, nullptr, nullptr }};
	unsigned int					m_depth = 0;
	unsigned int					m_maxDepth = std::numeric_limits<unsigned int>::max();
	QuadTree<T>*					m_parent = nullptr;

	// Query instrumentation
	inline static size_t			s_totalQueriesThisFrame = 0;
	inline static size_t			s_totalObjectsQueried = 0;
	inline static size_t			s_totalNodesVisited = 0;
	inline static size_t			s_queryCount = 0;

	// Helper to insert raw pointer
	bool InsertRawPointer(T* object)
	{
		if (!m_boundary.ContainsPoint(object->GetCentrePoint()))
			return false;

		if (m_objects.size() < m_capacity)
		{
			m_objects.push_back(object);
			return true;
		}
		else
		{
			if (!m_isSubDivided)
			{
				if (m_depth < m_maxDepth)
					SubDivide();
				else
				{
					m_objects.push_back(object);
					return true;
				}
			}

			if (m_isSubDivided)
			{
				for (auto& child : m_childTreePtrs)
				{
					if (child && child->InsertRawPointer(object))
						return true;
				}
				m_objects.push_back(object);
				return true;
			}
		}
		return false;
	}

	// Fast removal: swap with last, then pop
	void RemoveObjectFast(T* object) noexcept
	{
		auto it = std::find(m_objects.begin(), m_objects.end(), object);
		if (it != m_objects.end())
		{
			std::swap(*it, m_objects.back());
			m_objects.pop_back();
		}
	}

public:
	QuadTree() : m_boundary(BoundingBox(Vec2::Zero, Vec2::Zero)), m_capacity(1), m_depth(0), m_maxDepth(std::numeric_limits<unsigned int>::max()) {}

	QuadTree(BoundingBox Boundary, unsigned int n, unsigned int depth = 0, unsigned int maxDepth = std::numeric_limits<unsigned int>::max())
		: m_boundary(Boundary), m_capacity(std::max<unsigned int>(1u, n)), m_depth(depth), m_maxDepth(maxDepth) {}

	~QuadTree() { ClearTree(); }

	void ClearTree()
	{
		if (m_isSubDivided)
		{
			for (auto& child : m_childTreePtrs)
			{
				if (child)
					child->ClearTree();
				child.reset();
			}
			m_isSubDivided = false;
		}
		m_objects.clear();
	}

	void GetBoundary(std::vector<BoundingBox>& rBoundaries)
	{
		rBoundaries.push_back(m_boundary);
		if (m_isSubDivided)
		{
			for (auto& child : m_childTreePtrs)
			{
				if (child)
					child->GetBoundary(rBoundaries);
			}
		}
	}

	bool Insert(T* object)
	{
		return InsertRawPointer(object);
	}

	void SubDivide()
	{
		if (m_isSubDivided || m_depth >= m_maxDepth) return;

		const Vec2 centre = m_boundary.GetCentrePoint();
		const Vec2& tl = m_boundary.topLeft;
		const Vec2& br = m_boundary.bottomRight;

		m_childTreePtrs[0] = std::make_unique<QuadTree<T>>(BoundingBox(Vec2(tl.GetX(), tl.GetY()), Vec2(centre.GetX(), centre.GetY())), m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[1] = std::make_unique<QuadTree<T>>(BoundingBox(Vec2(centre.GetX(), tl.GetY()), Vec2(br.GetX(), centre.GetY())), m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[2] = std::make_unique<QuadTree<T>>(BoundingBox(Vec2(tl.GetX(), centre.GetY()), Vec2(centre.GetX(), br.GetY())), m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[3] = std::make_unique<QuadTree<T>>(BoundingBox(Vec2(centre.GetX(), centre.GetY()), Vec2(br.GetX(), br.GetY())), m_capacity, m_depth + 1, m_maxDepth);

		for (auto& child : m_childTreePtrs)
		{
			if (child)
				child->m_parent = this;
		}

		m_isSubDivided = true;

		if (!m_objects.empty())
		{
			std::vector<T*> remaining;
			remaining.reserve(m_objects.size());
			for (auto obj : m_objects)
			{
				bool moved = false;
				for (auto& child : m_childTreePtrs)
				{
					if (child && child->InsertRawPointer(obj))
					{
						moved = true;
						break;
					}
				}
				if (!moved)
					remaining.push_back(obj);
			}
			m_objects.swap(remaining);
		}
	}

	bool Query(std::vector<T*>& found, const BoundingBox& rQueryArea)
	{
		if (!m_boundary.Intersects(rQueryArea))
			return false;

		++s_totalNodesVisited;

		for (T* object : m_objects)
		{
			if (rQueryArea.ContainsPoint(object->GetCentrePoint()))
			{
				found.push_back(object);
				++s_totalObjectsQueried;
			}
		}

		if (m_isSubDivided)
		{
			for (auto& child : m_childTreePtrs)
			{
				if (child)
					child->Query(found, rQueryArea);
			}
		}

		return true;
	}

	bool Query(std::vector<T*>& found, const BoundingBox& rQueryArea, const T* rCurrentObj)
	{
		if (!m_boundary.Intersects(rQueryArea))
			return false;

		++s_totalNodesVisited;

		for (T* object : m_objects)
		{
			if (object != rCurrentObj && rQueryArea.ContainsPoint(object->GetCentrePoint()))
			{
				found.push_back(object);
				++s_totalObjectsQueried;
			}
		}

		if (m_isSubDivided)
		{
			for (auto& child : m_childTreePtrs)
			{
				if (child)
					child->Query(found, rQueryArea, rCurrentObj);
			}
		}
		return true;
	}

	bool Scan(T&, size_t value) { return false; }

	size_t Size()
	{
		size_t size = m_objects.size();
		if (!m_isSubDivided) return size;

		for (auto& child : m_childTreePtrs)
		{
			if (child) size += child->Size();
		}
		return size;
	}

	static void ResetQueryStats()
	{
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_totalNodesVisited = 0;
		s_queryCount = 0;
	}

	static void IncrementQueryCount() { ++s_queryCount; }
	static size_t GetTotalQueries() { return s_totalQueriesThisFrame; }
	static size_t GetTotalObjectsQueried() { return s_totalObjectsQueried; }
	static size_t GetTotalNodesVisited() { return s_totalNodesVisited; }
	static size_t GetQueryCount() { return s_queryCount; }
	static double GetAverageObjectsPerQuery() 
	{ 
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}

	static void ReserveQueryCapacity(std::vector<T*>& found, size_t entityCount)
	{
		found.reserve(std::max(size_t(16), entityCount / 100));
	}

	bool UpdatePosition(T* object)
	{
		const Vec2 objectPos = object->GetCentrePoint();

		if (m_boundary.ContainsPoint(objectPos))
		{
			if (m_isSubDivided)
			{
				for (auto& child : m_childTreePtrs)
				{
					if (child && child->m_boundary.ContainsPoint(objectPos))
					{
						RemoveObjectFast(object);
						return child->UpdatePosition(object);
					}
				}
			}

			auto it = std::find(m_objects.begin(), m_objects.end(), object);
			if (it == m_objects.end())
				m_objects.push_back(object);
			return true;
		}
		else
		{
			RemoveObjectFast(object);

			if (m_parent)
				return m_parent->UpdatePosition(object);
			else
				return InsertRawPointer(object);
		}
	}

	// Recursively remove entity from tree - CRITICAL for cleaning up dead entities
	bool RemoveEntityFromTree(T* object)
	{
		RemoveObjectFast(object);

		if (m_isSubDivided)
		{
			for (auto& child : m_childTreePtrs)
			{
				if (child)
					child->RemoveEntityFromTree(object);
			}
		}

		return true;
	}
};		
