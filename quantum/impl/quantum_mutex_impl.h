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
#include <quantum/interface/quantum_icoro_context.h>

namespace Bloomberg {
namespace quantum {

//fwd declarations
namespace local {
    VoidContextPtr context();
    TaskId taskId();
}

inline
void yield(ICoroSync::Ptr sync)
{
    if (sync)
    {
        sync->getYieldHandle()();
    }
    else {
        YieldingThread()();
    }
}

//==============================================================================================
//                                class Mutex
//==============================================================================================
inline
void Mutex::lock()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lock(nullptr);
}

inline
void Mutex::lock(ICoroSync::Ptr sync)
{
    while (!tryLock())
    {
        yield(sync);
    }
}

inline
bool Mutex::tryLock()
{
    assert(_taskId != local::taskId());
    bool rc = _spinlock.tryLock();
    if (rc) {
        //mutex is locked
        _taskId = local::taskId();
    }
    return rc;
}

inline
void Mutex::unlock()
{
    assert(_taskId == local::taskId());
    _taskId = TaskId{}; //reset the task id
    _spinlock.unlock();
}

inline
bool Mutex::isLocked() const
{
    return _spinlock.isLocked();
}

//==============================================================================================
//                                class Mutex::Guard
//==============================================================================================
inline
Mutex::Guard::Guard(Mutex& mutex) :
    Mutex::Guard::Guard(nullptr, mutex)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
Mutex::Guard::Guard(ICoroSync::Ptr sync,
                    Mutex& mutex) :
    _mutex(&mutex),
    _ownsLock(true)
{
    _mutex->lock(std::move(sync));
}

inline
Mutex::Guard::Guard(Mutex& mutex,
                    Mutex::TryToLock) :
    _mutex(&mutex),
    _ownsLock(_mutex->tryLock())
{
}

inline
Mutex::Guard::Guard(Mutex& mutex,
                    Mutex::AdoptLock) :
    _mutex(&mutex),
    _ownsLock(mutex.isLocked())
{
}

inline
bool Mutex::Guard::ownsLock() const
{
    return _ownsLock;
}

inline
Mutex::Guard::~Guard()
{
    if (ownsLock()) {
        unlock();
    }
}

inline
void Mutex::Guard::lock()
{
    //Application must use the other lock() overload if we are running inside a coroutine
    assert(!local::context());
    lock(nullptr);
}

inline
void Mutex::Guard::lock(ICoroSync::Ptr sync)
{
    assert(_mutex && !ownsLock());
    _mutex->lock(std::move(sync));
    _ownsLock = true;
}

inline
bool Mutex::Guard::tryLock()
{
    assert(_mutex && !ownsLock());
    _ownsLock = _mutex->tryLock();
    return _ownsLock;
}

inline
void Mutex::Guard::unlock()
{
    assert(_mutex && ownsLock());
    _mutex->unlock();
    _ownsLock = false;
}

inline
void Mutex::Guard::release()
{
    _ownsLock = false;
    _mutex = nullptr;
}

//==============================================================================================
//                                class Mutex::ReverseGuard
//==============================================================================================
inline
Mutex::ReverseGuard::ReverseGuard(Mutex& mutex) :
    Mutex::ReverseGuard::ReverseGuard(nullptr, mutex)
{
    //Application must use the other constructor overload if we are running inside a coroutine
    assert(!local::context());
}

inline
Mutex::ReverseGuard::ReverseGuard(ICoroSync::Ptr sync,
                                  Mutex& mutex) :
    _mutex(&mutex),
    _sync(sync)
{
    _mutex->unlock();
}

inline
Mutex::ReverseGuard::~ReverseGuard()
{
    _mutex->lock(_sync);
}

}}
