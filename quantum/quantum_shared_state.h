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
#ifndef BLOOMBERG_QUANTUM_SHARED_STATE_MUTEX_H
#define BLOOMBERG_QUANTUM_SHARED_STATE_MUTEX_H

#include <memory>
#include <exception>
#include <quantum/quantum_traits.h>
#include <quantum/quantum_future_state.h>
#include <quantum/quantum_yielding_thread.h>
#include <quantum/interface/quantum_icontext.h>
#include <quantum/quantum_condition_variable.h>
#include <quantum/quantum_buffer.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                 class SharedState
//==============================================================================================
/// @class SharedState.
/// @brief Shared state used between a Promise and a Future to exchange values.
/// @note For internal use only.
template <class T>
class SharedState
{
    friend class Promise<T>;
    
public:
    template <class V = T>
    int set(V&& value);
    
    template <class V = T>
    int set(ICoroSync::Ptr sync, V&& value);
    
    //Moves value out of the shared state
    T get();
    
    T get(ICoroSync::Ptr sync);
    
    const T& getRef() const;
    
    const T& getRef(ICoroSync::Ptr sync) const;
    
    void breakPromise();
    
    void wait() const;
    
    void wait(ICoroSync::Ptr sync) const;
    
    template<class REP, class PERIOD>
    std::future_status waitFor(const std::chrono::duration<REP, PERIOD> &time) const;
    
    template<class REP, class PERIOD>
    std::future_status waitFor(ICoroSync::Ptr sync,
                               const std::chrono::duration<REP, PERIOD> &time) const;
    
    int setException(std::exception_ptr ex);
    
    int setException(ICoroSync::Ptr sync,
                     std::exception_ptr ex);
private:
    SharedState();
    
    void conditionWait() const;
    
    void conditionWait(ICoroSync::Ptr sync) const;
    
    void checkPromiseState() const;
    
    bool stateHasChanged() const;
    
    // ============================= MEMBERS ==============================
    mutable ConditionVariable       _cond;
    mutable Mutex                   _mutex;
    FutureState                     _state;
    std::exception_ptr              _exception;
    T                               _value;
};

//==============================================================================================
//                       class SharedState<Buffer> (partial specialization)
//==============================================================================================
template <class T>
class SharedState<Buffer<T>>
{
    friend class Promise<Buffer<T>>;
    
public:
    template <class V = T>
    void push(V&& value);
    
    template <class V = T>
    void push(ICoroSync::Ptr sync, V&& value);
    
    T pull(bool& isBufferClosed);
    
    T pull(ICoroSync::Ptr sync, bool& isBufferClosed);
    
    void breakPromise();
    
    void wait() const;
    
    void wait(ICoroSync::Ptr sync) const;
    
    template<class REP, class PERIOD>
    std::future_status waitFor(const std::chrono::duration<REP, PERIOD> &time) const;
    
    template<class REP, class PERIOD>
    std::future_status waitFor(ICoroSync::Ptr sync,
                               const std::chrono::duration<REP, PERIOD> &time) const;
    
    int setException(std::exception_ptr ex);
    
    int setException(ICoroSync::Ptr sync,
                     std::exception_ptr ex);
    
    int closeBuffer();
private:
    SharedState();
    
    void checkPromiseState() const;
    
    bool stateHasChanged(BufferStatus status) const;
    
    // ============================= MEMBERS ==============================
    mutable ConditionVariable       _cond;
    mutable Mutex                   _mutex;
    FutureState                     _state;
    std::exception_ptr              _exception;
    Buffer<T>                       _reader;
    Buffer<T>                       _writer;
};

}}

#include <quantum/impl/quantum_shared_state_impl.h>

#endif //BLOOMBERG_QUANTUM_SHARED_STATE_MUTEX_H
