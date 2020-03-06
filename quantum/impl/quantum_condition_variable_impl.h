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
    notifyOneImpl(nullptr);
}

inline
void ConditionVariable::notifyOne(ICoroSync::Ptr sync)
{
    notifyOneImpl(sync);
}

inline
void ConditionVariable::notifyOneImpl(ICoroSync::Ptr sync)
{
    //LOCKED OR UNLOCKED SCOPE
    Mutex::Guard lock(sync, _thisLock);
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
    notifyAllImpl(nullptr);
}

inline
void ConditionVariable::notifyAll(ICoroSync::Ptr sync)
{
    notifyAllImpl(sync);
}

inline
void ConditionVariable::notifyAllImpl(ICoroSync::Ptr sync)
{
    //LOCKED OR UNLOCKED SCOPE
    Mutex::Guard lock(sync, _thisLock);
    for (auto&& waiter : _waiters)
    {
        (*waiter) = 1;
    }
    _waiters.clear();
}

inline
void ConditionVariable::wait(Mutex& mutex)
{
    waitImpl(nullptr, mutex);
}

inline
void ConditionVariable::wait(ICoroSync::Ptr sync,
                             Mutex& mutex)
{
    waitImpl(sync, mutex);
}

template <class PREDICATE>
void ConditionVariable::wait(Mutex& mutex,
                             PREDICATE predicate)
{
    waitImpl(nullptr, mutex, std::move(predicate));
}

template <class PREDICATE>
void ConditionVariable::wait(ICoroSync::Ptr sync,
                             Mutex& mutex,
                             PREDICATE predicate)
{
    waitImpl(sync, mutex, std::move(predicate));
}

template <class REP, class PERIOD>
bool ConditionVariable::waitFor(Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time)
{
    if (time == std::chrono::milliseconds(-1))
    {
        waitImpl(nullptr, mutex);
        return true;
    }
    return waitForImpl(nullptr, mutex, time);
}

template <class REP, class PERIOD>
bool ConditionVariable::waitFor(ICoroSync::Ptr sync,
                                Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time)
{
    if (time == std::chrono::milliseconds(-1))
    {
        waitImpl(sync, mutex);
        return true;
    }
    return waitForImpl(sync, mutex, time);
}

template <class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitFor(Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time,
                                PREDICATE predicate)
{
    if (time == std::chrono::milliseconds(-1))
    {
        waitImpl(nullptr, mutex, std::move(predicate));
        return true;
    }
    return waitForImpl(nullptr, mutex, time, std::move(predicate));
}

template <class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitFor(ICoroSync::Ptr sync,
                                Mutex& mutex,
                                const std::chrono::duration<REP, PERIOD>& time,
                                PREDICATE predicate)
{
    return waitForImpl(sync, mutex, time, std::move(predicate));
}

inline
void ConditionVariable::waitImpl(ICoroSync::Ptr sync,
                                 Mutex& mutex)
{
    std::atomic_int& signal = sync ? sync->signal() : s_threadSignal;
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _thisLock);
        if (_destroyed)
        {
            //don't release 'mutex' which is locked at this point
            return;
        }
        signal = 0; //clear signal flag
        _waiters.push_back(&signal);
    }
    //========= UNLOCKED SCOPE =========
    Mutex::ReverseGuard unlock(sync, mutex);
    while ((signal == 0) && !_destroyed)
    {
        //wait for signal
        yield(sync);
    }
    signal = -1; //reset
}

template <class PREDICATE>
void ConditionVariable::waitImpl(ICoroSync::Ptr sync,
                                 Mutex& mutex,
                                 PREDICATE predicate)
{
    //see: https://en.cppreference.com/w/cpp/thread/condition_variable/wait
    while (!predicate() && !_destroyed)
    {
        waitImpl(sync, mutex);
    }
}

template <class REP, class PERIOD>
bool ConditionVariable::waitForImpl(ICoroSync::Ptr sync,
                                    Mutex& mutex,
                                    const std::chrono::duration<REP, PERIOD>& time)
{
    if (time < std::chrono::milliseconds::zero())
    {
        //invalid time setting
        throw std::invalid_argument("Timeout cannot be negative");
    }
    std::atomic_int& signal = sync ? sync->signal() : s_threadSignal;
    {//========= LOCKED SCOPE =========
        Mutex::Guard lock(sync, _thisLock);
        if (_destroyed)
        {
            //don't release 'mutex' which is locked at this point
            return true;
        }
        if (time == std::chrono::duration<REP, PERIOD>::zero())
        {
            //immediate timeout.
            //reset flag if necessary and return w/o releasing mutex.
            int expected = 1;
            signal.compare_exchange_strong(expected, -1);
            return (expected==1);
        }
        signal = 0; //clear signal flag
        _waiters.push_back(&signal);
    }
    //========= UNLOCKED SCOPE =========
    Mutex::ReverseGuard unlock(sync, mutex);
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<REP, PERIOD>::zero();
    
    //wait until signalled or times out
    while ((signal == 0) && !_destroyed)
    {
        //wait for signal
        yield(sync);
        elapsed = std::chrono::duration_cast<std::chrono::duration<REP, PERIOD>>
                  (std::chrono::steady_clock::now() - start);
        if (elapsed >= time)
        {
            break; //expired
        }
    }
    bool rc = (signal == 1);
    signal = -1; //reset signal flag
    return rc;
}

template <class REP, class PERIOD, class PREDICATE>
bool ConditionVariable::waitForImpl(ICoroSync::Ptr sync,
                                    Mutex& mutex,
                                    const std::chrono::duration<REP, PERIOD>& time,
                                    PREDICATE predicate)
{
    //see: https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until
    while (!predicate() && !_destroyed)
    {
        if (!waitForImpl(sync, mutex, time))
        {
            //timeout
            return predicate();
        }
    }
    return true;
}

}}
