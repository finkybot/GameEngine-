/////////////////////////////////
// ParticleBVHSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
#include "AABB.h"
#include "EntityManager.h"
/////////////////////////////////



/////////////////////////////////
// ParticleData struct definition. This struct holds information about a particle, including its position, radius, associated entity, and bounding box (AABB). It is used in the ParticleBVHSystem for spatial partitioning and querying.
//								|
// 								|__________________________________________________________________
struct ParticleData {
	/////////////////////////////////
	// Public member variables for the ParticleData struct
	Vec2 position;
	float radius;
	Entity* entity;
	AABB bounds;
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// ParticleBVHNode struct definition. This struct represents a node in the Bounding Volume Hierarchy (BVH) for particles. It contains an axis-aligned bounding box (AABB), indices for the start and count of particles in the node,
// and pointers to left and right child nodes. It also has a method to check if the node is a leaf node (i.e., contains particles).
// 								|
// 								|_____________________________________________________________________________________
struct ParticleBVHNode {
	/////////////////////////////////
	// Public member variables for the ParticleBVHNode struct
	AABB bounds;
	int start;
	int count;
	ParticleBVHNode* left;
	ParticleBVHNode* right;
	bool isLeaf;
	ParticleData* particle;
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// ParticleBVHSystem class definition. This class manages a collection of particles and organizes them into a Bounding Volume Hierarchy (BVH) for efficient spatial queries. It provides methods to build the BVH, refit it when particles move, 
// and query for particles within a specified sphere.
// 								|
// 								|_____________________________________________________________________________________
class ParticleBVHSystem {
	/////////////////////////////////
	// Public member variables and methods for the ParticleBVHSystem class
public:
	/////////////////////////////////
	// Public member variables for the ParticleBVHSystem class
	std::vector<ParticleData> particles;
	ParticleBVHNode* root = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Public member functions for the ParticleBVHSystem class
	void Build(EntityManager& em);
	void Refit();
	void QuerySphere(const Vec2& center, float radius, std::vector<ParticleData*>& outResults);
	void Refit(ParticleBVHNode* node);
	/////////////////////////////////



	/////////////////////////////////
	// Destructor for the ParticleBVHSystem class. This destructor cleans up the BVH nodes by calling the ClearBVH method.
private:
	/////////////////////////////////
	// Private member functions for the ParticleBVHSystem class
	ParticleBVHNode* BuildRecursive(int start, int count);
	void QuerySphereRecursive(ParticleBVHNode* node, const Vec2& center, float radius, std::vector<ParticleData*>& outResults);
	/////////////////////////////////
};
/////////////////////////////////