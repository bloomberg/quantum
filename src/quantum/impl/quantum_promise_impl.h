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

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                class IThreadPromise
//==============================================================================================
template <template<class> class PROMISE, class T>
template <class V>
int IThreadPromise<PROMISE, T>::set(V&& value)
{
    return static_cast<impl*>(this)->template set<V>(std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class BUF, class V>
void IThreadPromise<PROMISE, T>::push(V &&value)
{
    static_cast<impl*>(this)->template push<BUF>(std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class BUF, class>
int IThreadPromise<PROMISE, T>::closeBuffer()
{
    return static_cast<impl*>(this)->template closeBuffer<BUF>();
}

//==============================================================================================
//                                class ICoroPromise
//==============================================================================================
template <template<class> class PROMISE, class T>
template <class V>
int ICoroPromise<PROMISE, T>::set(ICoroSync::ptr sync, V&& value)
{
    return static_cast<impl*>(this)->template set<V>(sync, std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class BUF, class V>
void ICoroPromise<PROMISE, T>::push(ICoroSync::ptr sync, V &&value)
{
    static_cast<impl*>(this)->template push<BUF>(sync, std::forward<V>(value));
}

template <template<class> class PROMISE, class T>
template <class BUF, class>
int ICoroPromise<PROMISE, T>::closeBuffer()
{
    return static_cast<impl*>(this)->template closeBuffer<BUF>();
}

//==============================================================================================
//                                class Promise
//==============================================================================================

template <class T>
Promise<T>::Promise() :
    IThreadPromise<Promise, T>(this),
    ICoroPromise<Promise, T>(this),
    _sharedState(new SharedState<T>()),
    _terminated(ATOMIC_FLAG_INIT)
{}

template <class T>
Promise<T>::~Promise()
{
    terminate();
}

template <class T>
void Promise<T>::terminate()
{
    if (!_terminated.test_and_set())
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
IThreadFutureBase::ptr Promise<T>::getIThreadFutureBase() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return std::shared_ptr<Future<T>>(new Future<T>(_sharedState));
}

template <class T>
ICoroFutureBase::ptr Promise<T>::getICoroFutureBase() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return std::shared_ptr<Future<T>>(new Future<T>(_sharedState));
}

template <class T>
template <class V>
int Promise<T>::set(V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->set(std::forward<V>(value));
}

template <class T>
typename IThreadFuture<T>::ptr Promise<T>::getIThreadFuture() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return std::shared_ptr<Future<T>>(new Future<T>(_sharedState));
}

template <class T>
template <class V>
int Promise<T>::set(ICoroSync::ptr sync, V&& value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->set(sync, std::forward<V>(value));
}

template <class T>
typename ICoroFuture<T>::ptr Promise<T>::getICoroFuture() const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return std::shared_ptr<Future<T>>(new Future<T>(_sharedState));
}

template <class T>
template <class BUF, class V>
void Promise<T>::push(V &&value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    _sharedState->template push<BUF>(std::forward<V>(value));
}

template <class T>
template <class BUF, class V>
void Promise<T>::push(ICoroSync::ptr sync, V &&value)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    _sharedState->template push<BUF>(sync, std::forward<V>(value));
}

template <class T>
template <class BUF, class>
int Promise<T>::closeBuffer()
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->template closeBuffer<BUF>();
}


}}
