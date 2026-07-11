/////////////////////////////////
// MainThreadTasks.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "MainThreadTasks.h"
/////////////////////////////////



/////////////////////////////////
// MainThreadTaskQueue implementation
MainThreadTaskQueue& MainThreadTaskQueue::Instance() {
	static MainThreadTaskQueue inst;
	return inst;
}
/////////////////////////////////



/////////////////////////////////
// Push a task to the main thread task queue
void MainThreadTaskQueue::Push(std::function<void()> task) {
	std::lock_guard<std::mutex> lk(m_mutex);
	m_queue.push(std::move(task));
}
/////////////////////////////////



/////////////////////////////////
// Drain all pending tasks into out (caller will execute them)
void MainThreadTaskQueue::Drain(std::vector<std::function<void()>>& out) {
	std::lock_guard<std::mutex> lk(m_mutex);
	while (!m_queue.empty()) {
		out.push_back(std::move(m_queue.front()));
		m_queue.pop();
	}
}
/////////////////////////////////
