// ***** QuadTree.h - Header file for QuadTree class *****
#pragma once
#include "Vec2.h"
#include <array>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>

// Forward declarations.
class Vec2;

// BoundingBox struct represents an axis-aligned bounding box defined by its top-left and bottom-right corners.
// It provides methods for calculating the center point, checking if a point is contained within the box,
// and checking for intersection with another bounding box. The class uses half-open intervals [topLeft, bottomRight)
// for containment and intersection checks to avoid ambiguity on boundaries.
struct BoundingBox {
	Vec2 topLeft{Vec2::Zero};	  // Top-left corner of the bounding box, initialized to zero vector
	Vec2 bottomRight{Vec2::Zero}; // Bottom-right corner of the bounding box, initialized to zero vector

	BoundingBox() = default; // Default constructor - initializes to zero-sized box at origin
	BoundingBox(
		const Vec2& topLeft,
		const Vec2&
			bottomRight) // Constructor to initialize the bounding box with specific top-left and bottom-right points
	{
		this->topLeft = topLeft;
		this->bottomRight = bottomRight;
	}

	// Get the center point of the bounding box by averaging the x and y coordinates of the top-left and bottom-right corners.
	Vec2 GetCentrePoint() const noexcept {
		return Vec2((topLeft.GetX() + bottomRight.GetX()) * 0.5f, (topLeft.GetY() + bottomRight.GetY()) * 0.5f);
	}

	// Use half-open interval [topLeft, bottomRight) for ContainsPoint to avoid ambiguous membership on boundaries
	bool ContainsPoint(const Vec2& point) const noexcept {
		return point.GetX() >= topLeft.GetX() && point.GetX() < bottomRight.GetX() && point.GetY() >= topLeft.GetY() &&
			   point.GetY() < bottomRight.GetY();
	}

	// For half-open intervals, boxes that touch at edges do not intersect.
	bool Intersects(const BoundingBox& otherRect) const noexcept {
		return !(otherRect.bottomRight.GetX() <= topLeft.GetX() || otherRect.topLeft.GetX() >= bottomRight.GetX() ||
				 otherRect.bottomRight.GetY() <= topLeft.GetY() || otherRect.topLeft.GetY() >= bottomRight.GetY());
	}

	Vec2 GetTopLeftPoint() const noexcept { return topLeft; } // Getter for the top-left point of the bounding box
	Vec2 GetBottomRightPoint() const noexcept {
		return bottomRight;
	} // Getter for the bottom-right point of the bounding box
	float GetHeight() const noexcept {
		return bottomRight.GetY() - topLeft.GetY();
	} // Getter for the height of the bounding box, calculated as the difference in y-coordinates between the bottom-right and top-left points
	float GetWidth() const noexcept {
		return bottomRight.GetX() - topLeft.GetX();
	} // Getter for the width of the bounding box, calculated as the difference in x-coordinates between the bottom-right and top-left points
};

// QuadTree class is a spatial partitioning data structure that recursively subdivides a 2D space into four quadrants (child nodes) to efficiently manage and query objects based on their spatial location.
template <typename T>
class QuadTree {
	// ***** Private Member Variables *****
private:
	BoundingBox m_boundary; // The bounding box that defines the area covered by this QuadTree node
	unsigned int
		m_capacity; // The maximum number of objects this node can hold before it needs to subdivide into child nodes
	std::vector<T*> m_objects;	 // The objects contained in this QuadTree node (raw pointers, not owned by QuadTree)
	bool m_isSubDivided = false; // Flag to indicate whether this node has been subdivided into child nodes
	std::array<std::unique_ptr<QuadTree<T>>, 4> m_childTreePtrs{
		{nullptr, nullptr, nullptr,
		 nullptr}}; // Unique pointers to the four child QuadTree nodes (top-left, top-right, bottom-left, bottom-right), initialized to nullptr
	unsigned int m_depth =
		0; // The depth of this node in the QuadTree hierarchy, with the root node at depth 0. This is used to limit the maximum depth of subdivision and prevent infinite recursion.
	unsigned int m_maxDepth = std::numeric_limits<unsigned int>::
		max(); // The maximum depth allowed for subdivision of the QuadTree. This prevents infinite subdivision and can be set to a specific value based on the expected density of objects and performance requirements.
	QuadTree<T>* m_parent =
		nullptr; // Pointer to the parent QuadTree node, used for potential upward traversal if needed (not currently utilized in this implementation but can be useful for certain operations like merging child nodes back into a parent)

	// Query instrumentation
	inline static size_t s_totalQueriesThisFrame =
		0; // Static variable to track the total number of queries made in the current frame, used for performance monitoring and debugging purposes. It is incremented each time a query is performed on the QuadTree, allowing developers to analyze how many spatial queries are being made and optimize accordingly.
	inline static size_t s_totalObjectsQueried =
		0; // Static variable to track the total number of objects queried in the current frame, used for performance monitoring and debugging purposes.
	inline static size_t s_totalNodesVisited =
		0; // Static variable to track the total number of QuadTree nodes visited during queries in the current frame, used for performance monitoring and debugging purposes. It is incremented each time a node is visited during a query, allowing developers to analyze the efficiency of the QuadTree structure and optimize it if necessary.
	inline static size_t s_queryCount =
		0; // Static variable to track the total number of queries performed on the QuadTree, used for performance monitoring and debugging purposes. It is incremented each time a query is performed, allowing developers to analyze how many spatial queries are being made and optimize accordingly.

	// Helper function to insert raw pointer
	bool InsertRawPointer(
		T* object) // This method attempts to insert an object into the QuadTree. It first checks if the object's center point is within the boundary of this node. If it is not, the method returns false, indicating that the object cannot be inserted into this node. If the object is within the boundary and there is capacity in this node (i.e., the number of objects currently in this node is less than the defined capacity), the object is added to this node's list of objects and the method returns true. If there is no capacity and this node has not yet been subdivided, it will subdivide itself into four child nodes (if it has not reached the maximum depth) and then attempt to insert the object into one of the child nodes. If the object cannot be inserted into any of the child nodes (e.g., if it lies on a boundary or if all child nodes are at capacity), it will be added to this node's list of objects as a fallback.
	{
		// Check if the object's center point is within the boundary of this QuadTree node. If it is not, return false to indicate that the object cannot be inserted into this node.
		if (!m_boundary.ContainsPoint(object->GetCentrePoint()))
			return false;

		// If there is capacity in this node (i.e., the number of objects currently in this node is less than the defined capacity), add the object to this node's list of objects and return true to indicate successful insertion.
		if (m_objects.size() < m_capacity) {
			m_objects.push_back(object);
			return true;
		} else // If there is no capacity and this node has not yet been subdivided, it will subdivide itself into four child nodes (if it has not reached the maximum depth) and then attempt to insert the object into one of the child nodes. If the object cannot be inserted into any of the child nodes (e.g., if it lies on a boundary or if all child nodes are at capacity), it will be added to this node's list of objects as a fallback.
		{
			// If this node is not already subdivided, check if we can subdivide further based on the maximum depth. If we can, call SubDivide() to create child nodes. If we cannot subdivide further (i.e., we have reached the maximum depth), add the object to this node's list of objects and return true.
			if (!m_isSubDivided) {
				// Check if we can subdivide further based on the maximum depth. If we can, call SubDivide() to create child nodes. If we cannot subdivide further (i.e., we have reached the maximum depth), add the object to this node's list of objects and return true.
				if (m_depth < m_maxDepth)
					SubDivide();
				else // If we cannot subdivide further (i.e., we have reached the maximum depth), add the object to this node's list of objects and return true.
				{
					m_objects.push_back(object);
					return true;
				}
			}

			// If this node is already subdivided, attempt to insert the object into one of the child nodes. If the object cannot be inserted into any of the child nodes (e.g., if it lies on a boundary or if all child nodes are at capacity), add the object to this node's list of objects as a fallback and return true.
			if (m_isSubDivided) {
				// Attempt to insert the object into one of the child nodes. If the object cannot be inserted into any of the child nodes (e.g., if it lies on a boundary or if all child nodes are at capacity), add the object to this node's list of objects as a fallback and return true.
				for (auto& child : m_childTreePtrs) {
					if (child && child->InsertRawPointer(object))
						return true;
				}
				m_objects.push_back(
					object); // If the object cannot be inserted into any of the child nodes (e.g., if it lies on a boundary or if all child nodes are at capacity), add the object to this node's list of objects as a fallback and return true.
				return true;
			}
		}
		return false; // This line should never be reached, but it is included to satisfy the compiler's requirement for a return statement. It returns false by default, but in practice, the method will return true if the object is successfully inserted into this node or one of its child nodes, or if it is added to this node's list of objects as a fallback.
	}

	// Fast removal: swap with last, then pop
	void RemoveObjectFast(T* object) noexcept {
		auto it = std::find(
			m_objects.begin(), m_objects.end(),
			object); // Find the object in the list of objects for this node. If it is found, swap it with the last object in the list and then remove the last object (which is now the target object) from the list. This allows for fast removal without needing to shift elements in the vector, but it does not preserve the order of objects in the list.

		// If the object is found in the list of objects for this node, swap it with the last object in the list and then remove the last object (which is now the target object) from the list. This allows for fast removal without needing to shift elements in the vector, but it does not preserve the order of objects in the list.
		if (it != m_objects.end()) {
			std::swap(*it, m_objects.back());
			m_objects.pop_back();
		}
	}

	// ***** Public Methods *****
public:
	QuadTree()
		: m_boundary(BoundingBox(Vec2::Zero, Vec2::Zero)), m_capacity(1), m_depth(0),
		  m_maxDepth(std::numeric_limits<unsigned int>::max()) {}

	QuadTree(BoundingBox Boundary, unsigned int n, unsigned int depth = 0,
			 unsigned int maxDepth = std::numeric_limits<unsigned int>::max())
		: m_boundary(Boundary), m_capacity(std::max<unsigned int>(1u, n)), m_depth(depth), m_maxDepth(maxDepth) {}

	~QuadTree() { ClearTree(); }

	void ClearTree() {
		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->ClearTree();
				child.reset();
			}
			m_isSubDivided = false;
		}
		m_objects.clear();
	}

	void GetBoundary(std::vector<BoundingBox>& rBoundaries) {
		rBoundaries.push_back(m_boundary);
		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->GetBoundary(rBoundaries);
			}
		}
	}

	bool Insert(T* object) { return InsertRawPointer(object); }

	void SubDivide() {
		if (m_isSubDivided || m_depth >= m_maxDepth)
			return;

		const Vec2 centre = m_boundary.GetCentrePoint();
		const Vec2& tl = m_boundary.topLeft;
		const Vec2& br = m_boundary.bottomRight;

		m_childTreePtrs[0] =
			std::make_unique<QuadTree<T>>(BoundingBox(Vec2(tl.GetX(), tl.GetY()), Vec2(centre.GetX(), centre.GetY())),
										  m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[1] =
			std::make_unique<QuadTree<T>>(BoundingBox(Vec2(centre.GetX(), tl.GetY()), Vec2(br.GetX(), centre.GetY())),
										  m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[2] =
			std::make_unique<QuadTree<T>>(BoundingBox(Vec2(tl.GetX(), centre.GetY()), Vec2(centre.GetX(), br.GetY())),
										  m_capacity, m_depth + 1, m_maxDepth);
		m_childTreePtrs[3] =
			std::make_unique<QuadTree<T>>(BoundingBox(Vec2(centre.GetX(), centre.GetY()), Vec2(br.GetX(), br.GetY())),
										  m_capacity, m_depth + 1, m_maxDepth);

		for (auto& child : m_childTreePtrs) {
			if (child)
				child->m_parent = this;
		}

		m_isSubDivided = true;

		if (!m_objects.empty()) {
			std::vector<T*> remaining;
			remaining.reserve(m_objects.size());
			for (auto obj : m_objects) {
				bool moved = false;
				for (auto& child : m_childTreePtrs) {
					if (child && child->InsertRawPointer(obj)) {
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

	bool Query(std::vector<T*>& found, const BoundingBox& rQueryArea) {
		if (!m_boundary.Intersects(rQueryArea))
			return false;

		++s_totalNodesVisited;

		for (T* object : m_objects) {
			if (rQueryArea.ContainsPoint(object->GetCentrePoint())) {
				found.push_back(object);
				++s_totalObjectsQueried;
			}
		}

		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->Query(found, rQueryArea);
			}
		}

		return true;
	}

	bool Query(std::vector<T*>& found, const BoundingBox& rQueryArea, const T* rCurrentObj) {
		if (!m_boundary.Intersects(rQueryArea))
			return false;

		++s_totalNodesVisited;

		for (T* object : m_objects) {
			if (object != rCurrentObj && rQueryArea.ContainsPoint(object->GetCentrePoint())) {
				found.push_back(object);
				++s_totalObjectsQueried;
			}
		}

		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->Query(found, rQueryArea, rCurrentObj);
			}
		}
		return true;
	}

	bool Scan(T&, size_t value) { return false; }

	size_t Size() {
		size_t size = m_objects.size();
		if (!m_isSubDivided)
			return size;

		for (auto& child : m_childTreePtrs) {
			if (child)
				size += child->Size();
		}
		return size;
	}

	static void ResetQueryStats() {
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
	static double GetAverageObjectsPerQuery() {
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}

	static void ReserveQueryCapacity(std::vector<T*>& found, size_t entityCount) {
		found.reserve(std::max(size_t(16), entityCount / 100));
	}

	bool UpdatePosition(T* object) {
		const Vec2 objectPos = object->GetCentrePoint();

		if (m_boundary.ContainsPoint(objectPos)) {
			if (m_isSubDivided) {
				for (auto& child : m_childTreePtrs) {
					if (child && child->m_boundary.ContainsPoint(objectPos)) {
						RemoveObjectFast(object);
						return child->UpdatePosition(object);
					}
				}
			}

			auto it = std::find(m_objects.begin(), m_objects.end(), object);
			if (it == m_objects.end())
				m_objects.push_back(object);
			return true;
		} else {
			RemoveObjectFast(object);

			if (m_parent)
				return m_parent->UpdatePosition(object);
			else
				return InsertRawPointer(object);
		}
	}

	// Recursively remove entity from tree - CRITICAL for cleaning up dead entities
	bool RemoveEntityFromTree(T* object) {
		RemoveObjectFast(object);

		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->RemoveEntityFromTree(object);
			}
		}

		return true;
	}
};
