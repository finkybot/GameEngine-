/////////////////////////////////
// QuadTree.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
#include <array>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>
/////////////////////////////////



/////////////////////////////////
// Forward declarations.
class Vec2;
/////////////////////////////////



/////////////////////////////////
// BoundingBox struct - Represents an axis-aligned bounding box defined by its top-left and bottom-right corners. It provides methods for calculating the center point, checking if a point is contained within the box, and checking for intersection with another bounding box. 
// The class uses half-open intervals [topLeft, bottomRight) for containment and intersection checks to avoid ambiguity on boundaries.
struct BoundingBox {
	/////////////////////////////////
	// Member variables for the top-left and bottom-right corners of the bounding box. These are initialized to zero vectors by default, representing a degenerate bounding box at the origin.
	Vec2 topLeft{Vec2::Zero};
	Vec2 bottomRight{Vec2::Zero};
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the BoundingBox struct. The default constructor initializes the bounding box to a zero-sized box at the origin, while the parameterized constructor allows for initializing the bounding box with specific top-left and bottom-right points.
	BoundingBox() = default;
	BoundingBox(const Vec2& topLeft, const Vec2& bottomRight)
	{
		this->topLeft = topLeft;
		this->bottomRight = bottomRight;
	}
	/////////////////////////////////



	/////////////////////////////////
	// GetCentrePoint - Calculates and returns the center point of the bounding box by averaging the x and y coordinates of the top-left and bottom-right corners. This method provides a convenient way to get the central position of the bounding box, 
	// which can be useful for various spatial calculations and queries.
	Vec2 GetCentrePoint() const noexcept {
		return Vec2((topLeft.GetX() + bottomRight.GetX()) * 0.5f, (topLeft.GetY() + bottomRight.GetY()) * 0.5f);
	}
	/////////////////////////////////



	/////////////////////////////////
	// ContainsPoint - Checks if a given point is contained within the bounding box. The method uses half-open intervals [topLeft, bottomRight) for containment checks, meaning that points on the left and top edges are considered inside the box, 
	// while points on the right and bottom edges are considered outside. This approach avoids ambiguity when checking points that lie exactly on the boundaries of the box.
	bool ContainsPoint(const Vec2& point) const noexcept {
		return point.GetX() >= topLeft.GetX() && point.GetX() < bottomRight.GetX() && point.GetY() >= topLeft.GetY() &&
			   point.GetY() < bottomRight.GetY();
	}
	/////////////////////////////////



	/////////////////////////////////
	// Intersects - Checks if this bounding box intersects with another bounding box. The method uses half-open intervals [topLeft, bottomRight) for intersection checks, meaning that boxes that touch at edges do not intersect.
	bool Intersects(const BoundingBox& otherRect) const noexcept {
		return !(otherRect.bottomRight.GetX() <= topLeft.GetX() || otherRect.topLeft.GetX() >= bottomRight.GetX() ||
				 otherRect.bottomRight.GetY() <= topLeft.GetY() || otherRect.topLeft.GetY() >= bottomRight.GetY());
	}
	/////////////////////////////////



	/////////////////////////////////
	// Getters for the top-left and bottom-right points of the bounding box, as well as the width and height of the box. The width and height are calculated as the difference in x and y coordinates between the bottom-right and top-left points, respectively.
	Vec2 GetTopLeftPoint() const noexcept { return topLeft; }
	Vec2 GetBottomRightPoint() const noexcept {	return bottomRight;	} 
	float GetHeight() const noexcept { return bottomRight.GetY() - topLeft.GetY(); }
	float GetWidth() const noexcept { return bottomRight.GetX() - topLeft.GetX(); }
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// QuadTree class template - A spatial partitioning data structure that recursively subdivides a 2D space into four quadrants (child nodes) to efficiently manage and query objects based on their spatial location. 
// The QuadTree is designed to hold pointers to objects of type T, which are expected to have a method GetCentrePoint() that returns their position in the 2D space. The QuadTree supports insertion of objects, querying for objects within a specified area, 
// updating object positions, and removing objects from the tree. It also includes instrumentation for tracking query performance metrics such as the total number of queries, total objects queried, and total nodes visited during queries.
template <typename T>
class QuadTree {
	/////////////////////////////////
	// Private member variables for the QuadTree class
private:
	/////////////////////////////////
	// The boundary of this QuadTree node, represented as a BoundingBox. This defines the spatial area that this node covers in the 2D space. Objects that fall within this boundary can be stored in this node or its child nodes, 
	// while objects outside this boundary cannot be stored in this node.
	BoundingBox m_boundary;
	/////////////////////////////////



	/////////////////////////////////
	// The capacity of this QuadTree node, which determines how many objects can be stored in this node before it needs to subdivide into child nodes. This is a critical parameter that affects the performance of the QuadTree, 
	// as a smaller capacity will lead to more subdivisions and potentially deeper trees, while a larger capacity may lead to fewer subdivisions but more objects stored in each node, which can affect query performance.
	unsigned int m_capacity;
	/////////////////////////////////



	/////////////////////////////////
	// The objects contained in this QuadTree node, stored as raw pointers. These pointers are not owned by the QuadTree, meaning that the QuadTree does not manage the memory for these objects and is not responsible for deleting them.
	std::vector<T*> m_objects;
	/////////////////////////////////



	/////////////////////////////////
	// Flag to indicate whether this QuadTree node has been subdivided into child nodes. If this flag is false, it means that this node is a leaf node and can store objects directly. If this flag is true, it means that this node has been 
	// subdivided into four child nodes (top-left, top-right, bottom-left, bottom-right), and objects should be stored in the appropriate child node based on their position.
	bool m_isSubDivided = false;
	/////////////////////////////////



	/////////////////////////////////
	// Unique pointers to the four child QuadTree nodes (top-left, top-right, bottom-left, bottom-right), initialized to nullptr. These child nodes are created when this node is subdivided, and they manage their own boundaries and capacities based on the parent node's boundary.
	std::array<std::unique_ptr<QuadTree<T>>, 4> m_childTreePtrs{{nullptr, nullptr, nullptr, nullptr}};
	/////////////////////////////////



	/////////////////////////////////
	// The depth of this node in the QuadTree hierarchy, with the root node at depth 0. This is used to limit the maximum depth of subdivision and prevent infinite recursion. Each time a node is subdivided, the child nodes will have a depth that is one greater than the parent node.
	unsigned int m_depth = 0;
	/////////////////////////////////



	/////////////////////////////////
	// The maximum depth allowed for subdivision of the QuadTree. This prevents infinite subdivision and can be set to a specific value based on the expected density of objects and performance requirements. If the maximum depth is reached, 
	// the node will not subdivide further and will store objects directly, even if it exceeds the defined capacity.
	unsigned int m_maxDepth = std::numeric_limits<unsigned int>::max();
	/////////////////////////////////



	/////////////////////////////////
	// Pointer to the parent QuadTree node, initialized to nullptr. This can be useful for certain operations that may require upward traversal of the tree, such as merging child nodes back into a parent node when objects are removed. However, in this implementation,
	QuadTree<T>* m_parent =	nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Static variables for tracking query performance metrics. These variables are incremented during query operations to allow developers to analyze the efficiency of the QuadTree and optimize it if necessary. They can be reset at the beginning of each frame or query session to track metrics for specific time periods.
	inline static size_t s_totalQueriesThisFrame =	0;	// Static variable to track the total number of queries performed on the QuadTree in the current frame, this is incremented each time a query is performed.
	inline static size_t s_totalObjectsQueried = 0;		// Static variable to track the total number of objects queried in the current frame.
	inline static size_t s_totalNodesVisited =	0;		// Static variable to track the total number of QuadTree nodes visited during queries in the current frame, It is incremented each time a node is visited during a query.
	inline static size_t s_queryCount =	0;				// Static variable to track the total number of queries performed on the QuadTree, It is incremented each time a query is performed.
	/////////////////////////////////
	
	

	/////////////////////////////////
	// InsertRawPointer - A helper method for inserting an object into the QuadTree using a raw pointer. This method is responsible for checking if the object's center point is within the boundary of this node, determining whether to store the object in this node or to subdivide 
	// and insert it into child nodes, and handling cases where the object cannot be inserted into child nodes. The method returns true if the object was successfully inserted into this node or one of its child nodes, or if it was added to this node's list of objects as a fallback, 
	// and false if the object cannot be inserted into this node (e.g., if it is outside the boundary).
	bool InsertRawPointer(T* object) {
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
	/////////////////////////////////

	

	/////////////////////////////////
	// RemoveObjectFast - A helper method for removing an object from this QuadTree node's list of objects using a raw pointer. This method finds the object in the list of objects for this node, and if it is found, it swaps it with the last object in the list and 
	// then removes the last object (which is now the target object) from the list.
	void RemoveObjectFast(T* object) noexcept {
		auto it = std::find(
			m_objects.begin(), m_objects.end(),	object); // Find the object in the list of objects for this node. If it is found, swap it with the last object in the list and then remove the last object (which is now the target object) from the list. This allows for fast removal without needing to shift elements in the vector, but it does not preserve the order of objects in the list.

		// If the object is found in the list of objects for this node, swap it with the last object in the list and then remove the last object (which is now the target object) from the list. This allows for fast removal without needing to shift elements in the vector, but it does not preserve the order of objects in the list.
		if (it != m_objects.end()) {
			std::swap(*it, m_objects.back());
			m_objects.pop_back();
		}
	}
	/////////////////////////////////


	
	/////////////////////////////////
	// Public member functions for the QuadTree class
public:
	/////////////////////////////////
	// Constructors and destructor for the QuadTree class. The default constructor initializes the QuadTree with a zero-sized boundary at the origin, a capacity of 1, and a depth of 0. The parameterized constructor allows for initializing the QuadTree with a specific boundary, 
	// capacity, depth, and maximum depth.
	QuadTree()
		: m_boundary(BoundingBox(Vec2::Zero, Vec2::Zero)), m_capacity(1), m_depth(0),
		  m_maxDepth(std::numeric_limits<unsigned int>::max()) {}

	QuadTree(BoundingBox Boundary, unsigned int n, unsigned int depth = 0,
			 unsigned int maxDepth = std::numeric_limits<unsigned int>::max())
		: m_boundary(Boundary), m_capacity(std::max<unsigned int>(1u, n)), m_depth(depth), m_maxDepth(maxDepth) {}

	~QuadTree() { ClearTree(); }
	/////////////////////////////////



	/////////////////////////////////
	// ClearTree - A method to clear the QuadTree by recursively clearing all child nodes and resetting the state of this node. This method is called in the destructor to ensure that all resources are properly released when the 
	// QuadTree is destroyed. It also clears the list of objects for this node.
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
	/////////////////////////////////



	/////////////////////////////////
	// GetBoundary - A method to retrieve the boundaries of this QuadTree node and all of its child nodes. This method takes a reference to a vector of BoundingBox objects and appends the boundary of this node to the vector, then recursively calls 
	// GetBoundary on each child node (if subdivided) to collect their boundaries as well.
	void GetBoundary(std::vector<BoundingBox>& rBoundaries) {
		rBoundaries.push_back(m_boundary);
		if (m_isSubDivided) {
			for (auto& child : m_childTreePtrs) {
				if (child)
					child->GetBoundary(rBoundaries);
			}
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// Insert - A public method to insert an object into the QuadTree using a pointer. This method is a wrapper around the InsertRawPointer helper method, which performs the actual insertion logic. The Insert method returns true if 
	// the object was successfully inserted into this node or one of its child nodes,
	bool Insert(T* object) { return InsertRawPointer(object); }
	/////////////////////////////////



	/////////////////////////////////
	// SubDivide - A method to subdivide this QuadTree node into four child nodes (top-left, top-right, bottom-left, bottom-right) based on the current boundary of this node. This method is called when the capacity of this node 
	// is exceeded and it needs to create child nodes to store additional objects.
	void SubDivide() {
		if (m_isSubDivided || m_depth >= m_maxDepth) return;

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
	/////////////////////////////////



	/////////////////////////////////
	// Query - A method to query the QuadTree for objects that are within a specified area defined by a BoundingBox. This method takes a reference to a vector of pointers to objects of type T, which will be populated with the objects that are found within the query area.
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
	/////////////////////////////////



	/////////////////////////////////
	// Query - An overloaded version of the Query method that takes an additional parameter for the current object being queried. This allows the method to exclude the current object from the results, which can be useful in scenarios where you want to 
	// find nearby objects without including the object itself in the results.
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
	/////////////////////////////////



	/////////////////////////////////
	// Scan - A method to perform a scan query on the QuadTree, which is a more general form of querying that can be used for various purposes. This method takes a reference to an object of type T and a size_t value, and it returns a boolean indicating whether the scan was successful.
	bool Scan(T&, size_t value) { return false; }
	/////////////////////////////////



	/////////////////////////////////
	// Size - A method to calculate and return the total number of objects stored in this QuadTree node and all of its child nodes. This method recursively counts the number of objects in this node and adds it to the counts from each child node 
	// (if subdivided) to get the total size of the QuadTree.
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
	/////////////////////////////////



	/////////////////////////////////
	// ResetQueryStats - A static method to reset the query performance metrics tracked by the QuadTree. This method sets the total number of queries, total objects queried, total nodes visited, and query count back to zero. 
	// It can be called at the beginning of each frame or query session to track metrics for specific time periods.
	static void ResetQueryStats() {
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_totalNodesVisited = 0;
		s_queryCount = 0;
	}
	/////////////////////////////////



	/////////////////////////////////
	// IncrementQueryCount - A static method to increment the query count by one. This method should be called each time a query is performed on the QuadTree to keep track of the total number of queries executed.
	static void IncrementQueryCount() { ++s_queryCount; }
	/////////////////////////////////



	/////////////////////////////////
	// Getters for the query performance metrics tracked by the QuadTree. These methods return the total number of queries performed in the current frame, total objects queried, total nodes visited during queries, total query count, and average number of objects per query.
	static size_t GetTotalQueries() { return s_totalQueriesThisFrame; }
	static size_t GetTotalObjectsQueried() { return s_totalObjectsQueried; }
	static size_t GetTotalNodesVisited() { return s_totalNodesVisited; }
	static size_t GetQueryCount() { return s_queryCount; }
	static double GetAverageObjectsPerQuery() {	return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0; }
	static void ReserveQueryCapacity(std::vector<T*>& found, size_t entityCount) {
		found.reserve(std::max(size_t(16), entityCount / 100));
	}
	/////////////////////////////////



	/////////////////////////////////
	// UpdatePosition - A method to update the position of an object in the QuadTree. This method checks if the object's center point is still within the boundary of this node, and if it is, it checks if the object should be moved to a child node (if subdivided) based on its new position.
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
	/////////////////////////////////



	/////////////////////////////////
	// RemoveEntityFromTree - A method to remove an object from the QuadTree using a pointer. This method calls the RemoveObjectFast helper method to remove the object from this node's list of objects, and then recursively calls RemoveEntityFromTree on 
	// each child node (if subdivided) to ensure that the object is removed from all nodes in the tree.
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
	/////////////////////////////////
};
/////////////////////////////////
