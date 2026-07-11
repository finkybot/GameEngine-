/////////////////////////////////
// MainThreadTasks.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <vector>
/////////////////////////////////



/////////////////////////////////
// MainThreadTaskQueue class - a thread-safe queue for scheduling tasks to be executed on the main thread.
//								|
//								|_______________________________________________________________________
class MainThreadTaskQueue {
	/////////////////////////////////
public:
	/////////////////////////////////
	// Singleton instance accessor for the MainThreadTaskQueue class. Returns a reference to the single instance of the queue.
	static MainThreadTaskQueue& Instance();
	/////////////////////////////////



	/////////////////////////////////
	// Push a task to the main thread task queue. The task is a std::function<void()> that will be executed on the main thread.
	void Push(std::function<void()> task);
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Drain all pending tasks into out (caller will execute them)
	void Drain(std::vector<std::function<void()>>& out);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables and methods.
private:
	/////////////////////////////////
	// Default constructor for the MainThreadTaskQueue class. Initializes the queue and mutex.
	MainThreadTaskQueue() = default;
	/////////////////////////////////



	/////////////////////////////////
	std::mutex m_mutex;
	std::queue<std::function<void()>> m_queue;
	/////////////////////////////////
};
/////////////////////////////////
