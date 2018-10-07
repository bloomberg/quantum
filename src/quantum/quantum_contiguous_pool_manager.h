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
#ifndef QUANTUM_POOL_MANAGER_H
#define QUANTUM_POOL_MANAGER_H

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
    //------------------------------ Typedefs ----------------------------------
    typedef ContiguousPoolManager<T>        this_type;
    typedef T                               value_type;
    typedef value_type*                     pointer;
    typedef const value_type*               const_pointer;
    typedef value_type&                     reference;
    typedef const value_type&               const_reference;
    typedef size_t                          size_type;
    typedef uint16_t                        index_type;
    typedef std::aligned_storage<sizeof(T), alignof(T)> storage_type;
    typedef typename storage_type::type aligned_type;
    
    //------------------------------- Methods ----------------------------------
    ContiguousPoolManager();
    ContiguousPoolManager(aligned_type* buffer, index_type size);
    ContiguousPoolManager(const this_type&) = delete;
    ContiguousPoolManager(this_type&&);
    ContiguousPoolManager& operator=(const this_type&) = delete;
    ContiguousPoolManager& operator=(this_type&&);
    virtual ~ContiguousPoolManager();
    
    // Accessors
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
    
private:
    pointer bufferStart();
    pointer bufferEnd();
    bool isManaged(pointer p);
    index_type blockIndex(pointer p);

    //------------------------------- Members ----------------------------------
    index_type      _size{0};
    aligned_type*   _buffer{nullptr}; //non-owning
    index_type*     _freeBlocks{nullptr};
    ssize_t         _freeBlockIndex{-1};
    size_t          _numHeapAllocatedBlocks{0};
    SpinLock        _spinlock;
};

}} //namespaces

#include <quantum/impl/quantum_contiguous_pool_manager_impl.h>

#endif //QUANTUM_POOL_MANAGER_H

