/*
** Copyright 2018 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef BLOOMBERG_QUANTUM_STACK_ALLOCATOR_H
#define BLOOMBERG_QUANTUM_STACK_ALLOCATOR_H

#include <quantum/quantum_contiguous_pool_manager.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                            struct StackAllocator
//==============================================================================
/// @struct StackAllocator.
/// @brief Provides a stack-based object pool to the underlying ContiguousPoolManager.
///        The default buffer size is 1000.
/// @tparam T The type to allocate.
/// @tparam SIZE The size of the stack buffer.
/// @note This allocator is thread safe. For internal use only.
template <typename T, unsigned int SIZE>
struct StackAllocator : public ContiguousPoolManager<T>
{
    //------------------------------ Typedefs ----------------------------------
    typedef StackAllocator<T, SIZE> this_type;
    typedef T                       value_type;
    typedef value_type*             pointer;
    typedef const value_type*       const_pointer;
    typedef value_type&             reference;
    typedef const value_type&       const_reference;
    typedef size_t                  size_type;
    typedef uint16_t                index_type;
    typedef std::ptrdiff_t          difference_type;
    typedef std::false_type         propagate_on_container_move_assignment;
    typedef std::false_type         propagate_on_container_copy_assignment;
    typedef std::false_type         propagate_on_container_swap;
    typedef std::false_type         is_always_equal;
    typedef std::true_type          default_constructor;
    typedef std::aligned_storage<sizeof(value_type),
                                 alignof(value_type)> storage_type;
    typedef typename storage_type::type aligned_type;
    
    template <typename U>
    struct rebind
    {
        typedef StackAllocator<U,SIZE> other;
    };
    //------------------------------- Methods ----------------------------------
    StackAllocator() : ContiguousPoolManager<T>(_buffer, SIZE)
    {}
    
    StackAllocator(const this_type&) : StackAllocator()
    {}
    StackAllocator(this_type&&) : StackAllocator()
    {}
    StackAllocator& operator=(const this_type&) = delete;
    StackAllocator& operator=(this_type&&) = delete;
    
    // Rebound types
    template <typename U>
    StackAllocator(const StackAllocator<U,SIZE>&) : StackAllocator()
    {}
    template <typename U>
    StackAllocator(StackAllocator<U,SIZE>&&) : StackAllocator()
    {}
    template <typename U>
    StackAllocator& operator=(const StackAllocator<U,SIZE>&) = delete;
    template <typename U>
    StackAllocator& operator=(StackAllocator<U,SIZE>&&) = delete;
    
    static StackAllocator select_on_container_copy_construction(const StackAllocator&) {
        return StackAllocator();
    }
    bool operator==(const this_type& other) const {
        return this == &other;
    }
    bool operator!=(const this_type& other) const {
        return !operator==(other);
    }
    index_type size() const { return SIZE; }
    
private:
    //------------------------------- Members ----------------------------------
    aligned_type            _buffer[SIZE];
};

}} //namespaces

#endif //BLOOMBERG_QUANTUM_STACK_ALLOCATOR_H
