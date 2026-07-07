#include "scheduler.hpp"

#include "../ikigaiScript.h"

namespace IkigaiScript {

std::atomic<uint64_t> ConcurrencyScheduler::nextTaskId_{0};

// ---------------------------------------------------------------------------

std::string ConcurrencyScheduler::makeTaskId(const std::string& funcName) {
    return funcName + "__task_" + std::to_string(++nextTaskId_);
}

// ---------------------------------------------------------------------------

TaskRef ConcurrencyScheduler::spawn(FunctionRef func, ScopePtr scope, Class* classs,
                                    TaskRef parentTask,
                                    CancellationToken::Ptr token) {
    auto task = std::make_shared<Task>();
    task->id     = makeTaskId(func->name);
    task->func   = func;
    task->scope  = scope;
    task->classs = classs;
    task->state  = TaskState::Created;

    if (token) {
        task->token = token;
    } else if (parentTask && parentTask->token) {
        task->token = parentTask->token->makeChild();
    } else {
        task->token = CancellationToken::make();
    }

    if (parentTask) {
        task->parent = parentTask;
        parentTask->children.push_back(task);
    }

    allTasks_.push_back(task);
    readyQueue_.push_back(task);
    return task;
}

// ---------------------------------------------------------------------------

void ConcurrencyScheduler::enqueue(TaskRef task) {
    if (task->state == TaskState::Suspended ||
        task->state == TaskState::Created) {
        task->state = TaskState::Suspended;
        readyQueue_.push_back(task);
    }
}

// ---------------------------------------------------------------------------

void ConcurrencyScheduler::suspendFor(TaskRef waiter, TaskRef waitee) {
    waiter->state        = TaskState::Suspended;
    waiter->awaitingTask = waitee;

    // Register: when waitee completes, re-enqueue waiter
    auto waiterWeak = std::weak_ptr<Task>(waiter);
    auto self = this;
    waitee->token->onCancel([waiterWeak, self]() {
        if (auto w = waiterWeak.lock()) {
            w->awaitingTask.reset();
            self->enqueue(w);
        }
    });
}

// ---------------------------------------------------------------------------

void ConcurrencyScheduler::cancel(TaskRef task) {
    if (task->token) task->token->cancel();
    // Children are cancelled recursively via the token tree
    for (auto& child : task->children) {
        if (child->isActive()) cancel(child);
    }
    task->state = TaskState::Cancelled;
}

// ---------------------------------------------------------------------------

bool ConcurrencyScheduler::resumeTask(IkigaiScriptInterpreter* interp, TaskRef task) {
    if (!task || !task->isActive()) return false;
    if (task->token && task->token->isCancelled()) {
        task->state = TaskState::Cancelled;
        onTaskCompleted(task);
        return false;
    }

    task->state = TaskState::Running;
    auto prev = currentTask_;
    currentTask_ = task;

    interp->callTask(task);

    currentTask_ = prev;

    if (task->state == TaskState::Running) {
        // callTask returned without suspending → task is done
        task->state = TaskState::Completed;
    }

    if (task->state == TaskState::Completed ||
        task->state == TaskState::Cancelled) {
        onTaskCompleted(task);
        return false;
    }
    return true;  // still alive (Suspended, will be re-enqueued when ready)
}

// ---------------------------------------------------------------------------

void ConcurrencyScheduler::onTaskCompleted(TaskRef task) {
    // Notify parent if it was awaiting this task
    if (auto parent = task->parent.lock()) {
        auto& awaiting = parent->awaitingTask;
        if (auto aw = awaiting.lock()) {
            if (aw == task) {
                awaiting.reset();
                enqueue(parent);
            }
        }
    }
}

// ---------------------------------------------------------------------------

int ConcurrencyScheduler::pump(IkigaiScriptInterpreter* interp, int maxSteps) {
    int steps = 0;
    while (!readyQueue_.empty()) {
        if (maxSteps >= 0 && steps >= maxSteps) break;
        auto task = readyQueue_.front();
        readyQueue_.pop_front();
        resumeTask(interp, task);
        ++steps;
    }
    return steps;
}

// ---------------------------------------------------------------------------

bool ConcurrencyScheduler::hasActiveTasks() const {
    for (auto& t : allTasks_) {
        if (t->isActive()) return true;
    }
    return false;
}

bool ConcurrencyScheduler::hasReadyTasks() const {
    return !readyQueue_.empty();
}

// ---------------------------------------------------------------------------

void ConcurrencyScheduler::clear() {
    readyQueue_.clear();
    allTasks_.clear();
    currentTask_.reset();
}

}  // namespace IkigaiScript
