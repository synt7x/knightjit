#pragma once

#include <vector>
#include <utility>
#include <memory>
#include <new>
#include <cstddef>

namespace vm {

/// @brief Reference type for nodes allocated in the arena
using arena_id = std::size_t;

/**
 * @brief A memory arena for allocating nodes of type `T`,
 * nodes are always referenced by index.
 * 
 * @tparam T the type of the nodes to be allocated
 */
template<typename T>
class arena {
    static_assert(alignof(T) <= alignof(std::max_align_t), "T must not be over-aligned");

public:
    /**
     * @brief Construct a new arena object
     * 
     * @param cap The capacity of each block in the arena
     * @note The capacity must not be 0. If the capacity
     * is set to 0, it will be treated as 1.
     */
    arena(arena_id cap = 512) : capacity(cap) {
        if (capacity == 0) capacity = 1;
        grow();
    }

    /*
     * The arena is non-copyable, but movable.
     */
    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;
    arena(arena&&) noexcept = default;
    arena& operator=(arena&&) noexcept = default;

    /**
     * @brief Destroy the arena object
     */
    ~arena() {
        for (auto& b : blocks) {
            for (arena_id i = 0; i < b.size; i++) std::destroy_at(&b.nodes[i]);
            operator delete[](b.nodes);
        }
    }

    /**
     * @brief Constructs a new node of type `T` in the arena
     * 
     * @tparam Args the types of the arguments to construct a new node of type `T`
     * @param args the arguments to construct a new node of type `T`
     * @return `arena_id` the index of the new node
     */
    template<typename... Args>
    [[nodiscard]]
    arena_id create(Args&&... args) {
        if (blocks.back().size >= capacity) grow();

        block& b = blocks.back();
        T* slot = &b.nodes[b.size++];
        std::construct_at(slot, std::forward<Args>(args)...);
        return last_idx++;
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
    T& at(arena_id index) {
        arena_id block_idx = index / capacity;
        arena_id node_idx = index % capacity;

        return blocks[block_idx].nodes[node_idx];
    }
    
    /**
     * @overload
     * @note const version of `at()`
     */
    [[nodiscard]]
    const T& at(arena_id index) const {
        arena_id block_idx = index / capacity;
        arena_id node_idx = index % capacity;

        return blocks[block_idx].nodes[node_idx];
    }
private:
    /**
     * @brief A block of nodes allocated in the arena
     */
    struct block {
        /// @brief The array of nodes in the block
        T* nodes;
        
        /// @brief The number of currently allocated nodes
        arena_id size;
    };

    /// @brief Last index allocated in the arena
    arena_id last_idx = 0;

    /// @brief The number of nodes to allocate per block
    arena_id capacity;

    /**
     * @brief Grows the arena by allocating a new block
     */
    void grow() {
        blocks.emplace_back();
        auto& b = blocks.back();

        b.nodes = static_cast<T*>(operator new[](capacity * sizeof(T)));
        b.size = 0;
    }

    /// @brief The vector of blocks allocated in the arena
    std::vector<block> blocks;
};

} // namespace vm