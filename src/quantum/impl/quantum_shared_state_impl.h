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

template <class T>
SharedState<T>::SharedState() :
    _state(FutureState::PromiseNotSatisfied),
    _value(T())
{
}

template <class T>
template <class V>
int SharedState<T>::set(V&& value)
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    {
        //========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if (_state != FutureState::PromiseNotSatisfied)
        {
            ThrowFutureException(_state);
        }
        _value = std::forward<V>(value);
        _state = FutureState::PromiseAlreadySatisfied;
    }
    _cond.notifyAll();
    return 0;
}

template <class T>
template <class V>
int SharedState<T>::set(ICoroSync::Ptr sync, V&& value)
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    {
        //========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _mutex);
        if (_state != FutureState::PromiseNotSatisfied)
        {
            ThrowFutureException(_state);
        }
        _value = std::forward<V>(value);
        _state = FutureState::PromiseAlreadySatisfied;
    }
    _cond.notifyAll();
    return 0;
}

template <class T>
T SharedState<T>::get()
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    conditionWait();
    _state = FutureState::FutureAlreadyRetrieved;
    return std::move(_value);
}

template <class T>
const T& SharedState<T>::getRef() const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    conditionWait();
    return _value;
}

template <class T>
T SharedState<T>::get(ICoroSync::Ptr sync)
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    conditionWait(sync);
    _state = FutureState::FutureAlreadyRetrieved;
    return std::move(_value);
}

template <class T>
const T& SharedState<T>::getRef(ICoroSync::Ptr sync) const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    conditionWait(sync);
    return _value;
}

template <class T>
void SharedState<T>::breakPromise()
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if (_state == FutureState::PromiseNotSatisfied)
        {
            _state = FutureState::BrokenPromise;
        }
    }
    _cond.notifyAll();
}

template <class T>
void SharedState<T>::wait() const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    _cond.wait(_mutex, [this]()->bool
    {
        return stateHasChanged();
    });
}

template <class T>
void SharedState<T>::wait(ICoroSync::Ptr sync) const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    _cond.wait(sync, _mutex, [this]()->bool
    {
        return stateHasChanged();
    });
}

template <class T>
template<class REP, class PERIOD>
std::future_status SharedState<T>::waitFor(const std::chrono::duration<REP, PERIOD> &time) const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    _cond.waitFor(_mutex, time, [this]()->bool
    {
        return stateHasChanged();
    });
    return _state == FutureState::PromiseNotSatisfied ? std::future_status::timeout : std::future_status::ready;
}

template <class T>
template<class REP, class PERIOD>
std::future_status SharedState<T>::waitFor(ICoroSync::Ptr sync,
                                           const std::chrono::duration<REP, PERIOD> &time) const
{
    if (Traits::IsBuffer<T>::value)
    {
        ThrowFutureException(FutureState::BufferingData);
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    _cond.waitFor(sync, _mutex, time, [this]()->bool
    {
        return stateHasChanged();
    });
    return _state == FutureState::PromiseNotSatisfied ? std::future_status::timeout : std::future_status::ready;
}

template <class T>
int SharedState<T>::setException(std::exception_ptr ex)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        _exception = ex;
    }
    _cond.notifyAll();
    return -1;
}

template <class T>
int SharedState<T>::setException(ICoroSync::Ptr sync,
                                 std::exception_ptr ex)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _mutex);
        _exception = ex;
    }
    _cond.notifyAll();
    return -1;
}

template <class T>
template <class BUF, class V>
void SharedState<T>::push(V&& value)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if ((_state != FutureState::PromiseNotSatisfied) && (_state != FutureState::BufferingData))
        {
            ThrowFutureException(_state);
        }
        BufferStatus status = _value.push(std::forward<V>(value));
        if (status == BufferStatus::Closed)
        {
            ThrowFutureException(FutureState::BufferClosed);
        }
        _state = FutureState::BufferingData;
    }
    _cond.notifyAll();
}

template <class T>
template <class BUF, class V>
void SharedState<T>::push(ICoroSync::Ptr sync, V&& value)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _mutex);
        if ((_state != FutureState::PromiseNotSatisfied) && (_state != FutureState::BufferingData))
        {
            ThrowFutureException(_state);
        }
        BufferStatus status = _value.push(std::forward<V>(value));
        if (status == BufferStatus::Closed)
        {
            ThrowFutureException(FutureState::BufferClosed);
        }
        _state = FutureState::BufferingData;
    }
    _cond.notifyAll();
}

template <class T>
template <class BUF, class V>
V SharedState<T>::pull(bool& isBufferClosed)
{
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    BufferStatus status;
    V out{};
    isBufferClosed = true;
    _cond.wait(_mutex, [&status, &out, this]()->bool
    {
        status = _value.pull(out);
        return bufferStateHasChanged(status);
    });
    checkPromiseState();
    isBufferClosed = (status == BufferStatus::Closed); //set output
    if (isBufferClosed)
    {
        //Mark the future as fully retrieved
        _state = FutureState::FutureAlreadyRetrieved;
    }
    return out;
}

template <class T>
template <class BUF, class V>
V SharedState<T>::pull(ICoroSync::Ptr sync, bool& isBufferClosed)
{
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    BufferStatus status;
    typename BUF::ValueType out;
    isBufferClosed = true;
    _cond.wait(sync, _mutex, [&status, &out, this]()->bool
    {
        status = _value.pull(out);
        return bufferStateHasChanged(status);
    });
    checkPromiseState();
    isBufferClosed = (status == BufferStatus::Closed);
    if (isBufferClosed)
    {
        //Mark the future as fully retrieved
        _state = FutureState::FutureAlreadyRetrieved;
    }
    return out;
}

template <class T>
template <class BUF, class>
int SharedState<T>::closeBuffer()
{
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    if ((_state == FutureState::PromiseNotSatisfied) || (_state == FutureState::BufferingData))
    {
        _state = FutureState::BufferClosed;
    }
    _value.close();
    _cond.notifyAll();
    return 0;
}

template <class T>
void SharedState<T>::conditionWait() const
{
    _cond.wait(_mutex, [this]()->bool
    {
        return stateHasChanged();
    });
    checkPromiseState();
}

template <class T>
void SharedState<T>::conditionWait(ICoroSync::Ptr sync) const
{
    _cond.wait(sync, _mutex, [this]()->bool
    {
        return stateHasChanged();
    });
    checkPromiseState();
}

template <class T>
void SharedState<T>::checkPromiseState() const
{
    if (_exception)
    {
        std::rethrow_exception(_exception);
    }
    if ((_state == FutureState::BrokenPromise) || (_state == FutureState::FutureAlreadyRetrieved))
    {
        ThrowFutureException(_state);
    }
}

template <class T>
bool SharedState<T>::stateHasChanged() const
{
    return (_state != FutureState::PromiseNotSatisfied) || (_exception != nullptr);
}

template <class T>
template <class BUF, typename>
bool SharedState<T>::bufferStateHasChanged(BufferStatus status) const
{
    return ((status == BufferStatus::DataReceived) || (status == BufferStatus::Closed)) ||
           ((_state == FutureState::BrokenPromise) || (_state == FutureState::FutureAlreadyRetrieved)) ||
           (_exception != nullptr);
}

}}
