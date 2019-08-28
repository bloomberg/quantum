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
#include <algorithm>

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
namespace Bloomberg {
namespace quantum {

template <typename T>
ContiguousPoolManager<T>::ContiguousPoolManager() :
    _control(std::make_shared<Control>())
{
}

template <typename T>
ContiguousPoolManager<T>::ContiguousPoolManager(aligned_type* buffer, index_type size) :
    _control(std::make_shared<Control>())
{
    setBuffer(buffer, size);
}

template <typename T>
template <typename U>
ContiguousPoolManager<T>::ContiguousPoolManager(const ContiguousPoolManager<U>& other) :
    _control(std::reinterpret_pointer_cast<Control>(other._control))
{
    if (!_control || !_control->_buffer)
    {
        throw std::runtime_error("Invalid allocator.");
    }
    //normalize size of buffer
    index_type newSize = std::min(_control->_size, (index_type)resize<U,T>(_control->_size));
    _control->_size = newSize; //resize buffer
    _control->_freeBlockIndex = newSize-1;
}

template <typename T>
template <typename U>
ContiguousPoolManager<T>::ContiguousPoolManager(ContiguousPoolManager<U>&& other) :
    _control(std::move(other._control))
{
    if (!_control || !_control->_buffer)
    {
        throw std::runtime_error("Invalid allocator.");
    }
    //normalize size of buffer
    index_type newSize = std::min(_control->_size, (index_type)resize<U,T>(_control->_size));
    _control->_size = newSize; //resize buffer
    _control->_freeBlockIndex = newSize-1;
}

template <typename T>
void ContiguousPoolManager<T>::setBuffer(aligned_type *buffer, index_type size)
{
    if (!_control) {
        throw std::bad_alloc();
    }
    if (!buffer) {
        throw std::runtime_error("Null buffer");
    }
    if (size == 0) {
        throw std::runtime_error("Invalid allocator pool size");
    }
    _control->_size = size;
    _control->_buffer = buffer;
    if (_control->_freeBlocks) {
        delete[] _control->_freeBlocks;
    }
    _control->_freeBlocks = new index_type[size];
    if (!_control->_freeBlocks) {
        throw std::bad_alloc();
    }
    //build the free stack
    for (index_type i = 0; i < size; ++i) {
        _control->_freeBlocks[i] = i;
    }
    _control->_freeBlockIndex = size-1;
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
    assert(bufferStart());
    {
        SpinLock::Guard lock(_control->_spinlock);
        if (findContiguous(static_cast<index_type>(n)))
        {
            _control->_freeBlockIndex -= (n - 1);
            return reinterpret_cast<pointer>(&_control->_buffer[_control->_freeBlocks[_control->_freeBlockIndex--]]);
        }
        // Use heap allocation
        ++_control->_numHeapAllocatedBlocks;
    }
    return (pointer)new char[sizeof(value_type)];
}

template <typename T>
void ContiguousPoolManager<T>::deallocate(pointer p, size_type n)
{
    if (p == nullptr) {
        return;
    }
    assert(bufferStart());
    if (isManaged(p)) {
        //find index of the block and return the individual blocks to the free pool
        SpinLock::Guard lock(_control->_spinlock);
        for (size_type i = 0; i < n; ++i) {
            _control->_freeBlocks[++_control->_freeBlockIndex] = blockIndex(p+i);
        }
    }
    else {
        delete[] (char*)p;
        SpinLock::Guard lock(_control->_spinlock);
        --_control->_numHeapAllocatedBlocks;
        assert(_control->_numHeapAllocatedBlocks >= 0);
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
    return _control->_size ? _control->_size - _control->_freeBlockIndex - 1 : 0;
}

template <typename T>
size_t ContiguousPoolManager<T>::allocatedHeapBlocks() const
{
    return _control->_numHeapAllocatedBlocks;
}

template <typename T>
bool ContiguousPoolManager<T>::isFull() const
{
    return _control->_freeBlockIndex == _control->_size-1;
}

template <typename T>
bool ContiguousPoolManager<T>::isEmpty() const
{
    return _control->_freeBlockIndex == -1;
}

template <typename T>
typename ContiguousPoolManager<T>::index_type ContiguousPoolManager<T>::size() const
{
    return _control->_size;
}

template <typename T>
ContiguousPoolManager<T>::operator bool() const
{
    return _control != nullptr;
}

template <typename T>
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::bufferStart()
{
    return reinterpret_cast<pointer>(_control->_buffer);
}

template <typename T>
typename ContiguousPoolManager<T>::pointer ContiguousPoolManager<T>::bufferEnd()
{
    return reinterpret_cast<pointer>(_control->_buffer + _control->_size);
}

template <typename T>
bool ContiguousPoolManager<T>::isManaged(pointer p)
{
    return (bufferStart() <= p) && (p < bufferEnd());
}

template <typename T>
typename ContiguousPoolManager<T>::index_type ContiguousPoolManager<T>::blockIndex(pointer p)
{
    return static_cast<index_type>(reinterpret_cast<aligned_type*>(p) - _control->_buffer);
}

template <typename T>
bool ContiguousPoolManager<T>::findContiguous(index_type n)
{
    if ((_control->_freeBlockIndex + 1) < n) {
        return false;
    }
    bool found = true;
    aligned_type* last = &_control->_buffer[_control->_freeBlocks[_control->_freeBlockIndex]];
    for (ssize_t i = _control->_freeBlockIndex-1; i > _control->_freeBlockIndex-n; --i) {
        aligned_type* first = &_control->_buffer[_control->_freeBlocks[i]];
        if ((last-first) != (_control->_freeBlockIndex-i)) {
            return false;
        }
    }
    return found;
}

}}

