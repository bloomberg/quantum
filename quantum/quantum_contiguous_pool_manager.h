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
#ifndef BLOOMBERG_QUANTUM_POOL_MANAGER_H
#define BLOOMBERG_QUANTUM_POOL_MANAGER_H

#include <memory>
#include <assert.h>
#include <limits>
#include <type_traits>
#include <utility>
#include <quantum/quantum_spinlock.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                        struct ContiguousPoolManager
//==============================================================================
/// @struct ContiguousPoolManager.
/// @brief Provides fast (quasi zero-time) in-place allocation for STL containers.
///        Objects are allocated from a contiguous buffer (aka object pool). When the
///        buffer is exhausted, allocation is delegated to the heap. The default
///        buffer size is 1000.
/// @tparam T The type to allocate.
/// @note This allocator is thread safe. For internal use only.
template <typename T>
struct ContiguousPoolManager
{
    template <typename U>
    friend struct ContiguousPoolManager;
    //------------------------------ Typedefs ----------------------------------
    typedef ContiguousPoolManager<T>        this_type;
    typedef T                               value_type;
    typedef value_type*                     pointer;
    typedef const value_type*               const_pointer;
    typedef value_type&                     reference;
    typedef const value_type&               const_reference;
    typedef size_t                          size_type;
    typedef uint16_t                        index_type;
    typedef std::ptrdiff_t                  difference_type;
    typedef std::true_type                  propagate_on_container_move_assignment;
    typedef std::true_type                  propagate_on_container_copy_assignment;
    typedef std::true_type                  propagate_on_container_swap;
    typedef std::false_type                 is_always_equal;
    typedef std::false_type                 default_constructor;
    typedef std::aligned_storage<sizeof(T), alignof(T)> storage_type;
    typedef typename storage_type::type     aligned_type;
    
    //------------------------------- Methods ----------------------------------
    ContiguousPoolManager();
    ContiguousPoolManager(aligned_type* buffer, index_type size);
    
    template <typename U>
    struct rebind
    {
        typedef ContiguousPoolManager<U> other;
    };
    //Rebound types
    template <typename U>
    ContiguousPoolManager(const ContiguousPoolManager<U>& other);
    template <typename U>
    ContiguousPoolManager(ContiguousPoolManager<U>&& other);
    template <typename U>
    ContiguousPoolManager& operator=(const ContiguousPoolManager<U>&) = delete;
    template <typename U>
    ContiguousPoolManager& operator=(ContiguousPoolManager<U>&&) = delete;
    
    static ContiguousPoolManager
    select_on_container_copy_construction(const ContiguousPoolManager& other) {
        return ContiguousPoolManager(other);
    }
    bool operator==(const this_type& other) const {
        return _control && other._control && (_control->_buffer == other._control->_buffer);
    }
    bool operator!=(const this_type& other) const {
        return !operator==(other);
    }
    
    //------------------------- Accessors ------------------------------
    void setBuffer(aligned_type* buffer, index_type size);
    pointer address(reference x) const;
    const_pointer address(const_reference x) const;
    size_type max_size() const;
    template <typename... Args >
    void construct(T* p, Args&&... args);
    void destroy(pointer p);
    pointer allocate(size_type = 1, const_pointer = 0);
    void deallocate(pointer p, size_type = 1);
    template <typename... Args >
    pointer create(Args&&... args);
    void dispose(pointer p);
    size_t allocatedBlocks() const;
    size_t allocatedHeapBlocks() const;
    bool isFull() const;
    bool isEmpty() const;
    index_type size() const;
    explicit operator bool() const;
    
private:
    pointer bufferStart();
    pointer bufferEnd();
    bool isManaged(pointer p);
    index_type blockIndex(pointer p);
    bool findContiguous(index_type n);

    //------------------------------- Members ----------------------------------
    struct Control {
        ~Control() {
            delete[] _freeBlocks;
        }
        index_type          _size{0};
        aligned_type*       _buffer{nullptr}; //non-owning
        index_type*         _freeBlocks{nullptr};
        ssize_t             _freeBlockIndex{-1};
        size_t              _numHeapAllocatedBlocks{0};
        mutable SpinLock    _spinlock;
    };
    std::shared_ptr<Control>  _control;
};

template <typename U, typename T>
size_t resize(size_t t_size)
{
    return (t_size*sizeof(U))/sizeof(T);
}

}} //namespaces

#include <quantum/impl/quantum_contiguous_pool_manager_impl.h>

#endif //BLOOMBERG_QUANTUM_POOL_MANAGER_H

