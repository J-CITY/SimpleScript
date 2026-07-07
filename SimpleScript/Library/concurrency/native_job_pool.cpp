#include "native_job_pool.hpp"
#include "scheduler.hpp"
#include "../value.hpp"

namespace IkigaiScript {

std::atomic<uint64_t> NativeJobPool::nextId_{0};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

NativeJobPool::NativeJobPool(size_t numThreads) {
    const size_t n = numThreads > 0 ? numThreads : 1;
    workers_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

NativeJobPool::~NativeJobPool() {
    shutdown();
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void NativeJobPool::shutdown() {
    {
        auto lock = std::unique_lock(workMutex_);
        stopping_ = true;
    }
    workCv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

void NativeJobPool::registerJob(const std::string& name, NativeJob job) {
    registry_[name] = std::move(job);
}

bool NativeJobPool::hasRegisteredJob(const std::string& name) const {
    return registry_.count(name) > 0;
}

// ---------------------------------------------------------------------------
// Submit (main thread)
// ---------------------------------------------------------------------------

TaskRef NativeJobPool::submit(const std::string& name,
                               const std::vector<ValuePtr>& args,
                               CancellationToken::Ptr token) {
    auto task = std::make_shared<Task>();
    task->id          = "__native_" + name + "_" + std::to_string(++nextId_);
    task->state       = TaskState::Suspended;
    task->isNativeJob = true;
    task->token       = token ? token : CancellationToken::make();

    {
        auto lock = std::unique_lock(workMutex_);
        workQueue_.push({task, name, args});
    }
    workCv_.notify_one();
    return task;
}

// ---------------------------------------------------------------------------
// drainCompletions (main thread) — called at top of pump()
// ---------------------------------------------------------------------------

void NativeJobPool::drainCompletions(ConcurrencyScheduler& scheduler) {
    std::queue<CompletedWork> done;
    {
        auto lock = std::unique_lock(completionMutex_);
        std::swap(done, completionQueue_);
    }

    while (!done.empty()) {
        auto cw = std::move(done.front());
        done.pop();

        if (cw.failed) {
            cw.task->state = TaskState::Cancelled;
        } else {
            cw.task->result = cw.result ? cw.result : std::make_shared<Value>();
            cw.task->state  = TaskState::Completed;
        }

        // Wake any parent that was awaiting this native task
        if (auto parent = cw.task->parent.lock()) {
            if (auto aw = parent->awaitingTask.lock()) {
                if (aw == cw.task) {
                    parent->awaitingTask.reset();
                    scheduler.enqueue(parent);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Worker loop (runs on worker threads — NO interpreter calls allowed here)
// ---------------------------------------------------------------------------

void NativeJobPool::workerLoop() {
    while (true) {
        PendingWork work;

        {
            auto lock = std::unique_lock(workMutex_);
            ++idleCount_;
            workCv_.wait(lock, [this] {
                return stopping_.load(std::memory_order_relaxed) || !workQueue_.empty();
            });
            --idleCount_;

            if (stopping_.load(std::memory_order_relaxed) && workQueue_.empty()) {
                return;
            }
            work = std::move(workQueue_.front());
            workQueue_.pop();
        }

        // Fast-path: task already cancelled before we even started
        if (work.task->token && work.task->token->isCancelled()) {
            auto lock = std::unique_lock(completionMutex_);
            completionQueue_.push({work.task, nullptr, true});
            continue;
        }

        // Execute the registered job (pure C++ — NO interpreter access)
        ValuePtr result;
        bool failed = false;
        auto it = registry_.find(work.jobName);
        if (it != registry_.end()) {
            try {
                result = it->second(work.args);
            } catch (...) {
                failed = true;
            }
        } else {
            // Unknown job name
            failed = true;
        }

        {
            auto lock = std::unique_lock(completionMutex_);
            completionQueue_.push({work.task, result, failed});
        }
    }
}

}  // namespace IkigaiScript
