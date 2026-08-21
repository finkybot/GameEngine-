/////////////////////////////////
// BVHSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "BVHSystem.h"
#include "Entity.h"
#include "CShape.h"
#include "Raycast.h"
#include <algorithm>
/////////////////////////////////



/////////////////////////////////
// Constructor and Destructor
BVHSystem::~BVHSystem() {
	DestroyRecursive(m_root);
	m_root = nullptr;
}
/////////////////////////////////



/////////////////////////////////
// Utility functions
// UnionBounds - returns the union of two sf::FloatRect bounding boxes. This function calculates the minimum and maximum coordinates of the two rectangles and constructs a new rectangle that encompasses both.
static sf::FloatRect UnionBounds(const sf::FloatRect& a, const sf::FloatRect& b) {
	float left = std::min(a.position.x, b.position.x);
	float top = std::min(a.position.y, b.position.y);
	float right = std::max(a.position.x + a.size.x, b.position.x + b.size.x);
	float bottom = std::max(a.position.y + a.size.y, b.position.y + b.size.y);
	return sf::FloatRect({left, top}, {right - left, bottom - top});
}
/////////////////////////////////



/////////////////////////////////
// GetEntityBounds - returns the bounding box of an entity based on its CShape component. If the entity does not have a CShape, it returns an empty rectangle. This function is used to retrieve the spatial 
// extent of an entity for BVH construction and raycasting.
static sf::FloatRect GetEntityBounds(Entity* e) {
	CShape* shape = e->GetShape();
	if (!shape)
		return sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(0.f, 0.f));

	return shape->GetShape().getGlobalBounds();
}
/////////////////////////////////



/////////////////////////////////
// Rebuild - rebuilds the BVH tree based on the provided list of entities. This method takes a vector of pointers to Entity objects and constructs a new BVH tree that organizes the entities
void BVHSystem::Rebuild(const std::vector<Entity*>& entities) {
	DestroyRecursive(m_root);		// Destroy the existing BVH tree to free memory and avoid memory leaks
	m_root = nullptr;				// Reset the root pointer to null before building a new tree

	if (entities.empty()) return;	// If there are no entities, there's nothing to build, so return early

	std::vector<Entity*> validEntities = entities;	// Create a vector to hold valid entities that have a CShape component
	m_root = BuildRecursive(validEntities, 4);		// Example leaf size
}
/////////////////////////////////



/////////////////////////////////
// Raycast - performs a raycast against the BVH tree. This method takes the origin and direction of the ray, along with a maximum distance, and checks for intersections with the entities in the BVH tree.
bool BVHSystem::Raycast(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit, Entity*& outEntity,
						BVHDebugTraversal* debug) const {
	if (!m_root) return false; // If the BVH tree is empty, there's nothing to raycast against, so return false

	outEntity = nullptr; // Initialize the output entity pointer to null
	outHit = RaycastHit{}; // Initialize the output hit information to default values

	return RaycastNode(m_root, origin, dirN, maxDist, outHit, outEntity, debug); // Start the recursive raycast from the root of the BVH tree
}
/////////////////////////////////



/////////////////////////////////
// BuildRecursive - recursively builds the BVH tree from a list of entities. This method partitions the entities into two groups based on their bounding boxes and creates child nodes for each group. The 
// recursion continues until the number of entities in a node is less than or equal to the specified leaf size, at which point a leaf node is created containing the entities.
BVHNode* BVHSystem::BuildRecursive(std::vector<Entity*>& entities, int leafSize) {
	if (entities.empty()) return nullptr; // Base case: if there are no entities, return null

	// leaf node
	if ((int)entities.size() <= leafSize) {
		BVHNode* leafNode = new BVHNode();
		BVHNode* node = leafNode;
		node->leafEntities = entities; // Store the entities in the leaf node

		sf::FloatRect bounds = GetEntityBounds(entities[0]); // Initialize bounds with the first entity's bounding box
		for (size_t i = 1; i < entities.size(); ++i) {
			bounds = UnionBounds(bounds, GetEntityBounds(entities[i])); // Expand bounds to include all entities
		}
		node->bounds = bounds;
		return node; // Return the created leaf node
	}

	// Internal node sort along X axis
	std::sort(entities.begin(), entities.end(), [](Entity* a, Entity* b) {
		return GetEntityBounds(a).position.x < GetEntityBounds(b).position.x; // Sort entities by the left edge of their bounding boxes
	});

	// Split the entities into two groups for the left and right child nodes
	int mid = entities.size() / 2; // Find the midpoint to split the entities into two groups
	std::vector<Entity*> leftEntities(entities.begin(), entities.begin() + mid); // Left group
	std::vector<Entity*> rightEntities(entities.begin() + mid, entities.end());	 // Right group

	// Create a new internal node and recursively build the left and right child nodes
	BVHNode* node = new BVHNode();
	node->left = BuildRecursive(leftEntities, leafSize); // Recursively build the left child node
	node->right = BuildRecursive(rightEntities, leafSize); // Recursively build the right child node

	if (node->left && node->right) {
		node->bounds = UnionBounds(node->left->bounds, node->right->bounds); // Set the bounds of the internal node to encompass both child nodes
	} else if (node->left) {
		node->bounds = node->left->bounds; // If only the left child exists, use its bounds
	} else if (node->right) {
		node->bounds = node->right->bounds; // If only the right child exists, use its bounds
	}

	// Return the created internal node
	return node; // Return the created internal node
}
/////////////////////////////////



/////////////////////////////////
// DestroyRecursive - recursively deletes the BVH tree starting from the given node. This function traverses the tree in a post-order manner, deleting child nodes before deleting the current nodes. 
// It ensures that all dynamically allocated memory for the BVH nodes is properly released to prevent memory leaks.
void BVHSystem::DestroyRecursive(BVHNode* node) {
	if (!node) return; // Base case: if the node is null, return

	DestroyRecursive(node->left);	// Recursively destroy the left child
	DestroyRecursive(node->right);	// Recursively destroy the right child
	delete node;					// Delete the current node
}
/////////////////////////////////



/////////////////////////////////
// RaycastNode - performs a raycast against the BVH tree starting from the given node. This method recursively traverses the tree, testing the ray against the bounding boxes of each node and
// its child nodes. It returns true if the ray intersects any entity within the BVH tree and updates the output hit information accordingly.
bool BVHSystem::RaycastNode(BVHNode* node, const Vec2& origin, const Vec2& dirN, float maxDistance, RaycastHit& outHit, Entity*& outEntity, BVHDebugTraversal* debug) const {
	std::cout << "RaycastNode: testing bounds " << node->bounds.position.x << ", " << node->bounds.position.y
			  << " size " << node->bounds.size.x << ", " << node->bounds.size.y << "\n";

	float dummyDistance = 0.0f;

	// Record traversal
	if (debug)
		debug->visited.push_back(node);

	// Prune subtree if ray misses node bounds
	if (!RayIntersectsAABB(
			origin, dirN, Vec2(node->bounds.position.x, node->bounds.position.y),
			Vec2(node->bounds.position.x + node->bounds.size.x, node->bounds.position.y + node->bounds.size.y),
			dummyDistance, maxDistance)) {
		return false;
	} else {
		std::cout << "RaycastNode: ray intersects bounds\n";
	}

	// Leaf node: perform raycast against entities in the leaf
	if (node->IsLeaf()) {
		bool hit = RaycastLeaf(node->leafEntities, origin, dirN, maxDistance, outHit, outEntity, debug);

		if (hit && debug)
			debug->hitLeaf = node;

		return hit;
	}

	// Internal node: traverse left and right children
	bool hitLeft = RaycastNode(node->left, origin, dirN, maxDistance, outHit, outEntity, debug);
	bool hitRight = RaycastNode(node->right, origin, dirN, maxDistance, outHit, outEntity, debug);

	return hitLeft || hitRight;
}
/////////////////////////////////



/////////////////////////////////
// RaycastLeaf - performs a raycast against the entities contained in a leaf node of the BVH tree. This method checks for intersections between the ray and the bounding boxes of the
// entities in the leaf node. It iterates through each entity, checks if it is alive and has a valid shape, and then performs a ray-AABB intersection test. If an intersection is found, 
// it updates the output hit information and returns true.
bool BVHSystem::RaycastLeaf(const std::vector<Entity*>& leaf, const Vec2& origin, const Vec2& dirN, float maxDistance, RaycastHit& outHit, Entity*& outEntity, BVHDebugTraversal* debug) const {
	// Iterate through each entity in the leaf node and perform a raycast against its bounding box
	float nearestDistance = maxDistance;
	bool hitAny = false;

	for (Entity* entity : leaf) {
		if (!entity || !entity->IsAlive()) continue; // Skip dead or null entities

		CShape* shape = entity->GetShape();
		if (!shape)	continue; // Skip entities without a shape

		sf::FloatRect bounds = shape->GetShape().getGlobalBounds(); // Get the bounding box of the shape
		if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f)	continue; // Skip degenerate bounding boxes

		float hitDistance = 0.0f;
		if (!RayIntersectsAABB(origin, dirN, 
								Vec2(bounds.position.x, bounds.position.y),
								Vec2(bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y), 
								hitDistance, nearestDistance)) {
			continue; // If the ray does not intersect the bounding box, continue to the next entity
		}

		if (hitDistance < 0.0f || hitDistance > nearestDistance) continue; // Skip hits that are out of range

		// Update the nearest hit information if this hit is closer than any previous hits
		nearestDistance = hitDistance;
		outEntity = entity;
		outHit.hit = true;
		outHit.distance = hitDistance;
		outHit.position = Vec2(origin.x + dirN.x * hitDistance, origin.y + dirN.y * hitDistance);


		// Compute normal based on the hit position and the center of the bounding box
		Vec2 center(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f);
		Vec2 delta = outHit.position - center;

		float nx = delta.x / (bounds.size.x * 0.5f);
		float ny = delta.y / (bounds.size.y * 0.5f);
		
		if (std::fabs(nx) > std::fabs(ny)) {
			outHit.normal = Vec2((nx > 0.0f) ? 1.0f : -1.0f, 0.0f); // Hit on left or right side
		} else {
			outHit.normal = Vec2(0.0f, (ny > 0.0f) ? 1.0f : -1.0f); // Hit on top or bottom side
		}
		hitAny = true; // Mark that we hit at least one entity
	}

	   // If this leaf produced a hit, record it
		if (hitAny && debug) debug->hitLeaf = debug->visited.back(); // last visited node is the leaf

	return hitAny;
}
/////////////////////////////////