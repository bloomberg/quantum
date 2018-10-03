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

template <typename STACK_TRAITS, unsigned int SIZE>
CoroutinePoolAllocator<STACK_TRAITS, SIZE>::CoroutinePoolAllocator() :
    _freeBlockIndex(SIZE-1),
    _numHeapAllocatedBlocks(0),
    _stackSize(std::min(std::max(traits::default_size(), traits::minimum_size()), traits::maximum_size()))
{
    static_assert(SIZE < std::numeric_limits<unsigned short>::max(), "Pool size too large");
    assert(_stackSize);
    
    //pre-allocate all the coroutine stack blocks
    for (size_t i = 0; i < SIZE; ++i) {
        _blocks[i] = reinterpret_cast<Header*>(new char[_stackSize]);
        if (!_blocks[i]) {
            throw std::bad_alloc();
        }
        reinterpret_cast<Header*>(_blocks[i])->_pos = i; //mark position
    }
    //initialize the free block list
    for (size_t i = 0; i < SIZE; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename STACK_TRAITS, unsigned int SIZE>
CoroutinePoolAllocator<STACK_TRAITS,SIZE>::CoroutinePoolAllocator(CoroutinePoolAllocator<STACK_TRAITS,SIZE>&& other)
{
    std::copy(std::begin(_blocks), std::end(_blocks), std::begin(other._blocks));
    std::copy(std::begin(_freeBlocks), std::end(_freeBlocks), std::begin(other._freeBlocks));
    _freeBlockIndex = other._freeBlockIndex;
    _numHeapAllocatedBlocks = other._numHeapAllocatedBlocks;
    
    // Reset other
    for (size_t i = 0; i < SIZE; _blocks[++i] = nullptr);
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename STACK_TRAITS, unsigned int SIZE>
CoroutinePoolAllocator<STACK_TRAITS,SIZE>& CoroutinePoolAllocator<STACK_TRAITS,SIZE>::operator=(CoroutinePoolAllocator<STACK_TRAITS,SIZE>&& other)
{
    std::copy(std::begin(_blocks), std::end(_blocks), std::begin(other._blocks));
    std::copy(std::begin(_freeBlocks), std::end(_freeBlocks), std::begin(other._freeBlocks));
    _freeBlockIndex = other._freeBlockIndex;
    _numHeapAllocatedBlocks = other._numHeapAllocatedBlocks;
    
    // Reset other
    for (size_t i = 0; i < SIZE; _blocks[++i] = nullptr);
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename STACK_TRAITS, unsigned int SIZE>
CoroutinePoolAllocator<STACK_TRAITS,SIZE>::~CoroutinePoolAllocator()
{
    for (size_t i = 0; i < SIZE; ++i) {
        delete[] (char*)_blocks[i];
    }
}

template <typename STACK_TRAITS, unsigned int SIZE>
boost::context::stack_context CoroutinePoolAllocator<STACK_TRAITS, SIZE>::allocate() {
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

template <typename STACK_TRAITS, unsigned int SIZE>
void CoroutinePoolAllocator<STACK_TRAITS, SIZE>::deallocate(const boost::context::stack_context& ctx) {
    if (!ctx.sp) {
        return;
    }
#if defined(BOOST_USE_VALGRIND)
    VALGRIND_STACK_DEREGISTER(ctx.valgrind_stack_id);
#endif
    if (isManaged(ctx)) {
        //find index of the block
        SpinLock::Guard lock(_spinlock);
        _freeBlocks[++_freeBlockIndex] = blockIndex(ctx);
    }
    else {
        delete[] (char*)getHeader(ctx);
        SpinLock::Guard lock(_spinlock);
        --_numHeapAllocatedBlocks;
        assert(_numHeapAllocatedBlocks >= 0);
    }
}

template <typename STACK_TRAITS, unsigned int SIZE>
size_t CoroutinePoolAllocator<STACK_TRAITS,SIZE>::allocatedBlocks() const
{
    return SIZE - _freeBlockIndex - 1;
}

template <typename STACK_TRAITS, unsigned int SIZE>
size_t CoroutinePoolAllocator<STACK_TRAITS,SIZE>::allocatedHeapBlocks() const
{
    return _numHeapAllocatedBlocks;
}

template <typename STACK_TRAITS, unsigned int SIZE>
bool CoroutinePoolAllocator<STACK_TRAITS,SIZE>::isFull() const
{
    return _freeBlockIndex == SIZE-1;
}

template <typename STACK_TRAITS, unsigned int SIZE>
bool CoroutinePoolAllocator<STACK_TRAITS,SIZE>::isEmpty() const
{
    return _freeBlockIndex == -1;
}

template <typename STACK_TRAITS, unsigned int SIZE>
typename CoroutinePoolAllocator<STACK_TRAITS,SIZE>::Header*
CoroutinePoolAllocator<STACK_TRAITS,SIZE>::getHeader(const boost::context::stack_context& ctx) const
{
    return reinterpret_cast<Header*>(reinterpret_cast<char*>(ctx.sp) - ctx.size - sizeof(Header));
}

template <typename STACK_TRAITS, unsigned int SIZE>
bool CoroutinePoolAllocator<STACK_TRAITS,SIZE>::isManaged(const boost::context::stack_context& ctx) const
{
    return blockIndex(ctx) != -1;
}

template <typename STACK_TRAITS, unsigned int SIZE>
int CoroutinePoolAllocator<STACK_TRAITS,SIZE>::blockIndex(const boost::context::stack_context& ctx) const
{
    return getHeader(ctx)->_pos;
}

}}
