#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

class MainThreadTaskQueue {
public:
	static MainThreadTaskQueue& Instance();

	void Push(std::function<void()> task);
	// Drain all pending tasks into out (caller will execute them)
	void Drain(std::vector<std::function<void()>>& out);

private:
	MainThreadTaskQueue() = default;
	std::mutex m_mutex;
	std::queue<std::function<void()>> m_queue;
};
