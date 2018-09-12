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

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
namespace Bloomberg {
namespace quantum {

template <typename T, unsigned int SIZE>
StackAllocator<T,SIZE>::StackAllocator()
{
    static_assert(SIZE > 0, "Allocator buffer size must be > 0");
    
    //build the free stack
    for (size_t i = 0; i < SIZE; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::pointer StackAllocator<T,SIZE>::address(reference x) const
{
    return &x;
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::const_pointer StackAllocator<T,SIZE>::address(const_reference x) const
{
    return &x;
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::size_type StackAllocator<T,SIZE>::max_size() const
{
    return 1; //only 1 supported for now
}

template <typename T, unsigned int SIZE>
template <typename... Args >
void StackAllocator<T,SIZE>::construct(T* p, Args&&... args)
{
    new((void *)p) T(std::forward<Args>(args)...); // construct in-place
}

template <typename T, unsigned int SIZE>
void StackAllocator<T,SIZE>::destroy(pointer p)
{
    if (p != nullptr)
    {
        p->~T();
    }
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::pointer StackAllocator<T,SIZE>::allocate(size_type,
                                                                          std::allocator<void>::const_pointer)
{
    {
        SpinLock::Guard lock(_spinlock);
        if (!isEmpty())
        {
            return reinterpret_cast<pointer>(&_buffer[_freeBlocks[_freeBlockIndex--]]);
        }
    }
    // Use heap allocation
    ++_numHeapAllocatedBlocks;
    return (pointer)new char[sizeof(value_type)];
}

template <typename T, unsigned int SIZE>
void StackAllocator<T,SIZE>::deallocate(pointer p, size_type)
{
    if (p == nullptr) {
        return;
    }
    if (isStack(p)) {
        //find index of the block
        SpinLock::Guard lock(_spinlock);
        _freeBlocks[++_freeBlockIndex] = blockIndex(p);
    }
    else {
        delete[](char*)p;
        --_numHeapAllocatedBlocks;
        assert(_numHeapAllocatedBlocks >= 0);
    }
}

template <typename T, unsigned int SIZE>
template <typename... Args >
typename StackAllocator<T,SIZE>::pointer StackAllocator<T,SIZE>::create(Args&&... args)
{
    T* p = allocate();
    construct(p, std::forward<Args>(args)...);
    return p;
}

template <typename T, unsigned int SIZE>
void StackAllocator<T,SIZE>::dispose(pointer p)
{
    destroy(p);
    deallocate(p);
}

template <typename T, unsigned int SIZE>
size_t StackAllocator<T,SIZE>::allocatedBlocks() const
{
    return SIZE - _freeBlockIndex - 1;
}

template <typename T, unsigned int SIZE>
size_t StackAllocator<T,SIZE>::allocatedHeapBlocks() const
{
    return _numHeapAllocatedBlocks;
}

template <typename T, unsigned int SIZE>
bool StackAllocator<T,SIZE>::isFull() const
{
    return _freeBlockIndex == SIZE-1;
}

template <typename T, unsigned int SIZE>
bool StackAllocator<T,SIZE>::isEmpty() const
{
    return _freeBlockIndex == -1;
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::pointer StackAllocator<T,SIZE>::bufferStart()
{
    return reinterpret_cast<pointer>(_buffer);
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::pointer StackAllocator<T,SIZE>::bufferEnd()
{
    return reinterpret_cast<pointer>(_buffer + SIZE);
}

template <typename T, unsigned int SIZE>
bool StackAllocator<T,SIZE>::isStack(pointer p)
{
    return (bufferStart() <= p) && (p < bufferEnd());
}

template <typename T, unsigned int SIZE>
typename StackAllocator<T,SIZE>::index_type StackAllocator<T,SIZE>::blockIndex(pointer p)
{
    return reinterpret_cast<buffer_type*>(p) - _buffer;
}

}}
