/////////////////////////////////
// JobSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <chrono>
/////////////////////////////////



/////////////////////////////////
// JobSystem class - A simple thread pool implementation for scheduling and executing jobs concurrently. It manages a pool of worker threads that process jobs from a queue, allowing for efficient parallel execution of tasks.
//								|
//								|_______________________________________________________________________
class JobSystem {
	/////////////////////////////////
	// Public interface for the JobSystem class
public:
	/////////////////////////////////
	// Job type definition - A job is represented as a std::function that takes no arguments and returns void. This allows for flexible job definitions, including lambdas, function pointers, and callable objects.
	using Job = std::function<void()>;
	/////////////////////////////////



	/////////////////////////////////
	// Public static methods for initializing, shutting down, scheduling jobs, and waiting for all jobs to complete. These methods provide a convenient interface for using the JobSystem without needing to manage an instance directly.
	static void Init(unsigned numThreads = std::thread::hardware_concurrency()) { Instance().start(numThreads); }
	/////////////////////////////////



	/////////////////////////////////
	// Shutdown - Stops the job system and joins all worker threads. This should be called before the application exits to ensure that all jobs are completed and resources are cleaned up properly.
	static void Shutdown() { Instance().stop(); }
	/////////////////////////////////



	/////////////////////////////////
	// Schedule - Enqueues a job to be executed by the job system. The job is moved into the queue to avoid unnecessary copies, and a worker thread will pick it up for execution.
	static void Schedule(Job job) { Instance().enqueue(std::move(job)); }
	/////////////////////////////////



	/////////////////////////////////
	// WaitIdle - Blocks the calling thread until all scheduled
	static void WaitIdle() { Instance().waitIdle(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetWorkerCount - Returns the number of worker threads currently managed by the job system. This can be useful for monitoring and debugging purposes.
	static size_t GetWorkerCount() { return Instance().workers.size(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetPendingJobCount - Returns the number of jobs currently pending in the job queue. This can be useful for monitoring and debugging purposes.
	static double GetLastFrameJobTimeMs() { return lastFrameJobTimeMs.load(); }
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the JobSystem class, including a vector of worker threads, a queue of jobs, synchronization primitives (mutex and condition variable), and atomic flags for running state and pending job count.
private:
	/////////////////////////////////
	// Private constructor and destructor for the JobSystem class. The constructor is defaulted, and the destructor ensures that the job system is stopped when the instance is destroyed.
	JobSystem() = default;
	~JobSystem() { stop(); }
	/////////////////////////////////


	JobSystem(const JobSystem&) = delete;
	JobSystem& operator=(const JobSystem&) = delete;
	JobSystem(JobSystem&&) = delete;
	JobSystem& operator=(JobSystem&&) = delete;


	/////////////////////////////////
	// WorkerQueue struct - Represents a queue of jobs for a worker thread, along with a mutex for synchronization and optional metrics for monitoring job execution and performance.
	struct WorkerQueue {
		std::deque<Job> jobs;
		std::mutex mutex;

		size_t jobsExecuted = 0;
		size_t jobsStolen = 0;
		size_t idleCycles = 0;

		WorkerQueue() = default;

		WorkerQueue(const WorkerQueue&) = delete;
		WorkerQueue& operator=(const WorkerQueue&) = delete;

		WorkerQueue(WorkerQueue&&) = delete;
		WorkerQueue& operator=(WorkerQueue&&) = delete;
	};
	/////////////////////////////////



	/////////////////////////////////
	// Instance - Returns a reference to the singleton instance of the JobSystem. This ensures that there is only one instance of the job system throughout the application.
	static JobSystem& Instance() {
		static JobSystem js;
		return js;
	}
	/////////////////////////////////



	/////////////////////////////////
	// Start - Initializes the job system with the specified number of worker threads. If the job system is already running, this method does nothing. It reserves space for the worker threads and starts them, each running the workerLoop method.
	void start(unsigned numThreads) {
		if (running)
			return;

		running = true;

		const unsigned count = numThreads ? numThreads : 1;

		workers.reserve(count);
		workerQueues.resize(count);

		// Allocate queues BEFORE starting threads
		for (unsigned i = 0; i < count; ++i) {
			workerQueues[i] = std::make_unique<WorkerQueue>();
		}

		// Now start workers
		for (unsigned i = 0; i < count; ++i) {
			workers.emplace_back([this, i]() { workerLoop(i); });
		}
	}
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Stop - Stops the job system and joins all worker threads. It sets the running flag to false, notifies all waiting threads, and then joins each worker thread to ensure they have completed before clearing the workers vector.	
	void stop() {
		//{
		//	std::lock_guard<std::mutex> lock(queueMutex);
		//	running = false;
		//}
		//cv.notify_all();
	
		running = false;
		// Join all worker threads to ensure they have completed before clearing the workers vector
		for (auto& t : workers) {
			if (t.joinable())
				t.join();
		}
		workers.clear();
	}
	/////////////////////////////////



	/////////////////////////////////
	// Enqueue - Adds a job to the job queue and notifies one worker thread to process it. The job is moved into the queue to avoid unnecessary copies, and the pending job count is incremented.
	void enqueue(Job job) {
		static std::atomic<size_t> rr{0};
		size_t index = rr++ % workerQueues.size();

		{
			std::lock_guard<std::mutex> lock(workerQueues[index]->mutex);
			workerQueues[index]->jobs.push_back(std::move(job));
		}

		++pendingJobs;
	}
	/////////////////////////////////



	/////////////////////////////////
	// Worker loop - The main loop for each worker thread. It waits for jobs to be available in the queue, processes them, and decrements the pending job count. If the job system is stopped and there are no more jobs, the loop exits.
	void workerLoop(unsigned workerIndex) {
		assert(workerIndex < workerQueues.size());
		assert(workerQueues[workerIndex] && "WorkerQueue pointer is null");

		WorkerQueue& myQueue = *workerQueues[workerIndex];

		while (running) {
			Job job{};

			// Own queue
			{
				std::lock_guard<std::mutex> lock(myQueue.mutex);
				if (!myQueue.jobs.empty()) {
					job = std::move(myQueue.jobs.front());
					myQueue.jobs.pop_front();
					myQueue.jobsExecuted++;
				}
			}

			if (job) {
				job();
				--pendingJobs;
				continue;
			}

			// Steal
			for (size_t i = 0; i < workerQueues.size(); ++i) {
				if (i == workerIndex)
					continue;

				WorkerQueue& other = *workerQueues[i];
				std::lock_guard<std::mutex> lock(other.mutex);

				if (!other.jobs.empty()) {
					job = std::move(other.jobs.back());
					other.jobs.pop_back();
					other.jobsStolen++;
					break;
				}
			}

			if (job) {
				job();
				--pendingJobs;
				continue;
			}

			myQueue.idleCycles++;
			std::this_thread::yield();
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// WaitIdle - Blocks the calling thread until all pending jobs are completed
	void waitIdle() {
		auto start = std::chrono::high_resolution_clock::now();

		while (pendingJobs.load(std::memory_order_relaxed) > 0) {
			std::this_thread::yield();
		}

		auto end = std::chrono::high_resolution_clock::now();
		lastFrameJobTimeMs.store(std::chrono::duration<double, std::milli>(end - start).count());
	}
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the JobSystem class, including a vector of worker threads, a queue of jobs, synchronization primitives (mutex and condition variable), and atomic flags for running state and pending job count.
private:
	/////////////////////////////////
	std::vector<std::thread> workers;
	std::vector<std::unique_ptr<WorkerQueue>> workerQueues;		

	//std::queue<Job> jobs; // marked for deletion
	//std::mutex queueMutex; // marked for deletion
	//std::condition_variable cv; // marked for deletion

	std::atomic<bool> running{false};
	std::atomic<int> pendingJobs{0};
	static std::atomic<double> lastFrameJobTimeMs;
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// Static member variable definition for lastFrameJobTimeMs, initialized to 0.0. This variable tracks the time taken for jobs in the last frame, allowing for performance monitoring and optimization.
inline std::atomic<double> JobSystem::lastFrameJobTimeMs{0.0};
/////////////////////////////////