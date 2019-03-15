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
//                                     class SharedState
//==============================================================================================
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
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    conditionWait();
    _state = FutureState::FutureAlreadyRetrieved;
    return std::move(_value);
}

template <class T>
const T& SharedState<T>::getRef() const
{
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    conditionWait();
    return _value;
}

template <class T>
T SharedState<T>::get(ICoroSync::Ptr sync)
{
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    conditionWait(sync);
    _state = FutureState::FutureAlreadyRetrieved;
    return std::move(_value);
}

template <class T>
const T& SharedState<T>::getRef(ICoroSync::Ptr sync) const
{
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

//==============================================================================================
//                       class SharedState<Buffer> (partial specialization)
//==============================================================================================
template <class T>
SharedState<Buffer<T>>::SharedState() :
    _state(FutureState::PromiseNotSatisfied)
{
}

template <class T>
void SharedState<Buffer<T>>::breakPromise()
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if ((_state == FutureState::PromiseNotSatisfied) ||
            (_state == FutureState::BufferingData))
        {
            _state = FutureState::BrokenPromise;
        }
    }
    _cond.notifyAll();
}

template <class T>
void SharedState<Buffer<T>>::wait() const
{
    if (!_reader.empty())
    {
        return; //there is still data available
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    _cond.wait(_mutex, [this]()->bool
    {
        BufferStatus status = _writer.empty() ?
            (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
        return stateHasChanged(status);
    });
}

template <class T>
void SharedState<Buffer<T>>::wait(ICoroSync::Ptr sync) const
{
    if (!_reader.empty())
    {
        return; //there is still data available
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    _cond.wait(sync, _mutex, [this]()->bool
    {
        BufferStatus status = _writer.empty() ?
            (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
        return stateHasChanged(status);
    });
}

template <class T>
template<class REP, class PERIOD>
std::future_status SharedState<Buffer<T>>::waitFor(const std::chrono::duration<REP, PERIOD> &time) const
{
    if (!_reader.empty())
    {
        return std::future_status::ready; //there is still data available
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(_mutex);
    _cond.waitFor(_mutex, time, [this]()->bool
    {
        BufferStatus status = _writer.empty() ?
            (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
        return stateHasChanged(status);
    });
    return (_writer.empty() && !_writer.isClosed()) ? std::future_status::timeout : std::future_status::ready;
}

template <class T>
template<class REP, class PERIOD>
std::future_status SharedState<Buffer<T>>::waitFor(ICoroSync::Ptr sync,
                                                   const std::chrono::duration<REP, PERIOD> &time) const
{
    if (!_reader.empty())
    {
        return std::future_status::ready; //there is still data available
    }
    //========= LOCKED SCOPE =========
    Mutex::Guard lock(sync, _mutex);
    _cond.waitFor(sync, _mutex, time, [this]()->bool
    {
        BufferStatus status = _writer.empty() ?
            (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
        return stateHasChanged(status);
    });
    return (_writer.empty() && !_writer.isClosed()) ? std::future_status::timeout : std::future_status::ready;
}

template <class T>
int SharedState<Buffer<T>>::setException(std::exception_ptr ex)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        _exception = ex;
    }
    _cond.notifyAll();
    return -1;
}

template <class T>
int SharedState<Buffer<T>>::setException(ICoroSync::Ptr sync,
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
template <class V>
void SharedState<Buffer<T>>::push(V&& value)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if ((_state != FutureState::PromiseNotSatisfied) && (_state != FutureState::BufferingData))
        {
            ThrowFutureException(_state);
        }
        BufferStatus status = _writer.push(std::forward<V>(value));
        if (status == BufferStatus::Closed)
        {
            ThrowFutureException(FutureState::BufferClosed);
        }
        _state = FutureState::BufferingData;
    }
    _cond.notifyAll();
}

template <class T>
template <class V>
void SharedState<Buffer<T>>::push(ICoroSync::Ptr sync, V&& value)
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _mutex);
        if ((_state != FutureState::PromiseNotSatisfied) && (_state != FutureState::BufferingData))
        {
            ThrowFutureException(_state);
        }
        BufferStatus status = _writer.push(std::forward<V>(value));
        if (status == BufferStatus::Closed)
        {
            ThrowFutureException(FutureState::BufferClosed);
        }
        _state = FutureState::BufferingData;
    }
    _cond.notifyAll();
}

template <class T>
T SharedState<Buffer<T>>::pull(bool& isBufferClosed)
{
    T out{};
    if (!_reader.empty())
    {
        T out{};
        _reader.pull(out);
        return out;
    }
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        _cond.wait(_mutex, [this]()->bool
        {
            BufferStatus status = _writer.empty() ?
                (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
            bool changed = stateHasChanged(status);
            if (changed) {
                // Move the writer to the reader for consumption
                _reader = std::move(_writer);
            }
            return changed;
        });
    }
    isBufferClosed = _reader.empty() && _reader.isClosed();
    if (isBufferClosed) {
        //Mark the future as fully retrieved
        _state = FutureState::FutureAlreadyRetrieved;
        return out;
    }
    _reader.pull(out);
    checkPromiseState();
    return out;
}

template <class T>
T SharedState<Buffer<T>>::pull(ICoroSync::Ptr sync, bool& isBufferClosed)
{
    T out{};
    if (!_reader.empty())
    {
        T out{};
        _reader.pull(out);
        return out;
    }
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        _cond.wait(sync, _mutex, [this]()->bool
        {
            BufferStatus status = _writer.empty() ?
                (_writer.isClosed() ? BufferStatus::Closed : BufferStatus::DataPending) : BufferStatus::DataPosted;
            bool changed = stateHasChanged(status);
            if (changed) {
                // Move the writer to the reader for consumption
                _reader = std::move(_writer);
            }
            return changed;
        });
    }
    isBufferClosed = _reader.empty() && _reader.isClosed();
    if (isBufferClosed) {
        //Mark the future as fully retrieved
        _state = FutureState::FutureAlreadyRetrieved;
        return out;
    }
    _reader.pull(out);
    checkPromiseState();
    return out;
}

template <class T>
int SharedState<Buffer<T>>::closeBuffer()
{
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(_mutex);
        if ((_state == FutureState::PromiseNotSatisfied) || (_state == FutureState::BufferingData))
        {
            _state = FutureState::BufferClosed;
        }
        _writer.close();
    }
    _cond.notifyAll();
    return 0;
}

template <class T>
void SharedState<Buffer<T>>::checkPromiseState() const
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
bool SharedState<Buffer<T>>::stateHasChanged(BufferStatus status) const
{
    return ((status == BufferStatus::DataPosted) || (status == BufferStatus::DataReceived) || (status == BufferStatus::Closed)) ||
           ((_state == FutureState::BrokenPromise) || (_state == FutureState::FutureAlreadyRetrieved)) ||
           (_exception != nullptr);
}

}}
