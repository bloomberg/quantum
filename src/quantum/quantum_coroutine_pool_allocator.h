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
#ifndef QUANUM_COROUTINE_POOL_ALLOCATOR
#define QUANUM_COROUTINE_POOL_ALLOCATOR

#include <memory>
#include <assert.h>
#include <limits>
#include <type_traits>
#include <utility>
#include <quantum/quantum_spinlock.h>
#include <boost/context/stack_context.hpp>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                        struct CoroutinePoolAllocator
//==============================================================================
/// @struct CoroutinePoolAllocator.
/// @brief Provides fast (quasi zero-time) in-place allocation for coroutines.
///        Coroutine stacks are pre-allocated from separate (i.e. non-contiguous)
///        heap blocks and maintained in a reusable list.
/// @note This allocator is thread safe. For internal use only and is meant to
///       replace the boost fixed size pool allocator which crashes.
template <typename STACK_TRAITS>
struct CoroutinePoolAllocator
{
    //------------------------------ Typedefs ----------------------------------
    typedef CoroutinePoolAllocator<STACK_TRAITS>  this_type;
    typedef size_t                                size_type;
    typedef uint16_t                              index_type;
    typedef STACK_TRAITS                          traits;
    
    //------------------------------- Methods ----------------------------------
    CoroutinePoolAllocator(index_type size);
    CoroutinePoolAllocator(const this_type&) = delete;
    CoroutinePoolAllocator(this_type&&);
    CoroutinePoolAllocator& operator=(const this_type&) = delete;
    CoroutinePoolAllocator& operator=(this_type&&);
    virtual ~CoroutinePoolAllocator();
    
    // Accessors
    boost::context::stack_context allocate();
    void deallocate(const boost::context::stack_context& ctx);
    size_t allocatedBlocks() const;
    size_t allocatedHeapBlocks() const;
    bool isFull() const;
    bool isEmpty() const;
    
private:
    struct Header {
        int _pos;
    };
    
    int blockIndex(const boost::context::stack_context& ctx) const;
    bool isManaged(const boost::context::stack_context& ctx) const;
    Header* getHeader(const boost::context::stack_context& ctx) const;
    
    //------------------------------- Members ----------------------------------
    index_type          _size;
    Header**            _blocks;
    index_type*         _freeBlocks;
    ssize_t             _freeBlockIndex;
    size_t              _numHeapAllocatedBlocks;
    size_t              _stackSize;
    mutable SpinLock    _spinlock;
};

template <typename STACK_TRAITS>
struct CoroutinePoolAllocatorProxy
{
    typedef std::false_type default_constructor;
    
    CoroutinePoolAllocatorProxy(uint16_t size) : _alloc(new CoroutinePoolAllocator<STACK_TRAITS>(size))
    {
        if (!_alloc) {
            throw std::bad_alloc();
        }
    }
    // Accessors
    boost::context::stack_context allocate() { return _alloc->allocate(); }
    void deallocate(const boost::context::stack_context& ctx) { return _alloc->deallocate(ctx); }
    size_t allocatedBlocks() const { return _alloc->allocatedBlocks(); }
    size_t allocatedHeapBlocks() const { return _alloc->allocatedHeapBlocks(); }
    bool isFull() const { return _alloc->isFull(); }
    bool isEmpty() const { return _alloc->isEmpty(); }
private:
    std::shared_ptr<CoroutinePoolAllocator<STACK_TRAITS>> _alloc;
};

}} //namespaces

#include <quantum/impl/quantum_coroutine_pool_allocator_impl.h>


#endif //QUANUM_COROUTINE_POOL_ALLOCATOR
