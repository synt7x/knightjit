#pragma once

#include <vector>
#include <cstdint>

namespace vm {
using bump_id = std::size_t;

class bump {
public:
    /**
     * @brief Construct a new bump allocator object
     * 
     * @param cap The capacity of each block in the bump allocator
     * @note The capacity must not be 0. If the capacity
     * is set to 0, it will be treated as 1.
     */
    bump(bump_id cap = 512) : capacity(cap) {
        if (capacity == 0) capacity = 1;
        grow();
    }

    /*
     * The bump allocator is non-copyable, but movable.
     */
    bump(const bump&) = delete;
    bump& operator=(const bump&) = delete;
    bump(bump&&) noexcept = default;
    bump& operator=(bump&&) noexcept = default;

    /**
     * @brief Destroy the bump allocator object
     */
    ~bump() {
        for (auto& b : blocks) {
            operator delete[](b.nodes);
        }
    }

    /**
     * @brief Allocates memory of size `size`
     * 
     * @param size the size of the memory to allocate
     * @return `bump_id` the index of the new node
     */
    [[nodiscard]]
    bump_id allocate(std::size_t size) {
        if (blocks.back().size + size > capacity) grow();

        auto& b = blocks.back();
        b.size += size;

        bump_id idx = index;
        index += size;

        return idx;
    }
    
    /**
     * @brief Retrieves a reference to the node at the specified index
     * the reference will be valid until the arena is freed.
     * 
     * @param index the index of the node to retrieve
     * @return `T&` a reference to the node at the specified index
     * @note the index must be valid, i.e. less than the number of nodes allocated in the arena
     */
    [[nodiscard]]
    std::byte& at(bump_id index) {
        bump_id block_idx = index / capacity;
        bump_id node_idx = index % capacity;

        return blocks[block_idx].nodes[node_idx];
    }
    
    /**
     * @overload
     * @note const version of `at()`
     */
    [[nodiscard]]
    const std::byte& at(bump_id index) const {
        bump_id block_idx = index / capacity;
        bump_id node_idx = index % capacity;

        return blocks[block_idx].nodes[node_idx];
    }

    /**
     * @brief Get pointer to object at `index`
     * 
     * @param index 
     * @return pointer
     */
    [[nodiscard]]
    std::byte* pointer_at(bump_id index) {
        bump_id block_idx = index / capacity;
        bump_id node_idx = index % capacity;

        return &blocks[block_idx].nodes[node_idx];
    }

    /**
     * @overload
     * @note const version of `pointer_at()`
     */
    [[nodiscard]]
    const std::byte* pointer_at(bump_id index) const {
        bump_id block_idx = index / capacity;
        bump_id node_idx = index % capacity;

        return &blocks[block_idx].nodes[node_idx];
    }

    /**
     * @brief A block of nodes allocated in the arena
     */
    struct block {
        /// @brief The array of nodes in the block
        std::byte* nodes;
        
        /// @brief The number of currently allocated nodes
        bump_id size;
    };

    /// @brief Last index allocated in the arena
    bump_id index = 0;

    /// @brief The number of nodes to allocate per block
    bump_id capacity;

    /**
     * @brief Grows the arena by allocating a new block
     */
    void grow() {
        blocks.emplace_back();
        auto& b = blocks.back();

        
        b.nodes = static_cast<std::byte*>(operator new[](capacity));
        b.size = 0;
    }

    /// @brief The vector of blocks allocated in the arena
    std::vector<block> blocks;
};

}