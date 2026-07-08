#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "task.hpp"

namespace IkigaiScript {

	class ConcurrencyScheduler;

	// ---------------------------------------------------------------------------
	// NativeJob — a C++ callable that runs in a worker thread.
	// Receives the arguments passed from script; returns the result value.
	// Must be pure C++ — NEVER call back into the interpreter from a worker.
	// ---------------------------------------------------------------------------
	using NativeJob = std::function<ValuePtr(const std::vector<ValuePtr>&)>;

	// ---------------------------------------------------------------------------
	// NativeJobPool
	// ---------------------------------------------------------------------------
	// Vyukov-style thread pool for CPU-heavy C++ work.
	//
	// Key invariant: Script code NEVER executes in worker threads.
	//   Workers only execute NativeJob lambdas (pure C++).
	//   Results are marshalled back to the main thread via completionQueue_
	//   and consumed by drainCompletions(), which is called at the top of pump().
	//
	// Typical usage:
	//   // Host setup (before script runs):
	//   interp.enableNativePool(4);
	//   interp.registerNativeJob("pathfind", [&map](const List& args) {
	//       return computePath(map, args[0]->getInt(), args[1]->getInt());
	//   });
	//
	//   // Script:
	//   var t = nativeJob("pathfind", x, y);
	//   var path = await t;
	// ---------------------------------------------------------------------------

	class NativeJobPool {
	public:
		explicit NativeJobPool(size_t numThreads = std::thread::hardware_concurrency());
		~NativeJobPool();

		// Non-copyable
		NativeJobPool(const NativeJobPool&) = delete;
		NativeJobPool& operator=(const NativeJobPool&) = delete;

		// -----------------------------------------------------------------------
		// Host-side setup (call before script starts)
		// -----------------------------------------------------------------------

		// Register a named C++ job. Thread-safe if called before any submit().
		void registerJob(const std::string& name, NativeJob job);

		bool hasRegisteredJob(const std::string& name) const;

		// -----------------------------------------------------------------------
		// Runtime API (called from main thread)
		// -----------------------------------------------------------------------

		// Submit a named job with script arguments.
		// Creates a TaskRef in Suspended state (isNativeJob = true) and enqueues
		// the work to a worker thread.
		// Returns the TaskRef so the caller can store it in a script variable.
		TaskRef submit(const std::string& name,
			const std::vector<ValuePtr>& args,
			CancellationToken::Ptr token = nullptr);

		// Drain all completed native jobs into the scheduler.
		// MUST be called from the main thread (e.g. at the top of pump()).
		// Sets task->result / task->state and re-enqueues parent tasks that
		// were awaiting the completed native task.
		void drainCompletions(ConcurrencyScheduler& scheduler);

		// Gracefully stop all worker threads (blocks until all join).
		void shutdown();

		// Number of idle workers at this instant (for diagnostics).
		size_t idleWorkers() const {
			return idleCount_.load(std::memory_order_relaxed);
		}
		size_t totalWorkers() const {
			return workers_.size();
		}

	private:
		// -----------------------------------------------------------------------
		// Internal types
		// -----------------------------------------------------------------------

		struct PendingWork {
			TaskRef               task;
			std::string           jobName;
			std::vector<ValuePtr> args;
		};

		struct CompletedWork {
			TaskRef  task;
			ValuePtr result;
			bool     failed = false;   // true if job threw or was cancelled
		};

		// -----------------------------------------------------------------------
		// Worker loop
		// -----------------------------------------------------------------------

		void workerLoop();

		// -----------------------------------------------------------------------
		// Worker pool (main-thread write at construction; workers read/run)
		// -----------------------------------------------------------------------

		std::vector<std::thread> workers_;

		std::queue<PendingWork>  workQueue_;
		std::mutex               workMutex_;
		std::condition_variable  workCv_;
		std::atomic<bool>        stopping_{false};
		std::atomic<size_t>      idleCount_{0};

		// -----------------------------------------------------------------------
		// Completion queue: workers WRITE, main thread READS
		// Protected by completionMutex_.
		// -----------------------------------------------------------------------

		std::queue<CompletedWork> completionQueue_;
		std::mutex                completionMutex_;

		// -----------------------------------------------------------------------
		// Job registry: written at setup, read-only thereafter (no lock needed).
		// -----------------------------------------------------------------------

		std::unordered_map<std::string, NativeJob> registry_;

		static std::atomic<uint64_t> nextId_;
	};

}  // namespace IkigaiScript
