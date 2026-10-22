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
#include <math.h>
#include "AABB.h"
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
    DestroyRecursive(m_root);
    m_root = nullptr;
	m_entityLeaf.clear();

    //if (entities.empty()) {
    //    std::cout << "[BVH] No entities — BVH empty\n";
    //    return;
    //}
	
    std::vector<Entity*> validEntities = entities;	// Create a vector to hold valid entities that have a CShape component
    m_root = BuildRecursive(validEntities, 1);

    std::cout << "[BVH] Build complete\n";
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
// BuildRecursive - recursively builds the BVH tree from a list of entities. This method divides the entities into two groups based on their bounding boxes and creates internal nodes until the number of entities in a node is less than or equal to the specified leaf size. Leaf nodes contain the actual entities, 
// while internal nodes contain bounding boxes that encompass their child nodes.
BVHNode* BVHSystem::BuildRecursive(std::vector<Entity*>& entities, int leafSize) {
	// Base case: if there are no entities, return null
	if (entities.empty()) return nullptr;

	// Leaf node: if the number of entities is less than or equal to the leaf size, create a leaf node containing the entities
	if ((int)entities.size() <= leafSize) {
		// Create a new leaf node
		BVHNode* node = new BVHNode();
		node->leafEntities = entities;

		// Compute the bounding box for the leaf node by combining the bounding boxes of all entities in the leaf
		AABB bounds = ComputeBounds(entities[0]);

		// Expand the bounding box to include all entities in the leaf
		for (size_t i = 1; i < entities.size(); ++i) bounds.expand(ComputeBounds(entities[i]));

		// Assign the computed bounding box to the leaf node
		node->bounds = bounds;

		// NEW: map each entity to this leaf
		for (Entity* e : entities)	m_entityLeaf[e] = node;

		// Return the created leaf node
		return node;
	}

	// Sort entities by their bounding box's minimum x-coordinate to prepare for splitting. This helps in creating a balanced BVH tree by dividing the entities into two groups based on their spatial distribution.
	std::sort(entities.begin(), entities.end(), [this](Entity* a, Entity* b) { return ComputeBounds(a).min.x < ComputeBounds(b).min.x; });

	// Split the entities into two groups for the left and right child nodes. The midpoint is calculated to divide the sorted entities into two halves.
	int mid = entities.size() / 2;
	std::vector<Entity*> leftEntities(entities.begin(), entities.begin() + mid);
	std::vector<Entity*> rightEntities(entities.begin() + mid, entities.end());

	// Create a new internal node and recursively build the left and right child nodes using the split entity groups.
	BVHNode* node = new BVHNode();
	node->left = BuildRecursive(leftEntities, leafSize);
	node->right = BuildRecursive(rightEntities, leafSize);

	// Set the parent pointers of the child nodes to the current node. This establishes the hierarchical relationship between the internal node and its children.
	if (node->left)	node->left->parent = node;
	if (node->right) node->right->parent = node;

	// Compute the bounding box for the internal node by combining the bounding boxes of its child nodes. This ensures that the internal node's bounding box encompasses all entities in its subtree.
	if (node->left && node->right) {
		AABB combined = node->left->bounds;
		combined.expand(node->right->bounds);
		node->bounds = combined;
	} else if (node->left) {
		node->bounds = node->left->bounds;
	} else if (node->right) {
		node->bounds = node->right->bounds;
	}

	return node;
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
	//std::cout << "RaycastNode: testing bounds " << node->bounds.min.x << ", " << node->bounds.min.y  << " size " << node->bounds.max.x - node->bounds.min.x << ", " << node->bounds.max.y - node->bounds.min.y << "\n";

	float dummyDistance = 0.0f;

	// Record traversal
	if (debug)
		debug->visited.push_back(node);

	// Prune subtree if ray misses node bounds
	if (!RayIntersectsAABB(
			origin, dirN, Vec2(node->bounds.min.x, node->bounds.min.y),
			Vec2(node->bounds.max.x, node->bounds.max.y),
			dummyDistance, maxDistance)) {
		return false;
	} else {
		//std::cout << "RaycastNode: ray intersects bounds\n";
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
// RaycastLeaf - performs a raycast against the entities contained in a leaf node of the BVH tree. This method checks for intersections between the ray and the bounding boxes of the entities in the leaf node. It iterates through each entity, checks if it is alive and has a valid shape, and then performs a ray-AABB 
// intersection test. If an intersection is found, it updates the output hit information and returns true.
bool BVHSystem::RaycastLeaf(const std::vector<Entity*>& leaf, const Vec2& origin, const Vec2& dirN, float maxDistance,
							RaycastHit& outHit, Entity*& outEntity, BVHDebugTraversal* debug) const {
	float nearestDistance = maxDistance;
	bool hitAny = false;

	for (Entity* entity : leaf) {
		if (!entity || !entity->IsAlive())
			continue;

		// Use engine-native AABB instead of SFML FloatRect
		AABB box = ComputeBounds(entity);

		// Skip degenerate boxes
		if (box.min.x == box.max.x || box.min.y == box.max.y)
			continue;

		float hitDistance = nearestDistance;

		// Use your AABB ray test
		if (!RIntersectsAABB(origin, dirN, box, hitDistance))
			continue;

		// Update nearest hit
		nearestDistance = hitDistance;
		outEntity = entity;
		outHit.hit = true;
		outHit.distance = hitDistance;
		outHit.position = origin + dirN * hitDistance;

		// Compute normal from AABB center
		Vec2 center((box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f);

		Vec2 delta = outHit.position - center;

		float halfW = (box.max.x - box.min.x) * 0.5f;
		float halfH = (box.max.y - box.min.y) * 0.5f;

		float nx = delta.x / halfW;
		float ny = delta.y / halfH;

		if (std::fabs(nx) > std::fabs(ny))
			outHit.normal = Vec2((nx > 0.f) ? 1.f : -1.f, 0.f);
		else
			outHit.normal = Vec2(0.f, (ny > 0.f) ? 1.f : -1.f);

		hitAny = true;
	}

	if (hitAny && debug)
		debug->hitLeaf = debug->visited.back();

	return hitAny;
}
/////////////////////////////////



/////////////////////////////////
AABB BVHSystem::ComputeBounds(Entity* e) const {
	auto* t = e->GetComponent<CTransform>();
	auto* s = e->GetComponent<CShape>();

	if (!t || !s)
		return AABB(Vec2(0, 0), Vec2(0, 0));

	Vec2 pos = t->position;
	auto& shape = s->GetShape();
	sf::Vector2f origin = shape.getOrigin();

	const float w = s->GetWidth();
	const float h = s->GetHeight();

	Vec2 min(pos.x - origin.x, pos.y - origin.y);
	Vec2 max(min.x + w, min.y + h);

	return AABB(min, max);
}
/////////////////////////////////



/////////////////////////////////
bool BVHSystem::RIntersectsAABB(const Vec2& origin, const Vec2& dirN, const AABB& box, float& outDist) const {
	float tmin = (box.min.x - origin.x) / dirN.x;
	float tmax = (box.max.x - origin.x) / dirN.x;

	if (tmin > tmax)
		std::swap(tmin, tmax);

	float tymin = (box.min.y - origin.y) / dirN.y;
	float tymax = (box.max.y - origin.y) / dirN.y;

	if (tymin > tymax)
		std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	float t = std::max(tmin, tymin);

	if (t < 0.0f || t > outDist)
		return false;

	outDist = t;
	return true;
}
/////////////////////////////////



/////////////////////////////////
void BVHSystem::Insert(Entity* e) {
	if (!e || !e->IsAlive())
		return;

	m_dynamicEntities.push_back(e);
	m_dirty = true;
}
/////////////////////////////////



/////////////////////////////////
void BVHSystem::Remove(Entity* e) {
	if (!e)
		return;

	// Remove every occurrence because callers may have inserted an entity more
	// than once while the BVH was being rebuilt or updated.
	m_dynamicEntities.erase(
		std::remove(m_dynamicEntities.begin(), m_dynamicEntities.end(), e),
		m_dynamicEntities.end());

	// The BVH stores raw Entity* values in leaf nodes. Remove the pointer from
	// its leaf before EntityManager destroys the Entity object. Do not call any
	// Entity method here: this function must also be safe during teardown.
	auto leafIt = m_entityLeaf.find(e);
	if (leafIt != m_entityLeaf.end()) {
		BVHNode* leaf = leafIt->second;
		if (leaf) {
			leaf->leafEntities.erase(
				std::remove(leaf->leafEntities.begin(), leaf->leafEntities.end(), e),
				leaf->leafEntities.end());
		}
		m_entityLeaf.erase(leafIt);
	}

	m_dirty = true;
}
/////////////////////////////////



///////////////////////////////
void BVHSystem::Clear() {
	DestroyRecursive(m_root);
	m_root = nullptr;
	m_dynamicEntities.clear();
	m_entityLeaf.clear();
	m_dirty = false;
}
///////////////////////////////



/////////////////////////////////
void BVHSystem::Update(Entity* e) {
	auto it = m_entityLeaf.find(e);
	if (it == m_entityLeaf.end())
		return;

	BVHNode* leaf = it->second;

	// 1. Update leaf bounds
	leaf->bounds = ComputeBounds(e);

	// 2. Bubble updated bounds up the tree
	BVHNode* parent = leaf->parent;
	while (parent) {
		AABB newBounds;

		if (parent->left)
			newBounds = parent->left->bounds;

		if (parent->right)
			newBounds.expand(parent->right->bounds);

		parent->bounds = newBounds;
		parent = parent->parent;
	}

	//ReportBVHSize();
}
/////////////////////////////////



/////////////////////////////////
void BVHSystem::ReportBVHSize() const {
	int depth = ComputeDepth(m_root);
	std::cout << "[BVH] Depth: " << depth << "\n";
}
/////////////////////////////////



/////////////////////////////////
// QueryRayCandidates - performs a raycast against the BVH tree and collects all entities that are potential candidates for intersection with the ray. This method traverses the BVH tree, checking for intersections between the ray and the bounding boxes of each node. If an intersection is found, 
// it adds the entities in the leaf nodes to the output vector of candidate entities.
void BVHSystem::QueryRayCandidates(const Vec2& origin, const Vec2& dir, float maxDist,
								   std::vector<Entity*>& outEntities) const {
	if (!m_root)
		return;

	struct StackItem {
		BVHNode* node;
	};

	std::vector<StackItem> stack;
	stack.push_back({m_root});

	while (!stack.empty()) {
		BVHNode* node = stack.back().node;
		stack.pop_back();

		// AABB bounds
		const AABB& b = node->bounds;

		Vec2 rectMin(b.min.x, b.min.y);
		Vec2 rectMax(b.max.x, b.max.y);

		float t = maxDist;
		if (!RIntersectsAABB(origin, dir, b, t))
			continue;

		if (node->IsLeaf()) {
			for (Entity* e : node->leafEntities)
				outEntities.push_back(e);
			continue;
		}

		if (node->left)
			stack.push_back({node->left});
		if (node->right)
			stack.push_back({node->right});
	}
}
/////////////////////////////////



/////////////////////////////////
int BVHSystem::ComputeDepth(BVHNode* node) const {
	if (!node)
		return 0;
	if (node->IsLeaf())
		return 1;
	return 1 + std::max(ComputeDepth(node->left), ComputeDepth(node->right));
}
/////////////////////////////////



/////////////////////////////////
// RaycastDynamic - performs a raycast against the dynamic entities stored in the BVHSystem. This method iterates through the list of dynamic entities, checking for intersections between the ray and the bounding boxes of each entity. If an intersection is found, it updates the 
// output hit information and returns true.
bool BVHSystem::RaycastDynamic(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
							   Entity*& outEntity) const {
	float closest = maxDist;
	bool hit = false;
	outEntity = nullptr;

	for (Entity* e : m_dynamicEntities) {
		if (!e || !e->IsAlive())
			continue;

		AABB box = ComputeBounds(e);

		float dist = closest;
		if (RIntersectsAABB(origin, dirN, box, dist)) {
			closest = dist;
			hit = true;
			outEntity = e;

			outHit.hit = true;
			outHit.distance = dist;
			outHit.position = origin + dirN * dist;
			outHit.normal = Vec2(0, 0); // optional
			outHit.tileX = -1;
			outHit.tileY = -1;
			outHit.tileValue = 0;
		}
	}

	return hit;
}
/////////////////////////////////