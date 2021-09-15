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
#include <quantum/util/quantum_spinlock_util.h>
#include <chrono>
#include <random>

namespace Bloomberg {
namespace quantum {

//==============================================================================
//                                   SpinLock
//==============================================================================
inline
SpinLock::SpinLock(SpinLock&& o) : _flag(o._flag.load()) { }

inline
SpinLock& SpinLock::operator=(SpinLock&& o)
{
    if(this != &o)
    {
        _flag.store(o._flag.load());
    }
    return *this;
}

inline
void SpinLock::lock()
{
    SpinLockUtil::lockWrite(_flag, LockTraits::Attempt::Unlimited);
}

inline
bool SpinLock::tryLock()
{
    return SpinLockUtil::lockWrite(_flag, LockTraits::Attempt::Once);
}

inline
void SpinLock::unlock()
{
    return SpinLockUtil::unlockWrite(_flag);
}

inline
bool SpinLock::isLocked() const
{
    return SpinLockUtil::isLocked(_flag);
}

//==============================================================================
//                            SpinLock::Guard
//==============================================================================
inline
SpinLock::Guard::Guard(SpinLock& lock) :
    _spinlock(lock),
    _ownsLock(true)
{
    _spinlock.lock();
}

inline
SpinLock::Guard::Guard(SpinLock& lock,
                       LockTraits::TryToLock) :
    _spinlock(lock),
    _ownsLock(_spinlock.tryLock())
{
}

inline
SpinLock::Guard::Guard(SpinLock& lock,
                       LockTraits::AdoptLock) :
    _spinlock(lock),
    _ownsLock(lock.isLocked())
{
}

inline
SpinLock::Guard::Guard(SpinLock& lock,
                       LockTraits::DeferLock) :
    _spinlock(lock)
{
}

inline
SpinLock::Guard::~Guard()
{
    if (_ownsLock) {
        _spinlock.unlock();
    }
}

inline
bool SpinLock::Guard::tryLock()
{
    assert(!_ownsLock);
    _ownsLock = _spinlock.tryLock();
    return _ownsLock;
}

inline
void SpinLock::Guard::lock()
{
    assert(!_ownsLock);
    _spinlock.lock();
    _ownsLock = true;
}

inline
bool SpinLock::Guard::ownsLock() const {
    return _ownsLock;
}

inline
void SpinLock::Guard::unlock() {
    assert(_ownsLock);
    _spinlock.unlock();
    _ownsLock = false;
}

inline
SpinLock::ReverseGuard::ReverseGuard(SpinLock& lock) :
    _spinlock(lock)
{
    assert(_spinlock.isLocked());
    _spinlock.unlock();
}

inline
SpinLock::ReverseGuard::~ReverseGuard()
{
    assert(!_spinlock.isLocked());
    _spinlock.lock();
}

}}
