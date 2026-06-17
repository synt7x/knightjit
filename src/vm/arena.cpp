#include "arena.hpp"

template<typename T, typename... Args>
T* arena::make(Args&&... args) {
    void* ptr = alloc(sizeof(T), alignof(T));
    return new (ptr) T(std::forward<Args>(args)...);
}

void* arena::alloc(size_t size, size_t align) {
    size_t offset = (used + align - 1) & ~(align - 1);

    if (offset + size > capacity) {
        grow(std::max(capacity * 2, size + align));

        offset = (used + align - 1) & ~(align - 1);
    }

    void* ptr = current + offset;
    used = offset + size;

    return ptr;
}

void arena::grow(std::size_t size) {
    auto* block = static_cast<std::byte*>(::operator new(size));

    blocks.push_back(block);

    current = block;
    used = 0;
    capacity = size;
}