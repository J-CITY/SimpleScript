#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

// value.hpp is included after the pool machinery so makeValue/copyValue see a
// complete Value type, but the pool itself only needs forward declarations.

namespace IkigaiScript {

struct Value;

// ─── ValuePool ────────────────────────────────────────────────────────────────
//
// Slab-based freelist pool for shared_ptr<Value> allocations.
//
// Usage model:
//   - All heap-allocated Value objects must be created through makeValue() or
//     copyValue() below, which use std::allocate_shared with PoolAllocator.
//   - std::allocate_shared allocates the Value + its shared_ptr control block
//     as a single contiguous chunk.  The chunk size is captured on the first
//     allocation call (it is constant for a given compiler/platform/sizeof(Value)).
//   - Freed chunks are returned to a per-pool free list and reused immediately
//     on the next makeValue, avoiding repeated calls to the system allocator.
//   - Thread safety: NOT thread-safe.  The interpreter is single-threaded
//     (worker threads in NativeJobPool never allocate Values directly).

class ValuePool {
    // Slab size chosen to match the AST arena for consistency.
    static constexpr size_t kSlabSize = 512 * 1024;

    // Free list node overlaid onto a freed block (blockSize_ >= sizeof(void*)).
    struct FreeNode { FreeNode* next = nullptr; };

    struct Slab {
        std::unique_ptr<uint8_t[]> data;
        size_t used     = 0;
        size_t capacity = 0;
        explicit Slab(size_t cap)
            : data(std::make_unique<uint8_t[]>(cap)), capacity(cap) {}
    };

    // Set to sizeof(control_block+Value) on first allocate() call; never changes.
    size_t    blockSize_ = 0;
    FreeNode* freeHead_  = nullptr;
    std::vector<Slab> slabs_;

    void* allocFromSlab() {
        if (slabs_.empty()
            || slabs_.back().used + blockSize_ > slabs_.back().capacity) {
            slabs_.emplace_back(std::max(kSlabSize, blockSize_));
        }
        auto& slab = slabs_.back();
        void* p    = slab.data.get() + slab.used;
        slab.used += blockSize_;
        return p;
    }

public:
    // Called by PoolAllocator<T>::allocate(n).
    // With allocate_shared<Value>, n==1 and T is the STL-internal combined type.
    void* allocate(size_t bytes) {
        if (blockSize_ == 0) blockSize_ = bytes;  // probe block size on first call

        if (bytes != blockSize_) [[unlikely]] {
            // Unexpected size (shouldn't happen in normal use) — fall through to heap.
            return ::operator new(bytes);
        }

        if (freeHead_) {
            FreeNode* n = freeHead_;
            freeHead_   = n->next;
            return n;
        }
        return allocFromSlab();
    }

    void deallocate(void* p, size_t bytes) noexcept {
        if (blockSize_ == 0 || bytes != blockSize_) [[unlikely]] {
            ::operator delete(p);
            return;
        }
        // Overlay a FreeNode on the freed memory (no ctor/dtor side-effects).
        auto* n  = reinterpret_cast<FreeNode*>(p);
        n->next  = freeHead_;
        freeHead_ = n;
    }

    // Returns the number of free blocks currently cached in the free list.
    size_t freeListDepth() const noexcept {
        size_t d = 0;
        for (auto* n = freeHead_; n; n = n->next) ++d;
        return d;
    }

    // Returns the number of slab chunks that have been allocated.
    size_t slabCount() const noexcept { return slabs_.size(); }
};

// ─── PoolAllocator ────────────────────────────────────────────────────────────
//
// STL-compatible allocator that routes all allocations through a ValuePool.
// The rebind mechanism ensures that allocate_shared's internal combined
// allocation (control block + Value) goes through the pool.

template<typename T>
struct PoolAllocator {
    using value_type                             = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;

    ValuePool* pool;

    explicit PoolAllocator(ValuePool& p) noexcept : pool(&p) {}

    // Rebind copy-constructor — called by allocate_shared internals.
    template<typename U>
    PoolAllocator(const PoolAllocator<U>& o) noexcept : pool(o.pool) {}

    T* allocate(size_t n) {
        return static_cast<T*>(pool->allocate(sizeof(T) * n));
    }

    void deallocate(T* p, size_t n) noexcept {
        pool->deallocate(p, sizeof(T) * n);
    }

    template<typename U>
    bool operator==(const PoolAllocator<U>& o) const noexcept { return pool == o.pool; }
    template<typename U>
    bool operator!=(const PoolAllocator<U>& o) const noexcept { return pool != o.pool; }
};

} // namespace IkigaiScript

// Include Value definition here so makeValue/copyValue below can use it.
// value.hpp does not include value_pool.hpp, so there is no circular dependency.
#include "value.hpp"

namespace IkigaiScript {

// ─── Factory functions ────────────────────────────────────────────────────────
//
// Use these everywhere instead of std::make_shared<Value>.

template<typename... Args>
inline ValuePtr makeValue(ValuePool& pool, Args&&... args) {
    return std::allocate_shared<Value>(
        PoolAllocator<Value>{pool}, std::forward<Args>(args)...);
}

inline ValuePtr copyValue(ValuePool& pool, const Value& v) {
    return std::allocate_shared<Value>(PoolAllocator<Value>{pool}, v);
}

} // namespace IkigaiScript
