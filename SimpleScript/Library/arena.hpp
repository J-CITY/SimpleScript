#pragma once

#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <cstddef>
#include <algorithm>

namespace IkigaiScript {

    class ArenaAllocator {
        struct Chunk {
            std::unique_ptr<uint8_t[]> data;
            size_t size;
            size_t used;
            Chunk(size_t size) : data(std::make_unique<uint8_t[]>(size)), size(size), used(0) {}
        };
        std::vector<Chunk> chunks;
        size_t defaultChunkSize;

        struct DestructorNode {
            void* obj;
            void (*destroy)(void*);
        };
        std::vector<DestructorNode> destructors;

    public:
        ArenaAllocator(size_t defaultChunkSize = 1024 * 1024) : defaultChunkSize(defaultChunkSize) {
            chunks.emplace_back(defaultChunkSize);
        }

        ~ArenaAllocator() {
            clear();
        }

        void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            auto& chunk = chunks.back();
            size_t aligned_used = (chunk.used + alignment - 1) & ~(alignment - 1);
            if (aligned_used + size > chunk.size) {
                size_t new_size = std::max(defaultChunkSize, size);
                chunks.emplace_back(new_size);
                aligned_used = 0;
            }
            chunks.back().used = aligned_used + size;
            return chunks.back().data.get() + aligned_used;
        }

        template<typename T, typename... Args>
        T* make(Args&&... args) {
            void* ptr = allocate(sizeof(T), alignof(T));
            T* obj = new (ptr) T(std::forward<Args>(args)...);
            if constexpr (!std::is_trivially_destructible_v<T>) {
                destructors.push_back({obj, [](void* p) { static_cast<T*>(p)->~T(); }});
            }
            return obj;
        }

        void clear() {
            for (auto it = destructors.rbegin(); it != destructors.rend(); ++it) {
                it->destroy(it->obj);
            }
            destructors.clear();
            chunks.clear();
            chunks.emplace_back(defaultChunkSize);
        }
    };

}
