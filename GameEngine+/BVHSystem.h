/////////////////////////////////
// BVHSystem.h
/////////////////////////////////



/////////////////////////////////
// includes
#pragma once
#include <vector>
#include "Entity.h"
#include "Raycast.h"
/////////////////////////////////



/////////////////////////////////
// BVHNode struct - represents a node in the Bounding Volume Hierarchy (BVH) tree. Each node contains a bounding box (sf::FloatRect) that encompasses the entities within that node,
struct BVHNode {
	sf::FloatRect bounds;
	BVHNode* left;
	BVHNode* right;
	std::vector<Entity*> leafEntities;

	bool IsLeaf() const { return left == nullptr && right == nullptr; }
};
/////////////////////////////////



/////////////////////////////////
//
//								|
//								|_______________________________________________________________________
class BVHSystem {
	/////////////////////////////////
	// Public
public:
	/////////////////////////////////
	BVHSystem() = default;
	~BVHSystem(); 
	///////////////////////////////// 
	


	/////////////////////////////////
	// Rebuild - rebuilds the BVH tree based on the provided list of entities. This method takes a vector of pointers to Entity objects and constructs a new BVH tree that organizes the entities 
	// in a way that optimizes spatial queries, such as raycasting. The rebuilt tree will allow for efficient collision detection and intersection tests with the entities in the scene.
	void Rebuild(const std::vector<Entity*>& entities);
	/////////////////////////////////
	


	/////////////////////////////////
	BVHNode* GetRoot() { return m_root; }
	/////////////////////////////////



	/////////////////////////////////
	bool Raycast(const Vec2& origin, const Vec2& dirN, float maxDist,
                 RaycastHit& outHit, Entity*& outEntity) const;
	/////////////////////////////////



	/////////////////////////////////
	// Private members and helper methods for the BVHSystem class. These methods are used internally to build the BVH tree, perform raycasting, and manage the nodes of the tree.
private:
	/////////////////////////////////
	BVHNode* m_root = nullptr; 
	/////////////////////////////////



	/////////////////////////////////
	// Internal helper methods for building the BVH tree and performing raycasting. These methods are used to recursively build the tree, perform ray-AABB intersection tests, and traverse the 
	// tree during raycasting.
	BVHNode* BuildRecursive(std::vector<Entity*>& entities, int leafSize);
	/////////////////////////////////



	/////////////////////////////////
	// Ray-AABB intersection test. Returns true if the ray intersects the AABB, and sets outDistance to the distance along the ray to the intersection point.
	void DestroyRecursive(BVHNode* node);
	/////////////////////////////////
	

	 
	/////////////////////////////////
	// RaycastNode - performs a raycast against the BVH tree starting from the given node. This method recursively traverses the tree, testing the ray against the bounding boxes of each node and 
	// checking for intersections with the entities contained in leaf nodes.
	bool RaycastNode(BVHNode* node, const Vec2& origin, const Vec2& dirN, float maxDistance, RaycastHit& outHit,Entity*& outEntity) const;
	/////////////////////////////////



	/////////////////////////////////
	// RaycastLeaf - performs a raycast against the entities contained in a leaf node of the BVH tree. This method checks for intersections between the ray and the bounding boxes of the 
	// entities in the leaf node. It iterates through each entity, checks if it is alive and has a valid shape, and then performs a ray-AABB intersection test. If an intersection is found, 
	// it updates the output hit information and returns true.
	bool RaycastLeaf(const std::vector<Entity*>& leaf, const Vec2& origin, const Vec2& dirN, float maxDistance, RaycastHit& outHit, Entity*& outEntity) const;
	/////////////////////////////////
};
/////////////////////////////////