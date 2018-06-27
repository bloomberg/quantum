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
//                                class IThreadFuture
//==============================================================================================
template <class T>
template <class BUF, class V>
V IThreadFuture<T>::pull(bool& isBufferClosed)
{
    return static_cast<impl*>(this)->template pull<BUF>(isBufferClosed);
}

//==============================================================================================
//                                class ICoroFuture
//==============================================================================================
template <class T>
template <class BUF, class V>
V ICoroFuture<T>::pull(ICoroSync::ptr sync, bool& isBufferClosed)
{
    return static_cast<impl*>(this)->template pull<BUF>(sync, isBufferClosed);
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
T Future<T>::get()
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->get();
}

template <class T>
const T& Future<T>::getRef() const
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
std::future_status Future<T>::waitFor(size_t timeMs) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->waitFor(std::chrono::milliseconds(timeMs));
}

template <class T>
T Future<T>::get(ICoroSync::ptr sync)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->get(sync);
}

template <class T>
const T& Future<T>::getRef(ICoroSync::ptr sync) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->getRef(sync);
}

template <class T>
void Future<T>::wait(ICoroSync::ptr sync) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->wait(sync);
}

template <class T>
std::future_status Future<T>::waitFor(ICoroSync::ptr sync,
                                      size_t timeMs) const
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->waitFor(sync, std::chrono::milliseconds(timeMs));
}

template <class T>
template <class BUF, class V>
V Future<T>::pull(bool& isBufferClosed)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->template pull<BUF>(isBufferClosed);
}

template <class T>
template <class BUF, class V>
V Future<T>::pull(ICoroSync::ptr sync, bool& isBufferClosed)
{
    if (!_sharedState) ThrowFutureException(FutureState::NoState);
    return _sharedState->template pull<BUF>(sync, isBufferClosed);
}


}}


