#pragma once

#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>

#include "task.hpp"

namespace IkigaiScript {

	class IkigaiScriptInterpreter;

	// ---------------------------------------------------------------------------
	// ConcurrencyScheduler
	// ---------------------------------------------------------------------------
	// Single-threaded cooperative scheduler.
	//
	// Invariant: ALL script execution happens on the calling thread.
	//            No OS threads are used by the scheduler itself.
	//
	// Lifecycle of a task:
	//   Created → (enqueue) → Suspended/Running → Completed / Cancelled
	//
	// pump(interp, maxSteps):
	//   Drains the ready queue up to maxSteps times.
	//   Each "step" calls resumeTask() once for the front task.
	//   Returns the number of steps actually taken.
	// ---------------------------------------------------------------------------

	class ConcurrencyScheduler {
	public:
		// Create a task and enqueue it as ready (state = Created → Suspended-ready)
		TaskRef spawn(FunctionRef func, ScopePtr scope, Class* classs,
			TaskRef parentTask = nullptr,
			CancellationToken::Ptr token = nullptr);

		// Enqueue an existing task to be resumed next
		void enqueue(TaskRef task);

		// Suspend the task that is currently executing.
		// Called from consolidated(Await) when we need to wait for a child.
		// Sets task->state = Suspended and registers on child completion.
		void suspendFor(TaskRef waiter, TaskRef waitee);

		// Cancel a task tree (recursive via token)
		void cancel(TaskRef task);

		// Process ready tasks.  Returns number of steps taken.
		// interp is needed because callTask lives on the interpreter.
		int pump(IkigaiScriptInterpreter* interp, int maxSteps = -1);

		// True if there are tasks queued or running
		bool hasActiveTasks() const;
		bool hasReadyTasks()  const;

		// Currently executing task (nullptr when not inside pump/step)
		TaskRef currentTask() const {
			return currentTask_;
		}

		// Clear all state (called from clearState())
		void clear();

		// All known tasks (for debugging / inspection)
		const std::vector<TaskRef>& allTasks() const {
			return allTasks_;
		}

		// Unique id generation
		static std::string makeTaskId(const std::string& funcName);

	private:
		// Resume one step of a task; returns true if task is still alive
		bool resumeTask(IkigaiScriptInterpreter* interp, TaskRef task);

		// Called when a task completes; notifies waiting parent
		void onTaskCompleted(TaskRef task);

		std::deque<TaskRef>  readyQueue_;
		std::vector<TaskRef> allTasks_;

		TaskRef currentTask_;

		static std::atomic<uint64_t> nextTaskId_;
	};

}  // namespace IkigaiScript
