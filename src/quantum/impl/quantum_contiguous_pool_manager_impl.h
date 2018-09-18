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
ContiguousPoolManager<T,SIZE>::ContiguousPoolManager() :
    _buffer(nullptr)
{
    static_assert(SIZE > 0, "Allocator buffer size must be > 0");
    //build the free stack
    for (size_t i = 0; i < SIZE; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename T, unsigned int SIZE>
ContiguousPoolManager<T,SIZE>::ContiguousPoolManager(aligned_type* buffer) :
    _buffer(buffer)
{
    if (!_buffer) {
        throw std::runtime_error("Null buffer");
    }
    //build the free stack
    for (size_t i = 0; i < SIZE; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename T, unsigned int SIZE>
ContiguousPoolManager<T,SIZE>::ContiguousPoolManager(ContiguousPoolManager<T,SIZE>&& other) :
    _buffer(other.buffer),
    _freeBlockIndex(other._freeBlockIndex),
    _numHeapAllocatedBlocks(other._numHeapAllocatedBlocks),
    _spinlock(std::move(other._spinlock))
{
    // Reset other
    std::copy(std::begin(_freeBlocks), std::end(_freeBlocks), std::begin(other._freeBlocks));
    other._buffer = nullptr;
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename T, unsigned int SIZE>
ContiguousPoolManager<T,SIZE>& ContiguousPoolManager<T,SIZE>::operator=(ContiguousPoolManager<T,SIZE>&& other)
{
    _buffer(other.buffer),
    std::copy(std::begin(_freeBlocks), std::end(_freeBlocks), std::begin(other._freeBlocks));
    _freeBlockIndex = other._freeBlockIndex;
    _numHeapAllocatedBlocks = other._numHeapAllocatedBlocks;
    _spinlock = std::move(other._spinlock);
    
    // Reset other
    other._buffer = nullptr;
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename T, unsigned int SIZE>
void ContiguousPoolManager<T,SIZE>::setBuffer(aligned_type *buffer)
{
    if (!buffer) {
        throw std::runtime_error("Null buffer");
    }
    _buffer = buffer;
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::pointer ContiguousPoolManager<T,SIZE>::address(reference x) const
{
    return &x;
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::const_pointer ContiguousPoolManager<T,SIZE>::address(const_reference x) const
{
    return &x;
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::size_type ContiguousPoolManager<T,SIZE>::max_size() const
{
    return 1; //only 1 supported for now
}

template <typename T, unsigned int SIZE>
template <typename... Args >
void ContiguousPoolManager<T,SIZE>::construct(T* p, Args&&... args)
{
    new((void *)p) T(std::forward<Args>(args)...); // construct in-place
}

template <typename T, unsigned int SIZE>
void ContiguousPoolManager<T,SIZE>::destroy(pointer p)
{
    if (p != nullptr)
    {
        p->~T();
    }
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::pointer ContiguousPoolManager<T,SIZE>::allocate(size_type,
                                                                    std::allocator<void>::const_pointer)
{
    assert(_buffer);
    {
        SpinLock::Guard lock(_spinlock);
        if (!isEmpty())
        {
            return reinterpret_cast<pointer>(_buffer + _freeBlocks[_freeBlockIndex--]);
        }
        // Use heap allocation
        ++_numHeapAllocatedBlocks;
    }
    return (pointer)new char[sizeof(value_type)];
}

template <typename T, unsigned int SIZE>
void ContiguousPoolManager<T,SIZE>::deallocate(pointer p, size_type)
{
    assert(_buffer);
    if (p == nullptr) {
        return;
    }
    if (isManaged(p)) {
        //find index of the block
        SpinLock::Guard lock(_spinlock);
        _freeBlocks[++_freeBlockIndex] = blockIndex(p);
    }
    else {
        delete[] (char*)p;
        SpinLock::Guard lock(_spinlock);
        --_numHeapAllocatedBlocks;
        assert(_numHeapAllocatedBlocks >= 0);
    }
}

template <typename T, unsigned int SIZE>
template <typename... Args >
typename ContiguousPoolManager<T,SIZE>::pointer ContiguousPoolManager<T,SIZE>::create(Args&&... args)
{
    T* p = allocate();
    construct(p, std::forward<Args>(args)...);
    return p;
}

template <typename T, unsigned int SIZE>
void ContiguousPoolManager<T,SIZE>::dispose(pointer p)
{
    destroy(p);
    deallocate(p);
}

template <typename T, unsigned int SIZE>
size_t ContiguousPoolManager<T,SIZE>::allocatedBlocks() const
{
    return SIZE - _freeBlockIndex - 1;
}

template <typename T, unsigned int SIZE>
size_t ContiguousPoolManager<T,SIZE>::allocatedHeapBlocks() const
{
    return _numHeapAllocatedBlocks;
}

template <typename T, unsigned int SIZE>
bool ContiguousPoolManager<T,SIZE>::isFull() const
{
    return _freeBlockIndex == SIZE-1;
}

template <typename T, unsigned int SIZE>
bool ContiguousPoolManager<T,SIZE>::isEmpty() const
{
    return _freeBlockIndex == -1;
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::pointer ContiguousPoolManager<T,SIZE>::bufferStart()
{
    return reinterpret_cast<pointer>(_buffer);
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::pointer ContiguousPoolManager<T,SIZE>::bufferEnd()
{
    return reinterpret_cast<pointer>(_buffer + SIZE);
}

template <typename T, unsigned int SIZE>
bool ContiguousPoolManager<T,SIZE>::isManaged(pointer p)
{
    return (bufferStart() <= p) && (p < bufferEnd());
}

template <typename T, unsigned int SIZE>
typename ContiguousPoolManager<T,SIZE>::index_type ContiguousPoolManager<T,SIZE>::blockIndex(pointer p)
{
    return reinterpret_cast<aligned_type*>(p) - _buffer;
}

}}

