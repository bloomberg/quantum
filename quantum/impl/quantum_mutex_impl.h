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
Mutex::Mutex()
{}

inline
void Mutex::lock()
{
    lockImpl(nullptr);
}

inline
void Mutex::lock(ICoroSync::Ptr sync)
{
    lockImpl(sync);
}

inline
void Mutex::lockImpl(ICoroSync::Ptr sync)
{
    while (!tryLock())
    {
        yield(sync);
    }
}

inline
bool Mutex::tryLock()
{
    return _spinlock.tryLock();
}

inline
void Mutex::unlock()
{
    _spinlock.unlock();
}

//==============================================================================================
//                                class Mutex::Guard
//==============================================================================================
inline
Mutex::Guard::Guard(Mutex& mutex,
                    bool tryLock) :
    _mutex(mutex)
{
    if (tryLock)
    {
        _ownsLock = _mutex.tryLock();
    }
    else
    {
        _mutex.lock();
        _ownsLock = true;
    }
}

inline
Mutex::Guard::Guard(ICoroSync::Ptr sync,
                    Mutex& mutex,
                    bool tryLock) :
    _mutex(mutex)
{
    if (tryLock)
    {
        _ownsLock = _mutex.tryLock();
    }
    else
    {
        _mutex.lock(sync);
        _ownsLock = true;
    }
}

inline
bool Mutex::Guard::ownsLock() const
{
    return _ownsLock;
}

inline
Mutex::Guard::~Guard()
{
    _mutex.unlock();
}

//==============================================================================================
//                                class Mutex::ReverseGuard
//==============================================================================================
inline
Mutex::ReverseGuard::ReverseGuard(Mutex& mutex) :
    _mutex(mutex)
{
    _mutex.unlock();
}

inline
Mutex::ReverseGuard::ReverseGuard(ICoroSync::Ptr sync,
                                  Mutex& mutex) :
    _mutex(mutex),
    _sync(sync)
{
    _mutex.unlock();
}

inline
Mutex::ReverseGuard::~ReverseGuard()
{
    _mutex.lock(_sync);
}

}}
