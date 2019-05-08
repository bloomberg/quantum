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

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
#include <quantum/quantum_allocator.h>

namespace Bloomberg {
namespace quantum {

#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    #ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        using FutureAllocator = HeapAllocator<Future<int>>;
    #else
        using FutureAllocator = StackAllocator<Future<int>, __QUANTUM_FUTURE_ALLOC_SIZE>;
    #endif
#else
    using FutureAllocator = StlAllocator<Future<int>>;
#endif

//==============================================================================================
//                                class IThreadFuture
//==============================================================================================
template <class T>
template <class V>
NonBufferRetType<V> IThreadFuture<T>::get()
{
    return static_cast<Impl*>(this)->get();
}

template <class T>
template <class V>
const NonBufferRetType<V>& IThreadFuture<T>::getRef() const
{
    return static_cast<const Impl*>(this)->getRef();
}

template <class T>
template <class V>
BufferRetType<V> IThreadFuture<T>::pull(bool& isBufferClosed)
{
    return static_cast<Impl*>(this)->pull(isBufferClosed);
}

//==============================================================================================
//                                class ICoroFuture
//==============================================================================================
template <class T>
template <class V>
NonBufferRetType<V> ICoroFuture<T>::get(ICoroSync::Ptr sync)
{
    return static_cast<Impl*>(this)->get(sync);
}

template <class T>
template <class V>
const NonBufferRetType<V>& ICoroFuture<T>::getRef(ICoroSync::Ptr sync) const
{
    return static_cast<const Impl*>(this)->getRef(sync);
}

template <class T>
template <class V>
BufferRetType<V> ICoroFuture<T>::pull(ICoroSync::Ptr sync, bool& isBufferClosed)
{
    return static_cast<Impl*>(this)->pull(sync, isBufferClosed);
}

//==============================================================================================
//                                class Future
//==============================================================================================
template <class T>
Future<T>::Future(std::shared_ptr<SharedState<T>> sharedState) :
    _sharedState(sharedState)
{}

template <class T>
bool Future<T>::valid() const
{
    return _sharedState != nullptr;
}

template <class T>
template <class V>
NonBufferRetType<V> Future<T>::get()
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->get();
}

template <class T>
template <class V>
const NonBufferRetType<V>& Future<T>::getRef() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->getRef();
}

template <class T>
void Future<T>::wait() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->wait();
}

template <class T>
std::future_status Future<T>::waitFor(std::chrono::milliseconds timeMs) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->waitFor(timeMs);
}

template <class T>
template <class V>
NonBufferRetType<V> Future<T>::get(ICoroSync::Ptr sync)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->get(sync);
}

template <class T>
template <class V>
const NonBufferRetType<V>& Future<T>::getRef(ICoroSync::Ptr sync) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->getRef(sync);
}

template <class T>
void Future<T>::wait(ICoroSync::Ptr sync) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->wait(sync);
}

template <class T>
std::future_status Future<T>::waitFor(ICoroSync::Ptr sync,
                                      std::chrono::milliseconds timeMs) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->waitFor(sync, timeMs);
}

template <class T>
template <class V>
BufferRetType<V> Future<T>::pull(bool& isBufferClosed)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->pull(isBufferClosed);
}

template <class T>
template <class V>
BufferRetType<V> Future<T>::pull(ICoroSync::Ptr sync, bool& isBufferClosed)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->pull(sync, isBufferClosed);
}

template <class T>
void* Future<T>::operator new(size_t)
{
    return Allocator<FutureAllocator>::instance(AllocatorTraits::futureAllocSize()).allocate();
}

template <class T>
void Future<T>::operator delete(void* p)
{
    Allocator<FutureAllocator>::instance(AllocatorTraits::futureAllocSize()).deallocate(static_cast<Future<int>*>(p));
}

template <class T>
void Future<T>::deleter(Future<T>* p)
{
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    Allocator<FutureAllocator>::instance(AllocatorTraits::futureAllocSize()).dispose(reinterpret_cast<Future<int>*>(p));
#else
    delete p;
#endif
}

}}


