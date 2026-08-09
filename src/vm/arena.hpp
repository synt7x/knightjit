#pragma once

#include "bump.hpp"

namespace vm {

/// @brief Reference type for nodes allocated in the arena
using arena_id = bump_id;

/**
 * @brief A memory arena for allocating nodes of type `T`,
 * nodes are always referenced by index.
 * 
 * @tparam T the type of the nodes to be allocated
 */
template<typename T>
class arena {
public:
    /**
     * @brief Construct a new arena object
     * 
     * @param cap The capacity of each block in the arena
     * @note The capacity must not be 0. If the capacity
     * is set to 0, it will be treated as 1.
     */
    arena(arena_id cap = 512) : allocator(cap * sizeof(T)) {}

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
        for (arena_id i = 0; i < last_idx; ++i) {
            T* node = reinterpret_cast<T*>(allocator.at(i * sizeof(T)));
            std::destroy_at(node);
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
        bump_id id = allocator.allocate(sizeof(T));
        T* slot = reinterpret_cast<T*>(allocator.pointer_at(id));

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
        return *reinterpret_cast<T*>(allocator.pointer_at(index * sizeof(T)));
    }
    
    /**
     * @overload
     * @note const version of `at()`
     */
    [[nodiscard]]
    const T& at(arena_id index) const {
        return *reinterpret_cast<const T*>(allocator.pointer_at(index * sizeof(T)));
    }

    /**
     * @brief The bump allocator used to allocate memory for the arena.
     */
    bump allocator;

    /// @brief Last index allocated in the arena
    arena_id last_idx = 0;

    /*
    * Special alignment of types is not allowed to be used
    * in the arena. Allocated types must fit inside of CPU
    * registers to guarantee performance.
    */
    static_assert(alignof(T) <= alignof(std::max_align_t), "T must not be over-aligned");
};

} // namespace vm