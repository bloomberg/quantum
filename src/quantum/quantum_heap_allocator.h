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
#ifndef QUANTUM_HEAP_ALLOCATOR_H
#define QUANTUM_HEAP_ALLOCATOR_H

#include <quantum/quantum_contiguous_pool_manager.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                            struct HeapAllocator
//==============================================================================
/// @struct HeapAllocator.
/// @brief Provides a heap-based object pool to the underlying ContiguousPoolManager.
///        The default buffer size is 1000.
/// @tparam T The type to allocate.
/// @note This allocator is thread safe. For internal use only.
template <typename T>
struct HeapAllocator : public ContiguousPoolManager<T>
{
    //------------------------------ Typedefs ----------------------------------
    typedef HeapAllocator<T>        this_type;
    typedef T                       value_type;
    typedef value_type*             pointer;
    typedef const value_type*       const_pointer;
    typedef value_type&             reference;
    typedef const value_type&       const_reference;
    typedef uint16_t                size_type;
    typedef std::ptrdiff_t          difference_type;
    typedef std::true_type          propagate_on_container_move_assignment;
    typedef std::false_type         propagate_on_container_copy_assignment;
    typedef std::true_type          propagate_on_container_swap;
    typedef std::true_type          is_always_equal;
    typedef std::false_type         default_constructor;
    typedef std::aligned_storage<sizeof(value_type),
                                 alignof(value_type)> storage_type;
    typedef typename storage_type::type aligned_type;
    
    template <typename U>
    struct rebind
    {
        typedef HeapAllocator<U> other;
    };
    //------------------------------- Methods ----------------------------------
    HeapAllocator(uint16_t size) :
        _size(size),
        _buffer(new aligned_type[size])
    {
        if (!_buffer) {
            throw std::bad_alloc();
        }
        this->setBuffer(_buffer, _size);
    }
    HeapAllocator(const this_type& other) :
        HeapAllocator(other._size)
    {}
    HeapAllocator(this_type&& other)
    {
        *this = std::move(other);
    }
    HeapAllocator& operator=(const this_type& other)
    {}
    HeapAllocator& operator=(this_type&& other)
    {
        static_cast<ContiguousPoolManager<T>>(*this) = static_cast<ContiguousPoolManager<T>&&>(other);
        _size = other._size;
        _buffer = other._buffer;
        other._size = 0;
        other._buffer = nullptr;
    }
    ~HeapAllocator() {
        delete[] _buffer;
    }
    static HeapAllocator select_on_container_copy_construction(const HeapAllocator& other) {
        return HeapAllocator(other.size());
    }
    template <typename U>
    HeapAllocator(const HeapAllocator<U>& other) : HeapAllocator(other.size())
    {}
    template <typename U>
    HeapAllocator& operator=(const HeapAllocator<U>&)
    {}
    bool operator==(const this_type& other) const {
        return true;
    }
    bool operator!=(const this_type& other) const {
        return false;
    }
    size_type size() const { return _size; }
    
private:
    //------------------------------- Members ----------------------------------
    size_type       _size;
    aligned_type*   _buffer;
};

}} //namespaces

#endif //QUANTUM_HEAP_ALLOCATOR_H
