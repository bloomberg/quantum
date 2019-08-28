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
//NOTE: DO NOT INCLUDE DIRECTLY
#include <type_traits>
#include <algorithm>
#include <assert.h>

#if defined(BOOST_USE_VALGRIND)
    #include <valgrind/valgrind.h>
#endif

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
namespace Bloomberg {
namespace quantum {

template <typename STACK_TRAITS>
CoroutinePoolAllocator<STACK_TRAITS>::CoroutinePoolAllocator(index_type size) :
    _size(size),
    _blocks(new Header*[size]),
    _freeBlocks(new index_type[size]),
    _freeBlockIndex(size-1),
    _numHeapAllocatedBlocks(0),
    _stackSize(std::min(std::max(traits::default_size(), traits::minimum_size()), traits::maximum_size()))
{
    if (!_blocks || !_freeBlocks) {
        throw std::bad_alloc();
    }
    if (_size == 0) {
        throw std::runtime_error("Invalid coroutine allocator pool size");
    }
    //pre-allocate all the coroutine stack blocks
    for (index_type i = 0; i < size; ++i) {
        _blocks[i] = reinterpret_cast<Header*>(new char[_stackSize]);
        if (!_blocks[i]) {
            throw std::bad_alloc();
        }
        _blocks[i]->_pos = i; //mark position
    }
    //initialize the free block list
    for (index_type i = 0; i < size; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename STACK_TRAITS>
CoroutinePoolAllocator<STACK_TRAITS>::CoroutinePoolAllocator(CoroutinePoolAllocator<STACK_TRAITS>&& other)
{
    *this = other;
}

template <typename STACK_TRAITS>
CoroutinePoolAllocator<STACK_TRAITS>& CoroutinePoolAllocator<STACK_TRAITS>::operator=(CoroutinePoolAllocator<STACK_TRAITS>&& other)
{
    _size = other._size;
    _blocks = other._blocks;
    _freeBlocks = other._freeBlocks;
    _freeBlockIndex = other._freeBlockIndex;
    _numHeapAllocatedBlocks = other._numHeapAllocatedBlocks;
    
    // Reset other
    other._blocks = nullptr;
    other._freeBlocks = nullptr;
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename STACK_TRAITS>
CoroutinePoolAllocator<STACK_TRAITS>::~CoroutinePoolAllocator()
{
    for (size_t i = 0; i < _size; ++i) {
        delete[] (char*)_blocks[i];
    }
    delete[] _blocks;
    delete[] _freeBlocks;
}

template <typename STACK_TRAITS>
boost::context::stack_context CoroutinePoolAllocator<STACK_TRAITS>::allocate() {
    boost::context::stack_context ctx;
    Header* block = nullptr;
    {
        SpinLock::Guard lock(_spinlock);
        if (!isEmpty())
        {
            block = _blocks[_freeBlocks[_freeBlockIndex--]];
        }
    }
    if (!block) {
        // Use heap allocation
        block = (Header*)new char[_stackSize];
        if (!block) {
            throw std::bad_alloc();
        }
        block->_pos = -1; //mark position as non-managed
        SpinLock::Guard lock(_spinlock);
        ++_numHeapAllocatedBlocks;
    }
    char* block_start = reinterpret_cast<char*>(block) + sizeof(Header);
    ctx.size = _stackSize - sizeof(Header);
    ctx.sp = block_start + ctx.size;
    #if defined(BOOST_USE_VALGRIND)
        ctx.valgrind_stack_id = VALGRIND_STACK_REGISTER(ctx.sp, block_start);
    #endif
    return ctx;
}

template <typename STACK_TRAITS>
void CoroutinePoolAllocator<STACK_TRAITS>::deallocate(const boost::context::stack_context& ctx) {
    if (!ctx.sp) {
        return;
    }
#if defined(BOOST_USE_VALGRIND)
    VALGRIND_STACK_DEREGISTER(ctx.valgrind_stack_id);
#endif
    int bi = blockIndex(ctx);
    assert(bi >= -1 && bi < _size); //guard against coroutine stack overflow or corruption
    if (isManaged(ctx)) {
        //find index of the block
        SpinLock::Guard lock(_spinlock);
        _freeBlocks[++_freeBlockIndex] = bi;
    }
    else {
        delete[] (char*)getHeader(ctx);
        SpinLock::Guard lock(_spinlock);
        --_numHeapAllocatedBlocks;
        assert(_numHeapAllocatedBlocks >= 0);
    }
}

template <typename STACK_TRAITS>
size_t CoroutinePoolAllocator<STACK_TRAITS>::allocatedBlocks() const
{
    return _size - _freeBlockIndex - 1;
}

template <typename STACK_TRAITS>
size_t CoroutinePoolAllocator<STACK_TRAITS>::allocatedHeapBlocks() const
{
    return _numHeapAllocatedBlocks;
}

template <typename STACK_TRAITS>
bool CoroutinePoolAllocator<STACK_TRAITS>::isFull() const
{
    return _freeBlockIndex == _size-1;
}

template <typename STACK_TRAITS>
bool CoroutinePoolAllocator<STACK_TRAITS>::isEmpty() const
{
    return _freeBlockIndex == -1;
}

template <typename STACK_TRAITS>
typename CoroutinePoolAllocator<STACK_TRAITS>::Header*
CoroutinePoolAllocator<STACK_TRAITS>::getHeader(const boost::context::stack_context& ctx) const
{
    return reinterpret_cast<Header*>(reinterpret_cast<char*>(ctx.sp) - ctx.size - sizeof(Header));
}

template <typename STACK_TRAITS>
bool CoroutinePoolAllocator<STACK_TRAITS>::isManaged(const boost::context::stack_context& ctx) const
{
    return blockIndex(ctx) != -1;
}

template <typename STACK_TRAITS>
int CoroutinePoolAllocator<STACK_TRAITS>::blockIndex(const boost::context::stack_context& ctx) const
{
    return getHeader(ctx)->_pos;
}

}}
