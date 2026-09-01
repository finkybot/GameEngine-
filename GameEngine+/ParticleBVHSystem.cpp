/////////////////////////////////
// ParticleBVHSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "ParticleBVHSystem.h"
#include "CKnowledgeParticle.h"
#include "CTransform.h"
#include "CParticleInfluence.h"
/////////////////////////////////



/////////////////////////////////
// Helper function to create an AABB (Axis-Aligned Bounding Box) from a position and radius. This function calculates the minimum and maximum points of the bounding box based on 
// the given position and radius, effectively creating a square bounding box that encompasses a circle defined by the position and radius.
static AABB MakeBounds(const Vec2& pos, float r) {
	return AABB{Vec2(pos.x - r, pos.y - r), Vec2(pos.x + r, pos.y + r)};
}
/////////////////////////////////



/////////////////////////////////
// ParticleBVHSystem class implementation
void ParticleBVHSystem::Build(EntityManager& em) {
	// Clear existing particles and BVH
	particles.clear();

	// Gather all knowledge particles from the entity manager
	for (auto& ePtr : em.GetEntities()) {
		// Only consider entities that have the required components
		Entity* e = ePtr.get();

		// Check if the entity has the necessary components: CKnowledgeParticle, CTransform, and CParticleInfluence
		auto* kp = e->GetComponent<CKnowledgeParticle>();
		auto* tr = e->GetComponent<CTransform>();
		auto* inf = e->GetComponent<CParticleInfluence>();

		// If any of the required components are missing, skip this entity
		if (!kp || !tr || !inf)	continue;

		// Create a ParticleData instance for the entity and add it to the particles vector
		ParticleData pd;

		// Set the position, radius, entity pointer, and bounding box for the particle
		pd.position = tr->position;
		pd.radius = inf->influenceRadius;
		pd.entity = e;

		// Calculate the bounding box for the particle based on its position and radius
		pd.bounds = MakeBounds(pd.position, pd.radius);
		
		// Add the particle data to the particles vector
		particles.push_back(pd);
	}

	// If there are no particles, set the root to nullptr and return
	if (particles.empty()) {
		root = nullptr;
		return;
	}

	// Simple sort along X
	std::sort(particles.begin(), particles.end(), [](const ParticleData& a, const ParticleData& b) { return a.position.x < b.position.x; });

	// Build the BVH recursively starting from the root node
	root = BuildRecursive(0, (int)particles.size());
}
/////////////////////////////////



/////////////////////////////////
// BuildRecursive - Recursively builds the BVH (Bounding Volume Hierarchy) for the particles. This function creates a new ParticleBVHNode, computes its bounding box based on 
// the particles it contains, and splits the particles into left and right child nodes if necessary.
ParticleBVHNode* ParticleBVHSystem::BuildRecursive(int start, int count) {
	// Create a new BVH node for the current range of particles
	auto* node = new ParticleBVHNode();
	
	// Set the start index and count of particles for this node
	node->start = start;
	node->count = count;
	node->left = nullptr;
	node->right = nullptr;

	// Compute bounds
	AABB b = particles[start].bounds;
	for (int i = start + 1; i < start + count; ++i) {
		b.min.x = std::min(b.min.x, particles[i].bounds.min.x);
		b.min.y = std::min(b.min.y, particles[i].bounds.min.y);
		b.max.x = std::max(b.max.x, particles[i].bounds.max.x);
		b.max.y = std::max(b.max.y, particles[i].bounds.max.y);
	}

	// Set the computed bounding box for the node
	node->bounds = b;

	// Leaf threshold
	const int leafSize = 8;
	if (count <= leafSize) {
		node->isLeaf = true;
		//node->particle = &particles[start];	
		return node;
	}

	// Split along X axis for now
	int mid = start + count / 2;
	node->left = BuildRecursive(start, mid - start);
	node->right = BuildRecursive(mid, start + count - mid);
	return node;
}
/////////////////////////////////



/////////////////////////////////
// Refit - Updates the BVH to account for changes in particle positions or sizes. This function is currently a placeholder and simply calls Build() again to rebuild the BVH from scratch.
void ParticleBVHSystem::Refit() {
	Refit(root);
}
/////////////////////////////////



/////////////////////////////////
// QuerySphere - Queries the BVH for particles that intersect with a given sphere defined by its center (Vec2) and radius (float). The results are stored in the outResults vector, which is cleared at the beginning of the function.
void ParticleBVHSystem::QuerySphere(const Vec2& center, float radius, std::vector<ParticleData*>& outResults) {
	// Clear the output results vector to ensure it starts empty
	outResults.clear();

	// If the root node is null, there are no particles to query, so return early
	if (!root) return;

	// Start the recursive query from the root node
	QuerySphereRecursive(root, center, radius, outResults);
}
/////////////////////////////////



/////////////////////////////////
// Refit - Recursively refits the BVH nodes to update their bounding boxes based on the current positions and sizes of the particles. 
// This function traverses the BVH tree, updating the bounds of each node based on its children or the particles it contains.
void ParticleBVHSystem::Refit(ParticleBVHNode* node) {
	// If the current node is null, return early
	if (!node) return;

	// If the node is a leaf, recompute its bounding box based on the particles it contains
	if (node->isLeaf) {
		// Leaf: recompute bounds from its particle range
		AABB b = particles[node->start].bounds;

		// Iterate through the particles in the leaf node and update the bounding box to encompass all particles
		for (int i = node->start + 1; i < node->start + node->count; ++i) {
			b.min.x = std::min(b.min.x, particles[i].bounds.min.x);
			b.min.y = std::min(b.min.y, particles[i].bounds.min.y);
			b.max.x = std::max(b.max.x, particles[i].bounds.max.x);
			b.max.y = std::max(b.max.y, particles[i].bounds.max.y);
		}

		// Set the computed bounding box for the leaf node
		node->bounds = b;
		return;
	}

	// Internal: refit children first
	Refit(node->left);
	Refit(node->right);

	// Merge child bounds
	node->bounds.min.x = std::min(node->left->bounds.min.x, node->right->bounds.min.x);
	node->bounds.min.y = std::min(node->left->bounds.min.y, node->right->bounds.min.y);
	node->bounds.max.x = std::max(node->left->bounds.max.x, node->right->bounds.max.x);
	node->bounds.max.y = std::max(node->left->bounds.max.y, node->right->bounds.max.y);
}
/////////////////////////////////



/////////////////////////////////
// QuerySphereRecursive - Recursively queries the BVH for particles that intersect with a given sphere. This function checks if the current node's bounding box intersects with the sphere, and if so, it either adds the particles in the leaf node to the results or continues querying the child nodes.
void ParticleBVHSystem::QuerySphereRecursive(ParticleBVHNode* node, const Vec2& center, float radius, std::vector<ParticleData*>& outResults) {
	// If the current node is null, return early
	if (!node)	return;

	// Check if the node's bounding box intersects with the sphere defined by center and radius
	if (!AABBIntersectsSphere(node->bounds, center, radius)) return;

	// If the node is a leaf, check each particle in the node for intersection with the sphere
	if (node->isLeaf) {
		// Iterate through the particles in the leaf node and check if they intersect with the sphere
		for (int i = node->start; i < node->start + node->count; ++i) {
			// Get the particle data for the current index
			ParticleData& pd = particles[i];

			// Calculate the squared distance from the particle's position to the center of the sphere
			float dx = pd.position.x - center.x;
			float dy = pd.position.y - center.y;
			float dist2 = dx * dx + dy * dy;

			// If the squared distance is less than or equal to the squared radius, the particle intersects with the sphere, so add it to the results
			if (dist2 <= radius * radius) {
				outResults.push_back(&pd);
			}
		}

		// Since this is a leaf node, we don't need to query further, so we can return
		return;
	}

	// If the node is not a leaf, recursively query the left and right child nodes for intersection with the sphere
	QuerySphereRecursive(node->left, center, radius, outResults);
	QuerySphereRecursive(node->right, center, radius, outResults);
}
/////////////////////////////////
