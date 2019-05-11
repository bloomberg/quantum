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

//==============================================================================================
//                                class IThreadPromise
//==============================================================================================
template <template<class> class PROMISE, class T>
template <class V, class>
int IThreadPromise<PROMISE, T>::set(V&& value)
{
    return static_cast<Impl*>(this)->set(std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class V, class>
void IThreadPromise<PROMISE, T>::push(V&& value)
{
    static_cast<Impl*>(this)->push(std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class V, class>
int IThreadPromise<PROMISE, T>::closeBuffer()
{
    return static_cast<Impl*>(this)->closeBuffer();
}

//==============================================================================================
//                                class ICoroPromise
//==============================================================================================
template <template<class> class PROMISE, class T>
template <class V, class>
int ICoroPromise<PROMISE, T>::set(ICoroSync::Ptr sync, V&& value)
{
    return static_cast<Impl*>(this)->set(sync, std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class V, class>
void ICoroPromise<PROMISE, T>::push(ICoroSync::Ptr sync, V&& value)
{
    static_cast<Impl*>(this)->push(sync, std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class V, class>
int ICoroPromise<PROMISE, T>::closeBuffer()
{
    return static_cast<Impl*>(this)->closeBuffer();
}

//==============================================================================================
//                                class Promise
//==============================================================================================
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    #ifdef __QUANTUM_ALLOCATE_POOL_FROM_HEAP
        using PromiseAllocator = HeapAllocator<Promise<int>>;
    #else
        using PromiseAllocator = StackAllocator<Promise<int>, __QUANTUM_PROMISE_ALLOC_SIZE>;
    #endif
#else
    using PromiseAllocator = StlAllocator<Promise<int>>;
#endif

template <class T>
Promise<T>::Promise() :
    IThreadPromise<Promise, T>(this),
    ICoroPromise<Promise, T>(this),
    _sharedState(new SharedState<T>()),
    _terminated(false)
{}

template <class T>
Promise<T>::~Promise()
{
    terminate();
}

template <class T>
void Promise<T>::terminate()
{
    bool value{false};
    if (_terminated.compare_exchange_strong(value, true))
    {
        if (_sharedState) _sharedState->breakPromise();
    }
}

template <class T>
bool Promise<T>::valid() const
{
    return _sharedState != nullptr;
}

template <class T>
int Promise<T>::setException(std::exception_ptr ex)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->setException(ex);
}

template <class T>
IThreadFutureBase::Ptr Promise<T>::getIThreadFutureBase() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return FuturePtr<T>(new Future<T>(_sharedState), Future<T>::deleter);
}

template <class T>
ICoroFutureBase::Ptr Promise<T>::getICoroFutureBase() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return FuturePtr<T>(new Future<T>(_sharedState), Future<T>::deleter);
}

template <class T>
template <class V, class>
int Promise<T>::set(V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->set(std::forward<V>(value));
}

template <class T>
ThreadFuturePtr<T> Promise<T>::getIThreadFuture() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return FuturePtr<T>(new Future<T>(_sharedState), Future<T>::deleter);
}

template <class T>
template <class V, class>
int Promise<T>::set(ICoroSync::Ptr sync, V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->set(sync, std::forward<V>(value));
}

template <class T>
CoroFuturePtr<T> Promise<T>::getICoroFuture() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return FuturePtr<T>(new Future<T>(_sharedState), Future<T>::deleter);
}

template <class T>
template <class V, class>
void Promise<T>::push(V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    _sharedState->push(std::forward<V>(value));
}

template <class T>
template <class V, class>
void Promise<T>::push(ICoroSync::Ptr sync, V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    _sharedState->push(sync, std::forward<V>(value));
}

template <class T>
template <class V, class>
int Promise<T>::closeBuffer()
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->closeBuffer();
}

template <class T>
void* Promise<T>::operator new(size_t)
{
    return Allocator<PromiseAllocator>::instance(AllocatorTraits::promiseAllocSize()).allocate();
}

template <class T>
void Promise<T>::operator delete(void* p)
{
    Allocator<PromiseAllocator>::instance(AllocatorTraits::promiseAllocSize()).deallocate(static_cast<Promise<int>*>(p));
}

template <class T>
void Promise<T>::deleter(Promise<T>* p)
{
#ifndef __QUANTUM_USE_DEFAULT_ALLOCATOR
    Allocator<PromiseAllocator>::instance(AllocatorTraits::promiseAllocSize()).dispose(reinterpret_cast<Promise<int>*>(p));
#else
    delete p;
#endif
}

}}
