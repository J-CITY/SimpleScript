#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <string>

namespace IkigaiScript {

// ---------------------------------------------------------------------------
// CancellationToken — cooperative cancellation tree
// ---------------------------------------------------------------------------
// A parent token cancels all its children when cancel() is called.
// Script tasks own a token; structured-concurrency blocks (sync/race/branch)
// create child tokens so cancellation propagates down the tree.
// ---------------------------------------------------------------------------

class CancellationToken : public std::enable_shared_from_this<CancellationToken> {
public:
    using Ptr = std::shared_ptr<CancellationToken>;

    static Ptr make() { return std::make_shared<CancellationToken>(); }

    // Create a child token that is cancelled when this token is cancelled
    Ptr makeChild() {
        auto child = make();
        auto lock = std::unique_lock(mtx_);
        if (cancelled_.load(std::memory_order_relaxed)) {
            child->cancel();
        }
        children_.push_back(child);
        return child;
    }

    // Register a callback invoked synchronously when cancel() is called
    void onCancel(std::function<void()> fn) {
        auto lock = std::unique_lock(mtx_);
        if (cancelled_.load(std::memory_order_relaxed)) {
            lock.unlock();
            fn();
            return;
        }
        callbacks_.push_back(std::move(fn));
    }

    void cancel() {
        bool expected = false;
        if (!cancelled_.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            return;  // already cancelled
        }
        std::vector<std::function<void()>> cbs;
        std::vector<Ptr> kids;
        {
            auto lock = std::unique_lock(mtx_);
            std::swap(cbs, callbacks_);
            std::swap(kids, children_);
        }
        for (auto& cb : cbs) cb();
        for (auto& kid : kids) kid->cancel();
    }

    bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> cancelled_{false};
    std::mutex mtx_;
    std::vector<std::function<void()>> callbacks_;
    std::vector<Ptr> children_;
};

}  // namespace IkigaiScript
