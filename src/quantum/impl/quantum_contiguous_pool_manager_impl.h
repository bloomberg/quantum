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
#include <assert.h>
#include <exception>

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
namespace Bloomberg {
namespace quantum {

template <typename T>
ContiguousPoolManager<T>::ContiguousPoolManager()
{
}

template <typename T>
ContiguousPoolManager<T>::ContiguousPoolManager(aligned_type* buffer, index_type size)
{
    setBuffer(buffer, size);
}

template <typename T>
ContiguousPoolManager<T>::ContiguousPoolManager(ContiguousPoolManager<T>&& other)
{
    *this = std::move(other);
}

template <typename T>
ContiguousPoolManager<T>& ContiguousPoolManager<T>::operator=(ContiguousPoolManager<T>&& other)
{
    _size = other._size;
    _buffer = other.buffer;
    _freeBlocks = other._freeBlocks;
    _freeBlockIndex = other._freeBlockIndex;
    _numHeapAllocatedBlocks = other._numHeapAllocatedBlocks;
    _spinlock = std::move(other._spinlock);
    
    // Reset other
    other._buffer = nullptr;
    other._freeBlocks = nullptr;
    other._freeBlockIndex = -1;
    other._numHeapAllocatedBlocks = 0;
}

template <typename T>
ContiguousPoolManager<T>::~ContiguousPoolManager()
{
    delete[] _freeBlocks;
}

template <typename T>
void ContiguousPoolManager<T>::setBuffer(aligned_type *buffer, index_type size)
{
    if (!buffer) {
        throw std::runtime_error("Null buffer");
    }
    if (size == 0) {
        throw std::runtime_error("Invalid allocator pool size");
    }
    _size = size;
    _buffer = buffer;
    _freeBlocks = new index_type[size];
    if (!_freeBlocks) {
        throw std::bad_alloc();
    }
    _freeBlockIndex = size-1;
    //build the free stack
    for (index_type i = 0; i < size; ++i) {
        _freeBlocks[i] = i;
    }
}

template <typename T>
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::address(reference x) const
{
    return &x;
}

template <typename T>
typename ContiguousPoolManager<T>::const_pointer ContiguousPoolManager<T>::address(const_reference x) const
{
    return &x;
}

template <typename T>
typename ContiguousPoolManager<T>::size_type ContiguousPoolManager<T>::max_size() const
{
    return 1; //only 1 supported for now
}

template <typename T>
template <typename... Args >
void ContiguousPoolManager<T>::construct(T* p, Args&&... args)
{
    new((void *)p) T(std::forward<Args>(args)...); // construct in-place
}

template <typename T>
void ContiguousPoolManager<T>::destroy(pointer p)
{
    if (p != nullptr)
    {
        p->~T();
    }
}

template <typename T>
typename ContiguousPoolManager<T>::pointer
ContiguousPoolManager<T>::allocate(size_type n, const_pointer)
{
    assert(_buffer);
    {
        SpinLock::Guard lock(_spinlock);
        if (findContiguous(static_cast<index_type>(n)))
        {
            _freeBlockIndex -= (n - 1);
            return reinterpret_cast<pointer>(&_buffer[_freeBlocks[_freeBlockIndex--]]);
        }
        // Use heap allocation
        ++_numHeapAllocatedBlocks;
    }
    return (pointer)new char[sizeof(value_type)];
}

template <typename T>
void ContiguousPoolManager<T>::deallocate(pointer p, size_type n)
{
    assert(_buffer);
    if (p == nullptr) {
        return;
    }
    if (isManaged(p)) {
        //find index of the block and return the individual blocks to the free pool
        SpinLock::Guard lock(_spinlock);
        for (ssize_t i = 0; i < n; ++i) {
            _freeBlocks[++_freeBlockIndex] = blockIndex(p+i);
        }
    }
    else {
        delete[] (char*)p;
        SpinLock::Guard lock(_spinlock);
        --_numHeapAllocatedBlocks;
        assert(_numHeapAllocatedBlocks >= 0);
    }
}

template <typename T>
template <typename... Args >
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::create(Args&&... args)
{
    T* p = allocate();
    construct(p, std::forward<Args>(args)...);
    return p;
}

template <typename T>
void ContiguousPoolManager<T>::dispose(pointer p)
{
    destroy(p);
    deallocate(p);
}

template <typename T>
size_t ContiguousPoolManager<T>::allocatedBlocks() const
{
    return _size ? _size - _freeBlockIndex - 1 : 0;
}

template <typename T>
size_t ContiguousPoolManager<T>::allocatedHeapBlocks() const
{
    return _numHeapAllocatedBlocks;
}

template <typename T>
bool ContiguousPoolManager<T>::isFull() const
{
    return _freeBlockIndex == _size-1;
}

template <typename T>
bool ContiguousPoolManager<T>::isEmpty() const
{
    return _freeBlockIndex == -1;
}

template <typename T>
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::bufferStart()
{
    return reinterpret_cast<pointer>(_buffer);
}

template <typename T>
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::bufferEnd()
{
    return reinterpret_cast<pointer>(_buffer + _size);
}

template <typename T>
bool ContiguousPoolManager<T>::isManaged(pointer p)
{
    return (bufferStart() <= p) && (p < bufferEnd());
}

template <typename T>
typename ContiguousPoolManager<T>::index_type ContiguousPoolManager<T>::blockIndex(pointer p)
{
    return static_cast<index_type>(reinterpret_cast<aligned_type*>(p) - _buffer);
}

template <typename T>
bool ContiguousPoolManager<T>::findContiguous(index_type n)
{
    if ((_freeBlockIndex + 1) < n) {
        return false;
    }
    bool found = true;
    aligned_type* last = &_buffer[_freeBlocks[_freeBlockIndex]];
    for (ssize_t i = _freeBlockIndex-1; i > _freeBlockIndex-n; --i) {
        aligned_type* first = &_buffer[_freeBlocks[i]];
        if ((last-first) != (_freeBlockIndex-i)) {
            return false;
        }
    }
    return found;
}

}}

