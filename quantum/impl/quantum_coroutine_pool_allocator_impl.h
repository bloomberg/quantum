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
#include <cassert>
#include <algorithm>
#include <cstring>

#if defined(_WIN32) && !defined(__CYGWIN__)
    //TODO: Windows headers for memory mapping and page protection
#else
    #include <sys/mman.h>
#endif

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
    _blocks(nullptr),
    _freeBlocks(nullptr),
    _freeBlockIndex(size-1), //point to the last element
    _numHeapAllocatedBlocks(0),
    _stackSize(std::min(std::max(traits::default_size(),
                                 traits::minimum_size()),
                        traits::maximum_size()))
{
    if ((_size == 0) || (_stackSize == 0))
    {
        throw std::runtime_error("Invalid coroutine allocator size specification");
    }
    //Make sure the stack is a multiple of the system page size
    size_t remainder = _stackSize % traits::page_size();
    if (remainder > 0) {
        //extend to the next page
        _stackSize += (traits::page_size()-remainder);
    }
    _freeBlocks = new index_type[size];
    if (!_freeBlocks)
    {
        throw std::bad_alloc();
    }
    _blocks = new uint8_t*[size];
    if (!_blocks)
    {
        delete[] _freeBlocks;
        throw std::bad_alloc();
    }
    //pre-allocate all the coroutine stack blocks and protect the last stack page to
    //track coroutine stack overflows.
    for (size_t i = 0; i < size; ++i)
    {
        _blocks[i] = allocateCoroutine(ProtectMemPage::On);
        if (!_blocks[i])
        {
            deallocateBlocks(i);
            throw std::bad_alloc();
        }
        //set the block position
        header(_blocks[i])->_pos = i;
    }
    //initialize the free block list
    std::iota(_freeBlocks, _freeBlocks + size, 0);
}

template <typename STACK_TRAITS>
CoroutinePoolAllocator<STACK_TRAITS>::CoroutinePoolAllocator(CoroutinePoolAllocator<STACK_TRAITS>&& other) noexcept
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
    deallocateBlocks(_size);
}

template <typename STACK_TRAITS>
void CoroutinePoolAllocator<STACK_TRAITS>::deallocateBlocks(size_t pos)
{
    for (size_t j = 0; j < pos; ++j)
    {
        deallocateCoroutine(_blocks[j]);
    }
    delete[] _blocks;
    delete[] _freeBlocks;
}

template <typename STACK_TRAITS>
uint8_t* CoroutinePoolAllocator<STACK_TRAITS>::allocateCoroutine(ProtectMemPage protect) const
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    return new uint8_t[_stackSize];
#else
    uint8_t* block = (uint8_t*)mmap(nullptr,
                                    _stackSize,
                                    PROT_WRITE | PROT_READ | PROT_EXEC,
                                    MAP_ANONYMOUS | MAP_PRIVATE,
                                    -1, //invalid fd
                                    0); //no offset
    if (block == MAP_FAILED)
    {
        return nullptr;
    }
    //Add protection to the lowest page
    if ((protect == ProtectMemPage::On) &&
         mprotect(block, traits::page_size(), PROT_NONE) != 0)
    {
        munmap(block, _stackSize); //free region
        return nullptr;
    }
    return block;
#endif
}

template <typename STACK_TRAITS>
int CoroutinePoolAllocator<STACK_TRAITS>::deallocateCoroutine(uint8_t* block) const
{
    assert(block);
#if defined(_WIN32) && !defined(__CYGWIN__)
    delete[] block;
    return 0;
#else
    return munmap(block, _stackSize);
#endif
}

template <typename STACK_TRAITS>
boost::context::stack_context CoroutinePoolAllocator<STACK_TRAITS>::allocate() {
    uint8_t* block = nullptr;
    {
        SpinLock::Guard lock(_spinlock);
        if (!isEmpty())
        {
            block = _blocks[_freeBlocks[_freeBlockIndex--]];
        }
    }
    if (!block)
    {
        //Do not protect last memory page for performance reasons
        block = allocateCoroutine(ProtectMemPage::Off);
        if (!block)
        {
            throw std::bad_alloc();
        }
        header(block)->_pos = -1; //mark position as non-managed
        SpinLock::Guard lock(_spinlock);
        ++_numHeapAllocatedBlocks;
    }
    //populate stack context
    boost::context::stack_context ctx;
    ctx.size = _stackSize - sizeof(Header);
    ctx.sp = block + ctx.size;
#if defined(BOOST_USE_VALGRIND)
    ctx.valgrind_stack_id = VALGRIND_STACK_REGISTER(ctx.sp, block);
#endif
    return ctx;
}

template <typename STACK_TRAITS>
void CoroutinePoolAllocator<STACK_TRAITS>::deallocate(const boost::context::stack_context& ctx) {
    if (!ctx.sp)
    {
        return;
    }
#if defined(BOOST_USE_VALGRIND)
    VALGRIND_STACK_DEREGISTER(ctx.valgrind_stack_id);
#endif
    int bi = blockIndex(ctx);
    assert(bi >= -1 && bi < _size); //guard against coroutine stack overflow or corruption
    if (isManaged(ctx))
    {
        //find index of the block
        SpinLock::Guard lock(_spinlock);
        _freeBlocks[++_freeBlockIndex] = bi;
    }
    else
    {
        //Unlink coroutine stack
        {
            SpinLock::Guard lock(_spinlock);
            --_numHeapAllocatedBlocks;
            assert(_numHeapAllocatedBlocks >= 0);
        }
        if (deallocateCoroutine(stackEnd(ctx)) != 0)
        {
            throw std::runtime_error("Bad de-allocation");
        }
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
CoroutinePoolAllocator<STACK_TRAITS>::header(const boost::context::stack_context& ctx) const
{
    return reinterpret_cast<Header*>(ctx.sp);
}

template <typename STACK_TRAITS>
typename CoroutinePoolAllocator<STACK_TRAITS>::Header*
CoroutinePoolAllocator<STACK_TRAITS>::header(uint8_t* block) const
{
    return reinterpret_cast<Header*>(block + _stackSize - sizeof(Header));
}

template <typename STACK_TRAITS>
uint8_t*
CoroutinePoolAllocator<STACK_TRAITS>::stackEnd(const boost::context::stack_context& ctx) const
{
    return static_cast<uint8_t*>(ctx.sp) - ctx.size;
}

template <typename STACK_TRAITS>
bool CoroutinePoolAllocator<STACK_TRAITS>::isManaged(const boost::context::stack_context& ctx) const
{
    return blockIndex(ctx) != -1;
}

template <typename STACK_TRAITS>
int CoroutinePoolAllocator<STACK_TRAITS>::blockIndex(const boost::context::stack_context& ctx) const
{
    return header(ctx)->_pos;
}

}}
