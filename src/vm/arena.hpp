#pragma once

#include <vector>

class arena {
public:
    arena(std::size_t initial = 4096) : capacity(initial) {}

    ~arena() {
        for (auto* block : blocks) delete block;
    }

    template<typename T, typename... Args>
    T* make(Args&&... args);
    void* alloc(size_t size, size_t align = alignof(max_align_t));
private:
    std::vector<std::byte*> blocks;
    std::byte* current = nullptr;

    size_t used = 0;
    size_t capacity = 0;

    void grow(size_t size);
};