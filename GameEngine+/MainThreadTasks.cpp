#include "MainThreadTasks.h"

MainThreadTaskQueue& MainThreadTaskQueue::Instance() {
	static MainThreadTaskQueue inst;
	return inst;
}

void MainThreadTaskQueue::Push(std::function<void()> task) {
	std::lock_guard<std::mutex> lk(m_mutex);
	m_queue.push(std::move(task));
}

void MainThreadTaskQueue::Drain(std::vector<std::function<void()>>& out) {
	std::lock_guard<std::mutex> lk(m_mutex);
	while (!m_queue.empty()) {
		out.push_back(std::move(m_queue.front()));
		m_queue.pop();
	}
}
