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

thread_local static std::atomic_int s_threadSignal{-1}; //thread specific (non-coroutine)

inline
ConditionVariable::ConditionVariable() :
    _destroyed(false)
{}

inline
ConditionVariable::~ConditionVariable()
{
    Mutex::Guard lock(_thisLock);
    _destroyed = true;
}

inline
void ConditionVariable::notifyOne()
{
    //LOCKED OR UNLOCKED SCOPE
    Mutex::Guard lock(_thisLock);
    if (_waiters.empty())
    {
        return;
    }
    (*_waiters.front()) = 1;
    _waiters.pop_front();
}

inline
void ConditionVariable::notifyAll()
{
    //LOCKED OR UNLOCKED SCOPE
    Mutex::Guard lock(_thisLock);
    for (auto&& waiter : _waiters)
    {
        (*waiter) = 1;
    }
    _waiters.clear();
}

inline
void ConditionVariable::wait(Mutex& mutex)
{
    waitImpl(YieldingThread(), mutex, s_threadSignal);
}

inline
void ConditionVariable::wait(ICoroSync::ptr sync, Mutex& mutex)
{
    waitImpl(sync->getYieldHandle(), mutex, sync->signal());
}

template <class PREDICATE>
void ConditionVariable::wait(Mutex& mutex,
                             PREDICATE predicate)
{
    waitImpl(YieldingThread(), mutex, predicate, s_threadSignal);
}

template <class PREDICATE>
void ConditionVariable::wait(ICoroSync::ptr sync,
                             Mutex& mutex,
                             PREDICATE predicate)
{
    waitImpl(sync->getYieldHandle(), mutex, predicate, sync->signal());
}

template <class REP, class PERIOD>
bool ConditionVariable::waitFor(Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time)
{
    return waitForImpl(YieldingThread(), mutex, time, s_threadSignal);
}

template <class REP, class PERIOD>
bool ConditionVariable::waitFor(ICoroSync::ptr sync,
                                Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time)
{
    return waitForImpl(sync->getYieldHandle(), mutex, time, sync->signal());
}

template <class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitFor(Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time,
                                PREDICATE predicate)
{
    return waitForImpl(YieldingThread(), mutex, time, predicate, s_threadSignal);
}

template <class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitFor(ICoroSync::ptr sync,
                                Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time,
                                PREDICATE predicate)
{
    return waitForImpl(sync->getYieldHandle(), mutex, time, predicate, sync->signal());
}

template <class YIELDING>
void ConditionVariable::waitImpl(YIELDING&& yield,
                                 Mutex& mutex,
                                 std::atomic_int& signal)
{
    //LOCKED SCOPE
    {
        Mutex::Guard lock(_thisLock);
        if (_destroyed)
        {
            return; //don't release the mutex
        }
        signal = 0; //clear signal flag
        _waiters.push_back(&signal);
    }
    
    Mutex::ReverseGuard unlock(mutex);
    
    //UNLOCKED SCOPE
    while ((signal == 0) && !_destroyed)
    {
        yield();
    }
    signal = -1; //reset
}

template <class YIELDING, class PREDICATE>
void ConditionVariable::waitImpl(YIELDING&& yield,
                                 Mutex& mutex,
                                 PREDICATE predicate,
                                 std::atomic_int& signal)
{
    while (!predicate() && !_destroyed)
    {
        waitImpl(std::forward<YIELDING>(yield), mutex, signal);
    }
}

template <class YIELDING, class REP, class PERIOD>
bool ConditionVariable::waitForImpl(YIELDING&& yield,
                                    Mutex& mutex,
                                    std::chrono::duration<REP, PERIOD>& time,
                                    std::atomic_int& signal)
{
    //LOCKED SCOPE
    {
        Mutex::Guard lock(_thisLock);
        if (_destroyed)
        {
            return true; //don't release the mutex
        }
        if (time == std::chrono::duration<REP, PERIOD>::zero())
        {
            return false; //timeout
        }
        signal = 0; //clear signal flag
        _waiters.push_back(&signal);
    }
    
    Mutex::ReverseGuard unlock(mutex);
    
    //UNLOCKED SCOPE
    auto start = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<REP, PERIOD>::zero();
    bool timeout = false;
    
    //wait until signalled or times out
    while ((signal == 0) && !_destroyed)
    {
        yield();
        elapsed = std::chrono::duration_cast<std::chrono::duration<REP, PERIOD>>(std::chrono::high_resolution_clock::now() - start);
        if (elapsed >= time)
        {
            timeout = true;
            break; //expired time
        }
    }
    
    signal = -1; //reset signal flag
    
    //adjust duration or set to zero if nothing remains
    time = timeout ? std::chrono::duration<REP, PERIOD>::zero() : time - elapsed;
    return !timeout;
}

template <class YIELDING, class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitForImpl(YIELDING&& yield,
                                    Mutex& mutex,
                                    const std::chrono::duration<REP, PERIOD>& time,
                                    PREDICATE predicate,
                                    std::atomic_int& signal)
{
    auto duration = time;
    while (!predicate() && !_destroyed)
    {
        if (!waitForImpl(std::forward<YIELDING>(yield), mutex, duration, signal))
        {
            //timeout
            return predicate();
        }
    }
    //duration has not yet expired
    return true;
}

}}
