// ******* MusicSystem.h *******
#pragma once
#include <memory>
#include <unordered_map>

// Forward declaration of CMusic to avoid including the entire CMusic header in this file.
namespace sf { 	class Music; /* Forward declaration of sf::Music to avoid including the entire SFML Audio module in this header */ } 

class Entity;
class EntityManager;

// MusicSystem class - manages the playback of music tracks in the game. It interacts with entities that have CMusic components, and uses the SFML Audio module to play music. 
// The MusicSystem is responsible for starting, stopping, and updating music playback based on the state of the CMusic components in the entities. It maintains a mapping of 
// active music tracks to their corresponding sf::Music instances for efficient management and control of music playback.
class MusicSystem
{
public:
	explicit MusicSystem(EntityManager& entityManager);		// Constructor that takes a reference to the EntityManager, which is used to access entities and their components.
	~MusicSystem();											// Destructor to clean up any active music instances when the MusicSystem is destroyed.
	void Update(float deltaSeconds);						// Update method that should be called every frame to manage music playback based on the state of CMusic components in the entities.

	void Process(); 										// Process method to handle starting, stopping, and updating music playback based on the state of CMusic components in the entities. This is where the main logic for managing music playback will be implemented.

private:
	EntityManager& m_entityManager;												// Reference to the EntityManager for accessing entities and their components.
	std::unordered_map<size_t, std::unique_ptr<sf::Music>> m_activeMusic;		// Map of entity ID to active sf::Music instance for entities that have a CMusic component currently playing. This allows us to manage multiple music tracks if needed, and ensures we can stop or update them as necessary.

	sf::Music* GetOrCreateMusic(Entity& entity);			// Helper method to get the existing sf::Music instance for an entity with a CMusic component, or create a new one if it doesn't exist. This method will handle loading the music file and configuring the sf::Music instance based on the properties of the CMusic component.

};

